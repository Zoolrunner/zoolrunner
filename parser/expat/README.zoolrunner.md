# Expat integration

The bundled version is Expat 2.8.4. Preserve its upstream attribution and
ZoolRunner's existing symbol prefixes, UTF-16 interface and compiler support.

Gecko's `nsExpatDriver` owns the input which remains after a parser pause.
Unlike upstream Expat's retained-buffer suspension, this integration reports
`XML_ERROR_SUSPENDED` when a callback calls `XML_StopParser`, exposes the
consumed byte position, and accepts replayed input after `XML_ResumeParser`.
Do not substitute upstream suspension semantics without updating the driver.

A stylesheet or script may pause parsing from an end-element callback. Parsing
must stop at that token, without delivering subsequent callbacks. Open tag
names must be copied out of the input buffer before returning, including when
parsing pauses; otherwise buffer reuse can produce a false tag mismatch.
Closing the root element while paused must also select the epilog processor.

`tests/blocking.c` covers repeated pauses, ordinary and empty root elements,
long CDATA, different chunk boundaries, UTF-8 and both UTF-16 byte orders.
The macOS packaging checks run it on runnable modern architectures; the
original-10.0 payload runs it in the target guest. The application GUI fixture
also loads DNS, offline and missing-file error pages and checks that their XML
and scripts produced the expected DOM. These tests are necessary when changing
this adapter or updating the bundled Expat sources.
