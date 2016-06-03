#!/usr/bin/env python2

# Copyright 2013-present Barefoot Networks, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

#
# Antonin Bas (antonin@barefootnetworks.com)
#
#

import random
import argparse
from scapy.all import Ether, IP, TCP, UDP
from scapy.all import fuzz

num_packets = 1000
num_patterns = 128

parser = argparse.ArgumentParser(description='Traffif generator')
parser.add_argument('--packets', help='Num packets',
                    type=int, action="store", default=1000)
parser.add_argument('--patterns', help='Num patterns (different packets)',
                    type=int, action="store", default=128)
parser.add_argument('--psize', help='Packet size (including payload)',
                    type=int, action="store", default=512)
parser.add_argument('--out', '-o', help='Destination binary file',
                    type=str, action="store", default="traffic_1.bin")
# best seed ever
parser.add_argument('--seed', help='Random seed',
                    type=str, action="store", default="antonin")

args = parser.parse_args()

def get_rand_mac():
    res = []
    for i in xrange(6):
        b = random.randint(0, 255)
        res.append(hex(b)[2:])
    return ":".join(res)

def main():
    random.seed(args.seed)
    patterns = []
    for i in xrange(args.patterns):
        transport = random.choice([UDP, TCP])
        dst, src = get_rand_mac(), get_rand_mac()
        # cannot use fuzz() for Ethernet, or scapy tries to resolve the MAC
        # addresses based on the IP addresses
        p = Ether(dst=dst, src=src) / fuzz(IP()) / fuzz(transport())
        payload = "a" * (args.psize - len(p))
        p = p / payload
        patterns.append(p)
    packets = []
    for i in xrange(args.packets):
        p = random.choice(patterns)
        packets.append(p)
    with open(args.out, 'w') as f:
        f.write("{} {}\n".format(args.packets, args.psize))
        for p in packets:
            f.write(str(p))

if __name__ == '__main__':
    main()
