# Enclave EVM

Enclave EVM (eEVM) is an open-source, standalone, embeddable, C++ implementation of the Ethereum Virtual Machine. It was originally built to run within a [TEE](https://en.wikipedia.org/wiki/Trusted_execution_environment) (ie, SGX enclave) on top of the [Open Enclave SDK](https://openenclave.io/sdk/), for use with Microsoft's [Confidential Consortium Framework](https://github.com/Microsoft/CCF).

[![CircleCI](https://circleci.com/gh/microsoft/eEVM.svg?style=svg&circle-token=b2b713983d1fe737e7c12e30dc935beb7323e80e)](https://circleci.com/gh/microsoft/eEVM)

## Description

The main entry point is `eevm::Processor::run()`. You will need to provide `eevm::Processor` with an implementation of `eevm::GlobalState` to handle all interactions with permanent state. The source includes `eevm::SimpleGlobalState` as an example backed by `std::map`, but other instances will likely want an implementation which provides permanent storage - perhaps a wrapper to read/write data from the Ethereum blockchain.

eEVM supports all opcodes from Ethereum's [Homestead release](http://ethdocs.org/en/latest/introduction/the-homestead-release.html), as listed in [opcode.h](include/opcode.h). Note that this does not include more recent opcodes such as `RETURNDATACOPY` or `RETURNDATASIZE` from [EIP #211](https://github.com/ethereum/EIPs/pull/211).

The implementation ignores all gas costs - gas is not spent, tracked, or updated during execution, and execution will never throw an outofgas exception. However, it may still be necessary to pass a sensible initial gas value to `eevm::Processor::run()` in case the bytecode calculates or verifies gas budgets itself. It also does not provide the precompiled contracts at addresses 1 to 8.

So far, the code is not particularly optimized in any dimension. In fact, it is in experimental state.

## Dependencies

* CMake. Minimum version 3.10.

## Build and Test

We build and test eEVM on Linux and Windows on x86-64, but it should be functional cross-platform.

### Linux

Build the static library and tests.

```bash
mkdir build
cd build
cmake ..
make
```

It is also possible to build with Ninja or another generator of choice, and the code will compile with either GCC or Clang (other compilers are untested).

Run the tests.

```bash
cd build
ctest -VV
```

### Windows / Visual Studio 2017

Open the Visual Studio 2017 developer command prompt. Create .sln and .vcxproj files and build the static library and tests as follows.

```cmd
mkdir build
cd build
cmake ..
msbuild ALL_BUILD.vcxproj
```

Run the tests.

```cmd
cd build
ctest -C debug
```

### More on tests

To run the tests outside of CTest you will need to provide the path to the test cases as an environment variable.

```bash
cd build
export TEST_DIR=../3rdparty/test_cases/
./eevm_tests
```

The full test suite contains some longer performance tests which are skipped by default. For full coverage, these can be run by passing the `no-skip` option to the test app - these should complete in minutes, while the default tests should complete in under a second.

```bash
cd build

time TEST_DIR=../3rdparty/test_cases/ ./eevm_tests
...
real    0m0.424s

time TEST_DIR=../3rdparty/test_cases/ ./eevm_tests -ns
...
real    2m11.306s
```

Note that the test harness skips several test cases. Some of these test features which are not supported (gas exhaustion, huge address space), while others appear malformed (do not match the documented test format).

## Third-party components

We rely on several open source third-party components, attributed under [THIRD_PARTY_NOTICES](THIRD_PARTY_NOTICES.txt).

### Keccak

The Keccak sources were created from the reference KeccakCodePackage by running `make FIPS202-opt64.pack`. Unfortunately, we cannot use the probably more optimized ASM versions for now, because they are neither PIC nor Visual Studio compatible.

## Contributing

This project welcomes contributions and suggestions. Most contributions require you to
agree to a Contributor License Agreement (CLA) declaring that you have the right to,
and actually do, grant us the rights to use your contribution. For details, visit
https://cla.microsoft.com.

When you submit a pull request, a CLA-bot will automatically determine whether you need
to provide a CLA and decorate the PR appropriately (e.g., label, comment). Simply follow the
instructions provided by the bot. You will only need to do this once across all repositories using our CLA.

This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/).
For more information see the [Code of Conduct FAQ](https://opensource.microsoft.com/codeofconduct/faq/)
or contact [opencode@microsoft.com](mailto:opencode@microsoft.com) with any additional questions or comments.
