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
import signal
import sys
import threading
import queue

parser = argparse.ArgumentParser(description='Mininet demo')
parser.add_argument('--grpc-addr', help='P4Runtime gRPC server address',
                    type=str, action="store", default='localhost:50051')

args = parser.parse_args()

def main():
    channel = grpc.insecure_channel(args.grpc_addr)
    stub = gnmi_pb2.gNMIStub(channel)

    stream_out_q = queue.Queue()
    stream_in_q = queue.Queue()

    def req_iterator():
        while True:
            req = stream_out_q.get()
            if req is None:
                break
            print("***************************")
            print("REQUEST")
            print(req)
            print("***************************")
            yield req

    def stream_recv(stream):
        for response in stream:
            print("***************************")
            print("RESPONSE")
            print(response)
            print("***************************")
            stream_in_q.put(response)

    stream = stub.Subscribe(req_iterator())
    stream_recv_thread = threading.Thread(
        target=stream_recv, args=(stream,))
    stream_recv_thread.start()

    req = gnmi_pb2.SubscribeRequest()
    subList = req.subscribe
    subList.mode = gnmi_pb2.SubscriptionList.STREAM
    subList.updates_only = True
    sub = subList.subscription.add()
    sub.mode = gnmi_pb2.ON_CHANGE
    path = sub.path
    for name in ["interfaces", "interface", "..."]:
        e = path.elem.add()
        e.name = name

    stream_out_q.put(req)

    try:
        while True:
            sleep(1)
    except KeyboardInterrupt:
        stream_out_q.put(None)
        stream_recv_thread.join()

if __name__ == '__main__':
    main()
