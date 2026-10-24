#!/usr/bin/env python3
"""Reject overlapping stereo declarations on one Wayland surface.

Run against WAYLAND_DEBUG=client output, including swapchain recreation. A
surface may be declared again after its prior declaration is destroyed.
"""
import re
import sys
from pathlib import Path


def check(text):
    surfaces, objects = {}, {}
    creates = destroys = 0
    for number, line in enumerate(text.splitlines(), 1):
        match = re.search(r'kde_stereo_content_manager_v1#\d+\.create\(new id kde_stereo_content_v1#(\d+), wl_surface#(\d+)\)', line)
        if match:
            obj, surface = map(int, match.groups())
            if surface in surfaces:
                raise ValueError(f'line {number}: surface {surface} has live declaration {surfaces[surface]} when declaration {obj} is created')
            surfaces[surface] = obj
            objects[obj] = surface
            creates += 1
        match = re.search(r'kde_stereo_content_v1#(\d+)\.destroy\(\)', line)
        if match:
            obj = int(match[1])
            surface = objects.pop(obj, None)
            if surfaces.get(surface) == obj:
                surfaces.pop(surface)
            destroys += 1
        match = re.search(r'wl_surface#(\d+)\.destroy\(\)', line)
        if match:
            surfaces.pop(int(match[1]), None)
    if creates == 0:
        raise ValueError('no stereo declaration was observed')
    return {'creates': creates, 'destroys': destroys}


if __name__ == '__main__':
    try:
        print(check(Path(sys.argv[1]).read_text(errors='replace')))
    except ValueError as error:
        print(f'FAIL: {error}', file=sys.stderr)
        sys.exit(1)
