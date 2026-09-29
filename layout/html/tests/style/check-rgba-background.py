#!/usr/bin/env python3
"""Check a 1x-scale screenshot of rgba-background-paint.html (requires Pillow)."""
import argparse
import json
from pathlib import Path
from PIL import Image, ImageDraw


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('screenshot', type=Path)
    parser.add_argument('--report', type=Path, required=True)
    args = parser.parse_args()
    actual = Image.open(args.screenshot).convert('RGB')
    anchor = next(((x, y) for y in range(actual.height) for x in range(actual.width)
                   if actual.getpixel((x, y)) == (255, 0, 255)), None)
    if anchor is None:
        raise SystemExit('Missing magenta alignment marker')
    left, top = anchor[0] - 10, anchor[1] - 10
    if left < 0 or top < 0 or left + 440 > actual.width or top + 300 > actual.height:
        raise SystemExit('Painting region is cropped')
    actual = actual.crop((left, top, left + 440, top + 300))
    expected = Image.new('RGBA', (440, 300), (255, 255, 255, 255))

    def rectangle(bounds, color):
        nonlocal expected
        layer = Image.new('RGBA', expected.size, (0, 0, 0, 0))
        x, y, right, bottom = bounds
        ImageDraw.Draw(layer).rectangle((x, y, right - 1, bottom - 1), fill=color)
        expected = Image.alpha_composite(expected, layer)

    rectangle((10, 10, 20, 20), (255, 0, 255, 255))
    rectangle((20, 40, 100, 100), (0, 0, 0, 128))
    rectangle((120, 40, 200, 100), (0, 0, 0, 1))
    rectangle((220, 40, 300, 100), (255, 0, 0, 0))
    rectangle((320, 40, 400, 100), (20, 80, 160, 255))
    rectangle((20, 130, 200, 230), (40, 80, 120, 255))
    rectangle((40, 150, 180, 210), (255, 0, 0, 128))
    rectangle((70, 160, 150, 200), (0, 0, 255, 128))
    rectangle((240, 130, 340, 210), (0, 160, 0, 255))
    rectangle((260, 150, 340, 210), (255, 255, 255, 128))
    rectangle((0, 250, 60, 300), (0, 0, 255, 128))
    rectangle((420, 0, 440, 40), (0, 255, 0, 128))
    expected = expected.convert('RGB')
    different = 0
    maximum = 0
    got_pixels, wanted_pixels = actual.load(), expected.load()
    for y in range(300):
        for x in range(440):
            error = max(abs(a - b) for a, b in zip(got_pixels[x, y], wanted_pixels[x, y]))
            maximum = max(maximum, error)
            different += error != 0
    result = {'pass': different == 0, 'pixels': 440 * 300,
              'differentPixels': different, 'maximumChannelError': maximum,
              'contentOrigin': [left, top]}
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(result, indent=2) + '\n')
    actual.save(args.report.with_suffix('.png'))
    print(json.dumps(result))
    return 0 if result['pass'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
