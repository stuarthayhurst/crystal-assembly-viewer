## Crystal
  - Inspect the assembly output from various compilers, locally
  - Syntax highlighting currently only supports x86, pull requests are welcome for others
    - Due to the size of the x86 ISA, many instructions are likely missing
    - Pull requests are welcome for those too

## Building:
  - `make build`: Builds the program
    - Supports multiple threads with `-j[THREAD COUNT]`
    - Use `make -j$(nproc)` to build with all available threads
  - `make install`: Installs the program
  - `make uninstall`: Uninstalls the program
  - `make clean`: Cleans the build environment
  - `DEBUG=[true/false]`: Environment variable to enable debug support
    - Includes debug symbols, retains the frame pointer and enables sanitisers
    - `make debug` runs `make build` in debug mode
  - `BUILD_DIR`: Environment variable to configure built object output
  - `PREFIX_DIR`: Environment variable to configure the installation prefix
    - Defaults to `/usr/local`

## Usage:
  - A file to open to can optionally be supplied as the first argument
  - Compilers can only be detected if they're reachable from `$PATH`

### In-place:
  - `make build -j$(nproc)`
  - `./build/crystal [FILE]`

### System:
  - `make build -j$(nproc)`
  - `sudo make install`
  - `crystal [FILE]`

## Dependencies:
  - Package names are correct for Debian, other distros may vary
  - `make`
  - `libadwaita-1-dev`
  - `libgtksourceview-5-dev`
  - A C23 compatible compiler

## Licence:
  - This project is available under the terms of the GPL-3.0 License
    - These terms can be found in `LICENCE.txt`
