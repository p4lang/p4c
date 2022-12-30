import argparse
import subprocess
import os
import sys
import socket
import random
from pathlib import Path

FILE_DIR = Path(__file__).parent.resolve()
TOOLS_PATH = FILE_DIR.joinpath("../../tools")
sys.path.append(str(TOOLS_PATH))
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


def processKill():
    kill = "pkill --signal 9 --list-name ptf"
    with subprocess.Popen(kill,
                          shell=True,
                          stdin=subprocess.PIPE,
                          universal_newlines=True) as _:
        pass

    print("Waiting 2 seconds before killing simple_switch_grpc ...")
    kill = "pkill --signal 9 --list-name simple_switch"
    with subprocess.Popen(kill,
                          shell=True,
                          stdin=subprocess.PIPE,
                          universal_newlines=True) as _:
        pass
    print(
        "Verifying that there are no simple_switch_grpc processes running any longer..."
    )
    ps = "ps axguwww | grep simple_switch"
    with subprocess.Popen(ps,
                          shell=True,
                          stdin=subprocess.PIPE,
                          universal_newlines=True) as _:
        pass


def run_test(options):
    """Define the test environment and compile the p4 target
    Optional: Run the generated model"""
    assert isinstance(options, Options)
    print("---------------------- Start p4c-bm2-ss ----------------------")
    p4c_bm2_ss = (
        f"{options.rootDir}/build/p4c-bm2-ss --target bmv2 --arch v1model --p4runtime-files {options.infoName} {options.p4Filename} -o {options.jsonName}"
    )
    p = subprocess.Popen(p4c_bm2_ss,
                         shell=True,
                         stdin=subprocess.PIPE,
                         universal_newlines=True)

    p.communicate()
    if p.returncode != 0:
        p.kill()
        print(
            "---------------------- End p4c-bm2-ss with errors ----------------------"
        )
        raise SystemExit("p4c-bm2-ss ended with errors")

    # Remove the log file.
    with subprocess.Popen("/bin/rm -f ss-log.txt",
                          shell=True,
                          stdin=subprocess.PIPE,
                          universal_newlines=True) as _:
        pass

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
    print(
        "---------------------- Start simple_switch_grpc ----------------------"
    )
    simple_switch_grpc = f"simple_switch_grpc --thrift-port {thrift_port} --log-console -i 0@veth0 -i 1@veth2 -i 2@veth4 -i 3@veth6 -i 4@veth8 -i 5@veth10 -i 6@veth12 -i 7@veth14 --no-p4 -- --grpc-server-addr localhost:{grpc_port}  "
    simple_switch_grpcP = subprocess.Popen(simple_switch_grpc,
                                           shell=True,
                                           stdin=subprocess.PIPE,
                                           universal_newlines=True)
    if simple_switch_grpcP.poll() is not None:
        processKill()
        print(
            "---------------------- End simple_switch_grpc with errors ----------------------"
        )
        raise SystemExit("simple_switch_grpc ended with errors")
    print("---------------------- Start ptf ----------------------")

    # Add the file location to the python path.
    pypath = FILE_DIR
    # Show list of the tests
    with subprocess.Popen(
            f"ptf --pypath {pypath} --test-dir {options.testdir} --list",
            shell=True,
            stdin=subprocess.PIPE,
            universal_newlines=True,
    ) as ptfTestList:
        ptfTestList.communicate()
    ifaces = "-i 0@veth0 -i 1@veth2 -i 2@veth4 -i 3@veth6 -i 4@veth8 -i 5@veth10 -i 6@veth12 -i 7@veth14"
    test_params = f"grpcaddr='localhost:{grpc_port}';p4info='{options.infoName}';config='{options.jsonName}';"
    ptf = f'ptf --verbose --pypath {pypath} {ifaces} --test-params="{test_params}" --test-dir {options.testdir}'
    ptfP = subprocess.Popen(ptf,
                            shell=True,
                            stdin=subprocess.PIPE,
                            universal_newlines=True)
    timeout = 3
    ptfP.communicate()
    while True:
        poll = (
            ptfP.poll()
        )  # returns the exit code or None if the process is still running
        timeout -= 0.5
        if timeout <= 0:
            break
        if poll is not None:
            break
    if poll is not None and poll != 0:
        print(
            "---------------------- End ptf with errors ----------------------"
        )
        processKill()
        raise SystemExit("PTF ended with errors")

    print("---------------------- End ptf ----------------------")
    processKill()


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
