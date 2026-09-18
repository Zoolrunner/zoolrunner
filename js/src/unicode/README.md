# JavaScript Unicode normalization data

The private `String.prototype.normalize` implementation uses Unicode 18.0.0,
the stable upstream release dated 2026-09-16. This adds a new JavaScript method;
it does not replace the historical XPCOM normalizer, identifier tables, casing
tables, layout, font or other platform Unicode implementations.

Sources: [Unicode 18.0.0](https://www.unicode.org/versions/Unicode18.0.0/),
[UAX #15 revision 58](https://www.unicode.org/reports/tr15/tr15-58.html), and
[the UCD directory](https://www.unicode.org/Public/18.0.0/ucd/).
Unicode data is distributed under [Unicode License V3](LICENSE.txt). The
application license pages also carry that notice; preserve it in binary
redistributions containing these tables.

`generate-normalization.py` verifies these inputs before generating the checked-in
`../jsnormalization-data.h`:

| Input | SHA-256 |
| --- | --- |
| UnicodeData.txt | `0736451de439ae7baf1425136617da495e09ee5afbe6e394374db7009ea08950` |
| DerivedNormalizationProps.txt | `98ac7f67d985fe781e317f6182e885e94cabb0c314769e6dd73e48b226931ccd` |

```sh
python3 js/src/unicode/generate-normalization.py --ucd /path/to/ucd-18.0.0 \
  --output js/src/jsnormalization-data.h
```

Generation is an optional maintenance operation. Ordinary builds use the
checked-in tables and require neither Python nor downloaded Unicode data.
The generator expands canonical/compatibility decompositions, deduplicates their
sequences, emits canonical combining classes and honors Full_Composition_Exclusion.
Hangul decomposition/composition remains algorithmic. Runtime canonical ordering
uses a stable counting sort of nonstarter runs, and composition preserves
blocking rules. UTF-16 lone surrogates are retained. Allocation limits preserve
the engine's historical string-length representation.

`js/tests/es6/test-normalization.py` verifies the pinned NormalizationTest.txt
(SHA-256 `25a50d816764b04abfb4a646d3eb2b2a803284c3873d9a06757b94fe4513dde3`).
It runs all 20,171 rows in all required form/input combinations and checks identity
for every other code point, including unassigned values and lone surrogates:
4,791,252 checks. See the [ES6 guide](../../tests/es6/README.md) for validation
scope. Passing these normalization checks does not establish full ES2015 support.
