[segedit for linux](https://github.com/wvengen/segedit)
=======================================================

Port MacOS X's segedit to Linux (extract only), based on Apple's
[cctools](https://github.com/opensource-apple/cctools) (version 855).

Compilation of the original sources uses a lot of header files,
including Apple-specific ones. This only includes the pieces that
are actually necessary to build it.

One application would be extraction of firmware from kext bundles.

Build
-----

Make sure you have basic development packages installed and run `make`.

Run
---

To extract segment `__DATA`, section `__foo` to the file `out.dat`, run:
```
segedit foo.kext -extract __DATA __foo out.dat
```

