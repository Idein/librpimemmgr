# `librpimemmgr`

A library that manages memory on Raspberry Pi.

`librpimemmgr` can allocate memory with VCSM (VideoCore Shared Memory) and/or
Mailbox interface.


## Requirements

- You need [mailbox](https://github.com/Terminus-IMRC/mailbox).
- You need to be:
  - belong to `video` group **or** be `root` user to use VCSM functions.
  - be `root` user to use Mailbox functions.


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


## Tests

### `test/addr`

<table>
<tr> <th rowspan=2>type</th> <th rowspan=2>flag</th> <th colspan=2>busaddr</th> </tr>
<tr> <th>BCM2835</th> <th>BCM2836, BCM2837</th> </tr>
<tr> <th rowspan=4>VCSM</th>    <th>NONE</th>             <td align="right"><code>0xde400000</code></td> <td align="right"><code>0xfe400000</code></td> </tr>
<tr>                            <th>HOST</th>             <td align="right"><code>0xde400000</code></td> <td align="right"><code>0xfe400000</code></td> </tr>
<tr>                            <th>VC</th>               <td align="right"><code>0x1e400000</code></td> <td align="right"><code>0x3e400000</code></td> </tr>
<tr>                            <th>HOST_AND_VC</th>      <td align="right"><code>0x1e400000</code></td> <td align="right"><code>0x3e400000</code></td> </tr>
<tr> <th rowspan=4>Mailbox</th> <th>NORMAL</th>           <td align="right"><code>0x1e400000</code></td> <td align="right"><code>0x3e400000</code></td> </tr>
<tr>                            <th>DIRECT</th>           <td align="right"><code>0xde400000</code></td> <td align="right"><code>0xfe400000</code></td> </tr>
<tr>                            <th>COHERENT</th>         <td align="right"><code>0x9e400000</code></td> <td align="right"><code>0xbe400000</code></td> </tr>
<tr>                            <th>L1_NONALLOCATING</th> <td align="right"><code>0x5e400000</code></td> <td align="right"><code>0xbe400000</code></td> </tr>
</table>

### `test/speed`

<table>
<tr> <th rowspan=2>type</th> <th rowspan=2>flag</th> <th colspan=2><code>memcpy</code> speed [B/s]</th> </tr>
<tr> <th>Raspberry Pi 1</th> <th>Raspberry Pi 2</th> </tr>
<tr> <th>malloc</th>            <th></th>                 <td align="right"><code>1.190533e+09</code></td> <td align="right"><code>1.040112e+09</code></td> </tr>
<tr> <th rowspan=4>VCSM</th>    <th>NONE</th>             <td align="right"><code>1.346804e+08</code></td> <td align="right"><code>1.541963e+08</code></td> </tr>
<tr>                            <th>HOST</th>             <td align="right"><code>3.958258e+08</code></td> <td align="right"><code>8.402653e+08</code></td> </tr>
<tr>                            <th>VC</th>               <td align="right"><code>1.351541e+08</code></td> <td align="right"><code>1.541585e+08</code></td> </tr>
<tr>                            <th>HOST_AND_VC</th>      <td align="right"><code>3.958881e+08</code></td> <td align="right"><code>8.394293e+08</code></td> </tr>
<tr> <th rowspan=2>Mailbox</th> <th>DIRECT</th>           <td align="right"><code>1.540711e+08</code></td> <td align="right"><code>1.541903e+08</code></td> </tr>
<tr>                            <th>L1_NONALLOCATING</th> <td align="right"><code>1.348967e+08</code></td> <td align="right"></td>                          </tr>
</table>
