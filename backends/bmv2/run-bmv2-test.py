#!/usr/bin/env python
# Runs the compiler on a sample P4 program generating code for the BMv2
# behavioral model simulator

from __future__ import print_function
from subprocess import Popen
from threading import Thread
import sys
import re
import os
import stat
import tempfile
import shutil
import difflib
import subprocess
import scapy.all as scapy

SUCCESS = 0
FAILURE = 1

class Options(object):
    def __init__(self):
        self.binary = ""                # this program's name
        self.cleanupTmp = True          # if false do not remote tmp folder created
        self.p4Filename = ""            # file that is being compiled
        self.compilerSrcDir = ""        # path to compiler source tree
        self.verbose = False
        self.replace = False            # replace previous outputs
        self.compilerOptions = []

def usage(options):
    name = options.binary
    print(name, "usage:")
    print(name, "rootdir [options] file.p4")
    print("Invokes compiler on the supplied file, possibly adding extra arguments")
    print("`rootdir` is the root directory of the compiler source tree")
    print("options:")
    print("          -b: do not remove temporary results for failing tests")
    print("          -v: verbose operation")
    print("          -f: replace reference outputs with newly generated ones")

def ByteToHex( byteStr ):
    # from http://code.activestate.com/recipes/510399-byte-to-hex-and-hex-to-byte-string-conversion/
    return ''.join( [ "%02X " % ord( x ) for x in byteStr ] ).strip()

def HexToByte( hexStr ):
    # from http://code.activestate.com/recipes/510399-byte-to-hex-and-hex-to-byte-string-conversion/
    bytes = []
    hexStr = ''.join( hexStr.split(" ") )
    for i in range(0, len(hexStr), 2):
        bytes.append( chr( int (hexStr[i:i+2], 16 ) ) )
    return ''.join( bytes )
    
def isError(p4filename):
    # True if the filename represents a p4 program that should fail
    return "_errors" in p4filename

class Local(object):
    # object to hold local vars accessable to nested functions
    pass

def run_timeout(options, args, timeout, stderr):
    if options.verbose:
        print("Executing ", " ".join(args))
    local = Local()
    local.process = None
    def target():
        procstderr = None
        if stderr is not None:
            procstderr = open(stderr, "w")
        local.process = Popen(args, stderr=procstderr)
        local.process.wait()
    thread = Thread(target=target)
    thread.start()
    thread.join(timeout)
    if thread.is_alive():
        print("Timeout ", " ".join(args), file=sys.stderr)
        local.process.terminate()
        thread.join()
    if local.process is None:
        # never even started
        if options.verbose:
            print("Process failed to start")
        return -1
    if options.verbose:
        print("Exit code ", local.process.returncode)
    return local.process.returncode

v12_timeout = 100

def compare_files(options, produced, expected):
    if options.replace:
        if options.verbose:
            print("Saving new version of ", expected)
        shutil.copy2(produced, expected)
        return SUCCESS

    if options.verbose:
        print("Comparing", produced, "and", expected)
    diff = difflib.Differ().compare(open(produced).readlines(), open(expected).readlines())
    result = SUCCESS

    marker = re.compile("\?[ \t\+]*");
    ignoreNextMarker = False
    message = ""
    for l in diff:
        if l.startswith("- #include") or l.startswith("+ #include"):
            # These can differ because the paths change
            ignoreNextMarker = True
            continue
        if ignoreNextMarker:
            ignoreNextMarker = False
            if marker.match(l): 
                continue
        if l[0] == ' ': continue
        result = FAILURE
        message += l

    if message is not "":
        print("Files ", produced, " and ", expected, " differ:", file=sys.stderr)
        print(message, file=sys.stderr)

    return result

def check_generated_files(options, tmpdir, expecteddir):
    files = os.listdir(tmpdir)
    for file in files:
        if options.verbose:
            print("Checking", file)
        produced = tmpdir + "/" + file
        expected = expecteddir + "/" + file
        if not os.path.isfile(expected):
            if options.verbose:
                print("Expected file does not exist; creating", expected)
            shutil.copy2(produced, expected)
        else:
            result = compare_files(options, produced, expected)
            if result != SUCCESS:
                return result
    return SUCCESS

def makeKey(key):
    if key.startswith("0x"):
        mask = "F"
    elif key.startswith("0b"):
        mask = "1"
    elif key.startswith("0o"):
        mask = "7"
    m = key.replace("*", mask)
    return key.replace("*", "0") + "&&&" + m

def nextWord(text, sep = " "):
    space = text.find(sep)
    if space < 0:
        return text, ""
    l, r = text[0:space].strip(), text[space+1:len(text)].strip()
    # print(text, "/", sep, "->", l, "#", r)
    return l, r

def translate_command(cmd):
    first, cmd = nextWord(cmd)
    if first != "add":
        return None
    tableName, cmd = nextWord(cmd)
    prio, cmd = nextWord(cmd)
    key = []
    actionArgs = []
    action = None
    while cmd != "":
        if action != None:
            # parsing action arguments
            word, cmd = nextWord(cmd, ",")
            k, v = nextWord(word, ":")
            actionArgs.append(v)
        else:
            # parsing table key
            word, cmd = nextWord(cmd)
            if word.find("(") >= 0:
                # found action
                action, arg = nextWord(word, "(")
                cmd = arg + cmd
                cmd = cmd.strip("()")
            else:
                k, v = nextWord(word, ":")
                key.append(makeKey(v))
            
    command = "table_add " + tableName + " " + action + " " + " ".join(key) + " => " + " ".join(actionArgs) + " " + prio
    return command

def createEmptyPcapFile(fname):
    os.system("cp empty.pcap " + fname);

class OutFiles(object):
    def __init__(self, cmdfile, folder, pcapPrefix):
        self.cmdfile = open(folder + "/" + cmdfile, "w")
        self.folder = folder
        self.pcapPrefix = pcapPrefix
        self.packets = {}
    def writeCommand(self, line):
        self.cmdfile.write(line + "\n")
    def filename(self, interface, direction):
        return self.folder + "/" + self.pcapPrefix + interface + "_" + direction + ".pcap"
    def close(self):
        self.cmdfile.close()
        for interface, v in self.packets.iteritems():
            for direction in ["in", "out-expected"]:
                file = self.filename(interface, direction)
                if direction in v:
                    packets = v[direction]
                    print("Creating", file)
                    scapy.wrpcap(file, packets)
                else:
                    print("Empty", file)
                    createEmptyPcapFile(file)
                    
    def writePacket(self, inout, interface, data):
        data = data.replace("*", "0")  # TODO: this is not right
        hexstr = HexToByte(data)
        packet = scapy.Ether(_pkt = hexstr)
        if not interface in self.packets:
            self.packets[interface] = {}
        if not inout in self.packets[interface]:
            self.packets[interface][inout] = []
        self.packets[interface][inout].append(packet)        
    def interfaces(self):
        # return list of interface names suitable for bmv2
        result = []
        for interface, v in self.packets.iteritems():
            result.append("-i " + interface + "@" + self.pcapPrefix + interface)
        return result
        
def translate_packet(line, outfiles):
    assert isinstance(outfiles, OutFiles)
    cmd, line = nextWord(line)
    if cmd == "expect":
        direction = "out-expected"
    elif cmd == "packet":
        direction = "in"
    else:
        return
    interface, line = nextWord(line)
    outfiles.writePacket(direction, interface, line)
    
def generate_model_inputs(stffile, outfiles):
    assert isinstance(outfiles, OutFiles)
    with open(stffile) as i:
        for line in i:
            cmd = translate_command(line)
            if cmd is not None:
                outfiles.writeCommand(cmd)
            else:
                translate_packet(line, outfiles)
            
def run_model(options, tmpdir, jsonfile):
    # We can do this if an *.stf file is present
    basename = os.path.basename(options.p4filename)
    base, ext = os.path.splitext(basename)
    dirname = os.path.dirname(options.p4filename)
    jsonfile = os.path.basename(jsonfile)
    
    file = dirname + "/" + base + ".stf"
    print("Check for ", file)
    if not os.path.isfile(file):
        return SUCCESS

    if options.verbose:
        print("Generating model inputs and commands")
        
    clifile = "cli.txt"
    outfiles = OutFiles(clifile, tmpdir, "pcap")
    generate_model_inputs(file, outfiles)
    outfiles.close()

    if options.verbose:
        print("Running model")
    interfaces = outfiles.interfaces()
    wait = 2

    runswitch = ["cd", tmpdir, ";", "simple_switch", "--use-files", str(wait)] + interfaces + [jsonfile]
    runcli = ["simple_switch_CLI", "--json", jsonfile, "<" + clifile]
    if options.verbose:
        command = runswitch + ["&", "sleep", "1", ";"] + runcli
        cmd = " ".join(command)
        print("Running", cmd)
        os.system(cmd)
    return SUCCESS

def process_file(options, argv):
    assert isinstance(options, Options)

    tmpdir = tempfile.mkdtemp(dir=".")
    basename = os.path.basename(options.p4filename)
    base, ext = os.path.splitext(basename)
    dirname = os.path.dirname(options.p4filename)
    expected_dirname = dirname + "_outputs"  # expected outputs are here
    
    if options.verbose:
        print("Writing temporary files into ", tmpdir)
    jsonfile = tmpdir + "/" + base + ".json"
    stderr = tmpdir + "/" + basename + "-stderr"

    if not os.path.isfile(options.p4filename):
        raise Exception("No such file " + options.p4filename)
    args = ["./p4c-bm2", "-o", jsonfile] + options.compilerOptions
    if "v1_samples" in options.p4filename:
        args.extend(["--p4v", "1.0"]);
    args.extend(argv)  # includes p4filename

    result = run_timeout(options, args, v12_timeout, stderr)
    if result != SUCCESS:
        print("Error compiling")
        print("".join(open(stderr).readlines()))

    expected_error = isError(options.p4filename)
    if expected_error:
        # invert result
        if result == SUCCESS:
            result = FAILURE
        else:
            result = SUCCESS

    #if result == SUCCESS:
    #    result = check_generated_files(options, tmpdir, expected_dirname);

    if result == SUCCESS:
        result = run_model(options, tmpdir, jsonfile);
            
    if options.cleanupTmp:
        if options.verbose:
            print("Removing", tmpdir)
        shutil.rmtree(tmpdir)
    return result

def isdir(path):
    try:
        return stat.S_ISDIR(os.stat(path).st_mode)
    except OSError:
        return False;

######################### main

def main(argv):
    options = Options()

    options.binary = argv[0]
    if len(argv) <= 2:
        usage(options)
        return FAILURE

    options.compilerSrcdir = argv[1]
    argv = argv[2:]
    if not os.path.isdir(options.compilerSrcdir):
        print(options.compilerSrcdir + " is not a folder", file=sys.stderr)
        usage(options)
        return FAILURE

    while argv[0][0] == '-':
        if argv[0] == "-b":
            options.cleanupTmp = False
        elif argv[0] == "-v":
            options.verbose = True
        elif argv[0] == "-f":
            options.replace = True
        elif argv[0] == "-a":
            if len(argv) == 0:
                print("Missing argument for -a option")
                usage(options)
                sys.exit(1)
            else:
                options.compilerOptions += argv[1].split();
                argv = argv[1:]
        else:
            print("Uknown option ", argv[0], file=sys.stderr)
            usage(options)
        argv = argv[1:]

    options.p4filename=argv[-1]
    options.testName = None
    if options.p4filename.startswith(options.compilerSrcdir):
        options.testName = options.p4filename[len(options.compilerSrcdir):];
        if options.testName.startswith('/'):
            options.testName = options.testName[1:]
        if options.testName.endswith('.p4'):
            options.testName = options.testName[:-3]

    result = process_file(options, argv)
    sys.exit(result)

if __name__ == "__main__":
    main(sys.argv)
