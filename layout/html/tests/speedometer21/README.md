# Speedometer 2.1 compatibility

Target: the **unmodified Speedometer 2.1 benchmark**, all 16 enabled workloads,
ten iterations and 480 workload checks. Language additions for this target are
limited to ES2015; preserve historical Mozilla/XULRunner applications.

Complete native LoongArch Suite runs are recorded for GTK2 and Xlib. These are
passes for frozen revisions, not every later checkout. Benchmark completion
does not establish full DOM/CSS conformance or correct painting.

[Documentation index](../../../../docs/README.md) ·
[Detailed instructions and validation history](DETAILS.md) ·
[Rendering probes and gaps](../style/README-probes.md)

## Run the benchmark

Build native LoongArch Suite first. Place an unchanged external benchmark copy
with its complete resource tree under the serving directory and record its
hashes. Run from the repository root, using your built runtime:

```sh
sh build/linux/with-display.sh python3 layout/html/tests/speedometer21/run-local-benchmark.py \
  --runtime obj-speedometer21-suite/dist/bin \
  --serve-root artifacts/speedometer21/upstream \
  --report artifacts/speedometer21/suite-local.json
```

This wrapper owns the HTTP server for the duration of the run and defaults to
a three-hour deadline. The runner uses a private HOME/profile, enables ES2015
HTML content there and observes the original completion callback. It records
JSON, console output and HTTP logs. Missing markers, crashes, timeouts or an
incomplete workload count fail; do not disable workloads or add page polyfills.

For an already running HTTP server, use `run-suite.py --url` as shown in the
[detailed runner instructions](DETAILS.md#speedometer-21-compatibility).

The older `https://browserbench.org/Speedometer/` URL serves **Speedometer 1.0**.
Select its separate gate explicitly; it requires all seven enabled workloads,
twenty iterations and 420 workload checks, using ordinary ES5 content defaults:

```sh
python3 layout/html/tests/speedometer21/run-suite.py \
  --runtime obj-zoolrunner-macos-arm64-suite/dist/bin \
  --url https://browserbench.org/Speedometer/ --benchmark-version 1.0 \
  --report artifacts/speedometer1/suite.json
```

The Suite runner also supports macOS and restores its previous native profile
selection after removing the disposable test profile. A 1.0 result is separate
from the existing 2.1 gate; selecting a version does not alter the benchmark's
workloads or iteration count.

Add `--navigator-window` to exercise unchanged Suite Navigator chrome and its
event listeners around the benchmark or content probe. The default uses the
minimal embedded-browser window.

## Run a focused content probe

```sh
sh build/linux/with-display.sh python3 layout/html/tests/speedometer21/run-suite.py \
  --runtime obj-speedometer21-suite/dist/bin --mode probe --timeout 60 \
  --url "file://$PWD/layout/html/tests/style/dom-selectors.html" \
  --report artifacts/speedometer21/selectors.json
```

Content fixtures expose `testDone`, `testFailures` and `testResults`. For
privileged XUL fixtures, use `--chrome-probe PATH`; they emit the same
`SPEEDOMETER-RESULT` JSON marker.

After the Suite gate, `run-toolkit.py --application browser|calendar|xulrunner`
runs probes in disposable application copies and profiles. See the detailed
reference for invocation examples and HTTP, storage and History drivers.

## Results and limitations

- [Latest recorded GTK2 benchmark at the dataset baseline](DETAILS.md#full-benchmark-at-the-native-dataset-baseline)
- [Xlib benchmark and XHR validation](DETAILS.md#completed-xlib-benchmark-and-corrected-xhr-matrix)
- [Application package matrix and CSS follow-up](DETAILS.md#html-attribute-names-in-css)
- [History state](DETAILS.md#history-state-bindings-native-loongarch-suite) and
  [same-document traversal](DETAILS.md#same-document-history-traversal)
- [XHR dispatch and callbacks](DETAILS.md#synthetic-xhr-dispatch-and-callback-regressions)
- [Dataset implementation and realm checks](DETAILS.md#dataset-interface-and-realm-checks)

The recorded GTK2 dataset benchmark uses frozen revision `46bd462d` and predates
subsequent CSS attribute-name, shadow grammar and computed-style changes. Keep
benchmark, focused standards, painting and application results separate, and
preserve runtime hashes and failed/incomplete reports. Remaining rendering gaps
are tracked in the [layout guide](../style/README-probes.md); History and other
DOM limitations remain in their linked implementation records.
