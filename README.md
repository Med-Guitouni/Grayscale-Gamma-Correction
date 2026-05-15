# Grayscale & Gamma Correction Engine

A high-performance image processing engine written in C that converts RGB images to grayscale and applies gamma correction. Three implementation variants are provided, each with increasing performance optimizations. An AI-assisted Python validation pipeline verifies correctness using MSE, histogram comparison, luminance analysis, and perceptual similarity (LPIPS / SSIM).

---

## Features

- Grayscale conversion using weighted RGB averaging
- Gamma correction via power-law transformation
- Three implementation variants: baseline, LUT-optimized, and multithreaded
- Configurable RGB coefficients and gamma value via CLI
- Benchmarking mode with configurable repeat count
- PPM image I/O (P6 input, P5 output)
- Python validation pipeline with ground truth comparison

---

## Implementation Variants

| Version | Description |
|---|---|
| V0 | Baseline — per-pixel gamma computed using custom `approx_ln` and `approx_exp` (Taylor series) |
| V1 | LUT (Look-Up Table) — precomputes all 256 gamma-corrected values at startup, eliminates per-pixel math |
| V2 | Multithreaded — divides pixel workload across all available CPU cores using POSIX threads |

---

## Build

```bash
make
```

Requires GCC with `-lm` and `-lpthread`.

---

## Usage

```bash
./main [OPTIONS] <input.ppm> -o <output.ppm>
```

### Options

| Option | Description | Default |
|---|---|---|
| `-V <0\|1\|2>` | Implementation version | 0 |
| `-B[<n>]` | Benchmark mode, n repeat runs | 1 |
| `-o <file>` | Output file path | required |
| `--gamma <value>` | Gamma correction value | 1.0 |
| `--coeffs <a,b,c>` | RGB weights for grayscale | 0.299,0.587,0.114 |
| `-h, --help` | Show help message | |

### Examples

```bash
# Baseline with gamma 2.2
./main -V0 --gamma 2.2 input.ppm -o output_v0.ppm

# LUT version with custom coefficients
./main -V1 --gamma 2.2 --coeffs 0.299,0.587,0.114 input.ppm -o output_v1.ppm

# Multithreaded version, benchmarked over 10 runs
./main -V2 -B10 --gamma 2.2 input.ppm -o output_v2.ppm
```

---

## Python Validation Pipeline

Validates all three C outputs against each other and optionally against a NumPy ground truth.

### Install dependencies

```bash
pip install numpy scikit-image lpips torch
```

### Run validation

```bash
python3 validate.py output_v0.ppm output_v1.ppm output_v2.ppm \
    --input input.ppm \
    --gamma 2.2 \
    --cr 0.299 --cg 0.587 --cb 0.114
```

### Validation checks

| Check | Method | Threshold |
|---|---|---|
| Pixel accuracy | MSE + PSNR | MSE < 5.0 |
| Distribution match | Chi-squared histogram | < 0.05 |
| Luminance stats | Mean, std, percentiles | Mean abs diff < 2.0 |
| Perceptual similarity | LPIPS (AlexNet) or SSIM | LPIPS < 0.05 / SSIM > 0.95 |

LPIPS uses a pretrained AlexNet backbone with no additional training. If LPIPS is unavailable, the pipeline automatically falls back to SSIM.

---


## Notes

- Input images must be PPM format P6 (binary RGB, max value 255)
- Output images are written as P5 (binary grayscale)
- V0 uses custom Taylor series approximations for `ln` and `exp` — intentionally avoids `<math.h>` for the core computation
- V2 automatically detects the number of available CPU cores via `sysconf(_SC_NPROCESSORS_ONLN)`
- Achieved < 0.8% MSE across all implementation variants on tested datasets
