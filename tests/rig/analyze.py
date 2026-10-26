#!/usr/bin/env python3
"""stereo3ddemo-241 rig: reads one case's capture and logs and prints the T3/T4/T6 measurements as JSON.

The screen capture of Stereo KWin holds both eyes side by side, left eye first, each at the output's size.
- the 3D area: the bounding box of the renderer's clear colour (#48536A) in the left eye;
- outside it: the count of pixels that differ between the eyes (the 2D parts must give 0);
- inside it: each marker's centroid per eye (red, green, blue cubes; the cyan cursor), the horizontal disparity
  left minus right, the vertical disparity, and the prediction from the program's last camera log line and the
  marker's world centre: (W/2) e (1/z - 1/d) / tan(hfov/2), where W is the displayed view width;
- the declarations in the WAYLAND_DEBUG log: which surfaces declared, which are subsurfaces, which top levels.
usage: analyze.py CASEDIR [markers.json] [--cursor]
"""
import json
import math
import re
import sys
from pathlib import Path

import numpy as np
from PIL import Image

case = Path(sys.argv[1])
markers = json.load(open(sys.argv[2])) if len(sys.argv) > 2 and not sys.argv[2].startswith("--") else None
want_cursor = "--cursor" in sys.argv
result = {"case": case.name}

# --- the capture
info = json.loads(open(case / "capture.txt").readline())
img = np.asarray(Image.open(case / "screen.png").convert("RGB")).astype(int)
H, W2 = img.shape[:2]
result["capture"] = {"width": W2, "height": H, "stereo": info.get("stereo")}
if info.get("stereo"):
    W = W2 // 2
    left, right = img[:, :W], img[:, W:]
else:
    W = W2
    left = right = img

clear = np.array([0x48, 0x53, 0x6A])
bg = np.all(left == clear, axis=2)
ys, xs = np.nonzero(bg)
if len(xs) == 0:
    result["area"] = None
else:
    x0, x1, y0, y1 = xs.min(), xs.max() + 1, ys.min(), ys.max() + 1
    result["area"] = {"x": int(x0), "y": int(y0), "w": int(x1 - x0), "h": int(y1 - y0), "clear_pixels": int(len(xs))}

if info.get("stereo"):
    diff = np.any(left != right, axis=2)
    outside = diff.copy()
    if result["area"]:
        outside[y0:y1, x0:x1] = False
        inside = int(diff[y0:y1, x0:x1].sum())
    else:
        inside = None
    result["eyes_differ"] = {"outside_area": int(outside.sum()), "inside_area": inside, "total": int(diff.sum())}
    if outside.sum():
        oy, ox = np.nonzero(outside)
        result["eyes_differ"]["outside_bbox"] = [int(ox.min()), int(oy.min()), int(ox.max()), int(oy.max())]


def centroid(eye, mask_fn):
    r, g, b = eye[..., 0], eye[..., 1], eye[..., 2]
    m = mask_fn(r, g, b)
    if result["area"]:
        box = np.zeros_like(m)
        box[y0:y1, x0:x1] = True
        m &= box
    ys_, xs_ = np.nonzero(m)
    if len(xs_) < 8:
        return None
    return {"x": float(xs_.mean()), "y": float(ys_.mean()), "n": int(len(xs_)),
            "bbox": [int(xs_.min()), int(ys_.min()), int(xs_.max()), int(ys_.max())]}


classes = {
    "red": lambda r, g, b: (r > 90) & (g < r * 0.45) & (b < r * 0.45),
    "green": lambda r, g, b: (g > 90) & (r < g * 0.45) & (b < g * 0.45),
    "blue": lambda r, g, b: (b > 90) & (r < b * 0.45) & (g < b * 0.75) & (g > b * 0.1),
}
if want_cursor:
    # the ring (a billboard 100 px across, partly hidden by nearer objects) and the ball at its centre
    classes["cursor"] = lambda r, g, b: (g > 120) & (b > 90) & (r < g * 0.55) & (b > g * 0.55)
    classes["ball"] = lambda r, g, b: (g > 190) & (b > 190) & (r > 90) & (r < g * 0.9) & (b > g * 0.8)

# --- the camera, from the program's last log line
cam = None
for line in open(case / "app.log", errors="replace"):
    m = re.search(r"camera position QVector3D\(([^)]*)\) forward QVector3D\(([^)]*)\) up QVector3D\(([^)]*)\) fov ([\d.e+-]+) aspect ([\d.e+-]+) near ([\d.e+-]+) far ([\d.e+-]+) e ([\d.e+-]+) d ([\d.e+-]+) mode (\d+) eye QSize\((\d+), (\d+)\)", line)
    if m:
        v = lambda s: np.array([float(t) for t in s.split(",")])
        cam = {"position": v(m.group(1)), "forward": v(m.group(2)), "up": v(m.group(3)), "fov": float(m.group(4)),
               "aspect": float(m.group(5)), "near": float(m.group(6)), "far": float(m.group(7)), "e": float(m.group(8)),
               "d": float(m.group(9)), "mode": int(m.group(10)), "eye_w": int(m.group(11)), "eye_h": int(m.group(12))}
if cam:
    result["camera"] = {k: (v.tolist() if isinstance(v, np.ndarray) else v) for k, v in cam.items()}
    f = cam["forward"] / np.linalg.norm(cam["forward"])
    rgt = np.cross(f, cam["up"])
    rgt /= np.linalg.norm(rgt)
    tan_half_h = math.tan(math.radians(cam["fov"]) / 2) * cam["aspect"]

result["markers"] = {}
for name, fn in classes.items():
    cl, cr = centroid(left, fn), centroid(right, fn) if info.get("stereo") else None
    entry = {"left": cl, "right": cr}
    if cl and cr:
        entry["disparity_px"] = cl["x"] - cr["x"]
        entry["vertical_px"] = cl["y"] - cr["y"]
    if cam and markers and name in markers and cl and result["area"]:
        p = np.array(markers[name]["center"])
        rel = p - cam["position"]
        z = float(np.dot(rel, f))
        render_prediction = (cam["eye_w"] / 2) * cam["e"] * (1 / z - 1 / cam["d"]) / tan_half_h
        # Captured centroids are in display pixels. KWin scales the render raster to one shown view.
        display_scale = result["area"]["w"] / cam["eye_w"]
        predicted = render_prediction * display_scale
        entry["depth"] = z
        entry["predicted_render_px"] = render_prediction
        entry["display_scale_x"] = display_scale
        entry["predicted_px"] = predicted
        if cr:
            entry["error_px"] = entry["disparity_px"] - predicted
    result["markers"][name] = entry

if want_cursor:
    def ring_center(eye):
        # The ball cursor has a circular billboard. Fit its outer ring, whose centre
        # remains the cursor point when nearer geometry occludes part of the ring.
        r, g, b = eye[..., 0], eye[..., 1], eye[..., 2]
        mask = classes['cursor'](r, g, b)
        if not result['area']:
            return None
        box = np.zeros_like(mask)
        box[y0:y1, x0:x1] = True
        y, x = np.nonzero(mask & box)
        if len(x) < 40:
            return None
        cx, cy = (x.min()+x.max())/2, (y.min()+y.max())/2
        radius = max(x.max()-x.min(), y.max()-y.min())/2
        if radius < 25:
            return None
        outer = np.hypot(x-cx, y-cy) > .8*radius
        xx, yy = x[outer]-cx, y[outer]-cy
        if len(xx) < 40:
            return None
        fit, _, rank, _ = np.linalg.lstsq(np.column_stack((2*xx, 2*yy, np.ones_like(xx))), xx*xx+yy*yy, rcond=None)
        fitted_radius = float(np.sqrt(fit[2] + sum(fit[:2]**2)))
        residual = float(np.sqrt(np.mean((np.hypot(xx-fit[0], yy-fit[1])-fitted_radius)**2)))
        if rank != 3 or not math.isfinite(residual) or residual > 1.5:
            return None
        return {'x': float(cx+fit[0]), 'y': float(cy+fit[1]), 'radius': fitted_radius,
                'residual_px': residual, 'n': int(len(xx))}
    cl, cr = ring_center(left), ring_center(right) if info.get('stereo') else None
    entry = {'left': cl, 'right': cr}
    if cl and cr:
        entry['disparity_px'] = cl['x'] - cr['x']
        entry['vertical_px'] = cl['y'] - cr['y']
    result['markers']['cursor_ring'] = entry

# --- declarations
decl = {"created": [], "set_content": [], "subsurfaces": {}, "toplevels": [], "x11_supported": None}
wl = case / "wl.txt"
if wl.exists():
    for line in open(wl, errors="replace"):
        m = re.search(r"kde_stereo_content_manager_v1#\d+\.create\(new id kde_stereo_content_v1#(\d+), wl_surface#(\d+)\)", line)
        if m:
            decl["created"].append({"object": int(m.group(1)), "surface": int(m.group(2))})
        m = re.search(r"kde_stereo_content_v1#(\d+)\.set_content\((\d+)\)", line)
        if m:
            decl["set_content"].append({"object": int(m.group(1)), "value": int(m.group(2))})
        m = re.search(r"get_subsurface\(new id wl_subsurface#(\d+), wl_surface#(\d+), wl_surface#(\d+)\)", line)
        if m:
            decl["subsurfaces"][int(m.group(2))] = int(m.group(3))
        m = re.search(r"get_xdg_surface\(new id xdg_surface#\d+, wl_surface#(\d+)\)", line)
        if m:
            decl["toplevels"].append(int(m.group(1)))
    declared = {c["surface"] for c in decl["created"]}
    decl["declared_surfaces"] = sorted(declared)
    decl["declared_subsurfaces"] = sorted(s for s in declared if s in decl["subsurfaces"])
    decl["declared_toplevels"] = sorted(s for s in declared if s in decl["toplevels"])
xr = case / "x11-root.txt"
if xr.exists():
    m = re.search(r"= (\d+)", xr.read_text())
    decl["x11_supported"] = int(m.group(1)) if m else None
result["declarations"] = decl

print(json.dumps(result, indent=1, default=float))
