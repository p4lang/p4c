#ifndef IR_UNPACKER_TABLE_H_
#define IR_UNPACKER_TABLE_H_

#include "ir/inode.h"
#include "lib/cstring.h"

namespace P4 {
class JSONLoader;

using NodeFactoryFn = IR::INode *(*)(JSONLoader &);
namespace IR {
extern std::map<cstring, NodeFactoryFn> unpacker_table;

}  // namespace IR

}  // namespace P4

#endif /* IR_UNPACKER_TABLE_H_ */
