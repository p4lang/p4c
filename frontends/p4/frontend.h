#ifndef _P4_FRONTEND_H_
#define _P4_FRONTEND_H_

#include "ir/ir.h"
#include "../common/options.h"

const IR::P4Program*
run_frontend(const CompilerOptions& options, const IR::P4Program* program, bool anyDeclOrder);

#endif /* _P4_FRONTEND_H_ */
