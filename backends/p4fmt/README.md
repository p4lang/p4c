<!--!
\page p4fmt p4fmt (P4 Formatter)                                                               
-->
<!--!
\internal
-->
# p4fmt (P4 Formatter)
<!--!
\endinternal
-->
<!--!
[TOC]
-->
p4fmt is a WIP formatter for P4. It's in a highly experimental phase
and, not yet stable/reliable for general use.
Contributions and feedbacks from the community
would be highly appreciated.

## Build
- Setup P4C correctly, from [here](https://github.com/p4lang/p4c#dependencies).

- Follow the instructions here to [build](https://github.com/p4lang/p4c#installing-p4c-from-source).

Later `p4fmt` executable can be found inside the `p4c/build/` dir, and can be invoked as `./build/p4fmt` from p4c's root dir.

## Usage
- Takes an output file with a `-o` flag and writes to it.

    `./build/p4fmt <p4 source file> -o <output_file>`

 - If an output file is not provided with `-o` flag, it just prints the output on the stdout.

    `./build/p4fmt <p4 source file>`

Sample Usage:

    ./build/p4fmtsample.p4
    ./build/p4fmt sample.p4 -o out.p4

## Reference Checker for P4Fmt

Sample Usage:
    `./build/checkfmt --file <p4 source file> --reference-file <p4 reference file>`
