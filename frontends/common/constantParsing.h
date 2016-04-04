#ifndef _FRONTENDS_COMMON_CONSTANTPARSING_H_
#define _FRONTENDS_COMMON_CONSTANTPARSING_H_

#include <gmpxx.h>
#include "ir/ir.h"
#include "lib/source_file.h"

// helper for parsing constants
IR::Constant* cvtCst(Util::SourceInfo srcInfo, const char* s, unsigned skip, unsigned base);

#endif /* _FRONTENDS_COMMON_CONSTANTPARSING_H_ */
