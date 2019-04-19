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

#ifndef IR_IR_TREE_MACROS_H_
#define IR_IR_TREE_MACROS_H_

/* macro table listing ALL IR subclasses of Node and all their direct and indirect bases
 * whenever a new IR Node subclass is addeded, it MUST be added to this table.  Target
 * specific subclasses might be broken out into a separate table and included here
 * Because we want to be able to forward-declare all these types, the class type MUST
 * be a simple identifier that will be declared in namespace IR.  Aliases can be defined
 * in other namespaces if desired.
 * When there's a templated subclass of Node, all of its instantiations need to appear in this
 * table, and it also needs to be in the IRNODE_ALL_TEMPLATES_AND_DIRECT_AND_INDIRECT_BASES
 * table below */
#include "ir/gen-tree-macro.h"

#define IRNODE_ALL_TEMPLATES_AND_DIRECT_AND_INDIRECT_BASES(M, D, B, TDA, ...)                   \
    M(Vector, D(Node), template<class T>, <T>, ##__VA_ARGS__)                                   \
    M(IndexedVector, D(Vector<T>) B(Node), template<class T>, <T>, ##__VA_ARGS__)               \
    M(NameMap, D(Node),                                                                         \
      IR_TREE_COPY(template<class T, template<class, class, class, class>                       \
                   class MAP TDA(= std::map),                                                   \
                   class COMP TDA(= std::less<cstring>),                                        \
                   class ALLOC TDA(= std::allocator<std::pair<cstring, const T*>>) >),          \
      IR_TREE_COPY(<T, MAP, COMP, ALLOC>), ##__VA_ARGS__)                                       \

#define IR_TREE_COPY(...)       __VA_ARGS__
#define IR_TREE_IGNORE(...)

/* all IR classes, including Node */
#define IRNODE_ALL_CLASSES_AND_BASES(M, B, ...)                                                 \
    M(Node, , ##__VA_ARGS__)                                                                    \
        IRNODE_ALL_SUBCLASSES_AND_DIRECT_AND_INDIRECT_BASES(M, M, B, B, ##__VA_ARGS__)

#define IRNODE_ALL_NON_TEMPLATE_CLASSES_AND_BASES(M, B, ...)                                    \
    M(Node, , ##__VA_ARGS__)                                                                    \
        IRNODE_ALL_SUBCLASSES_AND_DIRECT_AND_INDIRECT_BASES(M, IR_TREE_IGNORE, B, B, ##__VA_ARGS__)

/* all the subclasses with just the immediate bases */
#define IRNODE_ALL_SUBCLASSES(M, ...)   \
    IRNODE_ALL_SUBCLASSES_AND_DIRECT_AND_INDIRECT_BASES(M, M, IR_TREE_COPY, IR_TREE_IGNORE, \
                                                        ##__VA_ARGS__)
#define IRNODE_ALL_NON_TEMPLATE_SUBCLASSES(M, ...)      \
    IRNODE_ALL_SUBCLASSES_AND_DIRECT_AND_INDIRECT_BASES(M, IR_TREE_IGNORE, IR_TREE_COPY, \
                                                        IR_TREE_IGNORE, ##__VA_ARGS__)
#define IRNODE_ALL_TEMPLATES_AND_BASES(M, ...) \
    IRNODE_ALL_TEMPLATES_AND_DIRECT_AND_INDIRECT_BASES(M, IR_TREE_COPY, IR_TREE_IGNORE, \
                                                       IR_TREE_IGNORE, ##__VA_ARGS__)

/* all classes with no bases */
#define REMOVE_BASES_ARG(CLASS, BASES, M, ...) M(IR_TREE_COPY(CLASS), ##__VA_ARGS__)
#define IRNODE_ALL_CLASSES(M, ...)      \
    IRNODE_ALL_CLASSES_AND_BASES(REMOVE_BASES_ARG, IR_TREE_IGNORE, M, ##__VA_ARGS__)
#define IRNODE_ALL_NON_TEMPLATE_CLASSES(M, ...) \
    IRNODE_ALL_NON_TEMPLATE_CLASSES_AND_BASES(REMOVE_BASES_ARG, IR_TREE_IGNORE, M, ##__VA_ARGS__)

#define REMOVE_TEMPLATE_BASES_ARG(CLASS, BASES, TEMPLATE, TARGS, M, ...)         \
    M(IR_TREE_COPY(CLASS), IR_TREE_COPY(TEMPLATE), IR_TREE_COPY(TARGS), ##__VA_ARGS__)
#define IRNODE_ALL_TEMPLATES(M, ...) \
    IRNODE_ALL_TEMPLATES_AND_DIRECT_AND_INDIRECT_BASES(REMOVE_TEMPLATE_BASES_ARG,       \
                                IR_TREE_IGNORE, IR_TREE_IGNORE, IR_TREE_IGNORE, M, ##__VA_ARGS__)
#define IRNODE_ALL_TEMPLATES_WITH_DEFAULTS(M, ...) \
    IRNODE_ALL_TEMPLATES_AND_DIRECT_AND_INDIRECT_BASES(REMOVE_TEMPLATE_BASES_ARG,       \
                                IR_TREE_IGNORE, IR_TREE_IGNORE, IR_TREE_COPY, M, ##__VA_ARGS__)

#endif /* IR_IR_TREE_MACROS_H_ */
