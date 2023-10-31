#ifndef _TOOLS_TESTGEN_P4_
#define _TOOLS_TESTGEN_P4_

/// Add @param check as a necessary path constraint for all subsequent executions in the program.
/// Enabled by default, unless `--disable-assumption-mode` is toggled.
extern void testgen_assume(in bool assumption);

/// Add @param check as a necessary path constraint for all subsequent executions in the program.
/// Enabled by default, unless `--disable-assumption-mode` is toggled.
/// If `--assertion-mode` is toggled, P4Testgen will not apply the condition to the path
/// constraints, but instead will try to only generate tests that violate this assumption.
extern void testgen_assert(in bool assumption);

#endif
