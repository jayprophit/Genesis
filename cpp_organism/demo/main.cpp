/**
 * @brief Minimal demo: load weights, run one forward pass, print top-5 logits.
 */
#include "genesis_organism/IModelRuntime.h"
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <numeric>
#include <vector>

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <checkpoint-folder>\n";
        return 1;
    }

    namespace fs = std::filesystem;
    fs::path folder = argv[1];

    // If the folder doesn't have weights.bin but has weights.npz, convert it.
    if (!fs::exists(folder / "weights.bin") && fs::exists(folder / "weights.npz")) {
        std::cout << "weights.bin not found. Run scripts/convert_weights_to_binary.py first.\n";
        return 1;
    }

    auto runtime = genesis::organism::create_default_runtime();
    if (!runtime->load(folder.string())) {
        std::cerr << "Failed to load runtime from " << folder << "\n";
        return 1;
    }

    std::cout << "Loaded model: vocab=" << runtime->config().vocab_size
              << " d_model=" << runtime->config().d_model
              << " layers=" << runtime->config().n_layers
              << " heads=" << runtime->config().n_heads << "\n";

    // Dummy prompt: BOS + "genesis"
    std::vector<uint32_t> prompt = {1, 102, 101, 110, 101, 115, 105, 115};  // example ids
    runtime->reset_cache();

    auto logits = runtime->forward(prompt);
    std::cout << "Logits for next token: " << logits.size() << " values\n";

    // Top-5
    std::vector<size_t> idx(logits.size());
    std::iota(idx.begin(), idx.end(), 0);
    std::partial_sort(idx.begin(), idx.begin() + 5, idx.end(),
        [&](size_t a, size_t b) { return logits[a] > logits[b]; });

    std::cout << "Top-5 tokens:\n";
    for (size_t i = 0; i < 5; ++i) {
        std::cout << "  " << idx[i] << " -> " << logits[idx[i]] << "\n";
    }

    return 0;
}
