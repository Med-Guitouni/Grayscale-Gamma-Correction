"""

------------------
NumPy-based vectorized ground truth for grayscale conversion
and power-law gamma correction.
 
Matches exactly the weighted RGB averaging used in the C engine:
  gray = R*cr + G*cg + B*cb
  out  = gray^gamma  (clamped to [0, 255])
"""
 
import numpy as np
 
 
def load_ppm(path: str) -> np.ndarray:
    """
    Read a binary P6 PPM file.
    Returns an (H, W, 3) uint8 array.
    """
    with open(path, "rb") as f:
        raw = f.read()
 
    lines = []
    i = 0
    # Parse header, skipping comment lines
    while len(lines) < 3:
        end = raw.index(b"\n", i)
        line = raw[i:end].decode("ascii").strip()
        i = end + 1
        if line.startswith("#"):
            continue
        lines.append(line)
 
    magic = lines[0]
    if magic != "P6":
        raise ValueError(f"Expected P6 PPM, got {magic}")
 
    w, h = map(int, lines[1].split())
    max_val = int(lines[2])
    if max_val != 255:
        raise ValueError(f"Only 8-bit PPM supported, got max={max_val}")
 
    pixel_data = raw[i:]
    img = np.frombuffer(pixel_data, dtype=np.uint8).reshape(h, w, 3).copy()
    return img
 
 
def load_pgm(path: str) -> np.ndarray:
    """
    Read a binary P5 PGM file.
    Returns an (H, W) uint8 array.
    """
    with open(path, "rb") as f:
        raw = f.read()
 
    lines = []
    i = 0
    while len(lines) < 3:
        end = raw.index(b"\n", i)
        line = raw[i:end].decode("ascii").strip()
        i = end + 1
        if line.startswith("#"):
            continue
        lines.append(line)
 
    magic = lines[0]
    if magic != "P5":
        raise ValueError(f"Expected P5 PGM, got {magic}")
 
    w, h = map(int, lines[1].split())
    pixel_data = raw[i:]
    img = np.frombuffer(pixel_data, dtype=np.uint8).reshape(h, w).copy()
    return img
 
 
def ground_truth(img_rgb: np.ndarray,
                 cr: float = 0.299,
                 cg: float = 0.587,
                 cb: float = 0.114,
                 gamma: float = 1.0) -> np.ndarray:
    """
    Vectorized ground truth implementation.
 
    Parameters
    ----------
    img_rgb : (H, W, 3) uint8 array
    cr, cg, cb : RGB luminance coefficients
    gamma : power-law exponent
 
    Returns
    -------
    (H, W) uint8 array — gamma-corrected grayscale
    """
    # Work in float64 for maximum precision
    img_f = img_rgb.astype(np.float64)
 
    # Weighted grayscale
    gray = cr * img_f[:, :, 0] + cg * img_f[:, :, 1] + cb * img_f[:, :, 2]
 
    # Normalise to [0, 1], apply gamma, restore to [0, 255]
    gray_norm = gray / 255.0
    corrected = np.power(np.clip(gray_norm, 0.0, 1.0), gamma)
    result = np.clip(corrected * 255.0, 0, 255).astype(np.uint8)
 
    return result
