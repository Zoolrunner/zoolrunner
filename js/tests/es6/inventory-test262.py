#!/usr/bin/env python3
"""Inventory later Test262 coverage without using feature tags as exclusions.

This report is a review queue, not a conformance result or a test selection.
An es6id can survive a later normative change; every non-identical body still
needs edition review before it can establish an ES2015 requirement.
"""
import argparse
import collections
import hashlib
import json
from pathlib import Path
import re
import subprocess

import yaml

BASELINE = '5e653f2e6ca14ac1ad8e801955a709cae7ac8a11'
CURRENT = '35d566604512cba908054eec49f85e64a59f3091'
ROOTS = ('annexB', 'built-ins', 'language')
FRONTMATTER = re.compile(r'/\*---(.*?)---\*/', re.S)


def verify(root, expected):
    revision = subprocess.check_output(
        ['git', '-C', str(root), 'rev-parse', 'HEAD'], text=True).strip()
    dirty = subprocess.check_output(
        ['git', '-C', str(root), 'status', '--porcelain'], text=True)
    if revision != expected or dirty:
        raise ValueError('Expected a clean checkout at ' + expected + ': ' + str(root))


def files(root):
    return {path.relative_to(root / 'test').as_posix(): path
            for directory in ROOTS for path in (root / 'test' / directory).rglob('*.js')}


def record(path):
    data = path.read_bytes()
    source = data.decode('utf-8-sig')
    match = FRONTMATTER.search(source)
    metadata = (yaml.safe_load(match.group(1)) or {}) if match else {}
    # Keep executable bytes exact, including whitespace/line terminators. The
    # header before frontmatter is attribution, not part of an asserted body.
    body = source[match.end():] if match else source
    return metadata, hashlib.sha256(data).hexdigest(), hashlib.sha256(body.encode('utf-8')).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--baseline', type=Path, required=True)
    parser.add_argument('--current', type=Path, required=True)
    parser.add_argument('--report', type=Path, required=True)
    args = parser.parse_args()
    verify(args.baseline, BASELINE)
    verify(args.current, CURRENT)
    old, new = files(args.baseline), files(args.current)
    previous = {name: record(path) for name, path in old.items()}
    records = []
    for name, path in sorted(new.items()):
        metadata, digest, body = record(path)
        prior = previous.get(name)
        identical = prior is not None and prior[2] == body
        records.append(dict(
            path=name, sha256=digest, body_sha256=body,
            comparison='added' if prior is None else 'identical-body' if identical else 'changed-body',
            fixture=name.endswith('_FIXTURE.js') or not metadata,
            features=metadata.get('features', []), flags=metadata.get('flags', []),
            negative=metadata.get('negative'),
            specification={key: metadata[key] for key in ('es5id', 'es6id', 'es7id', 'esid') if key in metadata},
            edition_review='pending', conformance_result=None))
    result = dict(
        baseline_revision=BASELINE, current_revision=CURRENT,
        purpose='Coverage review only; no cases selected, excluded, or counted as passing.',
        baseline_files=len(old), current_files=len(new),
        comparison_counts=dict(collections.Counter(row['comparison'] for row in records)),
        feature_counts=dict(sorted(collections.Counter(feature for row in records for feature in row['features']).items())),
        removed_paths=sorted(old.keys() - new.keys()), records=records)
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps({key: value for key, value in result.items()
                      if key not in ('records', 'feature_counts', 'removed_paths')}, indent=2))


if __name__ == '__main__':
    main()
