#include <gc/gc_cpp.h>
#include "indent.h"
#include "config.h"

int indent_t::tabsz = 2;

static int indentctl_index = -1;

static void delete_indent(std::ios_base::event event, std::ios_base &out, int index) {
    if (event == std::ios_base::erase_event) {
        auto p = static_cast<indent_t *>(out.pword(index));
        delete p; }
}

indent_t &indent_t::getindent(std::ostream &out) {
    if (indentctl_index < 0) indentctl_index = out.xalloc();
    auto &p = out.pword(indentctl_index);
    /* DANGER -- there's a bug in the Garbage Collector on OSX where it doesn't
     * properly scan roots in the global cin/cout/clog objects, which we are
     * using here.  So to ensure that these indent_t objects are not prematurely
     * collected, we mark them as non-collectable, and delete them explicitly with
     * a callback */
#ifdef HAVE_LIBGC
    if (!p) {
        p = new(NoGC, 0, 0) indent_t();
        out.register_callback(delete_indent, indentctl_index);
    }
#endif
    return *static_cast<indent_t *>(p);
}

