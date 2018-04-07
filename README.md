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

### `test/addr`

<table>
<tr> <th>BCM2835</th> <th>BCM2836, BCM2837</th> </tr> <tr> <td><pre>

VCSM:    NONE:             busaddr=0xde400000
VCSM:    HOST:             busaddr=0xde400000
VCSM:    VC:               busaddr=0x1e400000
VCSM:    HOST_AND_VC:      busaddr=0x1e400000
Mailbox: NORMAL:           busaddr=0x1e400000
Mailbox: DIRECT:           busaddr=0xde400000
Mailbox: COHERENT:         busaddr=0x9e400000
Mailbox: L1_NONALLOCATING: busaddr=0x5e400000</pre></td> <td><pre> VCSM:    NONE:             busaddr=0xfe400000
VCSM:    HOST:             busaddr=0xfe400000
VCSM:    VC:               busaddr=0x3e400000
VCSM:    HOST_AND_VC:      busaddr=0x3e400000
Mailbox: NORMAL:           busaddr=0x3e400000
Mailbox: DIRECT:           busaddr=0xfe400000
Mailbox: COHERENT:         busaddr=0xbe400000
Mailbox: L1_NONALLOCATING: busaddr=0xbe400000</pre></td> </tr></table>

### `test/speed`

<table>
<tr> <th>BCM2835</th> <th>BCM2836, BCM2837</th> </tr> <tr> <td><pre>

VCSM:    NONE:             1.245705e-01 [s], 1.346804e+08 [B/s]
VCSM:    HOST:             4.238536e-02 [s], 3.958258e+08 [B/s]
VCSM:    VC:               1.241340e-01 [s], 1.351541e+08 [B/s]
VCSM:    HOST_AND_VC:      4.237868e-02 [s], 3.958881e+08 [B/s]
Mailbox: DIRECT:           1.088927e-01 [s], 1.540711e+08 [B/s]
Mailbox: L1_NONALLOCATING: 1.243708e-01 [s], 1.348967e+08 [B/s]</pre></td> <td><pre>VCSM:    NONE:             1.088042e-01 [s], 1.541963e+08 [B/s]
VCSM:    HOST:             1.996657e-02 [s], 8.402653e+08 [B/s]
VCSM:    VC:               1.088309e-01 [s], 1.541585e+08 [B/s]
VCSM:    HOST_AND_VC:      1.998646e-02 [s], 8.394293e+08 [B/s]
Mailbox: DIRECT:           1.088085e-01 [s], 1.541903e+08 [B/s]</pre></td> </tr></table>
