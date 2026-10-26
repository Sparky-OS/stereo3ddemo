#!/usr/bin/env python3
"""Assert installed stereo captures against geometry, camera and wire protocol evidence.

The expected eye raster and displayed size are supplied independently of the logs.
Cursor depth comes from the last ray hit (or the logged focus-plane fallback).
"""
import argparse
import json
import math
import re
from pathlib import Path
from check_protocol_lifetime import check as check_protocol_lifetime

p = argparse.ArgumentParser(__doc__)
p.add_argument('case', type=Path)
p.add_argument('--eye', nargs=2, type=int, required=True)
p.add_argument('--shown', nargs=2, type=int, default=[1544, 1080])
p.add_argument('--cursor', choices=['hit', 'focus', 'hidden'])
p.add_argument('--mono', action='store_true')
p.add_argument('--executable-sha256')
p.add_argument('--executable-path', default='/usr/bin/stereo3ddemo')
p.add_argument('--backend', choices=['vulkan', 'opengl'])
p.add_argument('--gpu', choices=['AMD', 'NVIDIA'])
a = p.parse_args()
r = json.loads((a.case / 'analysis.json').read_text())
raw = (a.case / 'app.raw').read_text(errors='replace')
log = (a.case / 'app.log').read_text(errors='replace')
checks = []

def require(condition, description):
    if not condition:
        raise SystemExit(f'FAIL {a.case.name}: {description}')
    checks.append(description)

require(bool(r.get('area')), 'captured scene area is present')
require([r['area']['w'], r['area']['h']] == a.shown, 'displayed area has one view size')
require([r['camera']['eye_w'], r['camera']['eye_h']] == a.eye, 'logged eye raster matches requested resolution')
if a.executable_sha256:
    require(a.executable_sha256 + '  ' + a.executable_path in (a.case / 'installed.txt').read_text(), 'executed binary hash matches the supplied candidate')
if a.backend:
    require(f'renderer quick3d-{a.backend} graphics api ' in log, 'requested renderer is active')
if a.gpu and a.backend == 'vulkan':
    selected = re.findall(r"Physical device \d+: '([^']*)'[^\n]*\nqt.rhi.general:     using this physical device", log)
    require(bool(selected) and all(a.gpu in device for device in selected), 'Vulkan selects the requested physical GPU')
elif a.gpu and a.backend == 'opengl':
    selected = re.findall(r'OpenGL VENDOR: ([^\n]*)', log)
    require(bool(selected) and all(a.gpu in device for device in selected), 'OpenGL uses the requested GPU')
if a.mono:
    require(not r['capture']['stereo'], '3D off produces a mono capture')
    require(not r['declarations']['declared_surfaces'], '3D off has no stereo declaration')
else:
    require(r['capture']['stereo'], 'capture contains a stereo pair')
    require(r['eyes_differ']['outside_area'] == 0, 'zero differing pixels outside the 3D area')
    require(r['eyes_differ']['inside_area'] > 0, 'the two scene images differ')
    try:
        check_protocol_lifetime(raw)
    except ValueError as error:
        require(False, str(error))
    require(True, 'stereo declarations have no overlapping surface lifetimes')
    for name in ('red', 'green', 'blue'):
        marker = r['markers'][name]
        require('error_px' in marker, f'{name} is present in both eyes')
        # Cursor overlap changes a marker centroid; the marker-only T3 rows are the accuracy test.
        if not a.cursor:
            require(abs(marker['error_px']) <= 1, f'{name} disparity is within one displayed pixel of the camera prediction')
            require(abs(marker['vertical_px']) <= 1, f'{name} vertical disparity is within one pixel')
    decl = r['declarations']
    require(len(decl['declared_subsurfaces']) == 1 and not decl['declared_toplevels'], 'only the scene subsurface declares stereo')
    surface = decl['declared_subsurfaces'][0]
    object_ids = {x['object'] for x in decl['created'] if x['surface'] == surface}
    require(any(x['object'] in object_ids and x['value'] == 3 for x in decl['set_content']), 'scene declares left-first side-by-side content')
    # Follow actual buffers attached to the declared surface, rather than accepting an unrelated allocation.
    buffers, pending_buffers, attachments, viewport, destination = {}, {}, [], {}, {}
    for line in raw.splitlines():
        m = re.search(r'create_immed\(new id wl_buffer#(\d+), (\d+), (\d+),', line)
        if m:
            buffers[int(m[1])] = [int(m[2]), int(m[3])]
        m = re.search(r'zwp_linux_buffer_params_v1#(\d+)\.create\((\d+), (\d+),', line)
        if m:
            pending_buffers[int(m[1])] = [int(m[2]), int(m[3])]
        m = re.search(r'zwp_linux_buffer_params_v1#(\d+)\.created\(new id wl_buffer#(\d+)\)', line)
        if m and int(m[1]) in pending_buffers:
            buffers[int(m[2])] = pending_buffers.pop(int(m[1]))
        m = re.search(r'wl_buffer#(\d+)\.destroy\(\)', line)
        if m:
            buffers.pop(int(m[1]), None)
        m = re.search(r'get_viewport\(new id wp_viewport#(\d+), wl_surface#(\d+)\)', line)
        if m:
            viewport[int(m[2])] = int(m[1])
        m = re.search(r'wp_viewport#(\d+)\.set_destination\((\d+), (\d+)\)', line)
        if m:
            destination[int(m[1])] = [int(m[2]), int(m[3])]
        m = re.search(r'wl_surface#(\d+)\.attach\(wl_buffer#(\d+),', line)
        if m and int(m[1]) == surface and int(m[2]) in buffers:
            attachments.append(buffers[int(m[2])])
    expected_pair = [2 * a.eye[0], a.eye[1]]
    require(bool(attachments) and attachments[-1] == expected_pair, 'attached scene buffer is exactly the requested pair raster')
    require(destination.get(viewport.get(surface)) == a.shown, 'KWin viewport destination is one displayed view')
    r['protocol_check'] = {'last_scene_buffer': attachments[-1], 'destination': destination[viewport[surface]]}
    if a.cursor:
        cursor = r['markers']['cursor']
        if a.cursor == 'hidden':
            require(cursor['left'] is None and cursor['right'] is None, 'system-only mode has no rendered 3D cursor')
        else:
            require('disparity_px' in cursor, '3D cursor is visible in both eyes')
            hits = list(re.finditer(
                r'cursor (?:(?P<kind>hit .* at|hits nothing: on focus plane) QVector3D\((?P<position>[^)]*)\)'
                r'|at QPointF\([^)]*\) hits nothing: on the focus plane at (?P<focus>[\d.e+-]+))', log))
            require(bool(hits), 'cursor hit position or focus-plane depth is logged')
            hit = hits[-1]
            require(bool(hit['kind'] and hit['kind'].startswith('hit ')) == (a.cursor == 'hit'), 'cursor uses the expected hit or focus-plane fallback')
            cam = r['camera']
            position = None
            if hit['focus']:
                depth = float(hit['focus'])
                require(abs(depth - cam['d']) <= .0001, 'fallback cursor lies on the camera convergence plane')
            else:
                position = [float(x) for x in hit['position'].split(',')]
                norm = math.sqrt(sum(x*x for x in cam['forward']))
                depth = sum((position[i] - cam['position'][i]) * cam['forward'][i] / norm for i in range(3))
            predicted = a.shown[0] * .5 * cam['e'] * (1/depth - 1/cam['d']) / (math.tan(math.radians(cam['fov'])/2) * cam['aspect'])
            error = cursor['disparity_px'] - predicted
            require(abs(error) <= 1, 'cursor disparity is within one pixel of its world depth')
            require(abs(cursor['vertical_px']) <= 1, 'cursor vertical disparity is within one pixel')
            r['cursor_check'] = {'world_position': position, 'depth': depth, 'predicted_px': predicted, 'error_px': error}
require('Unsupported image format' not in log, 'image format plugins decode the logos')
print(json.dumps({'case': a.case.name, 'checks': checks, 'protocol': r.get('protocol_check'), 'cursor': r.get('cursor_check')}, indent=2))
