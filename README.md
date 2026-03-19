# CPU Benchmark Suite

**Author**:

- Ly Huu Nhan To (Harry)
- Jordan Hunt
- Connor Dinh

## Introduction

This repository contains a suite of benchmarking programs designed to deeply evaluate and test various aspects of modern CPU architectures. The benchmarks test specific hardware capabilities from raw arithmetic logic unit (ALU) throughput to complex data-dependent branch predictions and memory hierarchies.

## Benchmark Objectives

We will conduct multiple benchmarks test to measure the following:

- **Single-Thread Performance**
- **Multiple Thread Scaling**
- **Memory & Cache Performance**
- **Branch Prediction & Control Flow**
- **Single instruction, multiple data (SIMD)**

## Experimental Setup and Control

Each benchmark will be executed multiple times, and average execution times will be recorded to increase accuracy. Additionally, the benchmarks will be compiled using the same optimization flags. Operating Systems, input sizes, and runtime parameters will be kept consistent across all test environments.

To ensure fair and accurate comparison:

- Single-thread benchmarks will be pinned to a single core.
- Multi-thread benchmarks will be run with varying thread counts to observe scaling behavior.
- Power-saving features and background processes will be minimized where possible.

## Included Benchmarks & Hardware Focus

This suite relies on specific workloads to stress particular sections of the processor pipeline:

### 1. Basic Math (basicmath.cpp)

- **How it works**: Performs fundamental mathematical calculations, including solving cubic equations, integer/floating-point square roots, and angle/radian conversions.
- **CPU Focus**: **Single-Thread Performance** and **SIMD**. Heavily stresses the Arithmetic Logic Unit (ALU) and Floating-Point Unit (FPU).

### 2. Bit Count (bitcount.cpp)

- **How it works**: Executes various algorithms to count the number of set bits (1s) in large arrays of integers.
- **CPU Focus**: **Branch Prediction & Control Flow**. Evaluates how well the processor predicts loops and conditional jumps, and measures raw bitwise instruction throughput.

### 3. CRC32 (CRC32.cpp)

- **How it works**: Computes a 32-bit Cyclic Redundancy Check (error-detecting code) across a data stream using lookup tables (LUT) and bitwise operations.
- **CPU Focus**: **Memory & Cache Performance**. The frequent table lookups heavily test L1/L2 cache latency and access speeds.

### 4. Dijkstra (dijkstra.cpp)

- **How it works**: Implements Dijkstra's algorithm to find the shortest paths between nodes in a large graph.
- **CPU Focus**: **Memory & Cache Performance** and **Branch Prediction**. The graph traversal uses dynamic memory allocation and pointer-chasing, resulting in unpredictable memory access patterns that heavily test the memory subsystem.

### 5. Fast Fourier Transform (FFT.cpp)

- **How it works**: Calculates the Fast Fourier Transform on heavily populated arrays of complex floating-point numbers.
- **CPU Focus**: **SIMD** and **Memory & Cache Performance**. This is highly vectorizable math that streams large data blocks, testing floating-point vector capabilities and memory bandwidth.

### 6. JPEG (jpeg.cpp)

- **How it works**: Simulates image compression algorithms (Discrete Cosine Transform, Huffman encoding).
- **CPU Focus**: **Multiple Thread Scaling**, **SIMD**, and mixed workloads. Represents a very realistic multimedia workload that benefits greatly from parallel execution and vector instructions.

### 7. Quick Sort (qsort.cpp)

- **How it works**: Sorts a massive array of randomized strings or integers using the standard QuickSort algorithm.
- **CPU Focus**: **Branch Prediction & Control Flow**. QuickSort relies on a pivot, which causes data-dependent branches. With random data, the CPU's branch predictor is aggressively tested for misprediction penalties.

### 8. String Search (stringsearch.cpp)

- **How it works**: Searches for specific substring patterns within a larger body of text.
- **CPU Focus**: **Branch Prediction** and **Memory & Cache Performance**. Tests the processor's ability to handle string/byte comparisons, sequential memory access, and sudden skips or condition checks.

## Installation & Execution

### Prerequisites

To compile and run these benchmarks, you will need a modern C++ compiler such as **GCC (g++)**, **Clang**, or **MSVC** (Visual Studio).

### Compilation

As stated in our methodology, all benchmarks must be compiled with the same optimization flags to ensure fair comparisons. Below is an example using `g++` (GCC) with standard optimization flags (`-O3` for maximum performance, and `-march=native` to allow the compiler to use the host CPU's specific instruction sets like SIMD).

#### Automation & Scripting Compilation (Recommended)

You can use the provided Python script compile_all.py to automatically compile all the benchmark .cpp files in the directory without having to run commands one by one.

**Requirements**:
- Python 3.x
- g++ (GCC) compiler installed and accessible via command line.

**Usage**:
Open a terminal in the project directory and run:

1. **Compile all files (skipping ones that are already compiled)**:
`powershell
python compile_all.py
`

2. **Force recompilation (removes all existing .exe files and compiles everything from scratch)**:
`powershell
python compile_all.py --recompile
`

#### Manual Compilation (Linux/macOS/Windows MinGW)

Below are the `g++` commands to compile each benchmark. We use `-O3` and `-march=native` to maximize performance. Benchmarks utilizing multiple threads may require `-pthread`.

```bash
# 1. Basic Math
g++ -O3 -march=native -o basicmath basicmath.cpp

# 2. Bit Count
g++ -O3 -march=native -o bitcount bitcount.cpp

# 3. CRC32
g++ -O3 -march=native -o CRC32 CRC32.cpp

# 4. Dijkstra
g++ -O3 -march=native -o dijkstra dijkstra.cpp

# 5. Fast Fourier Transform
g++ -O3 -march=native -o FFT FFT.cpp

# 6. JPEG
g++ -O3 -march=native -pthread -o jpeg jpeg.cpp

# 7. Quick Sort
g++ -O3 -march=native -o qsort qsort.cpp

# 8. String Search
g++ -O3 -march=native -o stringsearch stringsearch.cpp
```

### Running the Benchmarks

Once compiled, you must pass **2 parameters** when running the executable: `<mode>` and `<runs>`.

- **`<mode>`**: `1` for Single-Core (pinned to core 0), `2` for Multi-Core (uses maximum hardware threads).
- **`<runs>`**: The number of iterations to run the benchmark (an integer from `1` to `10`).

**On Linux / macOS:**

```bash
# Example: Run in single-core mode with 3 iterations
./basicmath 1 3

# Example: Run in multi-core mode with 5 iterations
./basicmath 2 5
```

**On Windows (PowerShell or CMD):**

```powershell
# Example: Run in single-core mode with 3 iterations
.\basicmath.exe 1 3

# Example: Run in multi-core mode with 5 iterations
.\basicmath.exe 2 5
```

### Automation & Scripting (Optional)

For conducting multiple benchmark tests, it is recommended to write a simple shell script (`.sh` on Linux/macOS or `.ps1` / `.bat` on Windows) to iterate through all executables, log the console output (time taken), and automatically pin core affinities if needed (e.g., using `taskset` on Linux or `Start-Process -Affinity` on Windows PowerShell).
