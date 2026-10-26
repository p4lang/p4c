#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2026 The P4 Language Consortium
#
# SPDX-License-Identifier: Apache-2.0
"""Run the PSA-eBPF PTF tests through the PTF library (ptf.runner), not
through the ptf binary.

test.sh calls this helper. The arguments carry the test parameters and
the PTF interfaces that test.sh passed on the ptf command line before."""

import argparse
import os
import sys

from ptf import runner

PARSER = argparse.ArgumentParser()
PARSER.add_argument(
    "--test-dir",
    dest="test_dir",
    default="ptf/",
    help="Directory containing the PTF tests.",
)
PARSER.add_argument(
    "--interface",
    "-i",
    dest="interfaces",
    metavar="IFACE",
    action="append",
    default=[],
    help="A port number and a dataplane interface, for example 0@s1-eth0.",
)
PARSER.add_argument(
    "--namespace",
    dest="namespace",
    default="switch",
    help="The network namespace that holds the switch (PSA-eBPF).",
)
PARSER.add_argument(
    "--switch-interfaces",
    dest="switch_interfaces",
    metavar="LIST",
    required=True,
    help="Comma-separated list of the dataplane ports of the switch, for "
    "example 'psa_recirc,eth0,eth1'. This helper passes the list to the "
    "tests as the 'interfaces' test parameter.",
)
PARSER.add_argument(
    "--trace",
    choices=["True", "False"],
    default="False",
    help="Whether the P4 programs were built with tracing logs.",
)
PARSER.add_argument(
    "--xdp",
    choices=["True", "False"],
    required=True,
    help="Whether the XDP hook is used.",
)
PARSER.add_argument(
    "--xdp2tc",
    choices=["meta", "head", "cpumap"],
    required=True,
    help="The mode that passes metadata from XDP to TC programs.",
)
PARSER.add_argument("test_specs", nargs="*", help="Tests / Groups to run")


def make_ptf_config(args) -> runner.PtfConfig:
    """Construct the PTF configuration from the script arguments."""
    interfaces = []
    for value in args.interfaces:
        dev_and_port, interface = value.split("@", 1)
        dev_and_port = dev_and_port.split("-")
        if len(dev_and_port) == 1:
            dev, port = 0, int(dev_and_port[0])
        else:
            dev, port = int(dev_and_port[0]), int(dev_and_port[1])
        interfaces.append(runner.Interface(device=dev, port=port, interface=interface))
    return runner.PtfConfig(
        test_selection=runner.TestSelectionOptions(
            test_dir=args.test_dir,
            test_specs=list(args.test_specs),
        ),
        platform=runner.PlatformOptions(interfaces=interfaces),
        test_behavior=runner.TestBehaviorOptions(
            test_params={
                "interfaces": args.switch_interfaces,
                "namespace": args.namespace,
                "trace": args.trace,
                "xdp": args.xdp,
                "xdp2tc": args.xdp2tc,
            },
        ),
    )


if __name__ == "__main__":
    ARGS = PARSER.parse_args()
    config = make_ptf_config(ARGS)
    try:
        RESULT = runner.run(
            config,
            output=runner.RunOutput(capture_root_logging=True),
        )
    except runner.PtfError as error:
        print("PTF error: %s" % error, file=sys.stderr)
        RESULT = 1
    # Flush the output and terminate the process directly. A test may
    # leave non-daemon threads behind. These threads can block a normal
    # exit. The ptf binary uses the same exit strategy.
    sys.stdout.flush()
    sys.stderr.flush()
    os._exit(RESULT)
