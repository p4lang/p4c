# Introduction

The P4 compiler `p4c` is a leading project of the P4 Consortium, and
used by several companies as the front end of P4 compilers for a
variety of hardware targets.  Most of these companies develop
significant proprietary back end code, and the resulting binary
executables are released, with no copyleft obligation for those
companies to release their back end source code.

This is allowed, because the `p4c` front source files are licensed
under the Apache 2.0 license.  This license was chosen for multiple
reasons, but in part to enable companies to do this.  It is the P4
Consortium's intent to continue enabling this.


# Linux Foundation review of P4.org software licenses in published code

In early 2024 P4.org became part of The Linux Foundation.  In November
2024 the Linux Foundation appointed someone to do a quick review of
software licenses in the public repositories in the
https://github.com/p4lang organization.

They recommended a fairly light-weight mechanism to make the software
license of all source files explicit and easy to track with automated
tools: adding an SPDX-License-Identifier comment line [1].

[1] https://spdx.org/licenses/

These look like the following examples, where the syntax of everything
_except_ the comment indicator is required by SPDX, i.e. there is a
precise specified syntax for ethe "SPDX-License-Identifier:"
characters to be present, and exactly what sequence of characters may
follow it on the line.

```c++
// SPDX-License-Identifier: Apache-2.0
```

```python
# SPDX-License-Identifier: GPL-2.0-only
```


# Why is the top level LICENSE file insufficient?

One short answer is that not all files in this repository can legally
be released under the Apache 2.0 license.  Source files that are an
exception are described below.  We expect them to be a fairly small
set of files, and that these exceptions are _not_ among the source
files that are compiled to create a P4 compiler executable binary.

Given that, a per-file software license tracking mechanism becomes
useful.  It is also useful when individual files or subsets of files
are copied into other open source software projects.


# Most source files will be released with Apache 2.0 license

The Apache 2.0 license is the default choice for all source files in
https://github.com/p4lang projects, unless this is not legally
allowed.


# Source files that cannot be released under the Apache 2.0 license

## Python source files that import the Scapy package

The Python library Scapy is released under the GNU General Public
License v2.0 [2].

[2] https://github.com/secdev/scapy/blob/master/LICENSE

Note: Scapy is released under the GPL, _not_ the Lesser GPL (aka
LGPL).

While the LGPL explicitly allows releasing proprietary binaries that
dynamically link with LGPL libraries, the GPL has no such provisions.

Unless someone can provide a legal ruling from an intellectual
property lawyer to the contrary, we believe we are required to release
all Python source files meeting the following conditions under the GPL
v2.0 license:

(a) files that directly import the `scapy` or `ptf` packages (the
    `ptf` package imports `scapy`).
(b) files that indirectly import the Scapy package, by importing a
    package that imports Scapy

We believe that this should not be a significant hindrance to
commercial users of this code, because all such Python source files
are test programs, run either during CI for testing p4c executables,
or during testing by P4 developers.  These Python source files that
use Scapy are not part of P4 compiler executables.

If a company wishes to distribute Python programs that import the
Scapy package, they will have to decide how to comply with Scapy's
license themselves.  P4.org can inform them of what we believe the
software license of Scapy requires, and make all efforts to keep Scapy
out of P4 compiler executables, or other programs that we expect that
companies will want to add proprietary extensions to.


## C files intended to be compiled and executed in the Linux kernel, e.g. EBPF

One or more published P4C back ends produce C source files that are
intended to be compiled and loaded into the kernel via the EBPF
feature, at least [3], [4] and perhaps others.

[3] https://github.com/p4lang/p4c/blob/main/backends/ebpf/README.md
[4] https://github.com/p4lang/p4c/blob/main/backends/tc/README.md

While there are a few examples of companies releasing binaries of
Linux driver code for their hardware and not releasing the source
code, we leave it to their legal teams to navigate any of the
intricacies that may be involved in that endeavor.

Our plan is to release all of the following kinds of source files
under the GPL v2.0 license, the same as the Linux kernel:

(a) files that are intended to be compiled and loaded into the kernel
    via EBPF.
(b) header files included from files in category (a).

If any subtle questions arise as to whether any P4 compiler
executable, e.g. `p4c-ebpf`, must thus be released under GPL v2.0, we
will raise these questions to the P4 Technical Steering Team for their
advice.

We expect that for P4 compiler back ends that do not produce source
code for use with EBPF, this question will never arise.


# Third party libraries used by p4c

There are many libraries used by `p4c` executable binaries, and for
some, several alternate libraries that can be used in their place.

TODO: We should summarize those, and their licenses, here, as advice
to those who wish to produce proprietary `p4c` executable binaries.
