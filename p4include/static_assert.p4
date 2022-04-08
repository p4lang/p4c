#ifndef _STATIC_ASSERT_P4_
#define _STATIC_ASSERT_P4_

/// Static assert evaluates a boolean expression
/// at compilation time.  If the expression evaluates to
/// false, compilation is stopped and the corresponding message is printed.
/// The function returns a boolean, so that it can be used
/// as a global constant value in a program, e.g.:
/// const version = static_assert(V1MODEL_VERSION > 20180000, "Expected a v1 model version >= 20180000");
extern bool static_assert(bool check, string message);

/// Like the above but using a default message.
extern bool static_assert(bool check);

#endif