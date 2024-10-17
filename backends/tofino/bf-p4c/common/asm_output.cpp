/**
 * Copyright 2013-2024 Intel Corporation.
 *
 * This software and the related documents are Intel copyrighted materials, and your use of them
 * is governed by the express license under which they were provided to you ("License"). Unless
 * the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
 * or transmit this software or the related documents without Intel's prior written permission.
 *
 * This software and the related documents are provided as is, with no express or implied
 * warranties, other than those that are expressly stated in the License.
 */

#include "asm_output.h"
#include "pragma/all_pragmas.h"

StringRef trim_asm_name(StringRef name) {
    if (auto *p = name.findlast('['))
        if (strchr(p, ':'))
            name = name.before(p);
    if (auto *p = name.findlast(':'))
        name = name.after(p+1);
    return name;
}

std::ostream &operator<<(std::ostream &out, canon_name n) {
    auto name = n.name;
    if (name[0] == '.') name++;
    if (auto cl = name.findstr("::"))
        name = name.after(cl);
    for (auto ch : name) {
        if (ch & ~0x7f) continue;
        if (isalnum(ch) || ch == '_' || ch == '.' || ch == '$' || ch == '-')
            out << ch;
        if (ch == '[')
            out << '$'; }
    return out;
}

std::ostream &operator<<(std::ostream &out, const Slice &sl) {
    if (sl.field) {
        out << canon_name(trim_asm_name(sl.field->externalName()));
        for (auto &alloc : sl.field->get_alloc()) {
            if (sl.lo < alloc.field_slice().lo) continue;
            if (sl.hi > alloc.field_slice().hi)
                LOG1("WARNING: Slice not contained within a single PHV container");
            if (alloc.field_slice().lo != 0 || alloc.width() != sl.field->size)
                out << '.' << alloc.field_slice().lo << '-' << alloc.field_slice().hi;
            if (sl.lo != alloc.field_slice().lo || sl.hi != alloc.field_slice().hi) {
                out << '(' << (sl.lo - alloc.field_slice().lo);
                if (sl.hi > sl.lo) out << ".." << (sl.hi - alloc.field_slice().lo);
                out << ')'; }
            break; }
    } else {
        out << sl.reg;
        if (sl.lo != 0 || sl.hi+1U != sl.reg.size()) {
            out << '(' << sl.lo;
            if (sl.lo != sl.hi) out << ".." << sl.hi;
            out << ')'; } }
    return out;
}

Slice Slice::join(Slice &a) const {
    if (field != a.field || reg != a.reg || hi + 1 != a.lo) return Slice();
    if (!field)
        return Slice(reg, lo, a.hi);
    /* don't join if the slices were allocated to different PHV containers */
    for (auto &alloc : field->get_alloc()) {
        if (lo < alloc.field_slice().lo) continue;
        if (a.hi > alloc.field_slice().hi) return Slice();
        break; }
    return Slice(field, lo, a.hi);
}

int Slice::align(int size) const {
    BUG_CHECK((size & (size - 1)) == 0, "size not power of two");
    if (field) {
        auto &alloc = field->for_bit(lo);
        return (lo - alloc.field_slice().lo + alloc.container_slice().lo) & (size-1); }
    return lo & (size-1);
}

Slice Slice::fullbyte() const {
    Slice rv;
    if (field) {
        auto &alloc = field->for_bit(lo);
        rv.reg = alloc.container();
        rv.lo = lo - alloc.field_slice().lo + alloc.container_slice().lo;
        rv.hi = rv.lo + hi - lo;
    } else {
        rv = *this; }
    rv.lo &= ~7;
    rv.hi |= 7;
    return rv;
}

safe_vector<Slice> Slice::split(const Slice &a, bool &split) {
    safe_vector<Slice> vec;
    if (field != a.field) {
        vec.push_back(*this);
        split = false;
    } else {
        if (a.lo > lo && a.lo <= hi) {
           vec.emplace_back(field, lo, a.lo - 1);
           split = true;
        }
        if (a.hi < hi && a.hi >= lo) {
            vec.emplace_back(field, a.hi + 1, hi);
            split = true;
        }
        if (a.hi == hi && a.lo == lo) {
            split = true;
            return vec;
        }

        if ((a.hi >= hi && a.lo < lo) || (a.hi > hi && a.lo <= lo)) {
            BUG("Split cannot work on this scenario");
        }
    }
    if (vec.size() == 0) {
        vec.push_back(*this);
        split = false;
    }
    return vec;
}

/* This algorithm is used to split a particular match fields from the ghost fields in the
   hash analysis in the MAU.  (*this) is the slice we want to split, vec are the
   potential ghost bits, the return value is the split slices, and the splitters vector are
   the ghost bits that affect the bits

   For an analysis for O(n^2), this really shouldn't be an issue.  The size of vec will
   be relatively small, and there is a guarantee that at most, the calculations will 
   be on the order of vec.size()^2.  In the only way that this is used, vec can really at most
   be sized about 12, and is almost entirely going to be sized 2-3 on pretty much every
   reasonable table 
*/
safe_vector<Slice> Slice::split(const safe_vector<Slice> &vec,
                                safe_vector<Slice> &splitters) {
    safe_vector<Slice> rv;
    rv.push_back(*this);
    for (auto mid_slice : vec) {
        safe_vector<Slice> temp;
        temp.clear();
        bool ever_split = false;
        for (auto whole_slice : rv) {
            bool split = false;
            safe_vector<Slice> single = whole_slice.split(mid_slice, split);
            temp.insert(temp.end(), single.begin(), single.end());
            if (split)
                ever_split = true;
        }
        if (ever_split)
            splitters.push_back(mid_slice);
        rv = temp;
    }
    return rv;
}

bool has_user_annotation(const IR::IAnnotated* node) {
    if (!node) return false;
    for (auto* annotation : node->getAnnotations()->annotations) {
        if (annotation->name == PragmaUserAnnotation::name) return true;
    }

    return false;
}

void emit_user_annotation_context_json(std::ostream &out,
                                       indent_t indent,
                                       const IR::IAnnotated* node,
                                       bool emit_dash) {
    std::stringstream context_json_entries;
    emit_user_annotation_context_json(indent, node, context_json_entries);
    if (context_json_entries.str().length() != 0) {
        out << indent++ << (emit_dash ? "- " : "") << "context_json" << ":" << std::endl;
        out << context_json_entries << std::endl;
    }
}

void emit_user_annotation_context_json(indent_t indent,
                                       const IR::IAnnotated* node,
                                       std::stringstream &context_json_entries) {
    if (!node) return;

    bool emitted_header = false;
    for (auto* annotation : node->getAnnotations()->annotations) {
        if (annotation->name != PragmaUserAnnotation::name) continue;

        if (!emitted_header) {
            indent++;
            context_json_entries << indent++ << "user_annotations:" << std::endl;
            emitted_header = true;
        }

        bool emitted_elts = false;
        for (auto* expr : annotation->expr) {
            auto str = expr->to<IR::StringLiteral>();
            BUG_CHECK(str, "User annotation not a string literal: %1%", expr);

            auto cur_elt = str->value;

            if (!emitted_elts) context_json_entries << indent << "- [";
            else
              context_json_entries << ", ";

            context_json_entries << "\"" << cur_elt.escapeJson() << "\"";
            emitted_elts = true;
        }

        if (emitted_elts) context_json_entries << "]" << std::endl;
    }
}
