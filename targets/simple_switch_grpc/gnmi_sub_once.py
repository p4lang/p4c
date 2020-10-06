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

import argparse
from time import sleep

import grpc
from gnmi import gnmi_pb2
import google.protobuf.text_format
import struct

parser = argparse.ArgumentParser(description='Mininet demo')
parser.add_argument('--grpc-addr', help='P4Runtime gRPC server address',
                    type=str, action="store", default='localhost:50051')

args = parser.parse_args()

def main():
    channel = grpc.insecure_channel(args.grpc_addr)
    stub = gnmi_pb2.gNMIStub(channel)

    def req_iterator():
        while True:
            req = gnmi_pb2.SubscribeRequest()
            subList = req.subscribe
            subList.mode = gnmi_pb2.SubscriptionList.ONCE
            sub = subList.subscription.add()
            path = sub.path
            for name in ["interfaces", "interface", "..."]:
                e = path.elem.add()
                e.name = name
            print("***************************")
            print("REQUEST")
            print(req)
            print("***************************")
            yield req
            return

    for response in stub.Subscribe(req_iterator()):
        print("***************************")
        print("RESPONSE")
        print(response)
        print("***************************")

if __name__ == '__main__':
    main()
