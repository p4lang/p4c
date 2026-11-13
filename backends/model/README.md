# The p4c-model compiler

This is a special backend for the p4c compiler which aids in building
other backends for the p4c compiler.

The input to this compiler is a *model.p4 file, describing
capabilities of a P4 target.  The output of this compiler is set of of
files:

- one *.def file, which defines compiler IR classes that represent the
  various objects defined in the model: externs, extern functions,
  controls, parsers, and packages.  Each such object has a custom
  associated IR class.  For example, when run on v1model.p4 this would
  produce an IR class named `counter_instance` for `counter` which can
  be used to represent counter instantiations.

```
class counter_instance {
   Type      type_parameter_I;
   Argument  size_constructor_argument;
   Argument  type_constructor_argument;
}
```

- one *.h/cpp pair of files, defining a Transform class.  The
  transform class takes a P4 program written for this specific model,
  validates that the model objects are used correctly (e.g., correct
  number of type parameters, correct number of arguments, etc), and
  replaces the general-purpose IR classes (i.e., `Declaration_Instance`
  for a `counter` extern) with more specific instances (i.e.,
  the just-defined `counter_instance` ).

The generated code is meant to be included in the backend of a
compiler for the specific model.  The generated visitor will automate
checking the correct usage of the model.  The generated IR classes
will make code generation much simpler and readable.
