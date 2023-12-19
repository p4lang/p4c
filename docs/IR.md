<!--
SPDX-FileCopyrightText: 2016 Barefoot Networks, Inc.

SPDX-License-Identifier: Apache-2.0
-->

<!--!
\page intermediate_representation_ir P4C Intermediate Representation (IR)                                                                                                                          
-->
<!-- 
Documentation Inclusion:
This README is integrated as a standalone page in the P4 compiler documentation.

Refer to the full page here: [P4C Intermediate Representation (IR)](https://p4lang.github.io/p4c/intermediate_representation_ir.html)
-->
<!--!
\internal
-->
# P4C Intermediate Representation (IR)
<!--!
\endinternal
-->

<!--!
[TOC]
-->
## Introduction

This document outlines the design of the P4_16 compiler.
It lays out the various classes and data structures used for the
compiler and describes the general flow of the compilation process.
The compiler is designed to operate in the `traditional’ manner,
converting the source P4 program to an internal representation, the
performing a series of transformational passes on the IR, culminating
in an IR representation that maps more or less directly to the target
code.  Along the way, the set of IR classes present in the code may
change, with certain constructs appearing only in the initial IR from
the parser, and other constructs being introduced through the
compilation process.

Novel to this design is the ability to always return to an earlier
version of the IR (backtracking across passes) or use multiple threads
to operate on the IR, allowing different strategies to be tried in the
search for a final result.  Allowing this forking and backtracking is
central to the IR and transformation design.  The IR is designed to
represent the program as a tree or DAG from a single root node, with
each node referring only to its children.  Transformations operate by
creating new IR nodes that are shallow copies of existing nodes, and
then creating new parent nodes to refer to these new nodes, up to the
root of the IR.  The internal representation is designed in C++ around a
single inheritance object hierarchy with an ultimate base Node type
used by all subclasses.  The base Node class supports the
functionality needed for visitor transformations – clone, equality
tests, and subclass dispatch.

The IR needs to be extensible, allowing for the addition of new
classes to the hierarchy as support for new targets is added.  The initial IR
produced by the parser contains constructs that are related only
to the source P4 language, and there will be frontend transformation
passes that convert things to a simpler and more canonical form.

The IR may be a tree or DAG, but allowing back/up references in the IR
(cycles) defeats much of the benefit of the immutable IR.  If cycles
(for example, leaf nodes for names directly referring to the named
object) were allowed, pretty much any minor transform would end up
cloning the entire IR graph, as modifying a leaf involves cloning the
object containing the leaf, and then any object referring to that
object, recursively.

## Structural comparison and ordered collections

Use `Node::structuralCompare` to compare IR contents independently of allocation
identity. It returns `std::weak_ordering`: compare the result with zero to test
for less, equivalent, or greater. Include `ir/compare.h` to use
`IR::StructuralLess` as the comparator for an ordered map or set:

```cpp
const IR::Node *a = /* ... */;
const IR::Node *b = /* ... */;
bool sameStructure = a->structuralCompare(*b) == 0;
std::set<const IR::Node *, IR::StructuralLess> nodes;
nodes.insert(a);
nodes.insert(b);  // Does not add another key if a and b are structurally equivalent.
```

The free function `IR::structuralCompare(a, b)` in `ir/structural_compare.h`
also compares nullable pointers, containers, tuples, and other field values.
`IR::StructuralLess` accepts node pointers or node references, including derived
nodes. A null pointer sorts before a non-null pointer.

### What equivalence means

Structural comparison provides a strict weak ordering of selected IR contents.
It does not prove that two programs compute the same result. For example,
`x + 0` and `x` have different structures. Paths compare by their stored names;
comparison does not resolve declarations or account for scopes, alpha-renaming,
or commutativity. Structurally equivalent subtrees from different scopes are
not necessarily interchangeable.

The compiler's comparison APIs have distinct contracts:

| API | Meaning |
| --- | --- |
| Pointer `==` | Same allocation. |
| Node `operator==` | Shallow comparison, with node-specific overrides; pointer fields generally compare by identity. |
| Node `equiv` | Recursive equality with its own node-specific overrides. |
| `structuralCompare` equal to zero | Equivalent under the structural ordering, including the exclusions below. |

Structural equivalence is deliberately separate from `operator==`. For example,
two separately created declarations can have different `declid` values and
compare unequal with `operator==`, yet occupy one key in a structural set.
It can also differ from `equiv`: constants with the same type and value but
different display bases compare structurally equivalent, while `equiv` includes
the base. Consequently this API is a named member and an explicit comparator;
it does not define `Node::operator<` or `Node::operator<=>`. Use
`structuralCompare(...) == 0` when equality must agree with `StructuralLess`.
Existing hashes must not be assumed to agree with this equivalence. A matching
structural hash is separate work.

This terminology follows the distinction between identity and structure used by
[TVM's structural equality](https://tvm.apache.org/ffi/concepts/structural_eq_hash.html).
The three-way result also follows the comparison approach used by
[LLVM's FunctionComparator](https://llvm.org/docs/MergeFunctions.html).
The details of p4c's equivalence are specified here, rather than inherited from
those frameworks.

### Ordering rules

Generated comparisons first check for the same node, then compare runtime
`typeId()` values. For equal runtime types they compare the base class once,
then fields in declaration order, stopping at the first difference.
Cross-type ordering is an implementation detail: it is not alphabetical,
a language-level ordering of types, or a stable serialization order across
compiler versions and build configurations.

Field comparison follows these rules:

- Pointers compare their pointees recursively; identical pointers short-circuit.
  Pointer addresses never break ties between distinct allocations.
- Tuples compare element by element. Ranges, including vectors and maps,
  compare lexicographically in iteration order; a proper prefix sorts first.
  Map entries compare their keys and then values. This is an order of the
  sequence exposed by the container, not an order-independent comparison of
  sets or maps. In particular, an address-ordered or unordered container of
  pointers does not become canonical just by comparing its elements deeply.
- Optionals order absence before presence, then compare contained values.
  Variants compare alternative indices first and then the active value.
  A valueless variant sorts last, as its index is `std::variant_npos`.
- Strings use their value order, preserving `cstring`'s distinction between
  null and empty. Scalars use their value comparison. A custom
  `structuralCompare` member takes priority over generic traversal.

Generated comparisons inspect fields declared to the IR generator. C++ members
written inside `#emit` blocks are not included automatically. They omit
`SourceInfo` fields and the identity metadata in `Node`. Custom comparisons also
exclude selected fields:

| Node or value | Excluded information |
| --- | --- |
| `ID` | Source location and original spelling; compares the current name. |
| `Declaration`, `Type_Declaration` | Unique declaration IDs. Derived fields still participate. |
| `Type_Any`, `Type_InfInt` | Unique declaration IDs. |
| `Constant` | Display base; type and numerical value still participate. |
| `SymbolicVariable`, `ConcolicVariable` | Fields other than the label, within the same runtime type. |
| P4Tools `StateVariable` | Type; uses the existing reference/path comparison. |
| P4Tools `Extracted_Varbits` | Assigned size from execution state. |
| Tofino `AliasMember` | Alias source; compares the inherited member expression. |
| Tofino `TableSeq` | Sequence tracking ID. |
| Tofino `HashGenExpression` | Static hash tracking ID and `any_alg_allowed`; dynamic hash IDs participate. |
| Tofino `Table` | Physical resource-allocation records (`resources`). |

The Tofino resource exclusion has a TODO in the implementation to determine
whether allocation contents need comparison. Comparing that separate graph
would require its own equivalence contract.

### Extending and using the comparison safely

For most new nodes, the generated implementation is sufficient. A custom
`.def` `structuralCompare` body must return `std::weak_ordering`, check runtime
types before casting, and preserve strict weak ordering. Compare a base class
or child **once** and return a nonzero result immediately:

```cpp
if (auto order = Parent::structuralCompare(a_); order != 0) return order;
auto &a = static_cast<const MyNode &>(a_);
return IR::structuralCompare(field, a.field);
```

This fragment assumes the usual identity and runtime-type checks precede it.
If a field type has no supported value comparison, supply a custom comparison
instead of ordering it by address. Document intentional field exclusions and
add gtests for equivalence and the first differing field. Custom scalar `<`
operators used by the generic helper must themselves be strict weak orderings.

Do not modify a key, or any transitively compared child, while it is stored in
an ordered container. Remove it before mutation and reinsert it afterward.
The ordering supports acyclic IR trees and DAGs; cycles are unsupported.

### Performance

Each recursive comparison produces all three outcomes in one traversal.
This avoids comparing an equal subtree again in the reverse direction to
establish equivalence. Repeating two boolean comparisons at each recursive
level can multiply the work exponentially even on a simple unary chain.
The gtests count leaf comparisons in equal unary chains, containers, and tuples,
and check that shared pointers and an earlier difference stop traversal.

Work is proportional to the inspected portion of the unfolded structure,
including field-value comparisons, and recursion uses stack space proportional
to its depth. Comparing the same pointer is constant-time. Comparing two
separately allocated DAGs may revisit the same node pair along multiple paths;
the implementation does not promise linear work in the number of unique DAG
nodes. It does not allocate a memoization table on each comparison or cache
results across mutations. If profiling shows repeated DAG pairs dominate,
a per-comparison pair cache is a possible follow-up; any structural hashing or
persistent cache must respect exactly the same field exclusions and mutation
rules.

## Visitors and Transforms

The compiler is organized as a series of `Visitor` and `Transform`
passes [Wikipedia](https://en.wikipedia.org/wiki/Visitor_pattern).  The `Visitor`
and `Transform` base classes make defining new passes easy -- a new
transform need only specify the IR types it is interested in, and can
ignore others.  A (constant) `Visitor` pass visits every node in the
IR tree and accumulates information about the tree but does not modify
it, while a Transform pass visits every node, possibly modifying the
node or replacing it with some other node

```C++
    /* pseudo-code for basic Transform visitor */
    visit(node, context=ROOT, visited={}) {
	if (node in visited) {
		if (visited[node] == INVALID)
			throw loop_detected
		if (VisitDagOnce)
			return visited[node] }
	visited[node] = INVALID
	copy = shallow_clone(node)
	if (VisitDagOnce)
		forward_children(copy, visited)
	copy = preorder(copy, context) // override in Transform subclass
	visit_children(copy, {copy, context}, visited)
	copy = postorder(copy, context) // override in Transform subclass
	if (*copy == *node) {
		visited[node] = node
		return node
	} else {
		visited[node] = copy
		return copy }
    }
    forward_children(node, visited) {
	for (child in children(node))
		if (child in visited)
			child = visited[child]
    }
    visit_children(node, context, visited) {
	for (child in children(node))
		child = visit(child, context, visited)
    }
```

A `Transform` pass is allowed to freely modify or completely replace a
node in its preorder and postorder routines, and the Transform visitor
will automatically rebuild the tree as needed.  The transform
subroutines cannot modify other nodes in the tree, however, though
they may access and refer to them. These routines can be overloaded to
differentiate between different IR subclasses automatically.  Any
given visitor need only override the routines it is interested in for
the IR types it is interested in.

There are several Visitor subclasses that describe different types of visitors:

Visitor       |  Description
--------------|--------------------------------
`Inspector`   | simplified visitor that does not modify any nodes, just collects information.
`Modifier`    | simplified visitor that does not change the tree/dag structure, but may modify nodes in place.
`Transform`   | full transformation visitor described above
`PassManager` | combines several visitors, run in a sequence

There are also some interfaces that Visitor subclasses can implement to alter how nodes are visited:

Interface  | Description
-----------|------------------------
`ControlFlowVisitor` | visit nodes in control-flow order, splitting (cloning) the visitor at conditions and merging the clone at join points
`Backtrack`	     | visitor that is notified when backtracking occurs and can have its state saved as a backtrack point.

#### ControlFlowVisitor

To support control flow analysis, there is a special `ControlFlowVisitor` interface that is
understood by certain IR subclasses that encapsulate control flow, and is used by them to
control the order in which children are visited and the visitor state passed to the visitor
preorder/postorder routines in order to implement control-flow analysis.

The basic idea is that control flow IR nodes, when visiting their children, will create
clones of the `ControlFlowVisitor` object to visit different children after a branch, and
will then call `flow_merge` at the join point to merge the visitor state.  Both the `Visitor`
base class and the `ControlFlowVisitor` interface define two virtual functions: `flow_clone`
and `flow_merge` to mediate this.  In the `Visitor` base class (used by all visitors that
do *not* inherit from the `ControlFlowVisitor` interface, these are implemented as no-ops --
`flow_clone` just returns the same object, and `flow_merge` does nothing.  In the
`ControlFlowVisitor` interface `flow_clone` calls the `clone` function (which must be
implemented by the leaf class) and `flow_merge` is an abstract virtual function that must
be implemented by the leaf class.

As an example for how these are used in the IR visitor routines, the IR::If class has
three children to visit -- a predicate and two consequents, with the value of the
predicate deciding which consequent to execute.  The child visitor for `IR::If` is

```C++
    visitor.visit(predicate);
    clone = visitor.flow_clone();
    visitor.visit(ifTrue);
    clone.visit(ifFalse);
    visitor.flow_merge(clone);
```

So both consequents are visited with a visitor state that corresponds to the control flow
immediately after evaluating the predicate, and then the control flow state after both
consequents are merged back together.

In order to make use of this functionality, a `Visitor` sub class need only inherit from
the `ControlFlowVisitor` interface and implement the `clone` and `flow_merge` routines.
Other `Visitor` classes don't need to do anything, but may want to implement `clone` for
other reasons.

This control flow analysis only works for branch-and-join control flow -- it is not adequate
for loops (which generally requires an iterate to fixed point process).  As our IR does not
(currently) support loops this is fine.  Since P4 does not allow loops in the
match-action part of the pipeline, this works well there, but may be inadequate for
parsers, which can support loops.

## Overall flow

The compiler flow can be roughly split into frontend, middle-end, and
backend.  The frontend does largely non-target specific transforms
designed to clean up the IR and get it into a canonical form.  The
middle make target-dependent IR transformations.  The back end performs resource allocation
decisions.  As this backend step may fail due
to constraint violations, it may need to backtrack through the
middle-end to try a different configuration.  We do not anticipate
needing to backtrack through frontend passes.  The split between
Front/Middle/Back-end is largely arbitrary, and some passes may end up
being moved between them if we later find it makes more sense to do
so.

### Frontend

The frontend parses the source code into IR that corresponds
directly to the source code.  It then runs a series of passes like
type-checking (modifying nodes to indicate their inferred types),
constant folding, dead code elimination, etc.  The initial IR tree
corresponds to the source code, and then various mostly target independent
transformation are done.  Complex scoping or structuring in
the code is flattened here.

### Mid-end

The mid-end is where transformations that depend somewhat on the
target, but are not specific resource allocations, take place.
At some point the mid-end transforms the IR into a form that
matches the target capabilities.

### Pass Managers

To manage backtracking and control
the order of passes and transforms, we use a `PassManager` object
which encapsulates a sequence of passes and runs them in order.  Those
passes that support the `Backtrack` interface are backtrack points.

The generic `PassManager` can manage a
dynamically chosen series of passes.  It tracks passes that
implement `Backtrack` and if a later pass fails, will backtrack to the
Backtrack pass.

```
    interface Backtrack {
       bool backtrack(trigger);
    }
```

The `backtrack` method is called when backtracking is triggered, with
a trigger reason.  Returns `true` if the pass can try a useful
alternative, `false` if manager should search for some other
backtracking point.

### Exception Use

Exceptions are usable for errors that are reportable to the user,
constraint problems that should result in backtracking and internal
compiler problems.  There are a number of exception classes designed for
these purposes.

- `Backtrack::trigger`  Used to control backtracking.  May be subclassed to
  attach additional information.
- `CompilerBug`  Used for internal compiler errors.  Use the BUG() macro wrapper
   instead of calling this directly.

## IR Classes

To make IR class management and extension easier, IR classes are defined in .def files,
which are processed by an ir-generator tool to produce the actual IR C++ code.
Conceptually, these .def files are just C++ class definitions with the boilerplate
removed, and the ir-generator program inserts the boilerplate and splits each class
definition into .h and .cpp parts as needed.

The bulk of the "normal" IR is defined in the ir subdirectory, with some frontends
and backends having their own extensions to the IR defined in their own ir subdirectories.
The ir-generator tool takes reads all the .def files in the tree and constructs a single
.h and .cpp file with all the IR class definition code.

All references to IR class objects from other IR class object MUST be as `const` pointers,
to maintain the write-only invariants.  Where that is too onerous, IR classes may be
directly embedded in other IR objects, rather than by reference.  When an IR object is
embedded in this way, it becomes modifiable in `Modifier`/`Transform` visitors when visiting
the containing node, and will not be directly visited itself.

The ir-generator understands both of these mechanisms.  Any field declared in an IR class
with another IR class as a type will be made a `const` pointer, unless it has an `inline`
modifier, in which case it will be a directly embedded sub-object.

The ir-generator understands a number of "standard" methods for IR classes --
`visit_children`, `operator==`, `dbprint`, `toString`, `apply`.  These methods can be
declared *without* any arguments or return types (ie, just the method name followed by
a {}-block of code), in which case the standard declaration will be synthesized.  If the
method is not declared in the .def file, a standard definition (based on the fields
declared in the class) will be created.  In this way, *most* classes can avoid including
this boilerplate code in the .def file.

## Classes
| **Class**      | **Description**  |
|----------------|------------------|
| `IR::Node`     | This is the ultimate abstract base class of all IR nodes and contains only a small amount of data for error reporting and debugging.  In general, this info is NEVER compared for equality (so subclasses should never call `Node::operator==`) as Nodes that differ only in this information should be considered equal, and not require cloning or the IR tree.   |
| `IR::Vector<T>`| This template class holds a vector of (`const`) pointers to nodes of a particular `IR::Node` subclass. |

<!--!
\include{doc} "../ir/README.md" 
-->
