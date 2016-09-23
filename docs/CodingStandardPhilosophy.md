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
