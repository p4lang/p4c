#!/usr/bin/env python
# Copyright 2013-present Barefoot Networks, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# Runs the compiler on a sample P4 program generating code for the BMv2
# behavioral model simulator

from __future__ import print_function
from subprocess import Popen
from threading import Thread
import json
import sys
import re
import os
import stat
import tempfile
import shutil
import difflib
import subprocess
import time
import random
import errno
from string import maketrans
try:
    from scapy.layers.all import *
    from scapy.utils import *
except ImportError:
    pass

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
        self.hasBMv2 = False            # Is the behavioral model installed?

def nextWord(text, sep = " "):
    # Split a text at the indicated separator.
    # Note that the separator can be a string.
    # Separator is discarded.
    pos = text.find(sep)
    if pos < 0:
        return text, ""
    l, r = text[0:pos].strip(), text[pos+len(sep):len(text)].strip()
    # print(text, "/", sep, "->", l, "#", r)
    return l, r

class ConfigH(object):
    # Represents an autoconf config.h file
    # fortunately the structure of these files is very constrained
    def __init__(self, file):
        self.file = file
        self.vars = {}
        with open(file) as a:
            self.text = a.read()
        self.ok = False
        self.parse()
    def parse(self):
        while self.text != "":
            self.text = self.text.strip()
            if self.text.startswith("/*"):
                end = self.text.find("*/")
                if end < 1:
                    reportError("Unterminated comment in config file")
                    return
                self.text = self.text[end+2:len(self.text)]
            elif self.text.startswith("#define"):
                define, self.text = nextWord(self.text)
                macro, self.text = nextWord(self.text)
                value, self.text = nextWord(self.text, "\n")
                self.vars[macro] = value
            elif self.text.startswith("#ifndef"):
                junk, self.text = nextWord(self.text, "#endif")
            else:
                reportError("Unexpected text:", self.text)
                return
        self.ok = True
    def __str__(self):
        return str(self.vars)

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

def ByteToHex(byteStr):
    return ''.join( [ "%02X " % ord( x ) for x in byteStr ] ).strip()

def HexToByte(hexStr):
    bytes = []
    hexStr = ''.join( hexStr.split(" ") )
    for i in range(0, len(hexStr), 2):
        bytes.append( chr( int (hexStr[i:i+2], 16 ) ) )
    return ''.join( bytes )

def isError(p4filename):
    # True if the filename represents a p4 program that should fail
    return "_errors" in p4filename

def reportError(*message):
    print("***", *message)

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
        reportError("Process failed to start")
        return -1
    if options.verbose:
        print("Exit code ", local.process.returncode)
    return local.process.returncode

timeout = 100

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

class ConcurrentInteger(object):
    # Generates exclusive integers in a range 0-max
    # in a way which is safe across multiple processes.
    # It uses a simple form of locking using folder names.
    # This is necessary because this script may be invoked
    # concurrently many times by make, and we need the many simulator instances
    # to use different port numbers.
    def __init__(self, folder, max):
        self.folder = folder
        self.max = max
    def lockName(self, value):
        return "lock_" + str(value)
    def release(self, value):
        os.rmdir(self.lockName(value))
    def generate(self):
        # try 10 times
        for i in range(0, 10):
            index = random.randint(0, self.max)
            file = self.lockName(index)
            try:
                os.makedirs(file)
                return index
            except:
                time.sleep(1)
                continue
        return None

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

class BMV2ActionArg(object):
    def __init__(self, name, width):
        # assert isinstance(name, str)
        # assert isinstance(width, int)
        self.name = name
        self.width = width

class TableKey(object):
    def __init__(self, ternary):
        self.fields = []
        self.ternary = ternary
    def append(self, name):
        self.fields.append(name)

class TableKeyInstance(object):
    def __init__(self, tableKey):
        assert isinstance(tableKey, TableKey)
        self.values = {}
        self.key = tableKey
        for f in tableKey.fields:
            self.values[f] = self.makeMask("0x*", tableKey.ternary)
    def set(self, key, value, ternary):
        array = re.compile("(.*)\$([0-9]+)(.*)");
        m = array.match(key)
        if m:
            key = m.group(1) + "[" + m.group(2) + "]" + m.group(3)

        found = False
        for i in self.key.fields:
            if key == i:
                found = True
        if not found:
            raise Exception("Unexpected key field " + key)
        self.values[key] = self.makeMask(value, ternary)
    def makeMask(self, value, ternary):
        if not ternary:
            return value
        if value.startswith("0x"):
            mask = "F"
            value = value[2:]
            prefix = "0x"
        elif value.startswith("0b"):
            mask = "1"
            value = value[2:]
            prefix = "0b"
        elif value.startswith("0o"):
            mask = "7"
            value = value[2:]
            prefix = "0o"
        values = "0123456789*"
        replacements = (mask * 10) + "0"
        trans = maketrans(values, replacements)
        m = value.translate(trans)
        return prefix + value.replace("*", "0") + "&&&" + prefix + m
    def __str__(self):
        result = ""
        for f in self.key.fields:
            if result != "":
                result += " "
            result += self.values[f]
        return result

class BMV2ActionArguments(object):
    def __init__(self, action):
        assert isinstance(action, BMV2Action)
        self.action = action
        self.values = {}
    def set(self, key, value):
        found = False
        for i in self.action.args:
            if key == i.name:
                found = True
        if not found:
            raise Exception("Unexpected action arg " + key)
        self.values[key] = value
    def __str__(self):
        result = ""
        for f in self.action.args:
            if result != "":
                result += " "
            result += self.values[f.name]
        return result
    def size(self):
        return len(self.action.args)

class BMV2Action(object):
    def __init__(self, jsonAction):
        self.name = jsonAction["name"]
        self.args = []
        for a in jsonAction["runtime_data"]:
            arg = BMV2ActionArg(a["name"], a["bitwidth"])
            self.args.append(arg)
    def __str__(self):
        return self.name
    def makeArgsInstance(self):
        return BMV2ActionArguments(self)

class BMV2Table(object):
    def __init__(self, jsonTable):
        self.match_type = jsonTable["match_type"]
        self.name = jsonTable["name"]
        self.key = TableKey(self.match_type == "ternary")
        self.actions = {}
        for k in jsonTable["key"]:
            name = ""
            for t in k["target"]:
                if name != "":
                    name += "."
                name += t
            self.key.append(name)
        actions = jsonTable["actions"]
        action_ids = jsonTable["action_ids"]
        for i in range(0, len(actions)):
            actionName = actions[i]
            actionId = action_ids[i]
            self.actions[actionName] = actionId
    def __str__(self):
        return self.name
    def makeKeyInstance(self):
        return TableKeyInstance(self.key)

# Represents enough about the program executed to be
# able to invoke the BMV2 simulator, create a CLI file
# and test packets in pcap files.
class RunBMV2(object):
    def __init__(self, folder, options, jsonfile):

        self.clifile = folder + "/cli.txt"
        self.jsonfile = jsonfile
        self.clifd = open(self.clifile, "w")
        self.folder = folder
        self.pcapPrefix = "pcap"
        self.packets = {}
        self.options = options
        self.json = None
        self.tables = []
        self.actions = []
        self.switchLogFile = "switch.log"  # .txt is added by BMv2
        self.readJson()
    def readJson(self):
        with open(self.folder + "/" + self.jsonfile) as jf:
            self.json = json.load(jf)
        for a in self.json["actions"]:
            self.actions.append(BMV2Action(a))
        for t in self.json["pipelines"][0]["tables"]:
            self.tables.append(BMV2Table(t))
        for t in self.json["pipelines"][1]["tables"]:
            self.tables.append(BMV2Table(t))
    def createEmptyPcapFile(self, fname):
        os.system("cp " + self.options.compilerSrcDir + "/tools/empty.pcap " + fname);
    def writeCommand(self, line):
        self.clifd.write(line + "\n")
    def filename(self, interface, direction):
        return self.folder + "/" + self.pcapPrefix + interface + "_" + direction + ".pcap"
    def translate_command(self, cmd):
        first, cmd = nextWord(cmd)
        if first == "add":
            return self.parse_table_add(cmd)
        elif first == "setdefault":
            return self.parse_table_set_default(cmd)
        else:
            return None
    def parse_table_set_default(self, cmd):
        tableName, cmd = nextWord(cmd)
        table = self.tableByName(tableName)
        actionName, cmd = nextWord(cmd, "(")
        action = self.actionByName(table, actionName)
        actionArgs = action.makeArgsInstance()
        cmd = cmd.strip(")")
        while cmd != "":
            word, cmd = nextWord(cmd, ",")
            k, v = nextWord(word, ":")
            actionArgs.set(k, v)
        command = "table_set_default " + tableName + " " + actionName
        if actionArgs.size():
            command += " => " + str(actionArgs)
        return command
    def parse_table_add(self, cmd):
        tableName, cmd = nextWord(cmd)
        table = self.tableByName(tableName)
        key = table.makeKeyInstance()
        actionArgs = None
        actionName = None
        prio, cmd = nextWord(cmd)
        ternary = True
        number = re.compile("[0-9]+")
        if not number.match(prio):
            # not a priority; push back
            cmd = prio + " " + cmd
            prio = ""
            ternary = False
        while cmd != "":
            if actionName != None:
                # parsing action arguments
                word, cmd = nextWord(cmd, ",")
                k, v = nextWord(word, ":")
                actionArgs.set(k, v)
            else:
                # parsing table key
                word, cmd = nextWord(cmd)
                if word.find("(") >= 0:
                    # found action
                    actionName, arg = nextWord(word, "(")
                    action = self.actionByName(table, actionName)
                    actionArgs = action.makeArgsInstance()
                    cmd = arg + cmd
                    cmd = cmd.strip("()")
                else:
                    k, v = nextWord(word, ":")
                    key.set(k, v, ternary)

        if prio != "":
            # Priorities in BMV2 seem to be reversed with respect to the stf file
            # Hopefully 10000 is large enough
            prio = str(10000 - int(prio))
        command = "table_add " + tableName + " " + actionName + " " + str(key) + " => " + str(actionArgs) + " " + prio
        return command
    def actionByName(self, table, actionName):
        id = table.actions[actionName]
        action = self.actions[id]
        return action
    def tableByName(self, tableName):
        for t in self.tables:
            if t.name == tableName:
                return t
        raise Exception("Could not find table " + tableName)
    def writePacket(self, inout, interface, data):
        if not interface in self.packets:
            self.packets[interface] = {}
        if not inout in self.packets[interface]:
            self.packets[interface][inout] = []
        data = data.replace(" ", "")
        if len(data) % 2 == 1:
            reportError("Packet data is not integral number of bytes:", len(data), "hex digits:", data)
            return FAILURE
        self.packets[interface][inout].append(data)
        return SUCCESS
    def interfaces(self):
        # return list of interface names suitable for bmv2
        result = []
        for interface, v in self.packets.iteritems():
            result.append("-i " + interface + "@" + self.pcapPrefix + interface)
        return result
    def translate_packet(self, line):
        cmd, line = nextWord(line)
        if cmd == "expect":
            direction = "out"
        elif cmd == "packet":
            direction = "in"
        else:
            return SUCCESS
        interface, line = nextWord(line)
        return self.writePacket(direction, interface, line)
    def generate_model_inputs(self, stffile):
        with open(stffile) as i:
            for line in i:
                cmd = self.translate_command(line)
                if cmd is not None:
                    if self.options.verbose:
                        print(cmd)
                    self.writeCommand(cmd)
                else:
                    success = self.translate_packet(line)
                    if success != SUCCESS:
                        return success
        # Write the data to the files
        self.clifd.close()
        for interface, v in self.packets.iteritems():
            direction ="in"
            file = self.filename(interface, direction)
            if direction in v:
                packets = v[direction]
                data = [Packet(_pkt=HexToByte(hx)) for hx in packets]
                print("Creating", file)
                wrpcap(file, data)
            else:
                print("Empty", file)
                self.createEmptyPcapFile(file)
        return SUCCESS
    def run(self):
        if self.options.verbose:
            print("Running model")
        interfaces = self.interfaces()
        wait = 2  # Time to wait before model starts running

        concurrent = ConcurrentInteger(os.getcwd(), 1000)
        rand = concurrent.generate()
        if rand is None:
            reportError("Could not find a free port for Thrift")
            return FAILURE
        thriftPort = str(9090 + rand)

        try:
            runswitch = ["simple_switch", "--log-file", self.switchLogFile, "--use-files",
                         str(wait), "--thrift-port", thriftPort] + interfaces + [self.jsonfile]
            if self.options.verbose:
                print("Running", " ".join(runswitch))
            sw = subprocess.Popen(runswitch, cwd=self.folder)

            runcli = ["simple_switch_CLI", "--thrift-port", thriftPort]
            if self.options.verbose:
                print("Running", " ".join(runcli))
            i = open(self.clifile, 'r')
            # The CLI has to run before the model starts running; so this is a little race.
            # If the race turns out wrong the test will fail, which is unfortunate.
            # This should be fixed by improving the CLI/simple_switch programs.
            cli = subprocess.Popen(runcli, cwd=self.folder, stdin=i)
            cli.wait()
            if cli.returncode != 0:
                reportError("CLI process failed with exit code", cli.returncode)
                return FAILURE
            # Give time to the model to execute
            time.sleep(3)
            sw.terminate()
            sw.wait()
            # This only works on Unix: negative returncode is
            # minus the signal number that killed the process.
            if sw.returncode != -15:  # 15 is SIGTERM
                reportError("simple_switch died with return code", sw.returncode);
            elif self.options.verbose:
                print("simple_switch exit code", sw.returncode)
        finally:
            concurrent.release(rand)
        if self.options.verbose:
            print("Execution completed")
        return SUCCESS
    def comparePacket(self, expected, received):
        received = ByteToHex(str(received)).replace(" ", "").upper()
        expected = expected.replace(" ", "").upper()
        if len(received) < len(expected):
            reportError("Received packet too short", len(received), "vs", len(expected))
            return FAILURE
        for i in range(0, len(expected)):
            if expected[i] == "*":
                continue;
            if expected[i] != received[i]:
                reportError("Packet different at position", i, ": expected", expected[i], ", received", received[i])
                return FAILURE
        return SUCCESS
    def showLog(self):
        with open(self.folder + "/" + self.switchLogFile + ".txt") as a:
            log = a.read()
            print("Log file:")
            print(log)
    def checkOutputs(self):
        if self.options.verbose:
            print("Comparing outputs")
        for interface, v in self.packets.iteritems():
            direction = "out"
            file = self.filename(interface, direction)
            if os.stat(file).st_size == 0:
                packets = []
            else:
                try:
                    packets = rdpcap(file)
                except:
                    reportError("Corrupt pcap file", file)
                    self.showLog()
                    return FAILURE
            if direction in v:
                expected = v[direction]
                if len(expected) != len(packets):
                    reportError("Expected", len(expected), "packets on port", interface, "got", len(packets))
                    self.showLog()
                    return FAILURE
                for i in range(0, len(expected)):
                    cmp = self.comparePacket(expected[i], packets[i])
                    if cmp != SUCCESS:
                        reportError("Packet", i, "on port", interface, "differs")
                        return FAILURE
            else:
                pass
        return SUCCESS

def run_model(options, tmpdir, jsonfile):
    if not options.hasBMv2:
        return SUCCESS

    # We can do this if an *.stf file is present
    basename = os.path.basename(options.p4filename)
    base, ext = os.path.splitext(basename)
    dirname = os.path.dirname(options.p4filename)
    jsonfile = os.path.basename(jsonfile)

    testfile = dirname + "/" + base + ".stf"
    print("Check for ", testfile)
    if not os.path.isfile(testfile):
        return SUCCESS

    bmv2 = RunBMV2(tmpdir, options, jsonfile)
    result = bmv2.generate_model_inputs(testfile)
    if result != SUCCESS:
        return result
    result = bmv2.run()
    if result != SUCCESS:
        return result
    result = bmv2.checkOutputs()
    return result

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
    args = ["./p4c-bm2-ss", "-o", jsonfile] + options.compilerOptions
    if "p4_14_samples" in options.p4filename or "v1_samples" in options.p4filename:
        args.extend(["--p4v", "1.0"]);
    args.extend(argv)  # includes p4filename

    result = run_timeout(options, args, timeout, stderr)
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

    options.compilerSrcDir = argv[1]
    argv = argv[2:]
    if not os.path.isdir(options.compilerSrcDir):
        print(options.compilerSrcDir + " is not a folder", file=sys.stderr)
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
                reportError("Missing argument for -a option")
                usage(options)
                sys.exit(1)
            else:
                options.compilerOptions += argv[1].split();
                argv = argv[1:]
        else:
            reportError("Uknown option ", argv[0], file=sys.stderr)
            usage(options)
        argv = argv[1:]

    config = ConfigH("config.h")
    if not config.ok:
        print("Error parsing config.h")
        return FAILURE

    options.hasBMv2 = "HAVE_SIMPLE_SWITCH" in config.vars
    if not options.hasBMv2:
        reportError("config.h indicates that BMv2 is not installed; will skip running BMv2 tests")

    options.p4filename=argv[-1]
    options.testName = None
    if options.p4filename.startswith(options.compilerSrcDir):
        options.testName = options.p4filename[len(options.compilerSrcDir):];
        if options.testName.startswith('/'):
            options.testName = options.testName[1:]
        if options.testName.endswith('.p4'):
            options.testName = options.testName[:-3]

    result = process_file(options, argv)
    if result != SUCCESS:
        reportError("Test failed")
    sys.exit(result)

if __name__ == "__main__":
    main(sys.argv)
