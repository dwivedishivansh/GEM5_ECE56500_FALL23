#! /usr/bin/python3

import os
import subprocess
import signal

ROOT = "/home/min/a/sinha93/GEM5_ECE56500_FALL23"
outdir = "/home/min/a/sinha93/final_results"

os.makedirs(outdir, exist_ok=True)

# List of benchmarks
benchmarks = [
    #"fotonik3d_s", "cactuBSSN_s", "imagick_s",
    "wrf_s", "xalancbmk_s",
    "x264_s", "exchange2_s", "mcf_s", "astar"
]

# L2 cache sizes and associativity values to sweep
l2_sizes = ["256kB", "2MB", "128MB"]
associativities = [2, 4, 8]

# Loop through each benchmark
for bench in benchmarks:
    for l2_size in l2_sizes:
        for assoc in associativities:
            # Build the output filename
            outpath = f"{outdir}/{bench}_{l2_size}_{assoc}.txt"

            # Run the command with the current benchmark and compression type
            command = [
                "./build/X86/gem5.fast",
                "configs/spec/spec_se.py",
                "--cpu-type=Timin",
                "--maxinsts=10000000",
                "--caches",
                "--l2cache",
                f"--l2_size={l2_size}",
                f"--l2_assoc={assoc}",
                "--cacheline_size", "128",
                "-b", bench
            ]

            print(f"Running: {' '.join(command)}")
            
            process = subprocess.Popen(command)
            try:
                process.wait(timeout=600)  # 10-minute timeout
            except subprocess.TimeoutExpired:
                print(f"Timeout expired for {bench} with L2 size {l2_size} and associativity {assoc}. Sending SIGINT for cleanup.")
                process.send_signal(signal.SIGINT)  # Send SIGINT for graceful termination
                try:
                    process.wait(timeout=5)  # Wait up to 5 seconds for cleanup
                except subprocess.TimeoutExpired:
                    print(f"Process did not terminate gracefully after SIGINT. Forcing kill.")
                    process.kill()  # Forcefully terminate the process

            print(f"Finished running {bench} with L2 size {l2_size}, associativity {assoc}, results saved in {outpath}")

            try:
                os.rename("/home/min/a/sinha93/GEM5_ECE56500_FALL23/m5out/stats.txt", outpath)
            except OSError as e:
                print(f"Failed to move stats file to {outpath}: {e}")
