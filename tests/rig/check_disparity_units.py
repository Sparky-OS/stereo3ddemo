#!/usr/bin/env python3
"""A reduced-raster capture must pass in display pixels and fail with the old render-pixel prediction.

Usage: check_disparity_units.py CASEDIR MARKERS_JSON
The fixture must be a captured stereo scene displayed at a larger size than its render raster.
"""
import json
from pathlib import Path
import subprocess
import sys

analysis = json.loads(subprocess.check_output([
    sys.executable, str(Path(__file__).with_name('analyze.py')), *sys.argv[1:]
], text=True))
assert analysis['capture']['stereo']
assert analysis['eyes_differ']['outside_area'] == 0
assert analysis['area']['w'] > analysis['camera']['eye_w']
old_failures = []
for name in ('red', 'green', 'blue'):
    marker = analysis['markers'][name]
    assert abs(marker['error_px']) <= 1.0, (name, marker)
    old_error = marker['disparity_px'] - marker['predicted_render_px']
    if abs(old_error) > 1.0:
        old_failures.append(name)
    print(f"{name}: measured={marker['disparity_px']:.4f} display_prediction={marker['predicted_px']:.4f} "
          f"error={marker['error_px']:.4f}; old_render_error={old_error:.4f}")
assert old_failures, 'The fixture does not reject the old prediction units'
print('PASS: all displayed-pixel errors are within one pixel; old units reject ' + ', '.join(old_failures))
