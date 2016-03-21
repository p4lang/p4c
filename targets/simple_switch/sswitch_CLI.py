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

import runtime_CLI

import sys
import os

from sswitch_runtime import SimpleSwitch

class SimpleSwitchAPI(runtime_CLI.RuntimeAPI):
    @staticmethod
    def get_thrift_services():
        return [("simple_switch", SimpleSwitch.Client)]

    def __init__(self, pre_type, standard_client, mc_client, sswitch_client):
        runtime_CLI.RuntimeAPI.__init__(self, pre_type,
                                        standard_client, mc_client)
        self.sswitch_client = sswitch_client

    def do_set_queue_depth(self, line):
        "Set depth of egress queue: set_queue_depth <nb_pkts>"
        depth = int(line)
        self.sswitch_client.set_egress_queue_depth(depth)

    def do_set_queue_rate(self, line):
        "Set rate of egress queue: set_queue_rate <rate_pps>"
        rate = int(line)
        self.sswitch_client.set_egress_queue_rate(rate)

    def do_mirroring_add(self, line):
        "Add mirroring mapping: mirroring_add <mirror_id> <egress_port>"
        args = line.split()
        mirror_id, egress_port = int(args[0]), int(args[1])
        self.sswitch_client.mirroring_mapping_add(mirror_id, egress_port)

    def do_mirroring_delete(self, line):
        "Delete mirroring mapping: mirroring_delete <mirror_id>"
        mirror_id = int(line)
        self.sswitch_client.mirroring_mapping_delete(mirror_id)

def main():
    args = runtime_CLI.get_parser().parse_args()

    args.pre = runtime_CLI.PreType.SimplePreLAG

    runtime_CLI.load_json(args.json)

    services = runtime_CLI.RuntimeAPI.get_thrift_services(args.pre)
    services.extend(SimpleSwitchAPI.get_thrift_services())

    standard_client, mc_client, sswitch_client = runtime_CLI.thrift_connect(
        args.thrift_ip, args.thrift_port, services
    )

    SimpleSwitchAPI(args.pre, standard_client, mc_client, sswitch_client).cmdloop()

if __name__ == '__main__':
    main()
