/*
Copyright 2013-present Barefoot Networks, Inc.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#ifndef _IR_IR_H_
#define _IR_IR_H_

#include <strings.h>
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
#include "lib/gmputil.h"

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

class JSONLoader;
#include "json_generator.h"

#include "pass_manager.h"
#include "ir-inline.h"
#include "dump.h"

#endif /* _IR_IR_H_*/
