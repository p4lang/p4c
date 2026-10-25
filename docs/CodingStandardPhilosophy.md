<!--
SPDX-FileCopyrightText: 2016 Barefoot Networks, Inc.

SPDX-License-Identifier: Apache-2.0
-->

<!-- 
Documentation Inclusion:
This README is integrated as a subsection of the "Contribute to the P4 Compiler Project" page in the P4 compiler documentation.

Refer to the specific section here: [Coding Standard - Subsection](https://p4lang.github.io/p4c/contribute.html#coding-standard)
-->

# Coding Standard

When writing code in any language the most important consideration is
readability.  Code will be read by many more people and many more times
than it will be written.  So the goal is to write code that clearly and
concisely describes what the code is doing, in terms of data structures,
algorithms and interfaces.  Expressing why the code is organized the way
it is and how it achieves its goals is also necessary, and should be done
in supporting documentation, as it cannot generally be expressed directly
in the structure of the code.  Having good supporting documentation is
not an excuse for poorly written code, and well written code cannot make
up for lacking documentation.

There are multiple modes in which people read code.  They may read the code
in depth to try to understand the algorithms used in the code, or they may
scan over the code looking to understand the structure and interfaces of
the code.  Good coding style supports both kinds of readers.  When writing
code, you need to consider how that code appears to readers who have limited
understanding of the code as well as readers who already mostly understand
it.  It should be easy for first-time readers to find where to start and
for experienced readers to find the details they are looking for.

There are many different kinds of code involved in any project.  Some code
is important algorithms that program uses.  Other code is primarily about
building and managing data structures that algorithms operate on.  Much
code is "boilerplate"; code that exists primarily to make the compiler or
build system work properly and is not really relevant to understanding
the program or what it is trying to do.  Code should ideally be designed
so the scanning eye can easily separate the relevant, important code
from the boilerplate.

Code should therefor be organized in "chunks" that can be though of as
corresponding to paragraphs in written text.  These chunks should be
visible blocks in a superficial scan of the code, separated from each
other by vertical white-space (blank lines).  A chunk should be at least
3-5 lines (less than that doesn't convey adequate information or stand
out enough from blank space), and at most 20-40 lines.  These limits are
not hard and fast rules, but guidelines -- when writing code try to see
how it appears to a surface scan trying to organize it visually in
paragraph-style chunks.

Applying this to C++ code, a function may naturally express a chunk,
or multiple chunks, or multiple small, related functions may be a chunk.
Similarly, a class or struct definition may naturally be one or multiple
chunks.  This chunking informs where the brace characters should be placed,
as brace by itself on a line makes a vertical space that separates chunks.
So braces can be placed by themselves when they coincide with a chunk
boundary, and should be placed with other text (generally the line above)
when they do not.

Oftentimes, boilerplate code does not fit naturally into this chunking
style -- while you can have a chunk of boilerplate followed by a chunk
of "real" code, doing this can interfere with readability.  To the
extent possible, boilerplate should be separated, to become part of the
white-space between chunks.  A one or two line (too small to be a real
chunk) element between two chunks can be good for this.

C++ code contains many nested constructs (loops, conditions, classes,
functions) that are delineated by brace ({..}) characters.  These
punctuation characters are easily missed on a quick scan or read of the
code, so it is important to use indentation to make this nesting clear.
Indentation should ALWAYS be kept consistent with code structure as defined
by the braces, so the braces themselves can be ignored by the reader.  At
the same time, this nesting (and indentation) is really independent of
the chunk/paragraph structuring of the code, so individual chunks can
both contain indentation (for nested structure within a chunk) and be
independently indented (for nested structures encapsulating multiple
chunks).  Too much indentation results in chunks that don't look like
chunks, obscuring the vertical spacing that splits chunks, so any time
indentation gets to more than 4-6 levels, it is a good indication that
the code should be refactored.

When you look at a visual item with a repeating pattern, the brain tends
to accentuate the pattern and edit out the differences.  When you have
code that is repetitive, this is pretty much exactly wrong -- in repetitive
code, the differences between the repeated things are usually what is
important, while the repeating pattern is mostly less important.  Thus,
repeating (visual) patterns should usually be avoided.  This is an aspect
of the DRY (do not repeat yourself) principle in software engineering,
since any repetition tends to lead to repeating patterns that are then
harder to read.

## Commenting the code

Code comments should be meaningful and aid to the understanding of the
code. We use [Doxygen](http://www.doxygen.nl/manual/docblocks.html)
style comments. Comments should reflect the programmer's intention
(rather than enumerating the code) and the reasoning behind certain
decisions. Comments should also capture invariants that are not
directly visible in the code. There are arguments that code should be
self-documenting, and indeed we encourage the writing of clean
self-explanatory code. Comments should contribute the additional
meaning that helps extensibility and maintainability by a large group
of developers.

## Handling errors

The main goal of issuing errors and warnings is for the programmer to
write correct P4 code. Errors and warnings should be _actionable_,
i.e., the programmer needs to understand what was wrong with the
program and if possible, get an idea on how to fix the problem. While
error messages are not intended to replace learning the language and
reading the language specification, there are many instances in which
the compiler messages really help emphasizing certain semantics
aspects that are overlooked. Therefore, please take the time to think
through the information you want to convey and write good, explicit
error messages.

An additional goal of the P4C compiler is to provide as many error
messages as possible in one go. Therefore, while there is support for
`FATAL_ERROR`s, it is desirable to try to continue execution and
report all possible errors, using the `error` and `warning`
calls. With the free form implementation of error messages, repeated
passes of the compiler will then issue the same message multiple
times. This results in frustrating the programmer.

To address the repeated message issue, as of Dec 2018, we introduce
error/warning types. They classify the errors and impose a format that
allows the compiler to automatically filter repeated messages. The
filtering is based on the type of error and the source code location
of the object that reports the error. Thus, it allows multiple error
types per source code line, and ensures that only one error is
reported even if the message is raised multiple times. We encourage
compiler developers to use this method for issuing errors. The error
codes and formats are defined in `lib/error_catalog.[h,cpp]`. Backends
can extend the codes and formats as needed (and they are encouraged to
do so).

## Diagnostic Messages

The `error` and `warning` functions and bug-reporting macros take a
message followed by the values to insert into it. Pass objects directly;
there is no need to convert them to strings first. For errors and
warnings, the diagnostic helpers add the severity, source location, and
source excerpt when available, so write only the message itself.

### Choosing placeholders

Prefer Abseil's printf-style placeholders for new diagnostic messages.
Boost-style placeholders are still supported, but may be deprecated in
the future. Both work through the same diagnostic functions; there is
no formatter to select.

- `%s`, `%d`, `%x`, `%.2f`, etc. consume arguments from left to right.
  Use `%s` for an object's usual text, including IR nodes and numbers;
  it is not restricted to strings. Use numeric conversions when the
  presentation matters: `%d` for an integer, `%x` for hexadecimal, or
  `%.2f` for a floating-point value with two decimal places. Abseil's
  `%v` conversion, which chooses a representation based on the type,
  is also supported.
- `%1$s`, `%2$x`, etc. combine explicit argument numbers with conversion
  options, counting from one. Use these when you need to repeat arguments
  or display them in a different order.
- Boost-style `%1%`, `%2%`, etc. insert the corresponding argument's usual
  text. The bracketed form `%|2$8x|` is also supported. Prefer `%1$s`,
  `%2$s`, and `%2$8x`, respectively, when writing new messages.

Choose either numbered or sequential arguments for each message. Different
placeholder spellings can be mixed as long as that choice is consistent:

```cpp
// Preferred: consume arguments from left to right.
error(ErrorType::ERR_INVALID, "%s has invalid mask 0x%x", field, mask);

// Also preferred when explicit argument numbers are useful.
error(ErrorType::ERR_INVALID, "%1$s has invalid mask 0x%2$x", field, mask);

// Supported: different spellings, but all placeholders are numbered.
error(ErrorType::ERR_INVALID, "%1% has invalid mask 0x%2$x", field, mask);

// Invalid: numbered and sequential placeholders in the same message.
error(ErrorType::ERR_INVALID, "%1% has invalid mask 0x%x", field, mask);
```

Width and precision options such as `%8s`, `%08x`, and `%.2f` are supported.
If width or precision is supplied by a `*` argument, that argument must
follow the same numbering rule. Write `%%` for a literal percent sign;
it does not consume an argument. Brace placeholders such as `{}`, Boost's
tabulation and centering options, and printf's `%n` are not supported.

Messages may be string literals or constructed at runtime. Formats are
checked when used: malformed placeholders, incorrect argument counts, and
incompatible numeric conversions are errors in the compiler, not errors
in the P4 program. For example, `%d` cannot format a floating-point value;
use `%s` for its usual text or `%f` to control its numeric presentation.

### Displaying objects and source locations

For `%s` (or `%N%`), errors and warnings use an object's `toString()` when
available, otherwise its stream output (`operator<<`). Bug reports use
`dbprint()` when available to show more detail, otherwise stream output.

Passing an IR node also supplies its source location. To attach a location
without printing an object, pass a `SourceInfo` argument. It still needs a
placeholder, but contributes no text there: the location and source excerpt
are displayed separately. Do not add a colon or space for that placeholder.
For example:

```cpp
// Given a pointer ref to an IR::NamedRef:
error(ErrorType::ERR_INVALID,
      "%sNo header or metadata named '%s'", ref->srcInfo, ref->name);
```

Example output:

```
missing_decls1.p4(6): [--Werror=invalid] error: No header or metadata named 'data'
    if (data.b2 == 0) {
        ^^^^
```

## Git commits and pull requests

Git histories are a beautiful tool to understand design decisions and
build a knowledge base for resolving issues. Commit messages allow
relating issues to resolutions and to explain the resolutions
to everyone. Writing good commit messages takes some practice,
fortunately, there are a number of simple guideline steps that go a
long way. Universally, the recommendation is to use a 50 character
summary line, followed by a detailed explanation of your commit, why
it is necessary (link to the relevant issue), how it addresses the
issue, what are its implications. For a more detailed exposure,
including guidelines on how to express the text, see this
[blog post](http://chris.beams.io/posts/git-commit/).

Github pull requests (PRs) created based on a single commit will
inherit the commit message, and thus allow your reviewers to
understand the work done without chasing multiple other issues, email
messages, etc. Multi-commit PRs do not have this feature, however, we
encourage you to cut and paste from your commit messages and provide a
synopsis of what the PR is trying to accomplish. It is also strongly
recommended that multi-commit PRs squash all their commits in to a
single commit when merged. The Github web interface makes it very easy
to do, and allows you to edit the final commit message directly in the
browser.
