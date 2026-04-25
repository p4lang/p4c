import argparse
import logging
import os
import subprocess
import sys
import tempfile
from pathlib import Path
from typing import Any, Optional

PARSER = argparse.ArgumentParser()
PARSER.add_argument(
    "-td",
    "--testdir",
    dest="testdir",
    help="The location of the test directory.",
)
PARSER.add_argument("p4_file", help="The absolute path for the p4 file to process.")
PARSER.add_argument(
    "p4rtsmith",
    help="The absolute path for the p4rtsmith bin file for this test.",
)
PARSER.add_argument(
    "-s",
    "--seed",
    default=1,
    dest="seed",
    help="Random seed for generating configs.",
)


# Parse options and process argv
ARGS, ARGV = PARSER.parse_known_args()

# Append the root directory to the import path.
FILE_DIR = Path(__file__).resolve().parent


class Options:
    """Options for this testing script. Usually correspond to command line inputs."""

    # File that is being compiled.
    p4_file: Path = Path(".")
    # The path for the p4rtsmith bin file for this test.
    p4rtsmith: Path = Path(".")
    # Actual location of the test framework.
    testdir: Path = Path(".")
    # Random seed for generating configs.
    seed: int = 1


def generate_config(p4rtsmith_path, seed, p4_program_path, testdir, config_file_path):
    command = f"{p4rtsmith_path} --target bmv2 --arch v1model --seed {seed} --output-dir {testdir} --generate-config {config_file_path} {p4_program_path}"
    subprocess.run(command, shell=True)


def run_test(run_test_script, p4_program_path, config_file_path):
    command = f"sudo -E {run_test_script} .. {p4_program_path} -tf {config_file_path}"
    subprocess.run(command, shell=True)


def find_p4c_dir():
    current_dir = Path(__file__).resolve()

    while True:
        p4c_build_dir = current_dir / "p4c"
        if p4c_build_dir.exists():
            return p4c_build_dir

        current_dir = current_dir.parent

        if current_dir == current_dir.parent:
            raise RuntimeError("p4c/build directory not found.")


def run_tests(options: Options) -> int:
    config_file_path = "initial_config.txtpb"

    seed = options.seed
    testdir = options.testdir
    p4rtsmith_path = options.p4rtsmith
    run_test_script = FILE_DIR / "run-bmv2-proto-test.py"
    p4_program_path = options.p4_file

    generate_config(p4rtsmith_path, seed, p4_program_path, testdir, config_file_path)
    run_test(run_test_script, p4_program_path, testdir / config_file_path)


def create_options(test_args: Any) -> Optional[Options]:
    """Parse the input arguments and create a processed options object."""
    options = Options()
    options.p4_file = Path(test_args.p4_file).absolute()
    options.p4rtsmith = Path(test_args.p4rtsmith).absolute()

    p4c_dir = find_p4c_dir()
    p4c_build_dir = p4c_dir / "build"
    os.chdir(p4c_build_dir)

    testdir = test_args.testdir
    if not testdir:
        testdir = tempfile.mkdtemp(dir=Path(".").absolute())
        os.chmod(testdir, 0o755)
    options.testdir = Path(testdir)
    options.seed = test_args.seed

    # Configure logging.
    logging.basicConfig(
        filename=options.testdir.joinpath("test.log"),
        format="%(levelname)s: %(message)s",
        level=getattr(logging, "WARNING"),
        filemode="w",
    )
    stderr_log = logging.StreamHandler()
    stderr_log.setFormatter(logging.Formatter("%(levelname)s: %(message)s"))
    logging.getLogger().addHandler(stderr_log)
    return options


if __name__ == "__main__":
    test_options = create_options(ARGS)
    if not test_options:
        sys.exit()

    run_tests(test_options)
