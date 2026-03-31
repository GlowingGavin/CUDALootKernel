# CUDALootKernel
CUDALootKernel is a GPU-accelerated Minecraft loot seed scanning tool that converts loot tables and constraints into optimized CUDA kernels for high-speed brute-force search.

It parses standard Minecraft loot table JSON (including pools, entries, weights, and functions) and generates a fully inlined simulation of the loot generation process using accurate Java RNG behavior. The generated kernel reproduces loot rolls exactly, including weighted entry selection, uniform and constant count functions, and enchantment-related RNG calls.

CUDALootKernel supports multi-pool loot tables and allows users to define constraints (e.g. minimum item counts) that are compiled directly into the kernel for maximum performance. Each thread simulates loot generation independently, enabling massively parallel scanning across billions of seeds on the GPU.

The tool is designed for seedfinding, RNG analysis, and reverse-engineering use cases where correctness and speed are critical. By eliminating interpretation overhead and embedding logic directly into CUDA code, CUDALootKernel achieves significantly higher throughput than traditional CPU-based or generic scanners.
