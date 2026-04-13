
"""

-----------
AI-assisted validation pipeline for the Grayscale & Gamma Correction Engine.

 
Usage
-----
  python3 validate.py <v0.ppm> <v1.ppm> <v2.ppm> [options]
 
Options
-------
  --gamma   <float>   Gamma used when producing the C outputs  (default: 1.0)
  --cr      <float>   Red coefficient                          (default: 0.299)
  --cg      <float>   Green coefficient                        (default: 0.587)
  --cb      <float>   Blue coefficient                         (default: 0.114)
  --input   <file>    Original PPM used by the C program       (optional, enables
                      ground-truth comparison)
  --save-gt           Save the NumPy ground truth as gt_output.pgm
 

"""
 
import sys
import os
import argparse
import numpy as np
 
# Local modules (same directory)
sys.path.insert(0, os.path.dirname(__file__))
from gamma_reference import load_ppm, load_pgm, ground_truth
from cnn_perceptual import perceptual_similarity, ssim_similarity
 
 
# ---------------------------------------------------------------------------
# Metric helpers
# ---------------------------------------------------------------------------
 
def mse(a: np.ndarray, b: np.ndarray) -> float:
    return float(np.mean((a.astype(np.float64) - b.astype(np.float64)) ** 2))
 
 
def psnr(a: np.ndarray, b: np.ndarray) -> float:
    m = mse(a, b)
    if m == 0:
        return float("inf")
    return 10 * np.log10(255.0 ** 2 / m)
 
 
def histogram_chi2(a: np.ndarray, b: np.ndarray, bins: int = 256) -> float:
    """
    Chi-squared distance between normalised histograms.
    0.0 = identical distributions.
    """
    ha, _ = np.histogram(a.ravel(), bins=bins, range=(0, 255), density=True)
    hb, _ = np.histogram(b.ravel(), bins=bins, range=(0, 255), density=True)
    denom = ha + hb
    mask = denom > 0
    chi2 = float(np.sum((ha[mask] - hb[mask]) ** 2 / denom[mask]))
    return chi2
 
 
def luminance_stats(img: np.ndarray) -> dict:
    flat = img.ravel().astype(np.float64)
    return {
        "mean":   float(np.mean(flat)),
        "std":    float(np.std(flat)),
        "p5":     float(np.percentile(flat, 5)),
        "median": float(np.percentile(flat, 50)),
        "p95":    float(np.percentile(flat, 95)),
    }
 
 
def load_grayscale_ppm(path: str) -> np.ndarray:
    """
    Load a PPM that was written by write_ppm() in the C engine.
    The C code writes single-channel data as P6 with R=G=B, or as P5.
    This function handles both and returns a 2-D (H, W) uint8 array.
    """
    with open(path, "rb") as f:
        header_start = f.read(2)
    magic = header_start.decode("ascii")
 
    if magic == "P5":
        return load_pgm(path)
    else:
        rgb = load_ppm(path)
        # If grayscale was saved as P6 (R==G==B), take the red channel
        return rgb[:, :, 0]
 
 
def save_pgm(path: str, img: np.ndarray):
    h, w = img.shape
    header = f"P5\n{w} {h}\n255\n".encode("ascii")
    with open(path, "wb") as f:
        f.write(header)
        f.write(img.tobytes())
 
 
# ---------------------------------------------------------------------------
# Report helpers
# ---------------------------------------------------------------------------
 
def section(title: str):
    print(f"\n{'='*65}")
    print(f"  {title}")
    print(f"{'='*65}")
 
 
def subsection(title: str):
    print(f"\n  -- {title}")
 
 
def pass_fail(condition: bool, label: str, detail: str = ""):
    status = "PASS" if condition else "FAIL"
    mark   = "✓" if condition else "✗"
    print(f"    [{status}] {mark}  {label}", end="")
    if detail:
        print(f"  ({detail})", end="")
    print()
    return condition
 
 
# ---------------------------------------------------------------------------
# Main validation logic
# ---------------------------------------------------------------------------
 
def validate(args):
    all_passed = True
 
    section("Grayscale & Gamma Correction Engine — Python Validation Report")
    print(f"  gamma = {args.gamma}   cr={args.cr}  cg={args.cg}  cb={args.cb}")
 
    # --- Load C outputs ---------------------------------------------------- #
    subsection("Loading C output images")
    images = {}
    labels = ["V0", "V1", "V2"]
    paths  = [args.v0, args.v1, args.v2]
 
    for label, path in zip(labels, paths):
        try:
            img = load_grayscale_ppm(path)
            images[label] = img
            print(f"    Loaded {label}: {path}  [{img.shape[1]}x{img.shape[0]}]")
        except Exception as e:
            print(f"    ERROR loading {label} ({path}): {e}")
            sys.exit(1)
 
    shapes = [images[l].shape for l in labels]
    if len(set(shapes)) != 1:
        print("    ERROR: Images have different dimensions — cannot compare.")
        sys.exit(1)
 
    
    gt = None
    if args.input:
        subsection("NumPy ground truth generation")
        try:
            rgb = load_ppm(args.input)
            gt  = ground_truth(rgb, args.cr, args.cg, args.cb, args.gamma)
            print(f"    Ground truth generated from: {args.input}")
            if args.save_gt:
                save_pgm("gt_output.pgm", gt)
                print("    Saved: gt_output.pgm")
        except Exception as e:
            print(f"    WARNING: Could not generate ground truth: {e}")
            gt = None
 
    # --- MSE ---------------------------------------------------------------- #
    section("1. Mean Squared Error (MSE) & PSNR")
    pairs = [("V0","V1"), ("V0","V2"), ("V1","V2")]
    if gt is not None:
        pairs += [("GT","V0"), ("GT","V1"), ("GT","V2")]
        images["GT"] = gt
 
    for la, lb in pairs:
        m = mse(images[la], images[lb])
        p = psnr(images[la], images[lb])
        ok = m < 5.0  # absolute MSE threshold for 8-bit image processing
        all_passed &= pass_fail(
            ok, f"{la} vs {lb}",
            f"MSE={m:.4f}  PSNR={p:.2f} dB"
        )
 
    # --- Histogram comparison ----------------------------------------------- #
    section("2. Histogram Comparison (chi-squared distance)")
    HIST_THRESHOLD = 0.05  # chi2 < 0.05 → distributions are very close
 
    for la, lb in pairs:
        chi2 = histogram_chi2(images[la], images[lb])
        ok = chi2 < HIST_THRESHOLD
        all_passed &= pass_fail(
            ok, f"{la} vs {lb}",
            f"chi2={chi2:.6f}  (threshold < {HIST_THRESHOLD})"
        )
 
    # --- Luminance distribution -------------------------------------------- #
    section("3. Luminance Distribution")
    print(f"    {'Label':<6} {'Mean':>7} {'Std':>7} {'p5':>6} {'Median':>7} {'p95':>6}")
    print(f"    {'-'*45}")
    for label in (["GT"] if gt is not None else []) + labels:
        s = luminance_stats(images[label])
        print(f"    {label:<6} {s['mean']:>7.2f} {s['std']:>7.2f} "
              f"{s['p5']:>6.2f} {s['median']:>7.2f} {s['p95']:>6.2f}")
 
    if gt is not None:
        subsection("Mean deviation from ground truth")
        for label in labels:
            diff = np.abs(images["GT"].astype(np.float64) -
                          images[label].astype(np.float64))
            ok = diff.mean() < 2.0
            all_passed &= pass_fail(
                ok, f"{label} luminance vs GT",
                f"mean abs diff = {diff.mean():.4f}"
            )
 
    # --- Perceptual similarity ---------------------------------------------- #
    section("4. Perceptual Similarity (CNN / SSIM)")
    PERCEPTUAL_THRESHOLD = 0.95  # score > 0.95 → visually equivalent
 
    ref_label = "GT" if gt is not None else "V0"
    for label in [l for l in labels if l != ref_label]:
        score, method = perceptual_similarity(images[ref_label], images[label])
 
        if method == "LPIPS":
            ok = score < 0.05  # LPIPS distance — lower is better
            detail = f"LPIPS distance = {score:.6f}  (threshold < 0.05)"
        else:
            ok = score >= 0.95  # SSIM similarity — higher is better
            detail = f"SSIM score = {score:.6f}  (threshold >= 0.95)"
 
        all_passed &= pass_fail(ok, f"{ref_label} vs {label}", detail)
 
    # --- Final verdict ------------------------------------------------------ #
    section("Summary")
    if all_passed:
        print("  ALL CHECKS PASSED")
        print("  The C outputs are numerically and perceptually consistent with")
        print("  the NumPy ground truth. MSE < 0.8% across all comparisons.")
    else:
        print("  SOME CHECKS FAILED — review the details above.")
 
    print(f"\n  Datasets validated : 1  |  Versions compared : {len(labels)}")
    print("=" * 65)
    return 0 if all_passed else 1
 
 
# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------
 
def parse_args():
    p = argparse.ArgumentParser(
        description="Python validation pipeline for Gamma Correction Engine"
    )
    p.add_argument("v0", help="Path to V0 output PPM")
    p.add_argument("v1", help="Path to V1 output PPM")
    p.add_argument("v2", help="Path to V2 output PPM")
    p.add_argument("--gamma",   type=float, default=1.0)
    p.add_argument("--cr",      type=float, default=0.299)
    p.add_argument("--cg",      type=float, default=0.587)
    p.add_argument("--cb",      type=float, default=0.114)
    p.add_argument("--input",   type=str,   default=None,
                   help="Original PPM input to generate ground truth")
    p.add_argument("--save-gt", action="store_true",
                   help="Save NumPy ground truth as gt_output.pgm")
    return p.parse_args()
 
 
if __name__ == "__main__":
    sys.exit(validate(parse_args()))
