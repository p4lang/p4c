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

import runtime_CLI
from runtime_CLI import UIn_Error

from functools import wraps
import os
import sys

from sswitch_runtime import SimpleSwitch
from sswitch_runtime.ttypes import *

def handle_bad_input(f):
    @wraps(f)
    @runtime_CLI.handle_bad_input
    def handle(*args, **kwargs):
        try:
            return f(*args, **kwargs)
        except InvalidMirroringOperation as e:
            error = MirroringOperationErrorCode._VALUES_TO_NAMES[e.code]
            print "Invalid mirroring operation (%s)" % error
    return handle

class SimpleSwitchAPI(runtime_CLI.RuntimeAPI):
    @staticmethod
    def get_thrift_services():
        return [("simple_switch", SimpleSwitch.Client)]

    def __init__(self, pre_type, standard_client, mc_client, sswitch_client):
        runtime_CLI.RuntimeAPI.__init__(self, pre_type,
                                        standard_client, mc_client)
        self.sswitch_client = sswitch_client

    def parse_int(self, arg, name):
        try:
            return int(arg)
        except:
            raise UIn_Error("Bad format for {}, expected integer".format(name))

    @handle_bad_input
    def do_set_queue_depth(self, line):
        "Set depth of one / all egress queue(s): set_queue_depth <nb_pkts> [<egress_port>]"
        args = line.split()
        self.at_least_n_args(args, 1)
        depth = self.parse_int(args[0], "nb_pkts")
        if len(args) > 1:
            port = self.parse_int(args[1], "egress_port")
            self.sswitch_client.set_egress_queue_depth(port, depth)
        else:
            self.sswitch_client.set_all_egress_queue_depths(depth)

    @handle_bad_input
    def do_set_queue_rate(self, line):
        "Set rate of one / all egress queue(s): set_queue_rate <rate_pps> [<egress_port>]"
        args = line.split()
        self.at_least_n_args(args, 1)
        rate = self.parse_int(args[0], "rate_pps")
        if len(args) > 1:
            port = self.parse_int(args[1], "egress_port")
            self.sswitch_client.set_egress_queue_rate(port, rate)
        else:
            self.sswitch_client.set_all_egress_queue_rates(rate)

    @handle_bad_input
    def do_mirroring_add(self, line):
        "Add mirroring session to unicast port: mirroring_add <mirror_id> <egress_port>"
        args = line.split()
        self.exactly_n_args(args, 2)
        mirror_id = self.parse_int(args[0], "mirror_id")
        egress_port = self.parse_int(args[1], "egress_port")
        config = MirroringSessionConfig(port=egress_port)
        self.sswitch_client.mirroring_session_add(mirror_id, config)

    @handle_bad_input
    def do_mirroring_add_mc(self, line):
        "Add mirroring session to multicast group: mirroring_add_mc <mirror_id> <mgrp>"
        args = line.split()
        self.exactly_n_args(args, 2)
        mirror_id = self.parse_int(args[0], "mirror_id")
        mgrp = self.parse_int(args[1], "mgrp")
        config = MirroringSessionConfig(mgid=mgrp)
        self.sswitch_client.mirroring_session_add(mirror_id, config)

    @handle_bad_input
    def do_mirroring_delete(self, line):
        "Delete mirroring session: mirroring_delete <mirror_id>"
        args = line.split()
        self.exactly_n_args(args, 1)
        mirror_id = self.parse_int(args[0], "mirror_id")
        self.sswitch_client.mirroring_session_delete(mirror_id)

    @handle_bad_input
    def do_mirroring_get(self, line):
        "Display mirroring session: mirroring_get <mirror_id>"
        args = line.split()
        self.exactly_n_args(args, 1)
        mirror_id = self.parse_int(args[0], "mirror_id")
        config = self.sswitch_client.mirroring_session_get(mirror_id)
        print config

    @handle_bad_input
    def do_get_time_elapsed(self, line):
        "Get time elapsed (in microseconds) since the switch started: get_time_elapsed"
        print self.sswitch_client.get_time_elapsed_us()

    @handle_bad_input
    def do_get_time_since_epoch(self, line):
        "Get time elapsed (in microseconds) since the switch clock's epoch: get_time_since_epoch"
        print self.sswitch_client.get_time_since_epoch_us()

def main():
    args = runtime_CLI.get_parser().parse_args()

    args.pre = runtime_CLI.PreType.SimplePreLAG

    services = runtime_CLI.RuntimeAPI.get_thrift_services(args.pre)
    services.extend(SimpleSwitchAPI.get_thrift_services())

    standard_client, mc_client, sswitch_client = runtime_CLI.thrift_connect(
        args.thrift_ip, args.thrift_port, services
    )

    runtime_CLI.load_json_config(standard_client, args.json)

    SimpleSwitchAPI(args.pre, standard_client, mc_client, sswitch_client).cmdloop()

if __name__ == '__main__':
    main()
