#pragma once

#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <vector>

namespace genesis::organism {

/**
 * @brief Genesis-native model runtime interface (C++ organism seam).
 *
 * This is the boundary between the Python training/export pipeline and the
 * C++ runtime.  The Python side writes a manifest.json + binary weight file;
 * this interface loads them and runs inference.
 */
class IModelRuntime {
public:
    struct Config {
        uint32_t vocab_size = 4096;
        uint32_t d_model = 256;
        uint32_t n_layers = 12;
        uint32_t n_heads = 8;
        uint32_t n_kv_heads = 4;
        uint32_t d_ff = 512;
        uint32_t max_seq_len = 256;
        float    rms_norm_eps = 1e-6f;
    };

    virtual ~IModelRuntime() = default;

    /** Load manifest + weights from the given folder. */
    virtual bool load(const std::string& folder_path) = 0;

    /** Run a single forward pass: token_ids -> logits for next token. */
    virtual std::vector<float> forward(
        std::span<const uint32_t> token_ids
    ) = 0;

    /** Reset KV cache (call between independent sequences). */
    virtual void reset_cache() = 0;

    const Config& config() const { return config_; }

protected:
    Config config_;
};

/**
 * @brief Factory: create the default C++ runtime implementation.
 */
std::unique_ptr<IModelRuntime> create_default_runtime();

}  // namespace genesis::organism
