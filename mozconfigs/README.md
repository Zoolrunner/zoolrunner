# Mozconfig Examples

Mozconfig examples are organized by operating system and architecture, matching
the layout used by related UXP-based trees:

```text
mozconfigs/
  linux/
    loongarch64/
      gtk2_browser_gcc.mozconfig
      gtk2_suite_gcc.mozconfig
      gtk2_xulrunner_gcc.mozconfig
      xlib_browser_gcc.mozconfig
      xlib_suite_gcc.mozconfig
```

Only currently exercised Linux frontends are represented here. Do not add Qt 3
or other historical frontend mozconfigs until those frontends are known to build
and run in this tree.

Use a config by copying it to the source root as `mozconfig`, then run
`client.mk`:

```sh
cp mozconfigs/linux/loongarch64/gtk2_browser_gcc.mozconfig mozconfig
make -f client.mk build
```

For the Xlib browser build:

```sh
cp mozconfigs/linux/loongarch64/xlib_browser_gcc.mozconfig mozconfig
make -f client.mk build
```

For the Xlib suite build:

```sh
cp mozconfigs/linux/loongarch64/xlib_suite_gcc.mozconfig mozconfig
make -f client.mk build
```
