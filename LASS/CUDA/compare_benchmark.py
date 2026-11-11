#!/usr/bin/env python3
"""Compare CSV files from CPU-only and CUDA-enabled LASS benchmark builds."""

import argparse
import csv
import math
from pathlib import Path


def load_results(path: Path, expected_backend: str) -> dict[str, dict[str, str]]:
    with path.open(newline="") as file:
        rows = list(csv.DictReader(file))
    if not rows:
        raise ValueError(f"{path}: no benchmark rows")
    results = {}
    for row in rows:
        name = row["case"]
        if name in results:
            raise ValueError(f"{path}: duplicate case {name}")
        if row["backend"] != expected_backend:
            raise ValueError(f"{path}: {name} has backend {row['backend']!r}")
        results[name] = row
    return results


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("cpu_csv", type=Path)
    parser.add_argument("cuda_csv", type=Path)
    args = parser.parse_args()

    cpu = load_results(args.cpu_csv, "cpu")
    cuda = load_results(args.cuda_csv, "cuda")
    if cpu.keys() != cuda.keys():
        raise ValueError(f"case sets differ: CPU-only={sorted(cpu)}, CUDA={sorted(cuda)}")

    print(f"{'case':<24} {'CPU ms':>11} {'CUDA ms':>11} {'CPU/CUDA':>11}  result")
    for name, cpu_row in cpu.items():
        cuda_row = cuda[name]
        for field in ("suite", "seconds", "channels", "partials",
                      "samples_per_channel", "repetitions"):
            if cpu_row[field] != cuda_row[field]:
                raise ValueError(f"{name}: {field} differs between builds")
        cpu_ms = float(cpu_row["median_ms"])
        cuda_ms = float(cuda_row["median_ms"])
        if not (math.isfinite(cpu_ms) and math.isfinite(cuda_ms)
                and cpu_ms > 0 and cuda_ms > 0):
            raise ValueError(f"{name}: invalid timing")
        ratio = cpu_ms / cuda_ms
        result = "CUDA faster" if ratio > 1 else "CUDA slower" if ratio < 1 else "tie"
        print(f"{name:<24} {cpu_ms:11.3f} {cuda_ms:11.3f} {ratio:10.2f}x  {result}")


if __name__ == "__main__":
    main()
