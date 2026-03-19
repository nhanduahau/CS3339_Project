import argparse
import os
import subprocess
import glob
import sys

def compile_benchmarks(source_files):
    print("=== Checking and compiling C++ files ===")
    for src in source_files:
        exe_name = os.path.splitext(src)[0] + ".exe"
        if not os.path.exists(exe_name):
            print(f"Compiling {src} -> {exe_name}...")
            # Use g++ with C++17 standard and -O3 optimization
            compile_cmd = ["g++", "-std=c++17", "-O3", src, "-o", exe_name]
            result = subprocess.run(compile_cmd)
            if result.returncode != 0:
                print(f"Error compiling {src}. Exiting.")
                sys.exit(1)
            print(f"-> Successfully compiled {exe_name}")
        else:
            print(f"Found existing {exe_name}, skipping compilation.")
    print("=========================================\n")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Script to automatically compile all C++ files.")
    
    parser.add_argument("--recompile", action="store_true",
                        help="Force recompilation of all C++ files")

    args = parser.parse_args()

    # Find all .cpp files in the current directory
    cpp_files = glob.glob("*.cpp")
    
    if not cpp_files:
        print("No .cpp files found in the current directory.")
        sys.exit(0)

    if args.recompile:
        for f in cpp_files:
            exe = os.path.splitext(f)[0] + ".exe"
            if os.path.exists(exe):
                os.remove(exe)

    compile_benchmarks(cpp_files)
    print("Compilation finished.")
