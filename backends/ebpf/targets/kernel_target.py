#!/usr/bin/env python3
# Copyright 2013-present Barefoot Networks, Inc.
# Copyright 2018 VMware, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import os
import subprocess
import sys
import time
from glob import glob
from pathlib import Path
from typing import Optional

from .ebpfenv import Bridge
from .target import EBPFTarget

# path to the tools folder of the compiler
# Append tools to the import path.
FILE_DIR = Path(__file__).resolve().parent
# Append tools to the import path.
sys.path.append(str(FILE_DIR.joinpath("../../../tools")))
import testutils


class Target(EBPFTarget):
    EBPF_PATH = Path("/sys/fs/bpf")

    def __init__(self, tmpdir, options, template):
        EBPFTarget.__init__(self, tmpdir, options, template)
        self.bpftool = self.runtimedir.joinpath("install/bpftool")
        if self.options.target == "xdp":
            self.ebpf_map_path = self.EBPF_PATH.joinpath(f"xdp/globals")
        else:
            self.ebpf_map_path = self.EBPF_PATH.joinpath(f"tc/globals")

    def compile_dataplane(self):
        # Use clang to compile the generated C code to a LLVM IR
        args = "make "
        # target makefile
        args += f"-f {self.options.target}.mk "
        # Source folder of the makefile
        args += f"-C {self.runtimedir} "
        # Input eBPF byte code
        args += f"{self.template}.o "
        # The bpf program to attach to the interface
        args += f"BPFOBJ={self.template}.o "
        # add the folder local to the P4 file to the list of includes
        args += f" INCLUDES+=-I{os.path.dirname(self.options.p4filename)}"
        if self.options.extern:
            # we inline the extern so we need a direct include
            args += f" INCLUDES+=-include{self.options.extern} "
            # need to include the temporary dir because of the tmp import
            args += f" INCLUDES+=-I{self.tmpdir} "
        result = testutils.exec_process(args)
        if result.returncode != testutils.SUCCESS:
            testutils.log.error("Failed to compile the eBPF byte code")
        return result.returncode

    def _create_runtime(self):
        args = self.get_make_args(self.runtimedir, self.options.target)
        # List of bpf programs to attach to the interface
        args += f"BPFOBJ={self.template} "
        args += "CFLAGS+=-DCONTROL_PLANE "
        # add the folder local to the P4 file to the list of includes
        args += f"INCLUDES+=-I{os.path.dirname(self.options.p4filename)} "
        args += f"LIBS+={self.runtimedir}/install/libbpf/libbpf.a "
        args += "LIBS+=-lz "
        args += "LIBS+=-lelf "
        result = testutils.exec_process(args)
        if result.returncode != testutils.SUCCESS:
            testutils.log.error("Failed to build the filter")
        return result.returncode

    def _create_bridge(self) -> Optional[Bridge]:
        # The namespace is the id of the process
        namespace = str(os.getpid())
        # Number of input files
        direction = "in"
        num_files = len(glob(self.filename("*", direction)))
        # Create the namespace and the bridge with all its ports
        bridge = Bridge(namespace)
        result = bridge.create_virtual_env(num_files)
        if result != testutils.SUCCESS:
            bridge.ns_del()
            return None
        if self.options.target != "xdp":
            # Add the qdisc. MUST be clsact layer.
            for port_name in bridge.edge_ports:
                result = bridge.ns_exec(f"tc qdisc add dev {port_name} clsact")
                if result != testutils.SUCCESS:
                    return None
        return bridge

    def _get_run_cmd(self):
        direction = "in"
        pcap_pattern = self.filename("", direction)
        num_files = len(glob(self.filename("*", direction)))
        testutils.log.info("Input file: %s", pcap_pattern)
        # Main executable
        cmd = self.template + " "
        # Input pcap pattern
        cmd += "-f " + pcap_pattern + " "
        # Number of input interfaces
        cmd += "-n " + str(num_files) + " "
        # Debug flag (verbose output)
        cmd += "-d"
        return cmd

    def _kill_processes(self, procs) -> None:
        for proc in procs:
            # kill process, 15 is SIGTERM
            os.kill(proc.pid, 15)

    def _attach_filters(self, bridge: Bridge, proc: subprocess.Popen) -> int:
        # Is this a XDP or TC (ebpf_filter) program?
        p_result = testutils.exec_process(f"objdump -hj xdp {self.template}.o").returncode
        is_xdp = p_result == testutils.SUCCESS
        # Load the specified eBPF object to "port_name" egress
        # As a side-effect, this may create maps in /sys/fs/bpf/
        # Get the command to load eBPF code to all the attached ports
        result = bridge.ns_proc_write(proc, f"mount -t bpf none {self.EBPF_PATH}")
        if result != testutils.SUCCESS:
            return result
        result = bridge.ns_proc_append(proc, f"mkdir -p {self.ebpf_map_path}")
        if result != testutils.SUCCESS:
            return result
        load_type = "xdp" if is_xdp else "tc"
        cmd = f"{self.bpftool} prog load {self.template}.o {self.EBPF_PATH}/ebpf_filter pinmaps {self.ebpf_map_path} type {load_type}"
        result = bridge.ns_proc_append(proc, cmd)
        if result != testutils.SUCCESS:
            return result

        attach_type = "xdp" if is_xdp else "tcx_egress"
        ports = bridge.br_ports if is_xdp else bridge.edge_ports
        if len(ports) > 0:
            for port_name in ports:
                result = bridge.ns_proc_append(
                    proc,
                    f"{self.bpftool} net attach {attach_type} pinned {self.EBPF_PATH}/ebpf_filter dev {port_name}",
                )
                if result != testutils.SUCCESS:
                    return result
        else:
            # No ports attached (no pcap files), load to bridge instead
            result = bridge.ns_proc_append(
                proc,
                f"{self.bpftool} net attach {attach_type} pinned {self.EBPF_PATH}/ebpf_filter dev {bridge.br_name}",
            )

        return result

    def _run_tcpdump(self, bridge, filename, port):
        cmd = f"{bridge.get_ns_prefix()} tcpdump -w {filename} -i {port}"
        return subprocess.Popen(cmd.split())

    def _init_tcpdump_listeners(self, bridge):
        # Listen to packets with tcpdump on all the ports of the bridge
        dump_procs = []
        for i, port in enumerate(bridge.br_ports):
            outfile_name = self.filename(i, "out")
            dump_procs.append(self._run_tcpdump(bridge, outfile_name, port))
        # Wait for tcpdump to initialise
        time.sleep(2)
        return dump_procs

    def _run_in_namespace(self, bridge):
        # Open a process in the new namespace
        proc = bridge.ns_proc_open()
        if not proc:
            return testutils.FAILURE
        dump_procs = self._init_tcpdump_listeners(bridge)
        result = self._attach_filters(bridge, proc)
        if result != testutils.SUCCESS:
            return result
        # Finally, append the actual runtime command to the process
        result = bridge.ns_proc_append(proc, self._get_run_cmd())
        if result != testutils.SUCCESS:
            return result
        # Execute the command queue and close the process, retrieve result
        result = bridge.ns_proc_close(proc)
        # Kill tcpdump but let it finish writing packets
        self._kill_processes(dump_procs)
        time.sleep(2)
        return result

    def run(self):
        # Root is necessary to load ebpf into the kernel
        if not testutils.check_root():
            testutils.log.warning("This test requires root privileges; skipping execution.")
            return testutils.SKIPPED
        result = self._create_runtime()
        if result != testutils.SUCCESS:
            return result
        # Create the namespace and the central testing bridge
        bridge = self._create_bridge()
        if not bridge:
            return testutils.FAILURE
        # Run the program in the generated namespace
        result = self._run_in_namespace(bridge)
        bridge.ns_del()
        return result
