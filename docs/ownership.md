<!--
SPDX-FileCopyrightText: 2026 The P4 Language Consortium

SPDX-License-Identifier: Apache-2.0
-->

# Ownership without the garbage collector

Configure with `-DDISABLE_GC=ON` to exercise explicit ownership. Keep a separate
GC-enabled build when changing shared infrastructure: the two configurations
use different definitions of the IR pointer aliases.

## IR and visitors

Use `IR::Ptr<T>` for an owning reference to a const node and
`IR::MutablePtr<T>` for an owning reference to a mutable node. In a no-GC build
these are intrusive reference-counted pointers; in a GC build they are raw
pointers. Code shared by both configurations must not depend on smart-pointer
members such as `.get()` on these aliases.

Raw pointers are suitable for borrowing an object whose owner remains alive.
Keep an owning reference when retaining an IR node across a transformation,
storing it in a result or cache, or receiving a newly constructed result.
Map keys need owners too if their nodes can disappear from the transformed IR.

```cpp
IR::Ptr<IR::Expression> expression = makeExpression();
const auto *constant = expression->to<IR::Constant>();  // borrows expression
```

Avoid converting an owning temporary to a raw pointer and using that pointer
after the full expression ends. This also applies to casts from temporary
results: keep the returned owner before calling `to<T>()` or `checkedTo<T>()`.

Transform callbacks retain raw pointer return types for covariant overrides.
When returning a result held by a local owner, use `guardReturn(result)` so the
visitor retains it while handing it to the traversal. Ordinary helper functions
which construct nodes should return the appropriate owning IR alias directly.

`PassManager` owns heap-allocated visitors in a no-GC build. Its references to
stack-allocated visitors do not extend their lifetime: those visitors must
outlive their use by the manager. The IR ownership mechanism also supports
nodes embedded in other objects; such references do not keep the enclosing
object alive.

## Supporting objects

Use `std::unique_ptr` for a single owner and `std::shared_ptr` for data shared by
passes, execution states, or returned results. Borrowed pointers and references
should have an identifiable owner. In particular, visitor clones often share
analysis data, so deleting a raw member in each clone is not a valid ownership
strategy.

Avoid cycles of owning pointers. Back-references to a traversal owner or parent
must borrow when the parent already owns the child and outlives it, or use a
weak reference when the lifetimes are independent.

`AutoCompileContext` takes ownership when passed a `std::unique_ptr`; its raw
pointer constructor borrows the context. The context is removed from the
active stack before an owned context is destroyed.

## Validation

Compiler regression tests check semantics, including rejected programs and IR
JSON round trips. Also run the compiler and Testgen unit tests in both build
configurations. The current migration focuses on the compiler core and
P4Testgen; backend-specific ownership still needs separate validation.

Use LeakSanitizer or Valgrind to check allocation lifetimes, including error
paths. Valgrind additionally detects invalid accesses that a leak-only check
can miss. Compare generated tests with a GC-enabled baseline using the same
seed, ignoring generated timestamps.

The [Testgen memory benchmark](../backends/p4tools/modules/testgen/benchmarks/README.md)
measures peak RSS at increasing test counts. A memory ceiling catches large
growth regressions, but passing it does not establish that allocations are
released; retain the leak checks as well.
