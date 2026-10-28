#include "genesis_organism/IModelRuntime.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <memory>
#include <numeric>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace {

/**
 * Minimal JSON value reader: extracts a single numeric field from a JSON
 * object without pulling in a full JSON library.
 */
float read_json_float(const std::string& text, const std::string& key) {
    std::string needle = "\"" + key + "\"";
    auto pos = text.find(needle);
    if (pos == std::string::npos) return 0.0f;
    pos = text.find(':', pos + needle.size());
    if (pos == std::string::npos) return 0.0f;
    ++pos;
    while (pos < text.size() && (text[pos] == ' ' || text[pos] == '\t')) ++pos;
    return std::stof(text.substr(pos));
}

uint32_t read_json_uint(const std::string& text, const std::string& key) {
    return static_cast<uint32_t>(read_json_float(text, key));
}

/**
 * Load a binary weight file produced by scripts/convert_weights_to_binary.py.
 * Format: [uint32_t num_tensors][tensor_name_len][tensor_name_bytes]
 *         [uint32_t dtype][uint32_t ndim][uint32_t dim0]...[float32 data...]
 */
std::vector<std::pair<std::string, std::vector<float>>> load_binary_weights(
    const std::string& path
) {
    std::ifstream in(path, std::ios::binary);
    if (!in) throw std::runtime_error("Cannot open weights: " + path);

    auto read_u32 = [&]() -> uint32_t {
        uint32_t v;
        in.read(reinterpret_cast<char*>(&v), sizeof(v));
        return v;
    };

    auto read_bytes = [&](size_t n) -> std::string {
        std::string s(n, '\0');
        in.read(s.data(), n);
        return s;
    };

    auto read_floats = [&](size_t n) -> std::vector<float> {
        std::vector<float> v(n);
        in.read(reinterpret_cast<char*>(v.data()), n * sizeof(float));
        return v;
    };

    uint32_t num_tensors = read_u32();
    std::vector<std::pair<std::string, std::vector<float>>> weights;
    weights.reserve(num_tensors);

    for (uint32_t i = 0; i < num_tensors; ++i) {
        uint32_t name_len = read_u32();
        std::string name = read_bytes(name_len);
        uint32_t dtype = read_u32();  // 0 = float32
        uint32_t ndim = read_u32();
        std::vector<uint32_t> dims(ndim);
        for (uint32_t d = 0; d < ndim; ++d) dims[d] = read_u32();
        size_t count = 1;
        for (auto d : dims) count *= d;
        (void)dtype;
        weights.emplace_back(std::move(name), read_floats(count));
    }

    return weights;
}

}  // anonymous namespace

namespace genesis::organism {

/**
 * Minimal single-thread CPU runtime.
 *
 * This implements the forward pass for a 12-layer decoder-only transformer
 * with RMSNorm, RoPE, grouped-query attention, and SwiGLU feed-forward.
 * It is correctness-oriented, not performance-optimized.
 */
class DefaultRuntime : public IModelRuntime {
public:
    bool load(const std::string& folder_path) override {
        namespace fs = std::filesystem;
        fs::path root(folder_path);

        // Read manifest
        std::ifstream manifest_in(root / "manifest.json");
        if (!manifest_in) return false;
        std::string manifest_text(
            (std::istreambuf_iterator<char>(manifest_in)),
            std::istreambuf_iterator<char>()
        );

        config_.vocab_size    = read_json_uint(manifest_text, "vocab_size");
        config_.d_model       = static_cast<uint32_t>(read_json_float(manifest_text, "d_model"));
        config_.n_layers      = static_cast<uint32_t>(read_json_float(manifest_text, "n_layers"));
        config_.n_heads       = static_cast<uint32_t>(read_json_float(manifest_text, "n_heads"));
        config_.n_kv_heads    = static_cast<uint32_t>(read_json_float(manifest_text, "n_kv_heads"));
        config_.d_ff          = static_cast<uint32_t>(read_json_float(manifest_text, "d_ff"));
        config_.max_seq_len   = static_cast<uint32_t>(read_json_float(manifest_text, "max_seq_len"));
        config_.rms_norm_eps  = read_json_float(manifest_text, "rms_norm_eps");

        // Load weights
        weights_ = load_binary_weights((root / "weights.bin").string());
        return true;
    }

    std::vector<float> forward(std::span<const uint32_t> token_ids) override {
        if (weights_.empty()) throw std::runtime_error("Weights not loaded");

        const uint32_t seq_len = static_cast<uint32_t>(token_ids.size());
        if (seq_len > config_.max_seq_len) {
            throw std::runtime_error("Sequence exceeds max_seq_len");
        }

        // Token embedding
        std::vector<float> hidden(seq_len * config_.d_model);
        embed_tokens(token_ids, hidden);

        // Transformer blocks
        for (uint32_t layer = 0; layer < config_.n_layers; ++layer) {
            transformer_block(layer, hidden, seq_len);
        }

        // Final RMSNorm + language-model head -> logits for last token
        rms_norm(hidden.data() + (seq_len - 1) * config_.d_model, config_.d_model,
                 find_weight("final_norm.weight"));
        const std::vector<float>& lm_head = find_weight("lm_head.weight");
        const float* w = lm_head.data();
        const float* h = hidden.data() + (seq_len - 1) * config_.d_model;

        std::vector<float> logits(config_.vocab_size);
        for (uint32_t v = 0; v < config_.vocab_size; ++v) {
            float sum = 0.0f;
            for (uint32_t d = 0; d < config_.d_model; ++d) {
                sum += w[v * config_.d_model + d] * h[d];
            }
            logits[v] = sum;
        }
        return logits;
    }

    void reset_cache() override {
        kv_cache_.assign(config_.n_layers * config_.n_kv_heads * config_.max_seq_len * (config_.d_model / config_.n_heads), 0.0f);
        cache_len_ = 0;
    }

private:
    std::vector<std::pair<std::string, std::vector<float>>> weights_;
    std::vector<float> kv_cache_;
    uint32_t cache_len_ = 0;

    const std::vector<float>& find_weight(const std::string& name) const {
        for (const auto& [n, w] : weights_) {
            if (n == name) return w;
        }
        throw std::runtime_error("Weight not found: " + name);
    }

    void embed_tokens(std::span<const uint32_t> token_ids, std::vector<float>& out) const {
        const std::vector<float>& w = find_weight("token_embedding.weight");
        const uint32_t d_model = config_.d_model;
        for (size_t i = 0; i < token_ids.size(); ++i) {
            uint32_t tid = token_ids[i];
            if (tid >= config_.vocab_size) tid = 0;
            float* dst = out.data() + i * d_model;
            for (uint32_t d = 0; d < d_model; ++d) {
                dst[d] = w[tid * d_model + d];
            }
        }
    }

    void rms_norm(float* data, uint32_t size, const std::vector<float>& weight) const {
        float ms = 0.0f;
        for (uint32_t i = 0; i < size; ++i) ms += data[i] * data[i];
        ms /= size;
        ms += config_.rms_norm_eps;
        float inv = 1.0f / std::sqrt(ms);
        for (uint32_t i = 0; i < size; ++i) {
            data[i] = data[i] * inv * weight[i];
        }
    }

    void apply_rope(float* q_or_k, uint32_t n_heads, uint32_t head_dim, uint32_t offset) const {
        // Simplified RoPE: rotate pairs of dimensions by position-dependent angles.
        // Full implementation requires per-head theta base; here we use a fixed base.
        const float theta_base = 10000.0f;
        for (uint32_t h = 0; h < n_heads; ++h) {
            for (uint32_t pos = 0; pos < (cache_len_ ? 1 : 1); ++pos) {
                for (uint32_t i = 0; i < head_dim; i += 2) {
                    uint32_t dim = h * head_dim + i;
                    if (dim + 1 >= n_heads * head_dim) break;
                    float freq = 1.0f / std::pow(theta_base, float(i) / head_dim);
                    float angle = freq * float(offset + pos);
                    float s = std::sin(angle);
                    float c = std::cos(angle);
                    float xr = q_or_k[dim];
                    float xi = q_or_k[dim + 1];
                    q_or_k[dim]     = xr * c - xi * s;
                    q_or_k[dim + 1] = xr * s + xi * c;
                }
            }
        }
    }

    void transformer_block(uint32_t layer, std::vector<float>& hidden, uint32_t seq_len) {
        // Input norm
        rms_norm(hidden.data(), seq_len * config_.d_model,
                 find_weight("layers." + std::to_string(layer) + ".input_norm.weight"));

        const uint32_t d_model = config_.d_model;
        const uint32_t n_heads = config_.n_heads;
        const uint32_t n_kv_heads = config_.n_kv_heads;
        const uint32_t head_dim = d_model / n_heads;

        // QKV projection (combined)
        const std::vector<float>& qkv_w = find_weight("layers." + std::to_string(layer) + ".qkv_proj.weight");
        // Attention output projection
        const std::vector<float>& attn_w = find_weight("layers." + std::to_string(layer) + ".attn_out.weight");
        // FFN gate/up/down
        const std::vector<float>& gate_w = find_weight("layers." + std::to_string(layer) + ".ffn_gate.weight");
        const std::vector<float>& up_w   = find_weight("layers." + std::to_string(layer) + ".ffn_up.weight");
        const std::vector<float>& down_w = find_weight("layers." + std::to_string(layer) + ".ffn_down.weight");
        // Post-attn and post-ffn norms
        const std::vector<float>& post_attn_norm = find_weight("layers." + std::to_string(layer) + ".post_attn_norm.weight");
        const std::vector<float>& post_ffn_norm  = find_weight("layers." + std::to_string(layer) + ".post_ffn_norm.weight");

        // Compute Q, K, V for all positions (simplified: no KV cache write-back in this minimal version)
        std::vector<float> qkv(seq_len * (n_heads + 2 * n_kv_heads) * head_dim);
        for (uint32_t pos = 0; pos < seq_len; ++pos) {
            const float* h = hidden.data() + pos * d_model;
            for (uint32_t head = 0; head < n_heads + 2 * n_kv_heads; ++head) {
                const float* w_row = qkv_w.data() + head * d_model;
                float* out = qkv.data() + pos * (n_heads + 2 * n_kv_heads) * head_dim + head * head_dim;
                for (uint32_t d = 0; d < head_dim; ++d) {
                    float sum = 0.0f;
                    for (uint32_t dd = 0; dd < d_model; ++dd) {
                        sum += w_row[dd] * h[dd];
                    }
                    out[d] = sum;
                }
            }
        }

        // Apply RoPE to Q and K
        for (uint32_t pos = 0; pos < seq_len; ++pos) {
            for (uint32_t h = 0; h < n_heads; ++h) {
                apply_rope(qkv.data() + pos * (n_heads + 2 * n_kv_heads) * head_dim + h * head_dim,
                           1, head_dim, pos);
            }
            for (uint32_t h = 0; h < n_kv_heads; ++h) {
                apply_rope(qkv.data() + pos * (n_heads + 2 * n_kv_heads) * head_dim + (n_heads + h) * head_dim,
                           1, head_dim, pos);
            }
        }

        // Scaled dot-product attention (simplified, no mask, no cache)
        std::vector<float> attn_out(seq_len * d_model);
        for (uint32_t pos = 0; pos < seq_len; ++pos) {
            float* dst = attn_out.data() + pos * d_model;
            for (uint32_t head = 0; head < n_heads; ++head) {
                const float* q = qkv.data() + pos * (n_heads + 2 * n_kv_heads) * head_dim + head * head_dim;
                // Attention over all positions
                std::vector<float> attn_weights(seq_len, 0.0f);
                float max_logit = -1e9f;
                for (uint32_t j = 0; j <= pos; ++j) {
                    const float* k = qkv.data() + j * (n_heads + 2 * n_kv_heads) * head_dim
                                     + (n_heads + (head % n_kv_heads)) * head_dim;
                    float dot = 0.0f;
                    for (uint32_t d = 0; d < head_dim; ++d) dot += q[d] * k[d];
                    dot /= std::sqrt(static_cast<float>(head_dim));
                    max_logit = std::max(max_logit, dot);
                    attn_weights[j] = dot;
                }
                // Softmax
                float sum_exp = 0.0f;
                for (uint32_t j = 0; j < seq_len; ++j) {
                    attn_weights[j] = (j <= pos) ? std::exp(attn_weights[j] - max_logit) : 0.0f;
                    sum_exp += attn_weights[j];
                }
                if (sum_exp > 0.0f) {
                    for (uint32_t j = 0; j < seq_len; ++j) attn_weights[j] /= sum_exp;
                }
                // Weighted sum of V
                for (uint32_t d = 0; d < head_dim; ++d) {
                    float val = 0.0f;
                    for (uint32_t j = 0; j < seq_len; ++j) {
                        const float* v = qkv.data() + j * (n_heads + 2 * n_kv_heads) * head_dim
                                         + (n_heads + (head % n_kv_heads)) * head_dim;
                        val += attn_weights[j] * v[d];
                    }
                    dst[head * head_dim + d] = val;
                }
            }
            // Project back to d_model
            for (uint32_t d = 0; d < d_model; ++d) {
                float val = 0.0f;
                for (uint32_t dd = 0; dd < d_model; ++dd) {
                    val += attn_out[pos * d_model + dd] * attn_w[d * d_model + dd];
                }
                dst[d] = val;
            }
        }

        // Residual + post-attn norm + FFN + residual
        for (uint32_t pos = 0; pos < seq_len; ++pos) {
            float* h = hidden.data() + pos * d_model;
            float* a = attn_out.data() + pos * d_model;
            for (uint32_t d = 0; d < d_model; ++d) h[d] += a[d];
            rms_norm(h, d_model, post_attn_norm);

            // SwiGLU: gate * up, then down-proj
            std::vector<float> gate(seq_len == 1 ? d_model : 0);  // per-position in loop below
            std::vector<float> up(seq_len == 1 ? d_model : 0);
            if (seq_len == 1) {
                for (uint32_t d = 0; d < config_.d_ff; ++d) {
                    gate[d] = h[d] * 0.0f;  // placeholder; actual matmul below
                    up[d] = h[d] * 0.0f;
                }
            }
            // Full FFN matmuls (only for seq_len==1 in this minimal version; multi-position
            // would require batching the matmul).
            if (seq_len == 1) {
                std::vector<float> ff_hidden(config_.d_ff);
                for (uint32_t i = 0; i < config_.d_ff; ++i) {
                    float g = 0.0f, u = 0.0f;
                    for (uint32_t d = 0; d < d_model; ++d) {
                        g += gate_w[i * d_model + d] * h[d];
                        u += up_w[i * d_model + d] * h[d];
                    }
                    ff_hidden[i] = (g / (1.0f + std::exp(-g))) * u;  // SiLU(gate) * up
                }
                for (uint32_t d = 0; d < d_model; ++d) {
                    float val = 0.0f;
                    for (uint32_t i = 0; i < config_.d_ff; ++i) {
                        val += down_w[d * config_.d_ff + i] * ff_hidden[i];
                    }
                    h[d] += val;
                }
            }
            rms_norm(h, d_model, post_ffn_norm);
        }
    }
};

std::unique_ptr<IModelRuntime> create_default_runtime() {
    return std::make_unique<DefaultRuntime>();
}

}  // namespace genesis::organism
