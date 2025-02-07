# bfas command line options

usage: bfas [ options ] file.bfa

### general options

* -h
  help

* --target *target*

  specify the target (obsolete as target is generally specified in the .bfa file)

* -Werror

  treat warnings as errors

### options for controlling output

* -a
* --allpipes

  Generate a binary that has explicit writes for all pipes, rather than just one

* -s
* --singlepipe
* --pipe*N*

* -G
* --gen\_json

  Generate .cfg.json files instead of binary

* --no-bin
* --num-stages-override*N*

* -M

  Attempt to match glass bit-for-bit

* -o *directory*

  Generate output in the specified directory rather than in the current working dir

### options for controling cfg details

* -C
  condense json by stripping out unset subtrees (default)

* --disable-egress-latency-padding

  Disable the padding of egress latency to avoid tofino1 TM overrun bus

* --disable-longbranch
* --enable-longbranch

  Disable or enable support for long branches

* --disable-tof2lab44-workaround

* --high\_availability\_disabled
* --multi-parsers
* --no-condense
* --noop-fill-instruction *opcode*
  
  Insert instructions (of the form *opcode* R, R, R) for noop slots in VLIW instructions
  where the slot is not used by any action in the stage.  *opcode* must be one that is an
  identity function when applied to two copies of the same value (and, or, alu\_a, alu\_b,
  mins, maxs, minu, or maxu)

* -p
  Disable power gating

* --singlewrite
* --stage\_dependency\_pattern *pattern*
* --table-handle-offset*N*

### options for logging/debugging

* -l *file*

  redirect logging output to file

* --log-hashes

* -q

  disable all logging output

* --no-warn

* -T *debug spec*

  enable logging of specific source files and specific levels

* -v

  increase logging verbosity
