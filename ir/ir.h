#ifndef _IR_IR_H_
#define _IR_IR_H_

#include <gmpxx.h>
#include <cassert>
#include <sstream>

#include "lib/log.h"
#include "lib/null.h"
#include "lib/error.h"
#include "lib/algorithm.h"
#include "lib/cstring.h"
#include "lib/map.h"
#include "lib/ordered_map.h"
#include "lib/exceptions.h"
#include "lib/source_file.h"
#include "lib/ltbitmatrix.h"
#include "lib/match.h"

// Special IR classes and types
#include "node.h"
#include "vector.h"
#include "indexed_vector.h"
#include "dbprint.h"
#include "namemap.h"
#include "nodemap.h"
#include "id.h"

// generated ir file
#include "ir/ir-generated.h"

#include "pass_manager.h"
#include "ir-inline.h"
#include "dump.h"

#endif /* _IR_IR_H_*/
