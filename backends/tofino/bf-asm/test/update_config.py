#!/usr/bin/env python3

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
from p4 import p4runtime_pb2
from p4.config import p4info_pb2
from p4.tmp import p4config_pb2
import google.protobuf.text_format
import struct

parser = argparse.ArgumentParser(description='Mininet demo')
parser.add_argument('--dev-id', help='Device id of switch',
                    type=int, action="store", default=0)
parser.add_argument('--p4info', help='text p4info proto',
                    type=str, action="store", required=True)
parser.add_argument('--tofino-bin', help='tofino bin',
                    type=str, action="store", required=True)
parser.add_argument('--cxt-json', help='context json',
                    type=str, action="store", required=True)

args = parser.parse_args()


def main():
    channel = grpc.insecure_channel('localhost:50051')
    stub = p4runtime_pb2.P4RuntimeStub(channel)

    print("Sending P4 config")
    request = p4runtime_pb2.SetForwardingPipelineConfigRequest()
    request.device_id = 0
    config = request.config
    with open(args.p4info) as p4info_f:
        google.protobuf.text_format.Merge(p4info_f.read(), config.p4info)
    device_config = p4config_pb2.P4DeviceConfig()
    with open(args.tofino_bin) as tofino_bin_f:
        with open(args.cxt_json) as cxt_json_f:
            device_config.device_data = ""
            prog_name = "easy"
            device_config.device_data += struct.pack("<i", len(prog_name))
            device_config.device_data += prog_name
            tofino_bin = tofino_bin_f.read()
            device_config.device_data += struct.pack("<i", len(tofino_bin))
            device_config.device_data += tofino_bin
            cxt_json = cxt_json_f.read()
            device_config.device_data += struct.pack("<i", len(cxt_json))
            device_config.device_data += cxt_json
    config.p4_device_config = device_config.SerializeToString()
    request.action = p4runtime_pb2.SetForwardingPipelineConfigRequest.VERIFY_AND_COMMIT
    # print request
    response = stub.SetForwardingPipelineConfig(request)

if __name__ == '__main__':
    main()
