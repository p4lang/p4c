import runtime_CLI

import sys
import os
_THIS_DIR = os.path.dirname(os.path.realpath(__file__))
sys.path.append(os.path.join(_THIS_DIR, "gen-py"))

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
