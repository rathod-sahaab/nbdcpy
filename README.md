# NBDcpy

This is a tool to copy contents from one NBD server to another over application layer [NBD protocol](https://github.com/NetworkBlockDevice/nbd/blob/master/doc/proto.md), using [io_uring](https://unixism.net/loti/what_is_io_uring.html) for async I/O for speed.

More documentation can be found in the docs folder.

## Usage
Currently only localhost is supported with oldstyle hand shake.
```
nbdcpy <source-port> <destination-port>
```

ex:
```
nbdcpy 10879 8080
```

This copies all data from NBD server at port 10879 to NBD server 8080.
## Setup

Run

```sh
mkdir build && cd build
conan install ..
cmake .. -DCMAKE_EXPORT_COMPILE_COMMANDS=1 # generates compile_commands.json
```

**Note:** Omit `ln -s ...` for windows manually copy compile_commands (AFAIK ln is not supported on windows).

## Compile

#### After file changes

```sh
cd build
make -jX
```

_**X**: number of CPU threads on your machine_

#### After dependencies changes

```sh
cd build
conan install ..
cmake .. -DCMAKE_EXPORT_COMPILE_COMMANDS=1
```

## Binary

Binary/app can be found in `build/bin/` which will be same as your project name.
