#!/usr/bin/env python

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

import sys
import md5

from thrift import Thrift
from thrift.transport import TSocket
from thrift.transport import TTransport
from thrift.protocol import TBinaryProtocol
from thrift.protocol import TMultiplexedProtocol

def check_JSON_md5(client, json_src, out=sys.stdout):
    with open(json_src, 'r') as f:
        m = md5.new()
        for L in f:
            m.update(L)
        md5sum = m.digest()

    def my_print(s):
        out.write(s)

    try:
        bm_md5sum = client.bm_get_config_md5()
    except:
        my_print("Error when requesting config md5 sum from switch\n")
        sys.exit(1)

    if md5sum != bm_md5sum:
        my_print("**********\n")
        my_print("WARNING: the JSON files loaded into the switch and the one ")
        my_print("you just provided to this CLI don't have the same md5 sum. ")
        my_print("Are you sure they describe the same program?\n")
        bm_md5sum_str = "".join("{:02x}".format(ord(c)) for c in bm_md5sum)
        md5sum_str = "".join("{:02x}".format(ord(c)) for c in md5sum)
        my_print("{:<15}: {}\n".format("switch md5", bm_md5sum_str))
        my_print("{:<15}: {}\n".format("CLI input md5", md5sum_str))
        my_print("**********\n")

def get_json_config(standard_client=None, json_path=None, out=sys.stdout):
    def my_print(s):
        out.write(s)

    if json_path:
        if standard_client is not None:
            check_JSON_md5(standard_client, json_path)
        with open(json_path, 'r') as f:
            return f.read()
    else:
        assert(standard_client is not None)
        try:
            my_print("Obtaining JSON from switch...\n")
            json_cfg = standard_client.bm_get_config()
            my_print("Done\n")
        except:
            my_print("Error when requesting JSON config from switch\n")
            sys.exit(1)
        return json_cfg

# services is [(service_name, client_class), ...]
def thrift_connect(thrift_ip, thrift_port, services, out=sys.stdout):
    def my_print(s):
        out.write(s)

    # Make socket
    transport = TSocket.TSocket(thrift_ip, thrift_port)
    # Buffering is critical. Raw sockets are very slow
    transport = TTransport.TBufferedTransport(transport)
    # Wrap in a protocol
    bprotocol = TBinaryProtocol.TBinaryProtocol(transport)

    clients = []

    for service_name, service_cls in services:
        if service_name is None:
            clients.append(None)
            continue
        protocol = TMultiplexedProtocol.TMultiplexedProtocol(bprotocol, service_name)
        client = service_cls(protocol)
        clients.append(client)

    # Connect!
    try:
        transport.open()
    except TTransport.TTransportException:
        my_print("Could not connect to thrift client on port {}\n".format(
            thrift_port))
        my_print("Make sure the switch is running ")
        my_print("and that you have the right port\n")
        sys.exit(1)

    return clients

def thrift_connect_standard(thrift_ip, thrift_port, out=sys.stdout):
    from bm_runtime.standard import Standard
    return thrift_connect(thrift_ip, thrift_port,
                          [("standard", Standard.Client)], out)[0]
