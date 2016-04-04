C++ (following the C++ 11 standard) is the implementation language for
the compiler.  This file contains notes on how we use C++ and conventions
we use for things that C++ does not handle well.

### Multiple virtual dispatch

C++ does not support multiple virtual dispatch, but we use it heavily in
the design of the Visitor classes for the compiler.  To support this use
(reasonably) cleanly, we use a preprocessor `ir-generator` to read the
IR classes from `.def` files and generate all the necessary boilerplate
for multiple dispatch in the IR and Visitor classes.

### Inner classes

C++ does not handle inner classes natively, so to implement an inner class,
we use a nested class with a reference member to the containing class
called `self`.
