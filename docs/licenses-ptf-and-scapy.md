# Considerations for ptf and scapy Python packages

The Python library Scapy is released under the GNU General Public
License v2.0 [1].

[1] https://github.com/secdev/scapy/blob/master/LICENSE


The ptf package is released under the Apache 2.0 license:

[2] https://github.com/p4lang/ptf/blob/main/LICENSE



# Compatibility of GPL v2.0 and Apache 2.0 licenses

See [this article](licenses-apache-and-gpl-v2.md).


# Consequences

Assuming everything above is correct, consider a Python program that
imports both the scapy and ptf packages _directly_, i.e. it imports
ptf directly, and it also imports scapy directly.

Should P4.org publish such a program, and if so, under what license
should it be released?

It seems that the answer is "no", because the argument for whether
this is allowed is too unclear.

Should P4.org publish such a program under Apache-2.0?  It seems
that the answer is "no", because the program imports scapy, which is
GPL-2.0-only.  It is too unclear whether this is allowed.

Should P4.org publish such a program under GPL-2.0-only?  It seems
that the answer is "no", because the program imports ptf, which is
Apache-2.0.  Again, it is too unclear whether this is allowed.

There are at least a few Python programs in p4lang repositories that
currently import both the scapy and ptf packages, including these:

+ https://github.com/p4lang/p4factory/blob/master/testutils/xnt.py
+ https://github.com/p4lang/p4c/blob/master/backends/ebpf/tests/ptf/test.py
+ https://github.com/p4lang/p4c/blob/master/backends/ebpf/tests/ptf/l2l3_acl.py
+ https://github.com/p4lang/switch/blob/master/testutils/xnt.py
+ https://github.com/p4lang/switch/blob/master/tests/ptf-tests/common/utils.py
+ https://github.com/p4lang/p4-dpdk-target/blob/main/third-party/ptf_grpc/ptf-tests/switchd/wcm/switchdverifywcmtraffic.py
+ https://github.com/p4lang/p4-dpdk-target/blob/main/third-party/ptf_grpc/ptf-tests/switchd/lpm/switchdverifytraffic.py
+ https://github.com/p4lang/ptf/blob/master/src/ptf/packet_scapy.py
+ https://github.com/p4lang/ptf/blob/master/utests/tests/ptf/test_mask.py


# What could we do about this?

One possibility _might_ be to change the license of ptf so that it is
multi-licensed [1], i.e. it can be used either as the Apache-2.0
license, or GPL v2, at the choice of the user.

Then in the cases where someone writes a program A that imports both
scapy and ptf in in A, they can choose to use ptf under the GPL v2
license, and also release program A under the GPL v2 license.

In cases where they write a program A that imports ptf, but not scapy,
then they have a choice whether to publish program A under the GPL v2
license, or the Apache-2.0 license.

[1] https://en.wikipedia.org/wiki/Multi-licensing
