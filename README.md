# `librpimemmgr`

A library that manages memory on Raspberry Pi.

`librpimemmgr` can allocate memory with VCSM (VideoCore Shared Memory) and/or
Mailbox interface.


## Requirements

- You need [mailbox](https://github.com/Terminus-IMRC/mailbox).
- You need to be **either**:
  - belong to `video` group.
  - be `root` user.


## Installation

```
$ git clone https://github.com/Idein/librpimemmgr
$ cd librpimemmgr/
$ cmake .
$ make
$ sudo make install
```

Or you can create `.deb` package:

```
$ make package
$ sudo dpkg -i librpimemmgr-x.y.z-system.deb
```


## Running tests

```
$ test/addr
$ sudo test/speed
```
