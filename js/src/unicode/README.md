# JavaScript Unicode normalization and modern casing data

The private `String.prototype.normalize` implementation uses Unicode 18.0.0,
the stable upstream release dated 2026-09-16. This adds a new JavaScript method;
it does not replace the historical XPCOM normalizer, identifier tables, legacy casing
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

## Modern JavaScript casing

ES2015 globals use separate Unicode 18.0.0 full upper/lowercase mappings, including
expansions, supplementary characters and language-independent Final_Sigma context.
Legacy globals retain the historical tables. XPCOM casing, identifiers, regular
expression case folding, layout and fonts are not changed. Explicit embedding
locale callbacks remain authoritative for the locale methods; without a callback,
the modern default uses the same full mappings as the nonlocale methods.

`generate-casing.py` verifies UnicodeData.txt above and these additional inputs:

| Input | SHA-256 |
| --- | --- |
| SpecialCasing.txt | `8538dea57c184f1ef3783885ea79677b10f6efa06423717157e63712f14d1ad2` |
| DerivedCoreProperties.txt | `09c928886a178fcafd93c29e4bd59073a058e5a100b716d425cb563ab50f68c9` |

```sh
python3 js/src/unicode/generate-casing.py --ucd /path/to/ucd-18.0.0 \
  --output js/src/jscasing-data.h
python3 js/tests/es6/test-casing.py --ucd /path/to/ucd-18.0.0 \
  --shell /path/to/runtime/xpcshell --log casing-unicode.log
```

The generator rejects unknown language-independent conditional rules rather than
silently dropping them. Final_Sigma uses original-text Cased/Case_Ignorable
properties; ignorables take precedence when a character has both properties.
Native loops are interruptible, string growth is checked, and lone surrogates
remain unchanged. Ordinary builds use the checked-in tables without Python or
network access. The exhaustive mapping runner checks all 1,114,112 code points
through four methods (4,456,448 comparisons); separate focused/native tests cover
context, garbage collection, callbacks, interrupts, clones and legacy behavior.
