#ifndef _FRONTENDS_COMMON_PARSEINPUT_H_
#define _FRONTENDS_COMMON_PARSEINPUT_H_

#include "ir/ir.h"
#include "options.h"

// Parse a P4 v1.0 or v1.2 file (as specified by the options)
// and return it in a P4 v1.2 representation
const IR::P4Program* parseP4File(CompilerOptions& options);

#endif /* _FRONTENDS_COMMON_PARSEINPUT_H_ */
