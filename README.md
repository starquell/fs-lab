# Lab FS

Assignment was meant to create simple filesystem simulator according to [spec](https://drive.google.com/file/d/0B-BUpwNPP_9JS2lobnpDVlBZMWc/view).

### Variant

* on demand buffering;
* directory state caching in write-through fashion;

### Requirements

* CMake 3.9+;
* C++20 aware compiler with standard library that supports `<charconv>` (tested with clang-11 and respective `libc++`);
* [fmt](https://github.com/fmtlib/fmt) 6.0+

### Build steps

```bash
git clone https://github.com/starquell/fs-lab/
cd fs-lab
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
```

### Usage

After build executable with shell available as `build/bin/shell`, just invoke it.
