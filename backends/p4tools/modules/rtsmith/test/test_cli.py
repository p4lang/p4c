#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 The P4 Language Consortium
# SPDX-License-Identifier: Apache-2.0

"""Check RtSmith's file output, stdout, and configuration error handling."""

import subprocess
import sys
import tempfile
from pathlib import Path


def main():
    binary, program = map(lambda arg: str(Path(arg).resolve()), sys.argv[1:])
    command = [binary, "--target", "bmv2", "--arch", "v1model", "--seed", "1"]

    def run(*args, success=True):
        result = subprocess.run(
            [*command, *map(str, args), program], capture_output=True, text=True
        )
        assert (result.returncode == 0) == success, result.stderr
        return result

    with tempfile.TemporaryDirectory(prefix="rtsmith-cli-") as temporary:
        directory = Path(temporary)
        info = directory / "p4info.txtpb"
        output = directory / "nested" / "output"
        result = run(
            "--generate-p4info",
            info,
            "--output-dir",
            output,
            "--config-name",
            "trial.part",
            "--print-to-stdout",
        )
        assert "Generated initial configuration:" in result.stdout
        assert "updates {" in result.stdout
        assert (output / "trial.part_initial_config.txtpb").is_file()
        assert not (output / "initial_config.txtpb").exists()
        assert not list(output.glob("update_*.txtpb"))
        # An empty P4Info is valid and proves --user-p4info overrides inferred tables.
        empty_info = directory / "empty.txtpb"
        empty_info.write_text("")
        supplied = directory / "supplied"
        run("--user-p4info", empty_info, "--output-dir", supplied)
        assert (supplied / "initial_config.txtpb").read_text() == ""
        config = directory / "config.toml"
        for value in [
            "maxTables = -1",
            "maxUpdateCount = -1",
            "minUpdateTimeInMicroseconds = 200000",
            "maxTables = [",
        ]:
            config.write_text(value)
            run("--toml", config, success=False)
        config.write_text("maxTables = 0")
        run("--toml", config, "--output-dir", supplied)
        assert (supplied / "initial_config.txtpb").read_text() == ""
        run("--control-plane", "BFRUNTIME", success=False)
        run("--config-name", "../escape", success=False)
    return 0


if __name__ == "__main__":
    sys.exit(main())
