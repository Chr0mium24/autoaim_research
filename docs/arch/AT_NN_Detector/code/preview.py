#!/usr/bin/env python3
"""Preview script for Pose26 ONNX model bundle.

Runs all 4 exported ONNX models on a given image (or a random validation image)
and saves a 2×2 comparison grid.

Usage:
    python preview.py                          # random val image
    python preview.py path/to/image.jpg        # specific image
    python preview.py --conf 0.3               # lower confidence threshold
    python preview.py --no-show                # only save, don't open viewer
"""

from __future__ import annotations

import argparse
import os
import random
import sys
from pathlib import Path

import cv2
import numpy as np

# ---------------------------------------------------------------------------
# Constants
# ---------------------------------------------------------------------------

NAMES = [
    "s0_o0", "s0_o2", "s0_o3", "s0_o4", "s0_o5", "s0_o6",
    "s1_o0", "s1_o1", "s1_o3", "s1_o4", "s1_o5", "s1_o7",
]

# Distinct BGR colours for up to 12 classes
PALETTE = [
    (255, 0, 0), (0, 255, 0), (0, 0, 255), (255, 255, 0),
    (255, 0, 255), (0, 255, 255), (128, 0, 255), (255, 128, 0),
    (0, 128, 255), (128, 255, 0), (255, 128, 128), (128, 128, 255),
]

# Model definitions: (filename, display_label, output_format)
# output_format: "e2e" -> [boxes(4), conf(1), cls(1), kpts(8)] = 14
#                "nms" -> [boxes(4), conf(1), cls(1), obj_partial(4), kpts(8)] = 18
MODELS = [
    ("praysky_c2psa_e2e_0228_640x640.onnx",   "C2PSA  e2e  640×640",   "e2e", (640, 640)),
    ("praysky_c2psa_e2e_0228_576x768.onnx",   "C2PSA  e2e  576×768",   "e2e", (576, 768)),
    ("praysky_coord_noe2e_0331_640x640.onnx", "Coord  noe2e 640×640",  "nms", (640, 640)),
    ("praysky_coord_noe2e_0331_576x768.onnx", "Coord  noe2e 576×768",  "nms", (576, 768)),
]

BUNDLE_DIR = Path(__file__).resolve().parent

# ---------------------------------------------------------------------------
# Preprocessing
# ---------------------------------------------------------------------------


def letterbox(img: np.ndarray, target_shape: tuple[int, int]) -> tuple[np.ndarray, float, tuple[int, int]]:
    """Resize + pad image to target (H, W) with letterbox, return (resized, scale, pad)."""
    h, w = img.shape[:2]
    th, tw = target_shape
    scale = min(tw / w, th / h)
    nw, nh = int(w * scale), int(h * scale)
    resized = cv2.resize(img, (nw, nh), interpolation=cv2.INTER_LINEAR)
    pad_w = tw - nw
    pad_h = th - nh
    top, left = pad_h // 2, pad_w // 2
    canvas = np.full((th, tw, 3), 114, dtype=np.uint8)
    canvas[top : top + nh, left : left + nw] = resized
    return canvas, scale, (left, top)


# ---------------------------------------------------------------------------
# Inference
# ---------------------------------------------------------------------------


def run_onnx(onnx_path: str | Path, img_bgr: np.ndarray, target_shape: tuple[int, int], conf_thres: float):
    """Run ONNX inference and return list of detections.

    Each detection: dict(box_xyxy, conf, cls_id, cls_name, kpts, color_bgr)
    """
    import onnxruntime as ort

    session = ort.InferenceSession(str(onnx_path), providers=["CUDAExecutionProvider", "CPUExecutionProvider"])
    input_name = session.get_inputs()[0].name

    # Preprocess (BGR -> RGB for model input)
    canvas, scale, (pad_l, pad_t) = letterbox(img_bgr, target_shape)
    blob = cv2.cvtColor(canvas, cv2.COLOR_BGR2RGB).astype(np.float32) / 255.0
    blob = blob.transpose(2, 0, 1)[None]  # (1, 3, H, W)

    # Inference
    out = session.run(None, {input_name: blob})[0]  # (1, max_det, C)
    raw_h, raw_w = img_bgr.shape[:2]

    detections = []
    for i in range(out.shape[1]):
        d = out[0, i]
        if np.all(d == 0):
            break

        # Parse box
        x1, y1, x2, y2 = d[0], d[1], d[2], d[3]

        # Map back to original image coordinates
        x1 = (x1 - pad_l) / scale
        y1 = (y1 - pad_t) / scale
        x2 = (x2 - pad_l) / scale
        y2 = (y2 - pad_t) / scale

        conf = float(d[4])
        if conf < conf_thres:
            continue

        cls_id = int(d[5])
        cls_name = NAMES[cls_id] if 0 <= cls_id < len(NAMES) else f"cls{cls_id}"

        # Parse keypoints & color (all models: [box(4), conf(1), cls(1), color(4), kpts(8)] = 18)
        color_scores = d[6:10]
        kpts = d[10:18].reshape(4, 2)

        # Map keypoints back
        for k in range(4):
            kpts[k, 0] = (kpts[k, 0] - pad_l) / scale
            kpts[k, 1] = (kpts[k, 1] - pad_t) / scale

        detections.append(dict(
            box=(x1, y1, x2, y2),
            conf=conf,
            cls_id=cls_id,
            cls_name=cls_name,
            kpts=kpts,
            color_scores=color_scores,
            color=PALETTE[cls_id % len(PALETTE)],
        ))

    return detections


# ---------------------------------------------------------------------------
# Drawing
# ---------------------------------------------------------------------------


def draw_detections(img: np.ndarray, dets: list[dict], show_conf: bool = True) -> np.ndarray:
    """Draw boxes, keypoints and labels on image (returns a copy)."""
    canvas = img.copy()
    for d in dets:
        x1, y1, x2, y2 = (int(v) for v in d["box"])
        c = d["color"]
        cv2.rectangle(canvas, (x1, y1), (x2, y2), c, 2)

        # Label: cls_name conf [color_pred]
        label = d["cls_name"]
        if show_conf:
            label += f" {d['conf']:.2f}"
        if d.get("color_scores") is not None:
            color_names = ["B", "R", "G", "P"]
            ci = int(np.argmax(d["color_scores"]))
            label += f" [{color_names[ci]}]"
        (tw, th), _ = cv2.getTextSize(label, cv2.FONT_HERSHEY_SIMPLEX, 0.5, 1)
        cv2.rectangle(canvas, (x1, y1 - th - 4), (x1 + tw + 2, y1), c, -1)
        cv2.putText(canvas, label, (x1, y1 - 2), cv2.FONT_HERSHEY_SIMPLEX, 0.5, (255, 255, 255), 1, cv2.LINE_AA)

        # Keypoints: draw polygon + circles
        kpts = d["kpts"]
        pts = kpts[:, :2].astype(np.int32).reshape(-1, 1, 2)
        cv2.polylines(canvas, [pts], isClosed=True, color=c, thickness=2, lineType=cv2.LINE_AA)
        for pt in kpts:
            cv2.circle(canvas, (int(pt[0]), int(pt[1])), 4, (0, 255, 0), -1, cv2.LINE_AA)
            cv2.circle(canvas, (int(pt[0]), int(pt[1])), 4, c, 1, cv2.LINE_AA)

    return canvas


# ---------------------------------------------------------------------------
# Grid layout
# ---------------------------------------------------------------------------


def make_grid(images: list[np.ndarray], titles: list[str], grid: tuple[int, int] = (2, 2)) -> np.ndarray:
    """Arrange images in a grid with titles."""
    rows, cols = grid
    h, w = images[0].shape[:2]
    # Resize all to same size
    target_h, target_w = h, w
    panels = []
    for img, title in zip(images, titles):
        img_r = cv2.resize(img, (target_w, target_h))
        # Add title bar
        bar = np.full((30, target_w, 3), 40, dtype=np.uint8)
        cv2.putText(bar, title, (8, 22), cv2.FONT_HERSHEY_SIMPLEX, 0.6, (255, 255, 255), 1, cv2.LINE_AA)
        panels.append(np.vstack([bar, img_r]))

    row_panels = []
    for r in range(rows):
        row_panels.append(np.hstack(panels[r * cols : (r + 1) * cols]))
    return np.vstack(row_panels)


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------


def pick_val_image() -> Path:
    """Pick a random image from datasets/val/images."""
    val_dir = BUNDLE_DIR.parent / "datasets" / "val" / "images"
    if not val_dir.exists():
        print(f"[error] validation dir not found: {val_dir}", file=sys.stderr)
        sys.exit(1)
    exts = {".jpg", ".jpeg", ".png", ".bmp", ".webp"}
    imgs = [p for p in val_dir.iterdir() if p.suffix.lower() in exts]
    if not imgs:
        print(f"[error] no images in {val_dir}", file=sys.stderr)
        sys.exit(1)
    return random.choice(imgs)


def main():
    parser = argparse.ArgumentParser(description="Preview Pose26 ONNX model bundle")
    parser.add_argument("image", nargs="?", help="Path to test image (default: random val image)")
    parser.add_argument("--conf", type=float, default=0.25, help="Confidence threshold (default: 0.25)")
    parser.add_argument("--no-show", action="store_true", help="Don't display result (only save)")
    parser.add_argument("--output", type=str, default=None, help="Output path (default: preview_result.jpg in bundle dir)")
    args = parser.parse_args()

    if args.image:
        img_path = Path(args.image)
    else:
        img_path = pick_val_image()
        print(f"[info] using random val image: {img_path.name}")

    if not img_path.exists():
        print(f"[error] image not found: {img_path}", file=sys.stderr)
        sys.exit(1)

    img_bgr = cv2.imread(str(img_path))
    if img_bgr is None:
        print(f"[error] failed to read image: {img_path}", file=sys.stderr)
        sys.exit(1)

    panels = []
    titles = []
    for fname, label, fmt, shape in MODELS:
        onnx_path = BUNDLE_DIR / fname
        if not onnx_path.exists():
            print(f"[warn] skipping missing: {fname}")
            panels.append(np.zeros_like(img_bgr))
            titles.append(f"{label} (MISSING)")
            continue

        print(f"[run] {label} ...", end=" ", flush=True)
        dets = run_onnx(onnx_path, img_bgr, shape, args.conf)
        vis = draw_detections(img_bgr, dets)
        panels.append(vis)
        titles.append(f"{label}  ({len(dets)} dets)")
        print(f"{len(dets)} detections")

    grid = make_grid(panels, titles)
    out_path = Path(args.output) if args.output else BUNDLE_DIR / "preview_result.jpg"
    out_path.parent.mkdir(parents=True, exist_ok=True)
    cv2.imwrite(str(out_path), grid)
    print(f"\n[done] saved to {out_path}")

    if not args.no_show:
        cv2.imshow("Pose26 ONNX Preview", cv2.resize(grid, None, fx=0.8, fy=0.8))
        cv2.setWindowTitle("Pose26 ONNX Preview", f"Pose26 Preview — {img_path.name}")
        print("[info] press any key to close ...")
        cv2.waitKey(0)
        cv2.destroyAllWindows()


if __name__ == "__main__":
    main()
