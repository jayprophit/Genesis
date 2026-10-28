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
 * Minimal JSON value reader: extracts numeric fields from a JSON object
 * without pulling in a full JSON library.
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
 * Format: [uint32_t num_tensors][name_len][name_bytes][dtype][ndim][dims...][float32 data...]
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
 * Implements the forward pass for a decoder-only transformer with:
 * - RMSNorm
 * - RoPE positional embeddings
 * - Grouped-query attention (separate Q, K, V projections)
 * - SwiGLU feed-forward
 *
 * This is correctness-oriented, not performance-optimized.
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

        config_.vocab_size   = read_json_uint(manifest_text, "vocab_size");
        config_.d_model      = static_cast<uint32_t>(read_json_float(manifest_text, "d_model"));
        config_.n_layers     = static_cast<uint32_t>(read_json_float(manifest_text, "n_layers"));
        config_.n_heads      = static_cast<uint32_t>(read_json_float(manifest_text, "n_heads"));
        config_.n_kv_heads   = static_cast<uint32_t>(read_json_float(manifest_text, "n_kv_heads"));
        config_.d_ff         = static_cast<uint32_t>(read_json_float(manifest_text, "d_ff"));
        config_.max_seq_len  = static_cast<uint32_t>(read_json_float(manifest_text, "max_seq_len"));
        config_.rms_norm_eps = read_json_float(manifest_text, "rms_norm_eps");
        rope_theta_ = read_json_float(manifest_text, "rope_theta");

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

        const uint32_t d_model = config_.d_model;
        const uint32_t n_heads = config_.n_heads;
        const uint32_t n_kv_heads = config_.n_kv_heads;
        const uint32_t head_dim = d_model / n_heads;

        // Token embedding
        std::vector<float> hidden(seq_len * d_model);
        embed_tokens(token_ids, hidden);

        // Transformer blocks
        for (uint32_t layer = 0; layer < config_.n_layers; ++layer) {
            transformer_block(layer, hidden, seq_len);
        }

        // Final RMSNorm + language-model head -> logits for last token
        rms_norm(hidden.data() + (seq_len - 1) * d_model, d_model,
                 find_weight("norm.weight"));
        const std::vector<float>& lm_head = find_weight("lm_head.weight");
        const float* w = lm_head.data();
        const float* h = hidden.data() + (seq_len - 1) * d_model;

        std::vector<float> logits(config_.vocab_size);
        for (uint32_t v = 0; v < config_.vocab_size; ++v) {
            float sum = 0.0f;
            for (uint32_t d = 0; d < d_model; ++d) {
                sum += w[v * d_model + d] * h[d];
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
    float rope_theta_ = 10000.0f;

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
            const float* src = w.data() + tid * d_model;
            float* dst = out.data() + i * d_model;
            for (uint32_t d = 0; d < d_model; ++d) {
                dst[d] = src[d];
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

    void apply_rope(float* q_or_k, uint32_t head_dim, uint32_t offset) const {
        const float theta_base = rope_theta_;
        for (uint32_t i = 0; i < head_dim; i += 2) {
            float freq = 1.0f / std::pow(theta_base, float(i) / head_dim);
            float angle = freq * float(offset);
            float s = std::sin(angle);
            float c = std::cos(angle);
            float xr = q_or_k[i];
            float xi = q_or_k[i + 1];
            q_or_k[i]     = xr * c - xi * s;
            q_or_k[i + 1] = xr * s + xi * c;
        }
    }

    void matmul_vec(const float* w, const float* x, float* out, uint32_t out_dim, uint32_t in_dim) const {
        for (uint32_t i = 0; i < out_dim; ++i) {
            float sum = 0.0f;
            for (uint32_t j = 0; j < in_dim; ++j) {
                sum += w[i * in_dim + j] * x[j];
            }
            out[i] = sum;
        }
    }

    void transformer_block(uint32_t layer, std::vector<float>& hidden, uint32_t seq_len) {
        const uint32_t d_model = config_.d_model;
        const uint32_t n_heads = config_.n_heads;
        const uint32_t n_kv_heads = config_.n_kv_heads;
        const uint32_t head_dim = d_model / n_heads;
        const uint32_t d_ff = config_.d_ff;

        // Input RMSNorm (per-token, matching norm2 pattern)
        for (uint32_t pos = 0; pos < seq_len; ++pos) {
            rms_norm(hidden.data() + pos * d_model, d_model,
                     find_weight("blocks." + std::to_string(layer) + ".norm1.weight"));
        }

        // Q, K, V projections
        const std::vector<float>& q_w = find_weight("blocks." + std::to_string(layer) + ".attn.q_proj.weight");
        const std::vector<float>& k_w = find_weight("blocks." + std::to_string(layer) + ".attn.k_proj.weight");
        const std::vector<float>& v_w = find_weight("blocks." + std::to_string(layer) + ".attn.v_proj.weight");

        // Compute Q, K, V for all positions
        std::vector<float> q(seq_len * n_heads * head_dim);
        std::vector<float> k(seq_len * n_kv_heads * head_dim);
        std::vector<float> v(seq_len * n_kv_heads * head_dim);

        for (uint32_t pos = 0; pos < seq_len; ++pos) {
            const float* h = hidden.data() + pos * d_model;
            matmul_vec(q_w.data(), h, q.data() + pos * n_heads * head_dim, n_heads * head_dim, d_model);
            matmul_vec(k_w.data(), h, k.data() + pos * n_kv_heads * head_dim, n_kv_heads * head_dim, d_model);
            matmul_vec(v_w.data(), h, v.data() + pos * n_kv_heads * head_dim, n_kv_heads * head_dim, d_model);

            // Apply RoPE to Q and K
            for (uint32_t h_idx = 0; h_idx < n_heads; ++h_idx) {
                apply_rope(q.data() + pos * n_heads * head_dim + h_idx * head_dim, head_dim, pos);
            }
            for (uint32_t h_idx = 0; h_idx < n_kv_heads; ++h_idx) {
                apply_rope(k.data() + pos * n_kv_heads * head_dim + h_idx * head_dim, head_dim, pos);
            }
        }

        // Scaled dot-product attention (simplified: no mask, no KV cache write-back)
        std::vector<float> attn_out(seq_len * d_model);
        const std::vector<float>& o_w = find_weight("blocks." + std::to_string(layer) + ".attn.o_proj.weight");

        for (uint32_t pos = 0; pos < seq_len; ++pos) {
            float* dst = attn_out.data() + pos * d_model;
            for (uint32_t head = 0; head < n_heads; ++head) {
                const float* q_head = q.data() + pos * n_heads * head_dim + head * head_dim;
                // Attention over all positions up to pos (causal)
                std::vector<float> attn_weights(seq_len, 0.0f);
                float max_logit = -1e9f;
                for (uint32_t j = 0; j <= pos; ++j) {
                    const float* k_head = k.data() + j * n_kv_heads * head_dim + (head % n_kv_heads) * head_dim;
                    float dot = 0.0f;
                    for (uint32_t d = 0; d < head_dim; ++d) dot += q_head[d] * k_head[d];
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
                        const float* v_head = v.data() + j * n_kv_heads * head_dim + (head % n_kv_heads) * head_dim;
                        val += attn_weights[j] * v_head[d];
                    }
                    dst[head * head_dim + d] = val;
                }
            }
            // Project back to d_model
            for (uint32_t d = 0; d < d_model; ++d) {
                float val = 0.0f;
                for (uint32_t dd = 0; dd < d_model; ++dd) {
                    val += attn_out[pos * d_model + dd] * o_w[d * d_model + dd];
                }
                dst[d] = val;
            }
        }

        // Residual + post-attn norm + FFN + residual
        const std::vector<float>& gate_w = find_weight("blocks." + std::to_string(layer) + ".mlp.gate.weight");
        const std::vector<float>& up_w   = find_weight("blocks." + std::to_string(layer) + ".mlp.up.weight");
        const std::vector<float>& down_w = find_weight("blocks." + std::to_string(layer) + ".mlp.down.weight");

        for (uint32_t pos = 0; pos < seq_len; ++pos) {
            float* h = hidden.data() + pos * d_model;
            float* a = attn_out.data() + pos * d_model;
            for (uint32_t d = 0; d < d_model; ++d) h[d] += a[d];

            rms_norm(h, d_model,
                     find_weight("blocks." + std::to_string(layer) + ".norm2.weight"));

            // SwiGLU: SiLU(gate) * up, then down-proj
            std::vector<float> ff_hidden(d_ff);
            for (uint32_t i = 0; i < d_ff; ++i) {
                float g = 0.0f, u = 0.0f;
                for (uint32_t d = 0; d < d_model; ++d) {
                    g += gate_w[i * d_model + d] * h[d];
                    u += up_w[i * d_model + d] * h[d];
                }
                ff_hidden[i] = (g / (1.0f + std::exp(-g))) * u;  // SiLU(gate) * up
            }
            for (uint32_t d = 0; d < d_model; ++d) {
                float val = 0.0f;
                for (uint32_t i = 0; i < d_ff; ++i) {
                    val += down_w[d * d_ff + i] * ff_hidden[i];
                }
                h[d] += val;
            }
        }
    }
};

std::unique_ptr<IModelRuntime> create_default_runtime() {
    return std::make_unique<DefaultRuntime>();
}

}  // namespace genesis::organism
