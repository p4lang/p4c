#!/usr/bin/env python3

import argparse
import datetime
import logging
import os
import random
import re
import sys
import tempfile
from pathlib import Path

import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
import seaborn as sns

# Append tools to the import path.
FILE_DIR = Path(__file__).resolve().parent
TOOLS_PATH = FILE_DIR.joinpath("../../../../../tools")
sys.path.append(str(TOOLS_PATH))
import testutils

TESTGEN_BIN = FILE_DIR.joinpath("../../../../../build/p4testgen")
OUTPUT_DIR = FILE_DIR.joinpath("../../../../../build/results")
# generate random seeds, increase the number for extra sampling
ITERATIONS = 1
MAX_TESTS = 0
TEST_BACKEND = "PROTOBUF"

PARSER = argparse.ArgumentParser()

PARSER.add_argument(
    "-p",
    "--p4-programs",
    dest="p4_programs",
    nargs="+",
    help="The P4 files to measure coverage on.",
    required=True,
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
PARSER.add_argument(
    "-t",
    "--test-backend",
    dest="test_backend",
    default=TEST_BACKEND,
    type=str,
    help="Which test back end to generate tests for.",
)
PARSER.add_argument(
    "-lm",
    "--large-memory",
    dest="large_memory",
    action="store_true",
    help="Run tests with a large memory configuration.",
)
PARSER.add_argument(
    "-tm",
    "--test-mode",
    dest="test_mode",
    default="bmv2",
    type=str,
    help="The target configuration.",
)
PARSER.add_argument(
    "-ea",
    "--extra-args",
    dest="extra_args",
    default="",
    type=str,
    help="Any extra args to append.",
)
PARSER.add_argument(
    "-ll",
    "--log_level",
    dest="log_level",
    default="WARNING",
    choices=["CRITICAL", "ERROR", "WARNING", "INFO", "DEBUG", "NOTSET"],
    help="The log level to choose.",
)


class Options:
    # The P4Testgen binary.
    p4testgen_bin = None
    # P4 Programs that are being measured.
    p4_programs = None
    # The output directory.
    out_dir = None
    # Program seed.
    seed = None
    # The max tests parameter.
    max_tests = None
    # The test back end to generate tests for.
    test_backend = None
    # Generic test config.
    test_mode = "bmv2"
    # Whether to execute tests with a large memory configuration for the garbage collector.
    large_memory = False
    # The target.
    target = "bmv2"
    # The architecture.
    arch = "v1model"
    # Extra arguments for P4Testgen execution.
    extra_args = None


class TestArgs:
    # The seed for this particular run.
    seed = None
    # Extra arguments for P4Testgen execution.
    extra_args = None
    # The testing directory associated with this test run.
    test_dir = None
    # The P4 program to run tests on.
    p4_program = None
    # The exploration strategy to execute.
    strategy = None


def get_test_files(input_dir, extension):
    test_files = input_dir.glob(extension)

    def natsort(s):
        return [int(t) if t.isdigit() else t.lower() for t in re.split("(\d+)", str(s))]

    test_files = sorted(test_files, key=natsort)
    return test_files


def parse_coverage_and_timestamps(test_files, parse_type):
    datestrs = []
    cov_percentages = []
    for test_file in test_files:
        with test_file.open("r") as file_handle:
            for line in file_handle.readlines():
                if "Date generated" in line:
                    if parse_type == "PROTOBUF":
                        datestr = line.replace('metadata: "Date generated: ', "")
                        datestr = datestr.replace('"\n', "")
                    elif parse_type == "PTF":
                        datestr = line.replace("Date generated: ", "")
                        datestr = datestr.replace("\n", "")
                    else:
                        datestr = line.replace("# Date generated: ", "")
                        datestr = datestr.replace("\n", "")
                    datestr = datestr.strip()
                    datestrs.append(datestr)
                if "Current node coverage:" in line:
                    if parse_type == "PROTOBUF":
                        covstr = line.replace('metadata: "Current node coverage: ', "")
                        covstr = covstr.replace('"\n', "")
                    elif parse_type == "PTF":
                        covstr = line.replace("Current node coverage: ", "")
                    else:
                        covstr = line.replace("# Current node coverage: ", "")
                    covstr = covstr.replace("\n", "")
                    cov_percentages.append(float(covstr))
    return cov_percentages, datestrs


def collect_data_from_folder(input_dir, parse_type):
    if parse_type == "PTF":
        files = get_test_files(input_dir, "*.py")
    elif parse_type == "PROTOBUF":
        files = get_test_files(input_dir, "*.txtpb")
    else:
        files = get_test_files(input_dir, "*.stf")
    return parse_coverage_and_timestamps(files, parse_type)


def convert_timestamps_to_timedelta(timestamps):
    time_df = pd.to_datetime(pd.Series(timestamps), unit="ns")
    time_df = time_df - time_df.iat[0]
    time_df = pd.to_timedelta(time_df, unit="nanoseconds")
    return time_df


def run_strategies_for_max_tests(options, test_args):
    cmd = (
        f"{options.p4testgen_bin} --target {options.target} --arch {options.arch}"
        f" -I/p4/p4c/build/p4include --test-backend {options.test_backend}"
        f" --seed {test_args.seed} --print-performance-report"
        f" --max-tests {options.max_tests} --out-dir {test_args.test_dir}"
        f" --path-selection {test_args.strategy} --stop-metric MAX_NODE_COVERAGE"
        f" --track-coverage STATEMENTS"
        f"{test_args.extra_args} {test_args.p4_program}"
    )
    start_timestamp = datetime.datetime.now()

    # TODO: Use result
    custom_env = os.environ.copy()
    # Some executions may need a lot of memory.
    if options.large_memory:
        custom_env["GC_INITIAL_HEAP_SIZE"] = "30G"
        custom_env["GC_MAXIMUM_HEAP_SIZE"] = "50G"
    testutils.exec_process(cmd, env=custom_env, capture_output=True, timeout=3600)
    end_timestamp = datetime.datetime.now()

    nodes_cov, timestamps = collect_data_from_folder(test_args.test_dir, options.test_backend)
    if not nodes_cov:
        print("No errors found!")
        return [], [], []
    num_tests = len(timestamps)
    final_cov = str(nodes_cov[-1])
    time_needed = (end_timestamp - start_timestamp).total_seconds()
    print(
        f"Pct Nodes Covered: {final_cov} Number of tests: {num_tests} Time needed:"
        f" {time_needed}"
    )

    perf_file = test_args.test_dir.joinpath(test_args.p4_program.stem + "_perf").with_suffix(".csv")
    if perf_file.exists():
        perf = pd.read_csv(perf_file, index_col=0)
        summarized_data = [
            float(final_cov) * 100,
            num_tests,
            time_needed,
            perf["Percentage"]["z3"],
            perf["Percentage"]["step"],
            perf["Percentage"]["backend"],
        ]
    else:
        # In some cases, we do not have performance data. Nullify it.
        summarized_data = [
            float(final_cov) * 100,
            num_tests,
            time_needed,
            None,
            None,
            None,
        ]
    return summarized_data, nodes_cov, timestamps


def plot_coverage(out_dir, coverage_pairs):
    df = pd.DataFrame()

    for label, coverage in coverage_pairs.items():
        df_series = pd.Series(coverage, name="Coverage")
        df_series = df_series.to_frame()
        df[label] = df_series
    axes = sns.lineplot(data=df, ci=None, legend=False)
    axes.set(xlabel="Generated tests", ylabel="Percentage covered")
    fig = axes.get_figure()
    fig.autofmt_xdate()
    plt_name = out_dir.joinpath("plot")
    plt_name = Path(str(plt_name) + "_coverage")
    plt.savefig(plt_name.with_suffix(".pdf"), bbox_inches="tight")
    plt.savefig(plt_name.with_suffix(".png"), bbox_inches="tight")
    plt.gcf().clear()


def main(args, extra_args):
    options = Options()
    options.p4_programs = args.p4_programs
    options.max_tests = args.max_tests
    options.out_dir = Path(args.out_dir).absolute()
    options.seed = args.seed
    p4testgen_bin = testutils.check_if_file(args.p4testgen_bin)
    if not p4testgen_bin:
        return
    options.p4testgen_bin = Path(p4testgen_bin)
    options.test_backend = args.test_backend.upper()
    options.extra_args = extra_args
    options.test_mode = args.test_mode.upper()
    options.large_memory = args.large_memory

    # Create the output directory, if it does not exist.
    testutils.check_and_create_dir(options.out_dir)
    # Configure logging.
    logging.basicConfig(
        filename=options.out_dir.joinpath("coverage.log"),
        format="%(levelname)s:%(message)s",
        level=getattr(logging, args.log_level),
        filemode="w",
    )
    stderr_log = logging.StreamHandler()
    stderr_log.setFormatter(logging.Formatter("%(levelname)s:%(message)s"))
    logging.getLogger().addHandler(stderr_log)

    if options.test_mode == "TOFINO":
        options.target = "tofino"
        options.arch = "tna"
        options.test_backend = "PTF"

    if options.test_mode == "DPDK":
        options.target = "dpdk"
        options.arch = "pna"
        options.test_backend = "PTF"

    # 7189 is an example of a good seed, which gets cov 1 with less than 100 tests
    # in random access stack.
    random.seed(options.seed)
    seeds = []
    for _ in range(args.iterations):
        seed = random.randint(0, sys.maxsize)
        seeds.append(seed)
    print(f"Chosen seeds: {seeds}")

    strategies = [
        "GREEDY_STATEMENT_SEARCH",
        "RANDOM_BACKTRACK",
        "DEPTH_FIRST",
    ]
    config = {
        "DEPTH_FIRST": "",
        "RANDOM_BACKTRACK": "",
        "GREEDY_STATEMENT_SEARCH": "",
    }
    p4_program = options.p4_programs[0]
    p4_program = testutils.check_if_file(p4_program)
    if not p4_program:
        return
    p4_program = Path(testutils.check_if_file(p4_program))
    p4_program_name = p4_program.stem
    # csv results file path
    for strategy in strategies:
        test_dir = options.out_dir.joinpath(f"{strategy.lower()}")
        testutils.check_and_create_dir(test_dir)
        summary_frame = pd.DataFrame(
            columns=[
                "Program",
                "Seed",
                "Coverage",
                "Generated tests",
                "Total time (s)",
                "% Z3",
                "% stepper",
                "% test generation",
            ]
        )
        coverage_pairs = {}
        timeseries_frame = pd.DataFrame(columns=["Seed", "Time", "Coverage"])
        for seed in seeds:
            data_row = [p4_program_name, seed]
            print(f"Seed {seed} Generating metrics for {strategy} up to {options.max_tests} tests")
            test_args = TestArgs()
            test_args.seed = seed
            test_args.test_dir = Path(tempfile.mkdtemp(dir=test_dir))
            test_args.p4_program = p4_program
            test_args.strategy = strategy
            test_args.extra_args = config[strategy]
            if options.extra_args:
                test_args.extra_args += " " + " ".join(options.extra_args[1:])
            summarized_data, nodes_cov, timestamps = run_strategies_for_max_tests(
                options, test_args
            )
            data_row.extend(summarized_data)
            summary_frame.loc[len(summary_frame)] = data_row
            coverage_pairs[str(seed)] = nodes_cov
            sub_frame = pd.DataFrame()
            sub_frame["Seed"] = [seed] * len(nodes_cov)
            sub_frame["Coverage"] = nodes_cov
            sub_frame["Time"] = convert_timestamps_to_timedelta(timestamps)
            sub_frame["Time"] = sub_frame["Time"] / pd.Timedelta(milliseconds=1)

            timeseries_frame = pd.concat([timeseries_frame, sub_frame])
        # timeseries_frame.set_index(["Seed"], inplace=True)
        summary_frame.set_index(["Program", "Seed"], inplace=True)
        timeseries_frame.set_index(["Seed"], inplace=True)
        sns.lineplot(x="Time", y="Coverage", data=timeseries_frame)
        summary_frame["Time per test (s)"] = (
            summary_frame["Total time (s)"] / summary_frame["Generated tests"]
        )
        summary_frame.loc["Mean"] = summary_frame.mean(numeric_only=True, axis=0, skipna=True)
        summary_frame.loc["Median"] = summary_frame.median(numeric_only=True, axis=0, skipna=True)
        summary_frame.loc["Min"] = summary_frame.min(numeric_only=True, axis=0, skipna=True)
        summary_frame.loc["Max"] = summary_frame.max(numeric_only=True, axis=0, skipna=True)

        summary_results_path = options.out_dir.joinpath(
            f"{p4_program_name}_{strategy.lower()}_summary_results.csv"
        )
        summary_frame.to_csv(summary_results_path)
        timeseries_results_path = options.out_dir.joinpath(
            f"{p4_program_name}_{strategy.lower()}_coverage_over_time.csv"
        )
        timeseries_frame.to_csv(timeseries_results_path)


if __name__ == "__main__":
    # Parse options and process argv
    arguments, argv = PARSER.parse_known_args()
    main(arguments, argv)
