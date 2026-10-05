# seri

[![C Standard](https://img.shields.io/badge/C-99-blue.svg)](https://en.wikipedia.org/wiki/C99)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

`seri` is a lightweight, high-performance, and endian-safe binary serialization and deserialization library written in standard C (C99). It is designed for safe, portable cross-platform binary file I/O where data must remain consistent regardless of host system architecture (e.g., transferring files between Little-Endian x86/ARM and Big-Endian systems).

## Features

- **Endian-Independent**: Transparently normalizes binary streams to Big-Endian on disk while seamlessly handling host conversion.
- **High Performance**: Features batched chunk-buffer I/O (8 KB chunks) for large arrays to minimize overhead and leverages compiler built-ins (`__builtin_bswap` / MSVC intrinsics) for native byte swapping.
- **Robust Safety Guards**: Includes strict parameter validation, null pointer safety checks, and multiplication overflow protection against malicious or corrupted file headers (`SIZE_MAX` boundaries).
- **Flexible APIs**: Supports scalars (`bool`, `int32_t`, `uint32_t`, `int64_t`, `uint64_t`, `float`, `double`), dynamic arrays, fixed-size byte buffers, and non-destructive dimension peeking (`seri_read_array_dim`).

## Getting Started

### Prerequisites

- A C99-compatible compiler (GCC, Clang, or MSVC)
- Make (optional, for the provided build system)

### Building the Library

Clone the repository and build the static (`libseri.a`) and shared (`libseri.so`) libraries using the provided Makefile:

```bash
make

```

### Running the Test Suite

To build and execute the comprehensive test suite:

```bash
make test

```

To clean build artifacts:

```bash
make clean

```

## Quick Start Example

Here is a simple example demonstrating how to write and read scalar values and arrays using `seri`:

```c
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "seri.h"

int main() {
    // 1. Write data to a binary file
    FILE* file = fopen("output.bin", "wb");
    if (!file) return 1;

    int32_t original_val = 1337;
    uint32_t numbers[] = { 10, 20, 30, 40, 50 };

    seri_write_int32(file, original_val);
    seri_write_array(file, sizeof(uint32_t), 5, numbers);
    
    fclose(file);

    // 2. Read data back from the binary file
    file = fopen("output.bin", "rb");
    if (!file) return 1;

    int32_t read_val = 0;
    uint8_t elem_size = 0;
    uint64_t num_elems = 0;
    uint32_t read_numbers[5] = { 0 };

    seri_read_int32(file, &read_val);
    seri_read_array(file, &elem_size, &num_elems, read_numbers);

    fclose(file);

    printf("Read scalar: %d\n", read_val);
    printf("Read array count: %llu\n", (unsigned long long)num_elems);

    return 0;
}

```

## API Summary

### Core Functions

* `seri_write_array(FILE* file, uint8_t element_size, uint64_t num_elements, const void* data)`
* `seri_read_array(FILE* file, uint8_t* element_size, uint64_t* num_elements, void* data)`
* `seri_read_array_dim(FILE* file, uint8_t* element_size, uint64_t* num_elements)` *(Peeks dimensions without advancing stream position permanently)*

### Fixed Buffers

* `seri_write_fixed_buffer(FILE* file, const void* data, uint64_t size)`
* `seri_read_fixed_buffer(FILE* file, void* data, uint64_t expected_size)`

### Scalar Wrappers

* **Booleans:** `seri_write_bool`, `seri_read_bool`
* **32-bit Integers:** `seri_write_int32`, `seri_read_int32`, `seri_write_uint32`, `seri_read_uint32`
* **64-bit Integers:** `seri_write_int64`, `seri_read_int64`, `seri_write_uint64`, `seri_read_uint64`
* **Floats / Doubles:** `seri_write_float`, `seri_read_float`, `seri_write_double`, `seri_read_double`

## License

This project is licensed under the [MIT License](https://www.google.com/search?q=LICENSE).

