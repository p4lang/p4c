import sys
import os
from glob import glob
# path to the tools folder of the compiler
sys.path.insert(0, 'p4c/tools')
# path to the framework repository of the compiler
sys.path.insert(0, os.path.dirname(
    os.path.realpath(__file__)) + '/../ebpf')
from targets.target import EBPFTarget
from targets.ebpfstf import parse_stf_file
from testutils import *


class Target(EBPFTarget):

    def __init__(self, tmpdir, options, template, outputs):
        EBPFTarget.__init__(self, tmpdir, options, template, outputs)
        # We use a different compiler, override the inherited default
        components = options.compiler.split("/")[0:-1]
        self.compiler = "/".join(components) + "/p4c-ubpf"
        print("Compiler is", self.compiler)

    def compile_dataplane(self):
        args = self.get_make_args(self.runtimedir, self.options.target)
        # List of bpf programs to attach to the interface
        args += "BPFOBJ=" + self.template + " "
        args += "CFLAGS+=-DCONTROL_PLANE "
        args += "EXTERNOBJ=" + self.options.extern + " "

        errmsg = "Failed to build the filter:"
        return run_timeout(self.options.verbose, args, TIMEOUT,
                           self.outputs, errmsg)

    def run(self):
        report_output(self.outputs["stdout"],
                      self.options.verbose, "Running model")
        direction = "in"
        pcap_pattern = self.filename('', direction)
        num_files = len(glob(self.filename('*', direction)))
        report_output(self.outputs["stdout"],
                      self.options.verbose,
                      "Input file: %s" % pcap_pattern)
        # Main executable
        args = self.template + " "
        # Input pcap pattern
        args += "-f " + pcap_pattern + " "
        # Number of input interfaces
        args += "-n " + str(num_files) + " "
        # Debug flag (verbose output)
        args += "-d"
        errmsg = "Failed to execute the filter:"
        result = run_timeout(self.options.verbose, args,
                             TIMEOUT, self.outputs, errmsg)
        return result

    def _generate_control_actions(self, cmds):
        generated = ""

        tables = { cmd.table for cmd in cmds }
        for tbl in tables:
            generated += "INIT_UBPF_TABLE(\"%s\", sizeof(struct %s_key), sizeof(struct %s_value));\n\t" \
                         % (tbl, tbl, tbl)

        for index, cmd in enumerate(cmds):
            key_name = "key_%s%d" % (cmd.table, index)
            value_name = "value_%s%d" % (cmd.table, index)
            if cmd.a_type == "add":
                generated += "struct %s_key %s = {};\n\t" % (cmd.table, key_name)
                for key_num, key_field in enumerate(cmd.match):
                    field = key_field[0].split('.')[1]
                    generated += ("%s.%s = %s;\n\t"
                                  % (key_name, field, key_field[1]))
            generated += ("struct %s_value %s = {\n\t\t" % (
                cmd.table, value_name))
            generated += ".action = %s,\n\t\t" % (cmd.action[0])
            generated += ".u = {.%s = {" % cmd.action[0]
            for val_num, val_field in enumerate(cmd.action[1]):
                generated += "%s," % val_field[1]
            generated += "}},\n\t"
            generated += "};\n\t"
            generated += ("ubpf_map_update"
                          "(&%s, &%s, &%s);\n\t"
                          % (cmd.table, key_name, value_name))
        return generated

    def create_ubpf_table_file(self, actions, tmpdir, file_name):
        err = ""
        try:
            with open(tmpdir + "/" + file_name, "w+") as control_file:
                control_file.write("#include \"test.h\"\n\n")
                control_file.write("static inline void setup_control_plane() {")
                control_file.write("\n\t")
                generated_cmds = self._generate_control_actions(actions)
                control_file.write(generated_cmds)
                control_file.write("}\n")
        except OSError as e:
            err = e
            return FAILURE, err
        return SUCCESS, err

    def generate_model_inputs(self, stffile):
        with open(stffile) as raw_stf:
            input_pkts, cmds, self.expected = parse_stf_file(
                raw_stf)
            result, err = self.create_ubpf_table_file(cmds, self.tmpdir, "control.h")
            if result != SUCCESS:
                return result
            result = self._write_pcap_files(input_pkts)
            if result != SUCCESS:
                return result
        return SUCCESS