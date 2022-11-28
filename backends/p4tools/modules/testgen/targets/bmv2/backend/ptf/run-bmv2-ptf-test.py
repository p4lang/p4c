import argparse
import subprocess
import os
import sys
import time
import ptf.testutils as tu

PARSER = argparse.ArgumentParser()
PARSER.add_argument("rootdir", help="the root directory of "
                    "the compiler source tree")
PARSER.add_argument("-pfn", dest="p4Filename", help="the p4 file to process")
PARSER.add_argument("-tf", "--testfile", dest="testfile",
                        help="Provide the path for the ptf py file for this test. ")


class Options(object):
    def __init__(self):
        self.p4Filename = ""            # File that is being compiled
        self.testfile = ""              # path to ptf test file that is used
        # Actual location of the test framework
        self.testdir = os.path.dirname(os.path.realpath(__file__))
        self.jsonName = "test.json"
        self.infoName = "test.p4info.txt"

def processKill():
    kill = 'pkill --signal 9 --list-name ptf'
    killP = subprocess.Popen(kill, shell=True,
                stdin=subprocess.PIPE,universal_newlines=True)
    time.sleep(2)
    
    print("Waiting 2 seconds before killing simple_switch_grpc ...")
    kill = 'pkill --signal 9 --list-name simple_switch'
    killP = subprocess.Popen(kill, shell=True,
                stdin=subprocess.PIPE,universal_newlines=True)
    time.sleep(4)
    print("Verifying that there are no simple_switch_grpc processes running any longer in 4 seconds ...")
    ps = 'ps axguwww | grep simple_switch'
    psP = subprocess.Popen(ps, shell=True,
                stdin=subprocess.PIPE,universal_newlines=True)

def run_test(options):
    """ Define the test environment and compile the p4 target
        Optional: Run the generated model """
    assert isinstance(options, Options)
    print("---------------------- Start p4c-bm2-ss ----------------------")
    p4c_bm2_ss = "p4c-bm2-ss --target bmv2 --arch v1model --p4runtime-files " + options.jsonName + "," + options.infoName + " " + options.p4Filename
    p = subprocess.Popen(p4c_bm2_ss,shell=True,stdin=subprocess.PIPE,
                universal_newlines=True)
    
    p.communicate()
    if p.returncode != 0 :
        p.kill()
        print("---------------------- End p4c-bm2-ss with errors ----------------------")
        raise SystemExit('p4c-bm2-ss ended with errors')
    else :
        p.kill()
        print("---------------------- End p4c-bm2-ss ----------------------")

    print("---------------------- Start simple_switch_grpc ----------------------")
    log = "/bin/rm -f ss-log.txt"
    simple_switch_grpc = "simple_switch_grpc --log-file ss-log --log-flush --dump-packet-data 10000 -i 0@veth0 -i 1@veth2 -i 2@veth4 -i 3@veth6 -i 4@veth8 -i 5@veth10 -i 6@veth12 -i 7@veth14 --no-p4" 
    logP = subprocess.Popen(log,shell=True,stdin=subprocess.PIPE,universal_newlines=True)
    simple_switch_grpcP = subprocess.Popen(simple_switch_grpc,shell=True,stdin=subprocess.PIPE,
                                            universal_newlines=True)
    time.sleep(2)
    if simple_switch_grpcP.poll() is not None:
        processKill()
        time.sleep(2)
        print("---------------------- End simple_switch_grpc with errors ----------------------")
        raise SystemExit('simple_switch_grpc ended with errors')
    print("---------------------- Start ptf ----------------------")
    pypath = 'T="`realpath ../../testlib`"; if [ x"" = "x" ]; then P=""; else P=":"; fi;'
    ptf = pypath + 'ptf --pypath "$P" -i 0@veth1 -i 1@veth3 -i 2@veth5 -i 3@veth7 -i 4@veth9 -i 5@veth11 -i 6@veth13 -i 7@veth15 --test-params="grpcaddr=\'localhost:9559\';p4info=\''+options.infoName+'\';config=\''+options.jsonName+ '\';" --test-dir ' + options.testDir
    ptfP = subprocess.Popen(ptf, shell=True,
                stdin=subprocess.PIPE,universal_newlines=True)
    timeout = 3
    ptfP.communicate()
    while True:
        poll = ptfP.poll()  # returns the exit code or None if the process is still running
        time.sleep(6)  # here we could actually do something useful
        timeout -= 0.5
        if timeout <= 0:
           break
        if poll is not None:
           break
    if poll is not None and poll != 0:
        print("---------------------- End ptf with errors ----------------------")
        processKill()
        time.sleep(2)
        raise SystemExit('PTF ended with errors')
         
    else:
        print("---------------------- End ptf ----------------------")
    processKill()
    

if __name__ == "__main__":
    # Parse options and process argv
    args, argv = PARSER.parse_known_args()
    options = Options()
    options.p4Filename = args.p4Filename
    options.testfile = args.testfile
    testDir = args.testfile.split('/')
    partOfTestName = testDir[-1].replace('.py','')
    del testDir[-1]
    testDir = "/".join(testDir)
    options.testDir = testDir
    options.jsonName = testDir + "/" + partOfTestName + ".json"
    options.infoName = testDir + "/" + partOfTestName + ".p4info.txt"

    # Run the test with the extracted options
    result = run_test(options)
    sys.exit(result)
