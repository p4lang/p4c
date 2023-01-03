import argparse
import subprocess
import os
import sys
import socket
import random
import time
from pathlib import Path
from datetime import datetime

FILE_DIR = Path(__file__).parent.resolve()
TOOLS_PATH = FILE_DIR.joinpath("../../tools")
sys.path.append(str(TOOLS_PATH))

BRIDGE_PATH = FILE_DIR.joinpath("../ebpf/targets")
sys.path.append(str(BRIDGE_PATH))
from ebpfenv import Bridge
import testutils as p4c_utils

PARSER = argparse.ArgumentParser()
PARSER.add_argument("rootdir",
                    help="the root directory of "
                    "the compiler source tree")
PARSER.add_argument("-pfn", dest="p4Filename", help="the p4 file to process")
PARSER.add_argument(
    "-tf",
    "--testfile",
    dest="testfile",
    help="Provide the path for the ptf py file for this test. ",
)


def is_port_in_use(port: int) -> bool:
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        return s.connect_ex(('localhost', port)) == 0


class Options:

    def __init__(self):
        self.p4Filename = ""  # File that is being compiled
        self.testfile = ""  # path to ptf test file that is used
        # Actual location of the test framework
        self.testdir = os.path.dirname(os.path.realpath(__file__))
        self.jsonName = "test.json"
        self.infoName = "test.p4info.txt"
        self.rootDir = "."


def _create_bridge():
    print("---------------------- Start creating of bridge ----------------------")
    random.seed(datetime.now().timestamp())
    outputs = {}
    outputs["stdout"] = sys.stdout
    outputs["stderr"] = sys.stderr
    bridge = Bridge(random.random(), outputs, True)
    result = bridge.create_virtual_env(8)
    if result != p4c_utils.SUCCESS:
        bridge.ns_del()
        print("---------------------- End creating of bridge with errors ----------------------")
        exit()
    print("---------------------- Bridge created ----------------------")
    return bridge


def _open_proc(bridge):
    proc = bridge.ns_proc_open()
    if not proc:
        bridge.ns_del()
        print("---------------------- Unable to open bash process in the namespace ----------------------")
        exit()
    return proc


def _kill_process(proc):
    # kill process
    os.kill(proc.pid, 15)


def _create_runtime(bridge):
    print("---------------------- Start p4c-bm2-ss ----------------------")
    proc = _open_proc(bridge)
    p4c_bm2_ss = (
        f"{options.rootDir}/build/p4c-bm2-ss --target bmv2 --arch v1model --p4runtime-files {options.infoName} {options.p4Filename} -o {options.jsonName}"
    )
    result = bridge.ns_proc_write(proc, p4c_bm2_ss)
    if result != p4c_utils.SUCCESS:
        _kill_process(proc)
        print(
            "---------------------- End p4c-bm2-ss with errors ----------------------"
        )
        raise SystemExit("p4c-bm2-ss ended with errors")
    bridge.ns_proc_close(proc)
    return bridge

def _run_proc_in_backgraund(bridge, cmd):
    namedCmd = bridge.get_ns_prefix() + " " + cmd
    return subprocess.Popen(namedCmd.split())

def _run_simple_switch_grpc(bridge, thrift_port, grpc_port):
    proc = _open_proc(bridge)
    # Remove the log file.
    removeLogFileCmd = "/bin/rm -f ss-log.txt"
    bridge.ns_proc_write(proc, removeLogFileCmd)

    print(
        "---------------------- Start simple_switch_grpc ----------------------"
    )

    bridge.ns_proc_close(proc)
    simple_switch_grpc = f"simple_switch_grpc --thrift-port {thrift_port} --log-console -i 0@0 -i 1@1 -i 2@2 -i 3@3 -i 4@4 -i 5@5 -i 6@6 -i 7@7 --no-p4 -- --grpc-server-addr localhost:{grpc_port}"
    
    switchProc = _run_proc_in_backgraund(bridge, simple_switch_grpc)
    # Wait for simple_switch_grpc to initialise
    time.sleep(2)
    if switchProc.poll() is not None:
        _kill_process(switchProc)
        print(
            "---------------------- End simple_switch_grpc with errors ----------------------"
        )
        raise SystemExit("simple_switch_grpc ended with errors")
    
    return [bridge, switchProc]


def _run_ptf(bridge, grpc_port):
    proc = _open_proc(bridge)
    print("---------------------- Start ptf ----------------------")
    # Add the file location to the python path.
    pypath = FILE_DIR
    # Show list of the tests
    testLostCmd = f"ptf --pypath {pypath} --test-dir {options.testdir} --list"
    bridge.ns_proc_write(proc,testLostCmd)
    bridge.ns_proc_close(proc)
    ifaces = "-i 0@br_0 -i 1@br_1 -i 2@br_2 -i 3@br_3 -i 4@br_4 -i 5@br_5 -i 6@br_6 -i 7@br_7"
    test_params = f"grpcaddr='localhost:{grpc_port}';p4info='{options.infoName}';config='{options.jsonName}';"
    ptf = f'ptf --verbose --pypath {pypath} {ifaces} --test-params="{test_params}" --test-dir {options.testdir}'
    ptfProc  = _run_proc_in_backgraund(bridge, ptf)
    timeout = 3
    ptfProc.communicate()
    while True:
        poll = (
            ptfProc.poll()
        )  # returns the exit code or None if the process is still running
        timeout -= 0.5
        if timeout <= 0:
            break
        if poll is not None:
            break
    if poll is not None and poll != p4c_utils.SUCCESS:
        print(
            "---------------------- End ptf with errors ----------------------"
        )
        _kill_process(ptfProc)
        raise SystemExit("PTF ended with errors")


    _kill_process(ptfProc)
    print("---------------------- End ptf ----------------------")

    return bridge

def _pick_port():
    # Pick an available port.
    grpc_port_used = True
    grpc_port = 28000
    while grpc_port_used:
        grpc_port = random.randrange(1024, 65535)
        grpc_port_used = is_port_in_use(grpc_port)
    thrift_port_used = True
    thrift_port = 28000
    while thrift_port_used or thrift_port == grpc_port:
        thrift_port = random.randrange(1024, 65535)
        thrift_port_used = is_port_in_use(thrift_port)

    return [thrift_port, grpc_port]

def run_test(options):
    """Define the test environment and compile the p4 target
    Optional: Run the generated model"""
    assert isinstance(options, Options)
    bridge = _create_bridge()
    bridge = _create_runtime(bridge)
    thrift_port, grpc_port = _pick_port()
    
    bridge, switchProc = _run_simple_switch_grpc(bridge, thrift_port, grpc_port)
    bridge = _run_ptf(bridge, grpc_port)


    _kill_process(switchProc)
    print("---------------------- End simple_switch_grpc ----------------------")
    bridge.ns_del()


if __name__ == "__main__":
    # Parse options and process argv
    args, argv = PARSER.parse_known_args()
    options = Options()
    options.p4Filename = args.p4Filename
    options.testfile = args.testfile
    testdir = args.testfile.split("/")
    partOfTestName = testdir[-1].replace(".py", "")
    del testdir[-1]
    testdir = "/".join(testdir)
    options.testdir = testdir
    options.jsonName = testdir + "/" + partOfTestName + ".json"
    options.infoName = testdir + "/" + partOfTestName + ".p4info.txt"
    options.rootDir = args.rootdir
    # Run the test with the extracted options
    run_test(options)
