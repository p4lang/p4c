# SPDX-FileCopyrightText: 2026 The P4 Language Consortium
# SPDX-License-Identifier: Apache-2.0

import json
import subprocess
import sys
import tempfile
from pathlib import Path


def main(compiler, program):
    with tempfile.TemporaryDirectory() as tmp:
        output = Path(tmp) / "out"
        target = ["--std", "p4-14", "--target", "tofino", "--arch", "v1model"]
        subprocess.run([compiler, *target, "-o", output, program], check=True)
        context = json.loads((output / "context.json").read_text())
        counts = next(table for table in context["tables"] if table["name"] == "counts")
        values = counts["initial_value_lo"], counts["initial_value_hi"]
        if values != (17, 42):
            raise AssertionError(f"expected register initializers (17, 42), got {values}")
        assembly = (output / (Path(program).stem + ".bfa")).read_text()
        # The sum of two negative inputs can overflow a signed 32-bit value.
        # lss.s tests the full sum; lss.uus tests bit 31 of the truncated sum.
        if not any(line.strip().startswith("- lss.s ") for line in assembly.splitlines()):
            raise AssertionError("expected a signed comparison of the full register sum")


if __name__ == "__main__":
    main(*sys.argv[1:])
