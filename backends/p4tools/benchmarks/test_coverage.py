#!/usr/bin/env python3

import argparse
import subprocess
import random
import csv
import sys
import re
import tempfile
from pathlib import Path
import datetime

FILE_DIR = Path(__file__).parent.resolve()
TOOLS_PATH = FILE_DIR.joinpath("../../../tools")
sys.path.append(str(TOOLS_PATH))
import testutils

TESTGEN_BIN = FILE_DIR.joinpath("../../../build/p4testgen")
OUTPUT_DIR = FILE_DIR.joinpath("../../../build/results")
# generate random seeds, increase the number for extra sampling
ITERATIONS = 1
MAX_TESTS = 0

PARSER = argparse.ArgumentParser()
PARSER.add_argument(
    "-p",
    "--p4-program",
    dest="p4_program",
    required=True,
    help="The P4 file to measure coverage on.",
)
PARSER.add_argument(
    "-o",
    "--out-dir",
    dest="out_dir",
    default=OUTPUT_DIR,
    help="The output folder where all tests are dumped.",
)
PARSER.add_argument(
    "-i",
    "--iterations",
    dest="iterations",
    default=ITERATIONS,
    type=int,
    help="How many iterations to run.",
)
PARSER.add_argument(
    "-m",
    "--max-tests",
    dest="max_tests",
    default=MAX_TESTS,
    type=int,
    help="How many tests to run for each test.",
)
PARSER.add_argument(
    "-s",
    "--seed",
    dest="seed",
    default=0,
    type=int,
    help="The random seed for the tests.",
)
PARSER.add_argument(
    "-b",
    "--testgen-bin",
    dest="p4testgen_bin",
    default=TESTGEN_BIN,
    help="Specifies the testgen binary.",
)


class Options:
    def __init__(self):
        self.p4testgen_bin = None  # The P4Testgen binary.
        self.p4_program = None  # P4 Program that is being measured.
        self.out_dir = None  # The output directory.
        self.seed = None  # Program seed.
        self.max_tests = None  # The max tests parameter.


class TestArgs:
    def __init__(self):
        self.seed = None  # The seed for this particular run.
        self.extra_args = None  # Extra arguments for P4Testgen execution.
        self.test_dir = None  # The testing directory associated with this test run.
        self.strategy = None  # The exploration strategy to execute.


def get_test_files(input_dir, extension):
    test_files = input_dir.glob(extension)

    def natsort(s):
        return [int(t) if t.isdigit() else t.lower() for t in re.split("(\d+)", str(s))]

    test_files = sorted(test_files, key=natsort)
    return test_files


def parse_stmt_cov(test_file):
    with test_file.open("r") as file_handle:
        for line in file_handle.readlines():
            if "Current statement coverage:" in line:
                covstr = line.replace('metadata: "Current statement coverage: ', "")
                covstr = covstr.replace('"\n', "")
                return covstr
    return None


def parse_timestamp(test_file):
    with test_file.open("r") as file_handle:
        for line in file_handle.readlines():
            if "Date generated" in line:
                datestr = line.replace('metadata: "Date generated: ', "")
                datestr = datestr.replace('"\n', "")
                datestr = datestr.strip()
                return datestr
    return None


def run_strategies_for_max_tests(data_row, options, test_args):

    cmd = (
        f"{options.p4testgen_bin} --target bmv2 --arch v1model --std p4-16"
        f" -I/p4/p4c/build/p4include --test-backend PROTOBUF --seed {test_args.seed} "
        f"--max-tests {options.max_tests}  --out-dir {test_args.test_dir}"
        f" --exploration-strategy {test_args.strategy} --stop-metric MAX_STATEMENT_COVERAGE "
        f" {test_args.extra_args} {options.p4_program}"
    )
    start_timestamp = datetime.datetime.now()
    try:
        # TODO: Use result
        _ = subprocess.run(
            cmd,
            shell=True,
            universal_newlines=True,
            capture_output=True,
            check=True,
        )
    except subprocess.CalledProcessError as e:
        returncode = e.returncode
        print(f"Error executing {e.cmd} {returncode}:\n{e.stdout}\n{e.stderr}")
        sys.exit(1)
    end_timestamp = datetime.datetime.now()

    test_files = get_test_files(test_args.test_dir, "*.proto")
    # Get the statement coverage of the last test generated.
    last_test = test_files[-1]
    statements_cov = parse_stmt_cov(last_test)

    time_needed = (end_timestamp - start_timestamp).total_seconds()
    num_tests = len(test_files)
    data_row.append(statements_cov)
    data_row.append(str(num_tests))
    data_row.append(time_needed)
    print(
        f"Pct Statements Covered: {statements_cov} Number of tests: {num_tests} Time needed: {time_needed}"
    )
    return data_row


def main(args):

    options = Options()
    options.p4_program = Path(testutils.check_if_file(args.p4_program))
    options.max_tests = args.max_tests
    options.out_dir = Path(args.out_dir).absolute()
    options.seed = args.seed
    options.p4testgen_bin = Path(testutils.check_if_file(args.p4testgen_bin))

    # 7189 is an example of a good seed, which gets cov 1 with less than 100 tests
    # in random access stack.
    random.seed(options.seed)
    seeds = []
    for _ in range(args.iterations):
        seed = random.randint(0, sys.maxsize)
        seeds.append(seed)
    print(f"Chosen seeds: {seeds}")
    p4_program_name = options.p4_program.stem

    header = [
        "seed",
        "coverage",
        "num_tests",
        "time (s)",
    ]

    strategies = ["RANDOM_ACCESS_STACK", "RANDOM_ACCESS_MAX_COVERAGE", "INCREMENTAL_STACK"]
    config = {
        "INCREMENTAL_STACK": "",
        "RANDOM_ACCESS_STACK": "--pop-level 3",
        "RANDOM_ACCESS_MAX_COVERAGE": "--saddle-point 3",
    }
    # csv results file path
    for strategy in strategies:
        test_dir = options.out_dir.joinpath(f"{strategy}")
        testutils.check_and_create_dir(test_dir)
        results_path_max = test_dir.joinpath(f"coverage_results_{p4_program_name}_{strategy}.csv")
        with open(results_path_max, "w", encoding="utf-8") as f:
            writer = csv.writer(f)
            writer.writerow(header)
            for seed in seeds:
                data_row = [seed]
                print(
                    f"Seed {seed} Generating metrics for {strategy} up to {options.max_tests} tests"
                )
                test_args = TestArgs()
                test_args.seed = seed
                test_args.test_dir = Path(tempfile.mkdtemp(dir=test_args.test_dir))
                test_args.strategy = strategy
                test_args.extra_args = config[strategy]
                data_row = run_strategies_for_max_tests(data_row, options, test_args)
                writer.writerow(data_row)


if __name__ == "__main__":
    # Parse options and process argv
    arguments, argv = PARSER.parse_known_args()
    main(arguments)
