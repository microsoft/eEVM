# intx

[![readme style: standard][readme style standard badge]][standard readme]

> Extended precision integer C++ library

Provides following types:

- `uint128` (standalone module [int128.hpp]),
- `uint256`,
- `uint512`.

## Usage

To build, test or benchmark.

```bash
git clone https://github.com/chfast/intx
cd intx
mkdir build
cd build

cmake ..
cmake --build . -- -j

test/intx-unittests
test/intx-bench
```

## Maintainer

Paweł Bylica [@chfast]

## License

Licensed under the [Apache License, Version 2.0].


[@chfast]: https://github.com/chfast
[Apache License, Version 2.0]: LICENSE
[int128.hpp]: include/intx/int128.hpp
[standard readme]: https://github.com/RichardLitt/standard-readme

[readme style standard badge]: https://img.shields.io/badge/readme%20style-standard-brightgreen.svg?style=flat-square

