## Crystal
  - Inspect the assembly output from various compilers, locally

## Building:
  - `make build`: Builds the program
    - Supports multiple threads with `-j[THREAD COUNT]`
    - Use `make -j$(nproc)` to build with all available threads
  - `make clean`: Cleans the build environment
  - `DEBUG=[true/false]`: Environment variable to enable debug support
    - Includes debug symbols, retains the frame pointer and enables sanitisers
    - `make debug` runs `make build` in debug mode
  - `BUILD_DIR`: Environment variable to configure built object output

## Usage:
  - `./build/crystal`

## Dependencies:
  - Package names are correct for Debian, other distros may vary
  - `make`
  - `libadwaita-1-dev`
  - A C23 compatible compiler

## Licence:
  - This project is available under the terms of the GPL-3.0 License
    - These terms can be found in `LICENCE.txt`
