#!/usr/bin/env python3

import os
import shlex
import subprocess
import pdb
import pprint
import sys
import traceback
import re

import bfas_lookup_cases

# Path to the stability test folder
BFAS_LOOKUP_TEST_PATH = os.path.dirname(os.path.realpath(__file__))
# Relative path to the bfas folder
BFAS_REL_PATH = os.path.join(BFAS_LOOKUP_TEST_PATH, "../../../build/bf-asm")
# Absolute path to the bfas folder
BFAS_ABS_PATH = os.path.abspath(BFAS_REL_PATH)
# Path to the bfas file
BFAS_TOOL_ABS = os.path.join(BFAS_ABS_PATH, "bfas")


class MatchCompareException(Exception):
    def __init__(self, expected, received):
        # Build the error message
        err_msg = "Expected and received results are not same!\n"
        err_msg += "\t* 8 bit - {} (expected), {} (received)\n".format(expected[0], received[0])
        err_msg += "\t* 16 bit - {} (expected), {} (received)\n".format(expected[1], received[1])
        super().__init__(err_msg)


def run_bfas_translation(bfa_file, bfa_options, log_file):
    """
    Run the bfas tool with enabled verbose and file logging.

    Parameters:
        * bfa_file - file to process
        * bfa_options - list of additional options which will be extended
                        with logging into a logfile
        * log_file - file we want to log into
    """
    try:
        bfa_command = [BFAS_TOOL_ABS, "-v", "-v", "-v", "-v", "-l", log_file] + bfa_options + [bfa_file]
        cmd_concat = ' '.join(bfa_command)
        p = subprocess.Popen(cmd_concat, shell=True)
        p.wait()
        ret_code = p.returncode
        if ret_code != 0:
            print("Return code: {}".format(ret_code))
            sys.exit(ret_code)
    except Exception:
        print("error invoking {}".format(bfa_command), file=sys.stderr)
        print(traceback.format_exc(), file=sys.stderr)
        sys.exit(1)


def parse_results(log_file):
    """
    Parse the log file and return the dictionary in the form of:
        * key - name of the match
        * value - tuple with 8 bit extractions (index 0) and 16 bit extractions (index 1)
    """
    if not os.path.exists(log_file):
        raise RuntimeError("Log file {} doesn't exists!".format(log_file))

    ret = {}
    logf = open(log_file, 'r')
    while True:
        # Analyze the file line by line
        line = logf.readline()
        if line == "":
            break

        matchobj = re.match(r"INFO: Used extractors for (.*) - 8bit:([0-9]+), 16bit:([0-9]+)", line)
        if not matchobj:
            continue

        match = matchobj.group(1)
        ext8 = int(matchobj.group(2))
        ext16 = int(matchobj.group(3))
        ret[match] = (ext8, ext16)
    logf.close()
    return ret


def run_test():
    # Name of the file with bfas log file
    log_file_path = os.path.join(BFAS_ABS_PATH, "bfas_translation.log")
    try:
        print("Starting the BFAS test of cache lookup test for Tofino ")
        for test in bfas_lookup_cases.TEST_LIST:
            bfa_file = os.path.join(BFAS_LOOKUP_TEST_PATH, test[0])
            bfa_options = test[1]
            expected_results = test[2]
            # Run the translation, parse output data and check results
            run_bfas_translation(bfa_file, bfa_options, log_file_path)
            translation_results = parse_results(log_file_path)

            # Check that both dictionaries contains same keys
            same_key_set = set(expected_results.keys()) == set(translation_results.keys())
            if not same_key_set:
                raise RuntimeError("Key sets are not same:\n\tExpected: {}\n\tReceived{}".format(
                    expected_results.keys(), translation_results.keys()))

            # Iterate over the dictionary and check results
            for match, values in expected_results.items():
                if translation_results[match] != values:
                    raise MatchCompareException(values, translation_results[match])

            # Cleanup of generated files between runs
            os.remove(log_file_path)

            files_to_remove = ["context.json", "mau.gfm.log", "tofino.bin"]
            for f in files_to_remove:
                os.remove(os.path.join(BFAS_LOOKUP_TEST_PATH, f))

        print("All tests passed!")
    except Exception:
        print(traceback.format_exc(), file=sys.stderr)
        sys.exit(1)


if __name__ == "__main__":
    run_test()
    sys.exit(0)
