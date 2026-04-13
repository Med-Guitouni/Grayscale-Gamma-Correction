

"""

-----------------
Perceptual similarity using LPIPS (Learned Perceptual Image Patch Similarity).
Uses a pretrained AlexNet backbone — no additional training required.

"""
 
import numpy as np
 
 
def ssim_similarity(img_a: np.ndarray, img_b: np.ndarray) -> float:
    """
    Structural Similarity Index between two (H, W) uint8 grayscale arrays.
    Returns a value in [0, 1]; 1.0 = identical.
    """
    from skimage.metrics import structural_similarity as ssim
    score, _ = ssim(img_a, img_b, full=True, data_range=255)
    return float(score)
 
 
def lpips_similarity(img_a: np.ndarray, img_b: np.ndarray) -> float:
    """
    LPIPS perceptual distance between two grayscale images using a
    pretrained AlexNet backbone.
 
    Returns a distance in [0, inf); lower = more similar.
    Typical values: < 0.05 visually identical, > 0.3 visibly different.
 
    Parameters
    ----------
    img_a, img_b : (H, W) uint8 grayscale arrays
    """
    import torch
    import lpips
 
    loss_fn = lpips.LPIPS(net="alex", verbose=False)
    loss_fn.eval()
 
    def to_tensor(img: np.ndarray) -> "torch.Tensor":
        # LPIPS expects (1, 3, H, W) float32 in [-1, 1]
        norm = img.astype(np.float32) / 127.5 - 1.0
        t = torch.from_numpy(norm).unsqueeze(0).unsqueeze(0)
        return t.repeat(1, 3, 1, 1)  # replicate to 3 channels
 
    with torch.no_grad():
        dist = loss_fn(to_tensor(img_a), to_tensor(img_b))
 
    return float(dist.item())
 
 
def perceptual_similarity(img_a: np.ndarray,
                           img_b: np.ndarray) -> tuple[float, str]:
    """
    Attempt LPIPS first, fall back to SSIM.
 
    Returns
    -------
    (score, method_name)
      LPIPS : score is a distance — lower is better (threshold < 0.05)
      SSIM  : score is similarity — higher is better (threshold > 0.95)
    """
    try:
        dist = lpips_similarity(img_a, img_b)
        return dist, "LPIPS"
    except (ImportError, Exception):
        score = ssim_similarity(img_a, img_b)
        return score, "SSIM"
