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

#include "config.h"
#if HAVE_LIBGC
#include <gc/gc_cpp.h>
#define NOGC_ARGS (NoGC, 0, 0)
#else
#define NOGC_ARGS
#endif /* HAVE_LIBGC */
#include "indent.h"

int indent_t::tabsz = 2;

static int indentctl_index = -1;

static void delete_indent(std::ios_base::event event, std::ios_base &out, int index) {
    if (event == std::ios_base::erase_event) {
        auto p = static_cast<indent_t *>(out.pword(index));
        delete p;
    } else if (event == std::ios_base::copyfmt_event) {
        auto &p = out.pword(index);
        p = new NOGC_ARGS indent_t(*static_cast<indent_t *>(p));
    }
}

indent_t &indent_t::getindent(std::ostream &out) {
    if (indentctl_index < 0) indentctl_index = out.xalloc();
    auto &p = out.pword(indentctl_index);
    /* DANGER -- there's a bug in the Garbage Collector on OSX where it doesn't
     * properly scan roots in the global cin/cout/clog objects, which we are
     * using here.  So to ensure that these indent_t objects are not prematurely
     * collected, we mark them as non-collectable, and delete them explicitly with
     * a callback */
    if (!p) {
        p = new NOGC_ARGS indent_t();
        out.register_callback(delete_indent, indentctl_index); }
    return *static_cast<indent_t *>(p);
}

