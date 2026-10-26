#!/usr/bin/env python3
"""Check a mono capture against the fixture's centre-camera projection."""
import json
import math
import sys
from pathlib import Path

case, fixture = map(Path, sys.argv[1:])
r = json.loads((case / 'analysis.json').read_text())
points = json.loads(fixture.read_text())
c, area = r['camera'], r['area']

def dot(a, b):
    return sum(x * y for x, y in zip(a, b))

def normalized(v):
    length = math.sqrt(dot(v, v))
    return [x / length for x in v]

def cross(a, b):
    return [a[1]*b[2]-a[2]*b[1], a[2]*b[0]-a[0]*b[2], a[0]*b[1]-a[1]*b[0]]

assert not r['capture']['stereo'], 'mono control must not capture a stereo pair'
assert not r['declarations']['declared_surfaces'], 'mono control must not declare stereo'
forward = normalized(c['forward'])
right = normalized(cross(forward, c['up']))
up = cross(right, forward)
scale = math.tan(math.radians(c['fov']) / 2)
measurements = {}
for name, marker in points.items():
    v = [x-y for x, y in zip(marker['center'], c['position'])]
    depth = dot(v, forward)
    expected = [area['x'] + area['w']/2 * (1 + dot(v, right) / (depth * scale * c['aspect'])) - .5,
                area['y'] + area['h']/2 * (1 - dot(v, up) / (depth * scale)) - .5]
    actual = r['markers'][name]['left']
    assert actual, f'{name}: marker is absent'
    error = [actual['x'] - expected[0], actual['y'] - expected[1]]
    measurements[name] = {'expected': expected, 'actual': [actual['x'], actual['y']], 'error': error}
    assert max(map(abs, error)) <= 1, f'{name}: not the centre camera, pixel error {error}'
print(json.dumps({'case': case.name, 'markers': measurements}, indent=2))
