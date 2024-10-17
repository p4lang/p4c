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

#include "bf-p4c/phv/phv_fields.h"

#include <string>
#include <boost/range/adaptor/reversed.hpp>
#include "bf-p4c/arch/bridge_metadata.h"
#include "bf-p4c/common/bridged_packing.h"
#include "bf-p4c/common/header_stack.h"
#include "bf-p4c/common/pragma/all_pragmas.h"
#include "bf-p4c/common/utils.h"
#include "bf-p4c/mau/table_summary.h"
#include "bf-p4c/mau/gateway.h"
#include "bf-p4c/lib/error_type.h"
#include "bf-p4c/lib/safe_width.h"
#include "bf-p4c/parde/parde_visitor.h"
#include "lib/stringref.h"

namespace {

bitvec remove_non_byte_boundary_starts(const PHV::Field* field, const bitvec& startPositions) {
    bitvec cloned = startPositions;
    for (int i : startPositions) {
        if (i % 8 != 0) {
            LOG3("setStartBits non-byte boundary " << i << " of checksummed metadata " << field
                                                   << ", ignored. ");
            cloned.clrbit(i);
        }
    }
    return cloned;
}

}  // namespace

FieldAlignment::FieldAlignment(nw_bitrange bitLayout)
    : align(7 - bitLayout.hi % 8)
{ }

FieldAlignment::FieldAlignment(le_bitrange bitLayout)
    : align(bitLayout.lo % 8)
{ }

bool FieldAlignment::operator==(const FieldAlignment& other) const {
    return align == other.align;
}

bool FieldAlignment::operator!=(const FieldAlignment& other) const {
    return !(*this == other);
}

std::ostream& operator<<(std::ostream& out, const FieldAlignment& alignment) {
    out << "alignment = " << alignment.align;
    return out;
}

const PHV::AllocContext* PHV::AllocContext::PARSER =
    new PHV::AllocContext(AllocContext::Type::PARSER);

const PHV::AllocContext* PHV::AllocContext::DEPARSER =
    new PHV::AllocContext(AllocContext::Type::DEPARSER);

//
//***********************************************************************************
//
// PhvInfo member functions
//
//***********************************************************************************
//

int PhvInfo::deparser_stage = -1;
ordered_map<cstring, std::set<int>> PhvInfo::table_to_min_stages;
ordered_map<cstring, std::set<int>> PhvInfo::table_to_physical_stages;
bool PhvInfo::darkSpillARA = PhvInfo::DARK_SPILL_ARA_DEFAULT;

void PhvInfo::clear() {
    *this = PhvInfo();
    PhvInfo::clearMinStageInfo();
    PhvInfo::resetDeparserStage();
}

PHV::Field* PhvInfo::add(
        cstring name, gress_t gress, int size, int offset, bool meta, bool pov,
        bool bridged, bool pad, bool overlay, bool flex, bool fixed_size,
        std::optional<Util::SourceInfo> srcInfo) {
    // Set the egress version of bridged fields to metadata
    if (gress == EGRESS && bridged)
        meta = true;
    if (all_fields.count(name)) {
        LOG3("Already added field; skipping: " << name);
        return &all_fields.at(name);
    }
    LOG3("PhvInfo adding " << (pad ? "padding" : (meta ? "metadata" : "header")) << " field " <<
         name << " size " << size << " offset " << offset << (flex? " flexible" : "") <<
         (fixed_size? " fixed_size" : ""));
    auto *info = &all_fields[name];
    info->name = name;
    info->id = by_id.size();
    info->gress = gress;
    info->size = size;
    info->offset = offset;
    info->metadata = meta;
    info->pov = pov;
    info->bridged = bridged;
    info->padding = pad;
    info->overlayable = overlay;
    info->set_flexible(flex);
    info->set_fixed_size_header(fixed_size);
    info->srcInfo = srcInfo;
    by_id.push_back(info);
    return info;
}

void PhvInfo::add_struct(
        cstring name,
        const IR::Type_StructLike *type,
        gress_t gress,
        bool meta,
        bool bridged,
        int offset) {
    BUG_CHECK(type, "No type for %1%", name);
    LOG3("PhvInfo adding " << (meta ? " metadata" : "header") << " for struct " << name);
    // header has its own offset regardless of the offset of struct.
    if (!meta) {
        offset = 0;
        for (auto f : type->fields) offset += safe_width_bits(f->type);
    }
    int start = by_id.size();
    for (auto f : type->fields) {
        int size = safe_width_bits(f->type);
        cstring f_name = name + '.' + f->name;
        if (f->type->is<IR::Type_StructLike>()) {
            auto* struct_type = f->type->to<IR::Type_StructLike>();
            bool meta = !struct_type->is<IR::Type_Header>();
            // Add fields inside nested structs and headers.
            add_struct(f_name, struct_type, gress, meta, bridged, offset);
        }
        // path-expression may point to an empty header, which require no phv.
        // path-expression can also point to an error field, which is invalid on Tofino
        if (size == 0)
            continue;
        // "hidden" annotation indicates padding introduced with bridged metadata fields
        bool isPad = f->getAnnotations()->getSingle("hidden"_cs) != nullptr ||
                     f->getAnnotations()->getSingle("padding"_cs) != nullptr;
        bool isOverlayable = f->getAnnotations()->getSingle("overlayable"_cs) != nullptr;
        // "flexible" annotation indicates flexible fields
        bool isFlexible = f->getAnnotations()->getSingle("flexible"_cs) != nullptr;
        bool isFixedSizeHeader = type->is<IR::BFN::Type_FixedSizeHeader>();
        std::optional<Util::SourceInfo> srcInfo = std::nullopt;
        if (f->srcInfo.isValid())
            srcInfo = f->srcInfo;
        add(f_name, gress, size, offset -= size, meta, false, bridged, isPad, isOverlayable,
            isFlexible, isFixedSizeHeader, srcInfo);
    }
    if (!meta) {
        int end = by_id.size();
        all_structs.emplace(name, StructInfo(meta, gress, start, end - start));
        LOG3("Adding struct  " << name << " to all_structs");
    }
}

void PhvInfo::add_hdr(cstring name, const IR::Type_StructLike *type, gress_t gress, bool meta) {
    if (!type) {
        LOG3("PhvInfo no type for " << name);
        return; }
    if (all_structs.count(name)) {
        LOG3("Already added header; skipping: " << name);
        return; }

    int start = by_id.size();
    int offset = 0;
    for (auto f : type->fields)
        offset += safe_width_bits(f->type);
    add_struct(name, type, gress, meta, false, offset);

    if (meta) {
        int end = by_id.size();
        all_structs.emplace(name, StructInfo(meta, gress, start, end - start));
        LOG3("Adding header  " << name << " to all_structs");
    }
}

void PhvInfo::addTempVar(const IR::TempVar *tv, gress_t gress) {
    BUG_CHECK(tv->type->is<IR::Type::Bits>() || tv->type->is<IR::Type::Boolean>(),
              "Can't create temp of type %s", tv->type);
    if (gress == GHOST) gress = INGRESS;  // treat ghost as ingress
    if (all_fields.count(tv->name)) {
        BUG_CHECK(all_fields.at(tv->name).gress == gress, "Inconsistent gress for %s (%s and %s)",
                  tv, all_fields.at(tv->name).gress, gress);
    } else {
        auto fld = add(tv->name, gress, safe_width_bits(tv->type), 0, true, tv->POV);
        if (fld)
            fields_to_tempvars_i[fld] = tv;
    }
}

const IR::TempVar* PhvInfo::getTempVar(const PHV::Field* f) const {
    if (fields_to_tempvars_i.count(f))
        return fields_to_tempvars_i.at(f);
    return nullptr;
}

bool PhvInfo::has_struct_info(cstring name_) const {
    cstring name = name_;
    if (all_structs.find(name) != all_structs.end())
        return true;
    if (auto *pos = name.find("::"))
        name = cstring(pos + 2);
    return all_structs.find(name) != all_structs.end();
}

/// Look up the fully qualified name of a header or metadata collection
///   PHV header names are fully qualified, but we sometimes look up a partial
///   name when it's unique.  Eg. using "ingress::bar" to find
///   "ingress::foo.bar" when there are no other "bar" suffixes.
/// ----
cstring PhvInfo::full_hdr_name(const cstring& name_) const {
    StringRef name = name_;
    if (simple_headers.find(name) != simple_headers.end())
        return name;

    if (all_structs.find(name) != all_structs.end())
        return name;

    StringRef prefixRef;
    StringRef suffixRef = name;
    if (auto* p = name.findstr("::")) {
        prefixRef = name.before(p);
        suffixRef = name.after(p + 2); }
    cstring prefix = prefixRef.toString();
    cstring suffix = suffixRef.toString();

    LOG4("Looking for PHV header " << name_);
    LOG4("    ...with prefix " << prefix);
    LOG4("    ...with suffix " << suffix);
    std::set<cstring> matches;
    for (auto& hdr : all_structs) {
        if ((hdr.first).endsWith(suffix.string()))
            LOG4("    ...found suffix: " << hdr.first);
        if ((hdr.first).startsWith(prefix.string()) && (hdr.first).endsWith(suffix.string()))
            matches.insert(hdr.first);
    }
    if (matches.size() > 1) {
        std::stringstream msg;
        for (auto& mp : matches)
            msg << " " << mp;
        LOG4("Name '" << name_ << "' could refer to:" << msg.str());
    } else if (matches.size() == 1) {
        return *(matches.begin());
    }

    return cstring("");
}

/// Get all the fields of header named @p name_ and store them in set @p flds
/// ---
void PhvInfo::get_hdr_fields(cstring name_, ordered_set<const PHV::Field*> & flds) const {
    const PhvInfo::StructInfo arr_info = struct_info(full_hdr_name(name_));

    size_t fld_id = arr_info.first_field_id;
    size_t last_fld = fld_id + arr_info.size;

    while ((fld_id < by_id.size()) && (fld_id < last_fld)) {
        flds.insert(by_id.at(fld_id));
        ++fld_id;
    }

    BUG_CHECK(fld_id == last_fld, "Something wrong going with fields of header %s", name_);
}

/**
 * @brief Get information about a collection of header or metadata fields, based on the partial or
 * complete @p name_ of the collection or of a field in the collection
 *
 * @param name_ Partial or complete name of a field, or a header or metadata collection
 * @return const PhvInfo::StructInfo* Information about collection
 */
const PhvInfo::StructInfo* PhvInfo::hdr(const cstring& name_) const {
    cstring full_name = full_hdr_name(name_);
    if (full_name.size() && all_structs.count(full_name))
        return &(all_structs.at(full_name));

    return nullptr;
}

void PhvInfo::addPadding(const IR::Padding *pad, gress_t gress) {
    BUG_CHECK(pad->type->is<IR::Type::Bits>(),
              "Can't create padding of type %s", pad->type);
    if (all_fields.count(pad->name) == 0) {
        dummyPaddingNames.insert(pad->name);
        add(pad->name, gress, pad->type->width_bits(),
                0, false, false, false, /* isPad = */ true);
    }
}

const PhvInfo::StructInfo PhvInfo::struct_info(cstring name_) const {
    StringRef name = name_;
    auto it = all_structs.find(name);
    if (it != all_structs.end())
        return it->second;
    if (auto *p = name.findstr("::")) {
        name = name.after(p+2); }
    it = all_structs.find(name);
    if (it != all_structs.end())
        return it->second;
    BUG("No PhvInfo::header for header named '%s'", name_);
}

//
//***********************************************************************************
//
// PhvInfo::field functions to extract field
// retrieve field from IR expression, IR member
//
//***********************************************************************************
//

const PHV::Field *PhvInfo::field(const IR::Expression *e, le_bitrange *bits) const {
    if (!e) return nullptr;
    BUG_CHECK(!e->is<IR::BFN::ContainerRef>(),
        "Looking for PHV::Fields but found an IR::BFN::ContainerRef: %1%", e);
    if (auto *fr = e->to<IR::Member>())
        return field(fr, bits);
    if (auto *cast = e->to<IR::BFN::ReinterpretCast>())
        return field(cast->expr, bits);
    // HACK. midend changes zero extend cast into 0 ++ expr. When the
    // expression is used in a 'add' operation, the compiler does not perform
    // any transformation to eliminate the '++' operator.
    if (auto *concat = e->to<IR::Concat>()) {
        if (auto k = concat->left->to<IR::Constant>())
            if (k->value == 0)
                return field(concat->right, bits); }
    if (auto *sl = e->to<IR::Slice>()) {
        auto *rv = field(sl->e0, bits);
        if (rv && bits) {
            le_bitrange sliceRange = FromTo(sl->getL(), sl->getH());
            if (!bits->contains(sliceRange)) {
                warning("slice %d..%d invalid for field %s of size %d", sl->getL(), sl->getH(),
                        sl->e0, bits->size());
                return nullptr; }
            auto intersection = bits->intersectWith(sliceRange);
            *bits = le_bitrange(StartLen(intersection.lo, intersection.size())); }
        return rv; }
    if (auto *tv = e->to<IR::TempVar>()) {
        if (auto *rv = getref(all_fields, tv->name)) {
            if (bits) {
                bits->lo = 0;
                bits->hi = rv->size - 1; }
            return rv;
        } else {
            // This can happen after a placement failure when we're trying to generate
            // minimal context.json to aid deubgging.  So we don't want to crash out
            /*BUG("TempVar %s not in PhvInfo", tv->name);*/ } }
    if (auto *pad = e->to<IR::Padding>()) {
        if (auto *rv = getref(all_fields, pad->name)) {
            if (bits) {
                bits->lo = 0;
                bits->hi = rv->size - 1; }
            return rv;
        } else {
            BUG("Padding %s not in PhvInfo", pad->name); } }
    if (auto *neg = e->to<IR::Neg>())
        return field(neg->expr, bits);
    return 0;
}

const PHV::Field *PhvInfo::field(const IR::Member *fr, le_bitrange *bits) const {
    if (bits) {
        bits->lo = 0;
        bits->hi = safe_width_bits(fr->type) - 1; }

    // No need to look up enums, which are really constants.  This sidesteps
    // the warning message for "can't find field" as well.
    if (fr->type->is<IR::Type_Enum>())
        return nullptr;

    return field(fr->toString());
}

const PHV::Field *PhvInfo::field(const cstring& name_) const {
    if (externalNameMap.count(name_)) {
        LOG4("  Looking up PHV field from external name map entry: " << name_);
        return externalNameMap.at(name_);
    }
    StringRef name = name_;
    if (auto *p = name.find('[')) {
        if (name.after(p).find(':'))
            name = name.before(p); }
    if (auto *rv = getref(all_fields, name))
        return rv;
    if (auto *p = name.findstr("::")) {
        name = name.after(p+2);
        if (auto *rv = getref(all_fields, name))
            return rv; }

    // PHV field names are fully qualified, but we sometimes look up a partial
    // name when it's unique.  Eg. using "ingress::bar.f" to find
    // "ingress::foo.bar.f" when there are no other "bar.f" suffixes.  This is
    // not common; it comes up mostly with aliases, which often use local
    // names.
    name = name_;
    StringRef prefixRef;
    StringRef suffixRef = name;
    if (auto* p = name.findstr("::")) {
        prefixRef = name.before(p);
        suffixRef = name.after(p + 2); }
    cstring prefix = prefixRef.toString();
    cstring suffix = suffixRef.toString();

    LOG4("Looking for PHV field " << name_);
    LOG4("    ...with prefix " << prefix);
    LOG4("    ...with suffix " << suffix);
    std::set<std::pair<cstring, const PHV::Field*>> matches;
    for (auto& kv : all_fields) {
        cstring name = kv.first;
        if (name.endsWith(suffix.string()))
            LOG4("    ...found suffix: " << kv.first);
        if (name.startsWith(prefix.string()) && name.endsWith(suffix.string()))
            matches.insert(std::make_pair(kv.first, &kv.second)); }
    if (matches.size() > 1) {
        std::stringstream msg;
        for (auto& kv : matches)
            msg << " " << kv.first;
        LOG4("Name '" << name_ << "' could refer to:" << msg.str());
    } else if (matches.size() == 1) {
        return matches.begin()->second;
    }

    // TODO: The warning spew from POV bits prior to allocatePOV() being
    // called is just too great. We need to improve how that's handled, but for
    // now, silence those warnings.
    if (!name.toString().endsWith(".$valid")) {
        LOG4("   ...can't find field " << name); }

    return nullptr;
}

void PhvInfo::allocatePOV(const BFN::HeaderStackInfo& stacks) {
    LOG3("BEGIN PhvInfo::allocatePOV");
    if (pov_alloc_done) BUG("trying to reallocate POV");
    pov_alloc_done = true;

    int size[GRESS_T_COUNT] = { 0, 0, 0 };
    int stacks_num = 0;
    for (auto &stack : stacks) {
        StructInfo info = struct_info(stack.name);
        LOG3("    ...preanalyzing header stack " << stack.name << " (" << info.gress << ")");
        BUG_CHECK(!info.metadata, "metadata stack?");
        size[info.gress] += stack.size + stack.maxpush + stack.maxpop;  // size for $stkvalid
        size[info.gress] += stack.size;                                 // size for $valid
        stacks_num++; }

    for (auto &hdr : simple_headers) {
        LOG3("    ...preanalyzing simple header " << hdr.first);
        auto hdr_info = hdr.second;
        ++size[hdr_info.gress]; }

    for (auto &field : *this)
        if (field.pov && field.metadata)
            size[field.gress] += field.size;

    for (auto gress : { INGRESS, EGRESS }) {
        if (size[gress] == 0) continue;
        for (auto &field : *this) {
            if (field.pov && field.metadata && field.gress == gress) {
                size[gress] -= field.size;
                field.offset = size[gress]; } }

        // Create fields for (non-header stack) validity bits.
        for (auto hdr : simple_headers)
            if (hdr.second.gress == gress)
                add(hdr.first + ".$valid", gress, 1, --size[gress], false, true);

        // Create header stack POVs, including header stack validity bits.
        for (auto &stack : stacks) {
            StructInfo info = struct_info(stack.name);
            if (info.gress == gress) {
                // Create $stkvalid.
                add(stack.name + ".$stkvalid", gress, stack.size + stack.maxpush + stack.maxpop,
                    size[gress], true, true);
                PHV::Field *pov_stk = &all_fields[stack.name + ".$stkvalid"];
                pov_stk->set_no_split(true);
                LOG3("Creating HEADER STACK " << pov_stk);
                size[gress] -= stack.size + stack.maxpush + stack.maxpop;
                // Create $valid.
                for (int i : boost::irange(0, stack.size))
                    add(stack.name + "[" + std::to_string(i) + "].$valid",
                        gress, 1, --size[gress], false, true); } }

        BUG_CHECK(size[gress] == 0, "Error creating %1% POV fields.  %2% bits not created.",
                  cstring::to_cstring(gress), size[gress]); }
}

//
//***********************************************************************************
//
// PhvInfo::container_to_fields related functions
//
//***********************************************************************************
//
void PhvInfo::add_container_to_field_entry(const PHV::Container c,
                                           const PHV::Field *f) {
    if (f == nullptr) return;
    container_to_fields[c].insert(f);
}

const ordered_set<const PHV::Field *>&
PhvInfo::fields_in_container(const PHV::Container c) const {
    static const ordered_set<const PHV::Field *> empty_list;
    if (container_to_fields.count(c))
        return container_to_fields.at(c);
    return empty_list;
}

std::vector<PHV::AllocSlice>
PhvInfo::get_slices_in_container(const PHV::Container c) const {
    std::vector<PHV::AllocSlice> rv;
    for (auto* field : fields_in_container(c)) {
        field->foreach_alloc([&](const PHV::AllocSlice &alloc) {
            if (alloc.container() != c) return;
            rv.push_back(alloc);
        });
    }
    return rv;
}

std::vector<PHV::AllocSlice>
PhvInfo::get_slices_in_container(
        const PHV::Container c,
        const PHV::AllocContext *ctxt,
        const PHV::FieldUse* use) const {
    std::vector<PHV::AllocSlice> rv;
    for (auto* field : fields_in_container(c)) {
        field->foreach_alloc(ctxt, use, [&](const PHV::AllocSlice &alloc) {
            if (alloc.container() != c) return;
            rv.push_back(alloc);
        });
    }
    return rv;
}

bitvec PhvInfo::bits_allocated(
        const PHV::Container c,
        const PHV::AllocContext *ctxt,
        const PHV::FieldUse* use) const {
    bitvec ret_bitvec;
    for (auto* field : fields_in_container(c)) {
        if (field->padding) continue;
        ret_bitvec |= bits_allocated(c, field, ctxt, use);
    }
    return ret_bitvec;
}

bitvec PhvInfo::bits_allocated(
        const PHV::Container c,
        const PHV::Field* field,
        const PHV::AllocContext* ctxt,
        const PHV::FieldUse* use) const {
    bitvec result;
    field->foreach_alloc(ctxt, use, [&](const PHV::AllocSlice& alloc) {
        if (alloc.container() != c) return;
        if (alloc.isUninitializedRead()) return;
        le_bitrange bits = alloc.container_slice();
        result.setrange(bits.lo, bits.size());
    });
    return result;
}


// Check which bits are allocated in Container (c) which contain field slices (writes) Based on the
// context (ctxt), slices in container are filtered and compared against field slices to check for
// container occupancy giving consideration to mutual exclusiveness, uninitialed read behavior,
// aliasing and dark overlay.
// The function returns a bitvec which sets occupied bits.
bitvec PhvInfo::bits_allocated(
        const PHV::Container c,
        const ordered_set<const PHV::Field*>& writes,
        const PHV::AllocContext *ctxt,
        const PHV::FieldUse* use) const {
    Log::TempIndent indent;
    LOG3("Allocating bits for container : " << c << indent);
    LOG3("AllocContext: " << *ctxt << ", use: " << *use);
    LOG3("Writes: " << writes);
    bitvec ret_bitvec;
    auto& fields = fields_in_container(c);

    if (fields.size() == 0) return ret_bitvec;
    LOG3("Fields: " << fields);

    // Gather all the slices of written fields allocated to container c
    std::vector<PHV::AllocSlice> write_slices_in_container;
    for (auto* field : writes) {
        // use canonical alias set representant, the other once can have wrong
        // defuse/mutex/meta-mutex
        // FIXME: this should be fixed by calcualting information for all aliases correctly
        auto aliasDest = getAliasDestination(field);
        if (aliasDest) {
            LOG6("  using alias dest " << aliasDest << " instead of " << field);
            field = aliasDest;
        }
        field->foreach_alloc(ctxt, use, [&](const PHV::AllocSlice &alloc) {
            if (alloc.container() != c) return;
            write_slices_in_container.push_back(alloc);
        });
    }
    LOG5("Write slices in container: " << write_slices_in_container);
    for (auto* field : fields) {
        if (field->padding) continue;
        Log::TempIndent loopIndent;
        LOG3("Container field: " << field->name << loopIndent);

        field->foreach_alloc(ctxt, use, [&](const PHV::AllocSlice &alloc) {
            LOG3("Alloc Container: " << alloc.container()
                << ", isUninitializedRead: " << (alloc.isUninitializedRead() ? "Y" : "N")
                << ", slice: " << alloc.container_slice());
            // Filter out alloc slices not in container
            if (alloc.container() != c) return;
            // If a slice is an uninitialized read then no bits are effectively occupied by that
            // slice within the container
            if (alloc.isUninitializedRead()) return;
            le_bitrange bits = alloc.container_slice();

            // Discard the slices that are mutually exclusive with any of the written slices
            bool mutually_exclusive = std::any_of(
                write_slices_in_container.begin(), write_slices_in_container.end(),
                [&](const PHV::AllocSlice& slice) {
                    return field_mutex_i(slice.field()->id, alloc.field()->id);
            });
            bool meta_overlay = std::any_of(
                write_slices_in_container.begin(), write_slices_in_container.end(),
                [&](const PHV::AllocSlice& slice) {
                    LOG5("Meta mutex with " << slice.field()->name << " is " <<
                        metadata_mutex_i(slice.field()->id, alloc.field()->id));
                    return metadata_mutex_i(slice.field()->id, alloc.field()->id);
            });
            bool dark_overlay = std::any_of(
                write_slices_in_container.begin(), write_slices_in_container.end(),
                [&](const PHV::AllocSlice& slice) {
                    LOG5("Dark Mutex with " << slice.field()->name << " is " <<
                        dark_mutex_i(slice.field()->id, alloc.field()->id));
                    return bits.overlaps(slice.container_slice()) &&
                        dark_mutex_i(slice.field()->id, alloc.field()->id);
            });
            bool noMutex = !mutually_exclusive && !meta_overlay;

            // In case of a dark container do not consider dark overlay feasibility
            if (Device::phvSpec().hasContainerKind(PHV::Kind::dark) && !c.is(PHV::Kind::dark))
                noMutex = noMutex && !dark_overlay;

            LOG3("For field " << field->name << "  mutex control:" << mutually_exclusive <<
                 " meta:" << meta_overlay << " dark:" << dark_overlay
                 << " noMutex: " << noMutex << " bits: " << bits);

            if (noMutex) {
                ret_bitvec.setrange(bits.lo, bits.size());
            }
        });
    }
    return ret_bitvec;
}

std::optional<cstring> PhvInfo::get_alias_name(const IR::Expression* expr) const {
    if (auto* alias = expr->to<IR::BFN::AliasMember>()) {
        const PHV::Field* aliasSourceField = field(alias->source);
        BUG_CHECK(aliasSourceField, "Alias source field %s not found", alias->source);
        return aliasSourceField->name;
    } else if (auto* alias = expr->to<IR::BFN::AliasSlice>()) {
        const PHV::Field* aliasSourceField = field(alias->source);
        BUG_CHECK(aliasSourceField, "Alias source field %s not found", alias->source);
        return aliasSourceField->name;
    } else if (auto sl = expr->to<IR::Slice>()) {
        get_alias_name(sl->e0);
    }
    return std::nullopt;
}

std::set<int> PhvInfo::minStages(const IR::MAU::Table *table) {
    if (LOGGING(6)) {
        std::stringstream ss;
        ss << " Reading stage(s) for table " << table->name << " = ";
        for (auto stg : ::get(table_to_min_stages, TableSummary::getTableName(table)))
            ss << stg << " ";
        LOG6(ss.str());
    }

    return ::get(table_to_min_stages, TableSummary::getTableName(table));
}

std::set<int> PhvInfo::physicalStages(const IR::MAU::Table *table) {
    std::set<int> rv;
    if (table_to_physical_stages.count(TableSummary::getTableIName(table)))
        rv = ::get(table_to_physical_stages, TableSummary::getTableIName(table));
    return rv;
}

void PhvInfo::addMinStageEntry(const IR::MAU::Table *table, int stage, bool remove_prev_stages) {
    auto tableName = TableSummary::getTableName(table);
    if (remove_prev_stages)
        table_to_min_stages[tableName].clear();

    table_to_min_stages[tableName].insert(stage);
    LOG6("Adding stage " << stage << " to table " << tableName);
    if (table->gateway_name && table->gateway_name != tableName) {
        table_to_min_stages[table->gateway_name].insert(stage);
        LOG6("Adding stage " << stage << " to gateway " << table->gateway_name);
    }
}

void PhvInfo::setPhysicalStages(const IR::MAU::Table *table, const std::set<int>& stages) {
    auto tableName = TableSummary::getTableIName(table);
    table_to_physical_stages[tableName] = stages;
    if (table->gateway_name && table->gateway_name != tableName) {
        table_to_physical_stages[table->gateway_name] = stages;
    }
}

bool PhvInfo::hasMinStageEntry(const IR::MAU::Table *table) {
    return table_to_min_stages.count(TableSummary::getTableName(table));
}

cstring PhvInfo::reportMinStages() {
    std::stringstream ss;
    ss << "  TABLES MIN STAGES:";
    for (auto entry : table_to_min_stages) {
        ss << "\n\t " << entry.first  << " stages: ";
        for (auto stg : entry.second) ss << " " << stg;
    }
    ss << std::endl;
    return ss.str();
}

bool PhvInfo::darkLivenessOkay(const IR::MAU::Table* gateway, const IR::MAU::Table* t) const {
    BUG_CHECK(gateway->conditional_gateway_only(), "Trying to merge non gateway table %1% with "
              "table %2%", gateway->name, t->name);
    auto t_stages = minStages(t);
    CollectGatewayFields collect_fields(*this);
    gateway->apply(collect_fields);
    static PHV::FieldUse use(PHV::FieldUse::READ);
    for (auto& field_info : collect_fields.info) {
        unsigned count = 0;
        field_info.first.field()->foreach_alloc(field_info.first.range(), t, &use,
                [&](const PHV::AllocSlice&) {
            ++count;
        });
        if (count == 0) return false;
    }
    return true;
}

PHV::Field* PhvInfo::create_dummy_padding(size_t sz, gress_t gress, bool overlayable) {
    cstring name = cstring::make_unique(
        dummyPaddingNames, cstring::to_cstring(gress) + "::" + "__phv_dummy_padding__");
    dummyPaddingNames.insert(name);
    add(name, gress, sz, 0, false, false, false, /* isPad = */ true, overlayable);
    return field(name);
}

std::vector<PHV::AllocSlice> PhvInfo::get_alloc(const IR::Expression* f,
                                                const PHV::AllocContext* ctxt,
                                                const PHV::FieldUse* use) const {
    CHECK_NULL(f);
    le_bitrange bits;
    auto* phv_field = field(f, &bits);
    BUG_CHECK(phv_field, "No PHV field for expression %1%", f);
    return get_alloc(phv_field, &bits, ctxt, use);
}

std::vector<PHV::AllocSlice> PhvInfo::get_alloc(const PHV::Field* phv_field, le_bitrange* bits,
                                                const PHV::AllocContext* ctxt,
                                                const PHV::FieldUse* use) const {
    std::vector<PHV::AllocSlice> slices;

    phv_field->foreach_alloc(bits, ctxt, use,
                             [&](const PHV::AllocSlice& alloc) { slices.push_back(alloc); });

    return slices;
}

//
//***********************************************************************************
//
// PHV::Field le_bitrange interface related member functions
//
//***********************************************************************************
//

// figure out how many disinct container bytes contain info from a le_bitrange of a particular field
//
int PHV::Field::container_bytes(std::optional<le_bitrange> optBits) const {
    BUG_CHECK(!alloc_slice_i.empty(), "Trying to get PHV container bytes for unallocated field %1%",
              this);
    le_bitrange bits = optBits.value_or(StartLen(0, size));
    int rv = 0, w;
    for (int bit = bits.lo; bit <= bits.hi; bit += w) {
        auto &sl = for_bit(bit);
        w = sl.width() - (bit - sl.field_slice().lo);
        if (bit + w > bits.hi)
            w = bits.hi - bit + 1;
        int cbit = bit + sl.container_slice().lo - sl.field_slice().lo;
        rv += (cbit+w-1)/8U - cbit/8U + 1; }
    return rv;
}

//
//***********************************************************************************
//
// PHV::Field Cluster Phv_Bind / alloc_i interface related member functions
//
//***********************************************************************************
//

const PHV::AllocSlice &PHV::Field::for_bit(int bit) const {
    for (auto &sl : alloc_slice_i)
        if (bit >= sl.field_slice().lo && bit < sl.field_slice().lo + sl.width())
            return sl;
    LOG1("ERROR: No allocation for bit " << bit << " in " << name);
    static PHV::AllocSlice invalid(nullptr, PHV::Container(), 0, 0, 0);
    return invalid;
}

const std::vector<PHV::AllocSlice> PHV::Field::get_combined_alloc_bytes(
        const PHV::AllocContext* ctxt,
        const PHV::FieldUse* use,
        SliceMatch useTblRefs) const {
    std::vector<PHV::AllocSlice> slicesToProcess;
    // Map of container to byte within the container to the set of alloc slices in that container
    // byte.
    LOG6("   get_combined_alloc_bytes for " << name);
    ordered_map<PHV::Container, ordered_map<int, std::vector<PHV::AllocSlice>>> containerBytesMap;
    foreach_byte(ctxt, use, [&](const PHV::AllocSlice& alloc) {
        int byte = alloc.container_slice().lo / 8;
        containerBytesMap[alloc.container()][byte].push_back(alloc); }, useTblRefs);
    for (auto container_and_byte : containerBytesMap) {
        LOG3("  Container: " << container_and_byte.first);
        for (auto byte_and_slices : container_and_byte.second) {
            LOG3("    Byte: " << byte_and_slices.first);
            if (byte_and_slices.second.size() == 1) {
                slicesToProcess.push_back(*(byte_and_slices.second.begin()));
                continue;
            }
            // If all the alloc slices within the same container are contiguous (both in terms of
            // field slice range and container slice range), then combine all of them into a single
            // alloc_slice that we return.
            safe_vector<std::pair<le_bitrange, le_bitrange>> field_cont_pairs;
            for (auto& slice : byte_and_slices.second) {
                LOG3("\t  Slice: " << slice);
                field_cont_pairs.emplace_back(slice.field_slice(), slice.container_slice());
            }

            std::sort(field_cont_pairs.begin(), field_cont_pairs.end(),
                [](const std::pair<le_bitrange, le_bitrange> &a,
                   const std::pair<le_bitrange, le_bitrange> &b) {
                return a.first < b.first;
            });

            for (size_t i = 0; i < field_cont_pairs.size() - 1; i++) {
                 auto a = field_cont_pairs[i];
                 auto b = field_cont_pairs[i+1];
                 if (a.first.hi + 1 == b.first.lo && a.second.hi + 1 == b.second.lo) {
                     le_bitrange comb_fb = { a.first.lo, b.first.hi };
                     le_bitrange comb_cb = { a.second.lo, b.second.hi };
                     field_cont_pairs[i] = std::make_pair(comb_fb, comb_cb);
                     field_cont_pairs.erase(field_cont_pairs.begin() + i + 1);
                     i--;
                 }
            }

            for (auto fb_cb_pair : field_cont_pairs) {
                LOG3("\t  Combined Slice: " << fb_cb_pair.first << " " << fb_cb_pair.second);
                slicesToProcess.emplace_back(this, container_and_byte.first, fb_cb_pair.first.lo,
                                             fb_cb_pair.second.lo, fb_cb_pair.first.size());
            }
        }
    }
    return slicesToProcess;
}

const std::vector<PHV::AllocSlice> PHV::Field::get_combined_alloc_slices(
        le_bitrange bits,
        const PHV::AllocContext* ctxt,
        const PHV::FieldUse* use) const {
    std::vector<PHV::AllocSlice> slicesToProcess;
    // Map of container to set of alloc slices within that container.
    ordered_map<PHV::Container, std::vector<PHV::AllocSlice>> containerToSlicesMap;
    foreach_alloc(bits, ctxt, use, [&](const PHV::AllocSlice& alloc) {
        containerToSlicesMap[alloc.container()].push_back(alloc);
    });
    for (auto container_and_slices : containerToSlicesMap) {
        LOG3("  Container: " << container_and_slices.first);
        if (container_and_slices.second.size() == 1) {
            slicesToProcess.push_back(*(container_and_slices.second.begin()));
            continue;
        }
        bitvec container_bits;
        bitvec field_bits;
        for (auto& slice : container_and_slices.second) {
            LOG3("\t  Slice: " << slice);
            container_bits |= bitvec(slice.container_slice().lo, slice.width());
            field_bits |= bitvec(slice.field_slice().lo, slice.width());
        }
        LOG3("\t\tContainer bits: " << container_bits << ", field_bits: " << field_bits);
        if (field_bits.is_contiguous() && container_bits.is_contiguous() &&
            container_bits.popcount() == field_bits.popcount()) {
            PHV::AllocSlice* newCombinedSlice = new PHV::AllocSlice(this,
                    container_and_slices.first, field_bits.min().index(),
                    container_bits.min().index(), field_bits.popcount());
            slicesToProcess.push_back(*newCombinedSlice);
        } else {
            for (auto& slice : container_and_slices.second)
                slicesToProcess.push_back(slice);
        }
    }
    return slicesToProcess;
}

void PHV::Field::foreach_byte(
        le_bitrange range,
        const PHV::AllocContext* ctxt,
        const PHV::FieldUse* use,
        std::function<void(const PHV::AllocSlice &)> fn,
        PHV::SliceMatch useTblRefs) const {
    // Iterate in reverse order, because alloc_i slices are ordered from field
    // MSB to LSB, but foreach_byte iterates from LSB to MSB.
    for (const auto& slice : boost::adaptors::reverse(alloc_slice_i)) {
        // Break after we've processed the last element.
        if (range.hi < slice.field_slice().lo)
            break;

        // Otherwise, skip allocations that don't overlap with the target
        // range.
        if (!range.overlaps(slice.field_slice()))
            continue;

        // TODO: HACK: clients of foreach_byte assume that @fn is invoked
        // exactly once for each byte of the field, which is violated if the
        // field is allocated in more than one place.  There was a time when
        // fields could be allocated to both PHV and TPHV (but this may not be
        // true any longer).  Hence, skip TPHV allocations.
        if (slice.container().is(PHV::Kind::tagalong))
            continue;

        // ignore other stage alloc slices
        if (!slice.isReferenced(ctxt, use, useTblRefs)) {
            continue;
        }

        // The requested range may only cover part of slice, so we create a new
        // slice that intersects with the range.
        auto overlap = range.intersectWith(slice.field_slice());
        PHV::AllocSlice intersectedSlice = *(slice.sub_alloc_by_field(overlap.lo, overlap.size()));

        // Apply @fn to each byte of the restricted allocation, taking care to
        // split the allocation along container byte boundaries.
        le_bitrange containerRange = intersectedSlice.container_slice();
        le_bitrange byte = StartLen(containerRange.loByte() * 8, 8);
        BUG_CHECK(byte.overlaps(containerRange),
                  "Bad alloc?\nByte: %1%\nContainer range: %2%\nField: %3%",
                  cstring::to_cstring(byte), cstring::to_cstring(containerRange),
                  cstring::to_cstring(this));
        while (byte.overlaps(containerRange)) {
            auto window = toClosedRange(byte.intersectWith(containerRange));
            BUG_CHECK(window, "Bad alloc slice");

            // Create an alloc_slice for this window.
            int offset = window->lo - containerRange.lo;
            PHV::AllocSlice tmp(this,
                                intersectedSlice.container(),
                                intersectedSlice.field_slice().lo + offset,
                                window->lo,
                                window->size());
            tmp.setLiveness(slice.getEarliestLiveness(), slice.getLatestLiveness());
            tmp.setIsPhysicalStageBased(slice.isPhysicalStageBased());

            // Invoke the function.
            fn(tmp);

            // Increment the window.
            byte = byte.shiftedByBits(8); } }
}

void PHV::Field::foreach_alloc(
        le_bitrange range,
        const PHV::AllocContext* ctxt,
        const PHV::FieldUse* use,
        std::function<void(const PHV::AllocSlice &)> fn,
        SliceMatch useTblRefs) const {
    // TODO: Maintain all the candidate alloc slices here. I am going to filter later based on
    // context and use on this vector during stage based allocation.
    auto apply_fn = [&] (const PHV::AllocSlice& slice) {
        LOG6("\tforeach_alloc slice: " << slice);
        fn(slice);
    };
    // Sort from hi to lo.
    for (auto it = alloc_slice_i.rbegin(); it != alloc_slice_i.rend(); ++it) {
        // ignore other stage alloc slices
        if (!it->isReferenced(ctxt, use, useTblRefs)) {
            continue;
        }

        int lo = it->field_slice().lo;
        int hi = it->field_slice().hi;
        // Required range is less than and disjoint with the allocated slice range.
        if (lo < range.lo && hi < range.lo) continue;
        // Required range is greater than and disjoint with the allocated slice range.
        if (lo > range.hi && hi > range.hi) continue;
        // Entire alloc slice is in the requested range, so add the slice to the candidates list.
        if (lo >= range.lo && hi <= range.hi) {
            apply_fn(*it);
            continue;
        }

        // Need to create a copy of the allocated slice with different width and modified range
        // because there is only a partial overlap with the requested range.
        auto overlap = range.intersectWith(it->field_slice());
        if (!overlap.empty()) {
            apply_fn(*(it->sub_alloc_by_field(overlap.lo, overlap.size())));
        }
    }
}

//
// constraints, phv_widths
//
bool PHV::Field::is_tphv_candidate(const PhvUse& uses) const {
    PHV::FieldSlice slice(this, StartLen(0, size));
    return slice.is_tphv_candidate(uses);
}

bool PHV::FieldSlice::is_tphv_candidate(const PhvUse& uses) const {
    if (field_i->padding) return false;  // __pad_ fields are not considered as tphv.
    // TODO derive these rather than hard-coding the name
    std::string f_name(field_i->name.c_str());
    // TODO somehow phv allocation can't derive this one
    if (f_name.find("$constant") != std::string::npos)
        return true;
    return !uses.is_used_mau(field_i, range_i) && !field_i->pov && !field_i->deparsed_to_tm() &&
        !field_i->is_digest() && (!field_i->metadata || uses.is_used_parde(field_i));
}

void PHV::Field::updateAlignment(PHV::AlignmentReason reason, const FieldAlignment& newAlignment,
                                 const Util::SourceInfo& newAlignmentSource) {
    LOG3("Inferred alignment " << newAlignment << " for field " << name);

    // If there's no existing alignment for this field, just take this one.
    if (!alignment) {
        alignment = newAlignment;
        alignmentSources.push_back({newAlignment, newAlignmentSource});

        // used by bridged packing
        alignment_i.addConstraint(reason, newAlignment.align);
        return;
    }

    // If there is an existing alignment, it must agree with this new one.
    // Otherwise the program is inconsistent and we can't compile it.
    if (*alignment != newAlignment) {
        auto alignmentSourceStr = [&](const FieldAlignment& alignment,
                                      const Util::SourceInfo& srcInfo) {
            std::stringstream ss;
            ss << (srcInfo.isValid() ? srcInfo.toPositionString() : "Source unknown")
               << ": alignment = " << alignment.align << " (little endian)" << std::endl
               << (srcInfo.isValid() ? srcInfo.toSourceFragment() : "");
            return ss.str();
        };
        std::stringstream inferredAlignments;
        inferredAlignments << alignmentSourceStr(newAlignment, newAlignmentSource)
                           << "Previously inferred alignments:" << std::endl;
        for (auto &alignmentSource : alignmentSources) {
            inferredAlignments << alignmentSourceStr(alignmentSource.first, alignmentSource.second);
        }

        std::string errorExplanation;
        switch (reason) {
            case PHV::AlignmentReason::PARSER :
                errorExplanation =
                    "Extracting or assigning values with different in-byte alignments "
                    "within packet to the same field is not supported in the parser.\n"
                    "Make sure that all values assigned to this field start at the same offset "
                    "within byte.";
                break;
            default:
                errorExplanation = "";
                break;
        }

        ::fatal_error("Inferred incompatible container alignments for field %1%:\n%2%%3%",
                      name, inferredAlignments.str(), errorExplanation);
    } else {
        alignmentSources.push_back({newAlignment, newAlignmentSource});

        alignment_i.updateConstraint(reason);
    }
}

void PHV::Field::eraseAlignment() {
    LOG3("Clear alignment for field " << name);
    // used by phv allocation
    alignment = std::nullopt;
    // used by bridged packing
    alignment_i.eraseConstraint();
}

void PHV::Field::setStartBits(PHV::Size size, bitvec startPositions) {
    if (metadata && is_checksummed() && !alignment) {
        startPositions = remove_non_byte_boundary_starts(this, startPositions);
    }
    startBitsByContainerSize_i[size] = startPositions;
}

bitvec PHV::Field::getStartBits(PHV::Size size) const {
    if (!startBitsByContainerSize_i.count(size))
        return bitvec(0, int(size));
    return startBitsByContainerSize_i.at(size);
}

void PHV::Field::setStartBitsToLowerBitsOfBottomByte() {
    int shiftable_bits = 0;
    if (size <= 8) {
        shiftable_bits = 8 - size;
    } else if (size > 8 && size <= 16) {
        shiftable_bits = 16 - size;
    } else {
        shiftable_bits = 7;
    }
    for (auto container_size : Device::phvSpec().containerSizes()) {
        startBitsByContainerSize_i[container_size] = bitvec(0, shiftable_bits + 1);
        LOG3("Setting field " << name << " to bottom byte: " <<
             startBitsByContainerSize_i[container_size]);
    }
}


PHV::AbstractField *PHV::AbstractField::create(const PhvInfo &info, const IR::Expression *e) {
    if (auto *c = e->to<IR::Constant>())
        return new PHV::Constant(c);
    le_bitrange bits = { };
    if (auto *field = info.field(e, &bits))
        return new PHV::FieldSlice(field, bits);
    BUG("Can't turn %s into an AbstractField", e);
    return nullptr;
}

PHV::FieldSlice::FieldSlice(
        const Field* field,
        le_bitrange range) : field_i(field), range_i(range) {
    BUG_CHECK(0 <= range.lo, "Trying to create field slice with negative start in range %1%",
              range);
    BUG_CHECK(range.size() <= field->size,
              "Trying to create field slice larger than field: %1%.size() > %2%", range, field);

    // Calculate relative alignment for this field slice.
    if (field->alignment) {
        le_bitrange field_range = StartLen(field->alignment->align, field->size);
        le_bitrange slice_range = field_range.shiftedByBits(range_i.lo)
            .resizedToBits(range_i.size());
        alignment_i = FieldAlignment(slice_range);
    }

    // Calculate valid container range for this slice by shrinking
    // the valid range of the field by the size of the "tail"
    // (i.e. the least significant bits) not in this slice.
    if (field->validContainerRange_i == ZeroToMax()) {
        validContainerRange_i = ZeroToMax();
    } else {
        int new_size = field->validContainerRange_i.size() - range.lo;
        // handle type upcast in extract.
        // e.g. in a parser state, if we have
        // struct md { bit<24> tunnel_id; }
        // header example {
        //     bit<16> tunnel_id;
        //     bit<16> junk;
        // }
        // md.tunnel_id = (bit<24>)hdr.example.tunnel_id
        //                input buffer
        // xxxxxx0.....................15..............31
        //          hdr.tunnel_id            hdr.junk
        //       |                     |
        //       v                     v
        //      15.....................0
        //         md.tunnel_id[15:0]
        // valid container range of md.tunnel_id is [0..15](Network Order),
        // because you can't extract from the outside of input buffer(those xxx).
        // The wired thing is that, the field is 24 bits, which can't fit into
        // the valid container range [0..15].
        // It indicates that
        // (1) the field must be split into [16, 8] to satisfy the valid container range.
        // (2) the 8-bit slice is not extracted, so no constraints introduced in this state.
        //     But if it is extracted in some other parser state, then
        //     the container valid range should be the intersection of all.
        // For (1), slicing iterator do not have this information but rely on searching.
        // For (2), it's complicated to take the intersection of those ranges under current
        // code structure. So we simply set it to the most conservative case: [0..size-1],
        // which means the slice must be place at the begining of the container, like there
        // are no other fields before it in the input buffer.
        if (new_size <= 0) {
            validContainerRange_i = StartLen(0, range.size());
        } else {
            validContainerRange_i = field->validContainerRange_i.resizedToBits(new_size);
        }
    }
}

bitvec PHV::FieldSlice::getStartBits(PHV::Size size) const {
    // The valid starting bits (by container size C) for a slice s[Y:X]
    // equal the valid bits for the field shifted by X mod C.  For example,
    // if a 12b field f can start at bits 0 and 8 in a 16b container, then
    // f[11:8] can start at bits 0 and 8.
    bitvec ret;
    const int checksum_bit_in_byte = range_i.lo % 8;
    for (auto idx : field_i->getStartBits(size)) {
        int bit_idx = (idx + range_i.lo) % int(size);
        // TODO: checksummed metadata can only start at byte boundary.
        if (field_i->is_checksummed() && field_i->metadata && !field_i->alignment &&
            bit_idx % 8 != checksum_bit_in_byte) {
            continue;
        }
        ret.setbit(bit_idx);
    }
    return ret;
}

void PHV::Field::updateValidContainerRange(nw_bitrange newValidRange) {
    LOG3("Inferred valid container range " << newValidRange <<
         " for field " << name);

    const auto intersection = validContainerRange_i.intersectWith(newValidRange);
    if (intersection.empty()) {
        ::fatal_error("Inferred container alignments for field %1%"
                      " that are impossible to satisfy", name);
        return;
    }

    validContainerRange_i = *toClosedRange(intersection);
}

bool PHV::Field::no_split() const {
    for (const auto& range : no_split_ranges_i) {
        if (range.size() == size) {
            return true; } }
    return false;
}

bool PHV::Field::has_no_split_at_pos() const {
    return (no_split_ranges_i.size() > 0);
}

void PHV::Field::set_no_split(bool b) {
    if (b) {
        set_no_split_at(le_bitrange(StartLen(0, size)));
    } else {
        no_split_ranges_i.clear();
    }
}

bool PHV::Field::no_split_at(int pos) const {
    // Note: A constaint that requires splitting a field at a specific bit
    // position means that we are allowed to split at the lsb position.
    // For example:
    //    x[63:0] = y[63:0] + 1
    //    x and y must be split into slices as [63:32] and [31:0].
    //    When creating slices, slicing at bit position 32 should be allowed.
    return std::any_of(
            no_split_ranges_i.begin(), no_split_ranges_i.end(), [&] (const le_bitrange& r) {
                return r.contains(pos) && r.lo != pos; });
}

void PHV::Field::set_no_split_at(le_bitrange range) {
    no_split_ranges_i.push_back(range);
}

//***********************************************************************************
//
// CollectPhvInfo implementation.
//
//***********************************************************************************

class ClearPhvInfo : public Inspector {
    PhvInfo& phv;
    bool preorder(const IR::BFN::Pipe*) override {
        phv.clear();
        return false;
    }

 public:
    explicit ClearPhvInfo(PhvInfo& phv) : phv(phv) { }
};

/** Populates a PhvInfo object with Fields for each PHV-backed object in the
 * program (header instances, TempVars, etc.), with the exception of POV
 * fields, which are created by the AllocatePOVBits pass.
 *
 * Note that this pass also collects field information for alias sources by
 * explicitly visiting the `source` children of AliasMembers and AliasSlices.
 * In this mode (i.e. when @p gress is supplied to the constructor), the internal
 * data structures are NOT cleared in init_apply.
 */
class CollectPhvFields : public Inspector {
    PhvInfo& phv;

    // WARNING(cole): CollectPhvFields is usually invoked on an entire IR, not
    // a subtree, which means that we can get the gress from
    // VisitingThread(this).  However, AliasMember and AliasSlice nodes have
    // untraversed pointers to the alias sources, which we need to get PHV info
    // for, as the allocation of their associated alias destinations will be
    // copies to the sources in AddAliasAllocation.
    //
    // In order to avoid code duplication, CollectPhvFields can be supplied
    // with a thread.  If present, (a) use this instead of
    // VisitingThread(this), and (b) don't clear the phv object in init_apply.
    //
    // This seems needlessly complex, but I can't think of a better way.

    /// Provided at construction when the visitor is intended to be invoked on
    /// a subtree, rather than the entire IR.  If not present, the gress is
    /// acquired from VisitingThread(this), which looks up the gress from the
    /// enclosing IR::BFN::Pipe.  This never changes after construction.
    std::optional<gress_t> gress;

    /// Tracks the header/metadata instances that have been added, to avoid
    /// duplication.
    ordered_set<cstring> seen;

    /// @returns the input gress (at construction) if provided, or
    /// VisitingThread(this) if not.
    gress_t getGress() const {
        auto rv = gress ? *gress : VisitingThread(this);
        // FIXME: for now treat GHOST thread as INGRESS for phv allocation
        return rv == GHOST ? INGRESS : rv;
    }

    Visitor::profile_t init_apply(const IR::Node* root) override {
        auto rv = Inspector::init_apply(root);

        // Only clear if this is a new pass, i.e. gress == std::nullopt.
        if (!gress) {
            seen.clear();
            LOG3("Begin CollectPhvFields");
        } else {
            LOG3("Begin CollectPhvFields (recursive)"); }

        return rv;
    }

    /// Collect field information for alias sources.
    bool preorder(const IR::BFN::AliasMember* alias) override {
        visit(alias->source);
        return true;
    }

    /// Collect field information for alias sources.
    bool preorder(const IR::BFN::AliasSlice* alias) override {
        visit(alias->source);
        return true;
    }

    bool preorder(const IR::Header* h) override {
        // Skip headers that have already been visited.
        if (seen.find(h->name) != seen.end()) return false;
        seen.insert(h->name);

        LOG1("add header " << h);

        int start = phv.by_id.size();
        phv.add_hdr(h->name.name, h->type, getGress(), false);
        int end = phv.by_id.size();

        // Ensure that the header hasn't been seen before
        auto it = phv.simple_headers.find(h->name.name);
        BUG_CHECK(it == phv.simple_headers.end(),
            "Header %1% already seen!", h->name.name);

        // Add the header to the simple_headers vector, as they
        // are seen in the program order
        phv.simple_headers.emplace(h->name.name, PhvInfo::StructInfo(false, getGress(),
                                                                     start, end - start));
        LOG3("Adding header  " << h->name.name << " to simple_headers");

        return false;
    }

    bool preorder(const IR::HeaderStack* h) override {
        // Skip headers that have already been visited.
        if (seen.find(h->name) != seen.end()) return false;
        seen.insert(h->name);

        if (!h->type) return false;
        char buffer[16];
        int start = phv.by_id.size();
        for (int i = 0; i < h->size; i++) {
            snprintf(buffer, sizeof(buffer), "[%d]", i);
            phv.add_hdr(h->name.name + buffer, h->type, getGress(), false); }
        int end = phv.by_id.size();
        LOG3("Adding header stack " << h->name);
        phv.all_structs.emplace(
            h->name.name,
            PhvInfo::StructInfo(false, getGress(), start, end - start));
        return false;
        LOG3("Adding headerStack  " << h->name.name << " to all_structs");
    }

    bool preorder(const IR::Metadata* h) override {
        // Skip headers that have already been visited.
        if (seen.find(h->name) != seen.end()) return false;
        seen.insert(h->name);

        LOG1("add meta " << h);
        phv.add_hdr(h->name.name, h->type, getGress(), true);
        return false;
    }

    bool preorder(const IR::TempVar* tv) override {
        phv.addTempVar(tv, getGress());

        if (findContext<IR::BFN::MatchLVal>()) {
            PHV::Field* f = phv.field(tv);
            BUG_CHECK(f, "No PhvInfo entry for a field we just added?");
            f->set_avoid_alloc(true);
        }
        // bridged_metadata_indicator must be placed in 8-bit container
        if (tv->name.endsWith(BFN::BRIDGED_MD_INDICATOR.string())) {
            PHV::Field* f = phv.field(tv);
            BUG_CHECK(f, "No PhvInfo entry for a field we just added?");
            f->set_exact_containers(true);
            f->set_solitary(PHV::SolitaryReason::ARCH);
            f->set_no_split(true);
        }
        return false;
    }

    bool preorder(const IR::Padding* pad) override {
        LOG3("Set constraints on padding " << pad);
        phv.addPadding(pad, getGress());
        PHV::Field* f = phv.field(pad);
        CHECK_NULL(f);
        f->set_exact_containers(true);
        f->set_deparsed(true);
        f->set_emitted(true);
        f->set_ignore_alloc(true);
        return false;
    }

    void postorder(const IR::BFN::LoweredParser*) override {
        BUG("Running CollectPhvInfo after the parser IR has been lowered; "
            "this will produce invalid results.");
    }

    void end_apply() override {
        for (auto& f : phv) {
            std::string f_name(f.name.c_str());
            if (f_name.find(BFN::COMPILER_META) != std::string::npos
             && f_name.find("residual_checksum_") != std::string::npos) {
                f.set_exact_containers(true);
                f.set_solitary(PHV::SolitaryReason::CHECKSUM);
                f.set_no_split(true);
            }
            if (f_name == "ingress::ig_intr_md_for_dprsr.digest_type")
                f.set_solitary(PHV::SolitaryReason::DIGEST);
            LOG5("CollectPHV Includes field : " << f);
        }
    }

 public:
    explicit CollectPhvFields(PhvInfo& phv, std::optional<gress_t> gress = std::nullopt)
    : phv(phv), gress(gress) { }
};

/// Allocate POV bits for each header instance, metadata instance, or header
/// stack that we visited in CollectPhvFields.
struct AllocatePOVBits : public Inspector {
    explicit AllocatePOVBits(PhvInfo& phv) : phv(phv) { }

 private:
    profile_t init_apply(const IR::Node* root) override {
        LOG3("BEGIN AllocatePOVBits");
        return Inspector::init_apply(root);
    }

    bool preorder(const IR::BFN::Pipe* pipe) override {
        BUG_CHECK(pipe->headerStackInfo != nullptr,
                  "Running AllocatePOVBits without running "
                  "CollectHeaderStackInfo first?");
        phv.allocatePOV(*pipe->headerStackInfo);
        return false;
    }

    PhvInfo& phv;
};

/// Examine how fields are used in the parser and deparser to compute their
/// alignment requirements.
struct ComputeFieldAlignments : public Inspector {
    explicit ComputeFieldAlignments(PhvInfo& phv) : phv(phv) { }

 private:
    bool preorder(const IR::BFN::Extract* extract) override {
        // Only extracts from the input buffer introduce alignment constraints.
        auto* bufferSource = extract->source->to<IR::BFN::InputBufferRVal>();
        if (!bufferSource) return false;
        auto lval = extract->dest->to<IR::BFN::FieldLVal>();
        if (!lval) return false;

        auto* fieldInfo = phv.field(lval->field);
        if (!fieldInfo) {
            warning(BFN::ErrorType::WARN_PHV_ALLOCATION, "No allocation for field %1%",
                      extract->dest);
            return false;
        }

        // The alignment required for a parsed field is determined by the
        // position from which it's read from the wire.
        const auto extractedBits = bufferSource->range;
        const auto alignment = FieldAlignment(extractedBits);
        LOG3("A. Updating alignment of " << fieldInfo->name << " to " << alignment);
        LOG3("Extract: " << extract << ", parser state: " <<
                findContext<IR::BFN::ParserState>()->name);
        fieldInfo->updateAlignment(PHV::AlignmentReason::PARSER, alignment,
                                   lval->field->getSourceInfo());

        // If a parsed field starts at a container bit index larger than the bit
        // index at which it's located in the input buffer, we won't be able to
        // extract it, because we'd have to read past the beginning of the input
        // buffer. For example (all indices are in network order):
        //
        //   Container: [ 0  1  2  3  4  5  6  7  8  9  10 11 12 13 14 15 ]
        //                                      [       field      ]
        //   Input buffer:                      [ 0  1  2  3  4  5 ]
        //
        // This field begins at position 0 in the input buffer, but because the
        // parser has to write to the entire container, to place the field at
        // position 8 in the container would require that bits [0, 7] of the
        // container came from a negative position in the input buffer.
        //
        // To avoid this, we generate a constraint that prevents PHV allocation
        // from placing the field in a problematic position in the container.
        //
        // In simple terms, what we want to avoid is placing the field too far
        // to the right in the container. For fields which come from the mapped
        // input buffer region (which has a fixed coordinate system that cannot
        // be shifted) or from the end of the headers in the input packet (where
        // we'll end up treating some of the packet payload as header if we
        // introduce an extra shift), a symmetric issue exists where we need to
        // avoid placing the field too far to the *left* in the container. Both
        // of these limitations need to be converted into constraints for PHV
        // allocation.
        // TODO: Unfortunately we can't capture the "too far to the left"
        // case with our current representation of valid container ranges.
        const nw_bitrange validContainerRange = FromTo(0, extractedBits.hi);
        fieldInfo->updateValidContainerRange(validContainerRange);

        return false;
    }

    bool preorder(const IR::BFN::Deparser* deparser) override {
        unsigned currentBit = 0;

        for (auto* emitPrimitive : deparser->emits) {
            if (auto* checksum = emitPrimitive->to<IR::BFN::EmitChecksum>()) {
                for (auto source : checksum->sources) {
                    auto f = phv.field(source->field->field);
                    if (f && f->metadata && f->size % 8) {
                        const auto alignment = FieldAlignment(le_bitrange(
                                    StartLen((source->offset + f->size), f->size)));
                        LOG3("B. Updating alignment of " << f->name << " to " << alignment);
                        f->updateAlignment(PHV::AlignmentReason::DEPARSER, alignment,
                                           source->field->field->getSourceInfo());
                    }
                }
            }
            auto* emit = emitPrimitive->to<IR::BFN::EmitField>();
            if (!emit) continue;

            auto* fieldInfo = phv.field(emit->source->field);
            if (!fieldInfo) {
                warning(BFN::ErrorType::WARN_PHV_ALLOCATION, "No allocation for field %1%",
                          emit->source);
                currentBit = 0;
                continue;
            }

            // For other deparsed fields, the alignment requirement is induced
            // by the position at which the field will be written on the wire.
            // We can behave as if every field will be deparsed (i.e., all POV
            // bits are set) because any sequential group of deparsed fields
            // with the same POV bits must be byte-aligned in a valid Tofino P4
            // program. (If not, you'd end up with garbage bits on the wire.)
            nw_bitrange emittedBits(currentBit, currentBit + fieldInfo->size - 1);
            LOG3("C. Updating alignment of " << fieldInfo->name << " to " << emittedBits);
            LOG3("Emit primitive: " << emitPrimitive);
            fieldInfo->updateAlignment(PHV::AlignmentReason::DEPARSER, FieldAlignment(emittedBits),
                                       emit->source->field->getSourceInfo());
            currentBit += fieldInfo->size;
        }

        return false;
    }

    bool preorder(const IR::MAU::Instruction* instr) {
        Log::TempIndent indent;
        LOG5("Preorder Instruction: " << *instr << "  name: " << instr->name << indent);
        PHV::Field *dst_f;

        // For non-set instructions accessing an AttachedOutput
        if ((instr->operands.size() == 3) && (instr->name != "set")) {
            int op_id = 0;
            for (auto op_f : instr->operands) {
                if (!op_id) {
                    // Keep destination field that may need alignment setting
                    dst_f = phv.field(op_f);
                } else {
                    // Check if source operand is slice of AttachedOutput node
                    if (auto slc = op_f->to<IR::Slice>()) {
                        if (slc->e0->to<IR::MAU::AttachedOutput>()) {
                            int rng = safe_width_bits(slc->e0->to<IR::Expression>()->type);
                            LOG5("Slice: " << slc->e0 << "[" << slc->getH() << ":" << slc->getL()
                                 << "]" << " width: " << rng);
                            size_t lo_bit = slc->getL();
                            for (auto cnt_sz : Device::phvSpec().containerSizes()) {
                                if ((int)cnt_sz < rng) continue;
                                // Set container LSbit constraint
                                dst_f->setStartBits(cnt_sz, bitvec(lo_bit, 1));
                                LOG5("setStartBits(" << lo_bit << ", 1) for " << dst_f->name <<
                                     " at container size: " << cnt_sz);
                            }
                            break;
                        }
                    }
                }
                op_id++;
            }
        }
        return false;
    }

    PhvInfo& phv;
};


/** Mark certain intrinsic metadata fields as invalidate_from_arch so that
 * we don't initialize them in the parser (see ComputeInitZeroContainers pass
 * in lower_parser.cpp)
 *
 * "Metadata is always considered to be valid. This is further explained in
 * Section 2.2.1. Metadata instances are initialized to 0 by default."
 *
 * For Tofino applications, we deviated from the spec for a few user-accessible
 * intrinsic metadata fields that are read at the deparser, because a valid
 * value of 0 has undesired behavior at the deparser (see field list below).
 *
 * Note that Glass does not expose the mirror field list type as a user-facing
 * field, but that may also apply here. (The learning digest type may also
 * apply here too.)
 *
 * All other metadata fields (intrinsic and user) should have their container
 * valid bit set coming out of the parser.
 *
 * P4_16 changed the semantic to be such that uninitialized metadata has an
 * undefined value.
 */

class AddIntrinsicConstraints : public Inspector {
    PhvInfo& phv;

    bool preorder(const IR::BFN::Pipe *pipe) override {
        // make hw_constrained_fields visible to this Inspector
        visit(pipe->thread[INGRESS].hw_constrained_fields);
        visit(pipe->thread[EGRESS].hw_constrained_fields);
        return true;
    }

    void end_apply() override {
        for (auto& f : phv) {
            if (f.pov) continue;

            for (auto& intr : {"ingress::ig_intr_md", "egress::eg_intr_md"}) {
                std::string f_name(f.name.c_str());
                if (f_name.find(intr) != std::string::npos) {
                    f.set_intrinsic(true);
                    LOG3("\tMarking field " << f.name << " as intrinsic");
                }
            }
        }
    }

    bool preorder(const IR::BFN::HardwareConstrainedField *hw_constrained_field) override {
        auto set_invalidate_from_arch = [&](const IR::Expression* expr) {
            BUG_CHECK(expr->is<IR::Member>() || expr->is<IR::Slice>(),
                "invalidate from arch field %1% cannot be found", expr);
            auto f = phv.field(expr);
            if (!f) return;
            f->set_solitary(PHV::SolitaryReason::ARCH);
            f->set_invalidate_from_arch(true);
            LOG3("\tMarking field " << f->name << " as invalidate from arch");
        };

        if (!(hw_constrained_field->constraint_type.getbit(
            IR::BFN::HardwareConstrainedField::INVALIDATE_FROM_ARCH))) return false;

        set_invalidate_from_arch(hw_constrained_field->expr);

        if (auto alias_member = hw_constrained_field->expr->to<IR::BFN::AliasMember>()) {
            set_invalidate_from_arch(alias_member->source);
        } else if (auto alias_slice = hw_constrained_field->expr->to<IR::BFN::AliasSlice>()) {
            set_invalidate_from_arch(alias_slice->source);
        }
        return false;
    }

 public:
    explicit AddIntrinsicConstraints(PhvInfo& phv) : phv(phv) { }
};

/**
 * TODO: padding field in header must be parsed, but it is often unused in
 * MAU or deparser, is overlayed with other fields. For example, when a header
 * field is deparsed in a digest, compiler will generate a new padding field in
 * place of the original padding field in the digest field list.
 *
 * As a result, the original padding field is not marked as 'deparsed', which
 * causes error in slicing, because slicing requires all fields in the same
 * slice must be either all 'deparsed' or all 'not-deparsed'. Mixing 'deparsed'
 * and 'not-deparsed' field in a slice is not allowed.
 *
 * This pass marks the padding as 'deparsed' whenever the actual field being
 * padded is deparsed. In another words, the 'deparsed' property on a padding
 * field is controlled by the padded field, not the padding itself.
 */
class MarkPaddingAsDeparsed : public Inspector {
    PhvInfo& phv;

    bool preorder(const IR::HeaderRef* hr) {
        LOG5("Mark padding in HeaderRef " << hr);

        if (auto hdrref = hr->to<IR::HeaderStackItemRef>()) {
            // Tofino doesn't support for indexing with a virable for header stack.
            auto idx = (hdrref->index())->to<IR::Constant>();
            // Is the index a constant?
            if (idx == nullptr) {
               BUG("For Tofino, Index of the header stack \"%s\" has to be a const value and "
                   "can't be a variable. (Please Note: Don't use a const value with width.)",
                   hdrref->base()->toString());
            }

            // If the constant has non-zero width, It might be propagated from some variable.
            auto tb = (idx->type)->to<IR::Type_Bits>();
            if ((tb != nullptr) && (tb->size != 0)) {
               BUG("For Tofino, Index of the header stack \"%s\" has to be a const value and "
                   "can't be a variable. (Please Note: Don't use a const value with width.)",
                   hdrref->base()->toString());
            }
        }

        const PhvInfo::StructInfo& struct_info = phv.struct_info(hr);

        // Only analyze headers, not metadata structs.
        if (struct_info.metadata || struct_info.size == 0) {
            LOG5("  ignoring metadata or zero struct info: " << hr);
            return false;
        }

        // unfortunately, deparsed and deparsed_to_tm are two separate
        // properties on PHV::Field.  PHV::Field in a slice must be either all
        // 'deparsed' or all 'deparsed_to_tm', not a mixture.
        bool lastDeparsed = false;
        bool lastDeparsedToTM = false;

        for (int fid : boost::adaptors::reverse(struct_info.field_ids())) {
            PHV::Field* field = phv.field(fid);
            CHECK_NULL(field);
            LOG5("  found field " << field);

            if (lastDeparsed && field->is_padding()) {
                field->set_deparsed(true);
                field->set_emitted(true);
                LOG5("    marking as deparsed " << field);
            }

            if (lastDeparsedToTM && field->is_padding()) {
                field->set_deparsed_to_tm(true);
                LOG5("    marking as deparsed_to_tm " << field);
            }

            lastDeparsed = !field->is_padding() && field->deparsed();
            lastDeparsedToTM = !field->is_padding() && field->deparsed_to_tm();
        }
        return false;
    }

 public:
    explicit MarkPaddingAsDeparsed(PhvInfo& phv) : phv(phv) { }
};

/** Sets constraint properties in PHV::Field objects based on constraints
 * induced by the parser/deparser.
 */
class CollectPardeConstraints : public Inspector {
    PhvInfo& phv;

    void handle_parser_write(const IR::BFN::ParserPrimitive *prim) {
        auto lval = prim->getWriteDest();
        if (!lval)
            return;
        auto f = phv.field(lval->field);
        BUG_CHECK(f, "Found extract %1% to a non-field object", lval->field);
        f->set_parsed(true);
        LOG3("\tMarking " << f->name << " as parsed");

        // TODO: this constraint can be refined, if the multi-write
        // happens before other writes, it can still be packed.
        if (prim->getWriteMode() == IR::BFN::ParserWriteMode::CLEAR_ON_WRITE) {
            // Keep POVs as simply solitary - compiler can handle it
            if (f->pov) {
                f->set_solitary(PHV::SolitaryReason::PRAGMA_SOLITARY);
                LOG3("Marking parser multi-write field " << f << " as no_pack");
            // Non POVs can at least be grouped among themselves (ones that
            // are extracted together)
            // Otherwise invalid superclusters are created for any field that is not
            // 8 bit aligned
            // Essentially this allows something like vlan.pcp (3b), vlan.cfi (1b)
            // and vlan.vlan_id (12b) to be packed together (since the clear-on-write
            // rewrites all of them together anyway)
            } else {
                f->set_solitary(PHV::SolitaryReason::CLEAR_ON_WRITE);
                LOG3("Marking parser multi-write field " << f << " no_pack(clear_on_write)");
            }
        }
    }

    bool preorder(const IR::BFN::Extract* extract) override {
        LOG4("\t CollectPardeConstraints: Extract " << *extract);
        handle_parser_write(extract);
        return false;
    }

    bool preorder(const IR::BFN::ParserChecksumWritePrimitive* checksum) override {
        LOG4("\t CollectPardeConstraints: Checksum " << *checksum);
        handle_parser_write(checksum);
        return false;
    }

    // If two fields in deparser are predicated by different POV bits,
    // we cannot pack them in the same container (deparser can only deparse whole
    // container, not byte in container).
    bool preorder(const IR::BFN::Deparser* deparser) override {
        for (auto it = deparser->emits.begin(); it != deparser->emits.end(); it++) {
            auto efi = (*it)->to<IR::BFN::EmitField>();
            if (!efi) continue;

            auto fi = phv.field(efi->source->field);
            auto pi = phv.field(efi->povBit->field);

            if (!fi) continue;  // FIXME(hanw): padding field. Need a better way to identify it.

            BUG_CHECK(fi && pi, "Deparser Emit with a non-PHV or non-POV source: %1%", efi);

            for (auto jt = deparser->emits.begin(); jt != deparser->emits.end(); jt++) {
                auto efj = (*jt)->to<IR::BFN::EmitField>();
                if (!efj) continue;

                auto fj = phv.field(efj->source->field);
                auto pj = phv.field(efj->povBit->field);

                if (!fj) continue;  // FIXME(hanw): padding field. Need a better way to identify it.

                if (pi != pj) {
                    phv.addDeparserNoPack(fi, fj);
                    LOG3("\tMarking " << fi->name << " and " << fj->name << " as deparser_no_pack");
                }
            }
        }

        return true;
    }

    void postorder(const IR::BFN::EmitChecksum* checksum) override {
        for (const auto* flval : checksum->sources) {
            PHV::Field* f = phv.field(flval->field->field);
            BUG_CHECK(f != nullptr, "Field not created in PhvInfo");
            f->set_is_checksummed(true);
            if (f->metadata && f->size % 8) {
                f->set_solitary(PHV::SolitaryReason::CHECKSUM);
                LOG3("Marking checksummed field " << f << " as no_pack");
                // TODO: Allocate multiple metadatas in a container instead of
                // allocation only one in a container. Allocation must consider the
                // alignment constraint on metadata due to its offset in checksum list.
            }
            LOG3("Checksummed field: " << f);
        }
    }

    void postorder(const IR::BFN::EmitField* emit) override {
        auto* src_field = phv.field(emit->source->field);
        BUG_CHECK(src_field, "Deparser Emit with a non-PHV source: %1%",
                  cstring::to_cstring(emit));
        // TODO: These two constraints will be subsumed by deparser schema.
        src_field->set_deparsed(true);
        src_field->set_exact_containers(true);
        src_field->set_emitted(true);
        LOG3("   marking field " << src_field << " as emitted");
    }

    static std::vector<cstring> bottom_bit_aligned_deparser_params() {
        static std::vector<cstring> rv;

        if (rv.empty()) {
            rv = { "eg_intr_md.egress_port"_cs,
                   "ig_intr_md_for_tm.mcast_grp_a"_cs,
                   "ig_intr_md_for_tm.mcast_grp_b"_cs,
                   "ig_intr_md_for_tm.ucast_egress_port"_cs};

            if (Device::currentDevice() == Device::TOFINO) {
                rv.insert(rv.end(), { "ig_intr_md_for_tm.level1_mcast_hash"_cs,
                                      "ig_intr_md_for_tm.level2_mcast_hash"_cs,
                                      "ig_intr_md_for_tm.level1_exclusion_id"_cs,
                                      "ig_intr_md_for_tm.level2_exclusion_id"_cs,
                                      "ig_intr_md_for_tm.rid"_cs});
            }
        }

        return rv;
    }

    // make hw_constrained_fields visible to this Inspector
    bool preorder(const IR::BFN::Pipe* pipe) override {
        visit(pipe->thread[INGRESS].hw_constrained_fields);
        visit(pipe->thread[EGRESS].hw_constrained_fields);
        return true;
    }

    bool preorder(const IR::BFN::HardwareConstrainedField *hw_constrained_field) override {
        auto set_deparsed_bottom_bits = [&](const IR::Expression* expr) {
            BUG_CHECK(expr->is<IR::Member>() || expr->is<IR::Slice>(),
                "deparsed bottom bits field %1% cannot be found", expr);
            auto f = phv.field(expr);
            if (!f) return;
            LOG3("D. Updating alignment of " << f->name << " to " <<
                FieldAlignment(le_bitrange(StartLen(0, f->size))));
            f->updateAlignment(PHV::AlignmentReason::DEPARSER,
                FieldAlignment(le_bitrange(StartLen(0, f->size))),
                hw_constrained_field->expr->getSourceInfo());
            f->set_deparsed_bottom_bits(true);
        };

        if (!(hw_constrained_field->constraint_type.getbit(
            IR::BFN::HardwareConstrainedField::DEPARSED_BOTTOM_BITS))) return false;

        set_deparsed_bottom_bits(hw_constrained_field->expr);
        if (auto alias_member = hw_constrained_field->expr->to<IR::BFN::AliasMember>()) {
            set_deparsed_bottom_bits(alias_member->source);
        } else if (auto alias_slice = hw_constrained_field->expr->to<IR::BFN::AliasSlice>()) {
            set_deparsed_bottom_bits(alias_slice->source);
        }
        return false;
    }

    void postorder(const IR::BFN::DeparserParameter* param) override {
        if (!param->source) return;

        // extract deparser constraints from Deparser & Digest IR nodes ref: bf-p4c/ir/parde.def
        // set deparser constaints on field
        PHV::Field* f = phv.field(param->source->field);
        BUG_CHECK(f != nullptr, "Field not created in PhvInfo");

        f->set_deparsed_to_tm(true);
        f->set_no_split(true);

        if (!f->deparsed_bottom_bits()) {
            // The lsb of DeparserParameter field must in the bottom byte.
            // The limit comes from a hardware limit that the deparser only has a 3-bit 'shift'
            // field, it can only (right) shift the value by at most 7 when extracting output
            // intrinsic metadata from the container.
            f->setStartBitsToLowerBitsOfBottomByte();
            LOG3("setting StartBitsToBottomByte for " << f << ", because it is DeparserParameter");
        }

        BUG_CHECK(f->size <= 32, "The architecture requires that field %1% not be split "
                  "across PHV containers, but the field is larger than the largest PHV container.",
                  cstring::to_cstring(f));
    }

    void postorder(const IR::BFN::Digest* digest) override {
        // learning, mirror, resubmit
        // have differing constraints -- bottom-bits, bridge-metadata mirror packing
        // learning, mirror field list in bottom bits of container, e.g.,
        // 301:ingress::$learning<3:0..2>
        // 590:egress::$mirror<3:0..2> specifies 1 of 8 field lists
        // currently, IR::BFN::Digest node has a string field to distinguish them by name
        if (digest->name != "learning" && digest->name != "mirror"
            && digest->name != "resubmit" && digest->name != "pktgen")
            return;

        PHV::Field* f = phv.field(digest->selector->field);
        BUG_CHECK(f != nullptr, "Field not created in PhvInfo");
        f->set_deparsed_bottom_bits(true);
        f->set_no_split(true);
        f->set_solitary(PHV::SolitaryReason::DIGEST);

        LOG1("Visit digest " << digest);
        for (auto fieldList : digest->fieldLists) {
            for (auto flval : fieldList->sources) {
                if (auto f = phv.field(flval->field)) {
                    if (digest->name == "learning")
                        f->set_digest(PHV::DigestType::LEARNING);
                    else if (digest->name == "mirror")
                        f->set_digest(PHV::DigestType::MIRROR);
                    else if (digest->name == "resubmit")
                        f->set_digest(PHV::DigestType::RESUBMIT);
                    else if (digest->name == "pktgen")
                        f->set_digest(PHV::DigestType::PKTGEN);
                    LOG1("\tf " << f);
                }
            }
        }

        if (digest->name == "resubmit") {
            LOG3("\t resubmit metadata field (" << f << ") is set to be "
                 << "exact container.");
            for (auto fieldList : digest->fieldLists) {
                LOG3("\t.....resubmit metadata field list....." << fieldList);
                int total_bits = 0;
                for (auto resubmit_field_expr : fieldList->sources) {
                    PHV::Field* resubmit_field = phv.field(resubmit_field_expr->field);
                    if (!resubmit_field) {
                        BUG("\t\t resubmit field does not exist: %1%",
                            resubmit_field_expr->field);
                    }
                    total_bits += resubmit_field->is_padding() ? 0 : resubmit_field->size;
                }
                for (auto resubmit_field_expr : fieldList->sources) {
                    PHV::Field* resubmit_field = phv.field(resubmit_field_expr->field);
                    if (resubmit_field) {
                        resubmit_field->set_exact_containers(true);
                        // an edge case for resubmit fields,
                        // that if they are in a 8-byte-long
                        // resubmit field list, then they must take the whole container.
                        if (resubmit_field->metadata || total_bits == 64) {
                            resubmit_field->set_is_marshaled(true);
                            LOG3("\t\t" << resubmit_field);
                        } else if (resubmit_field->padding) {
                            resubmit_field->set_deparsed(true);
                            LOG3("\t\t" << resubmit_field);
                        }
                    } else {
                        BUG("\t\t resubmit field does not exist: %1%",
                            resubmit_field_expr->field);
                    }
                }
            }
            return;
        }

        if (digest->name == "learning" || digest->name == "pktgen") {
            // Add byte-aligned constraint to metadata field in learning field_list
            // TODO: This constraint can be relaxed to be no_pack in a same byte.
            for (auto fieldList : digest->fieldLists) {
                for (auto fieldListEntry : fieldList->sources) {
                    auto fieldInfo = phv.field(fieldListEntry->field);
                    CHECK_NULL(fieldInfo);
                    if (fieldInfo->metadata) {
                        LOG3("E. Updating alignment of metadata field " << fieldInfo->name << " to "
                                << FieldAlignment(le_bitrange(StartLen(0, fieldInfo->size))));
                        fieldInfo->updateAlignment(
                                PHV::AlignmentReason::DIGEST,
                                FieldAlignment(le_bitrange(StartLen(0, fieldInfo->size))),
                                fieldListEntry->field->getSourceInfo());
                        LOG3(fieldInfo << " is marked to be byte_aligned "
                             "because it's in a field_list and digested.");
                    } else if (fieldInfo->padding) {
                        fieldInfo->set_exact_containers(true);
                        fieldInfo->set_deparsed(true);
                        LOG3("\t\t" << fieldInfo);
                    }
                }
            }

            if (LOGGING(3)) {
                for (auto fieldList : digest->fieldLists) {
                    LOG3("\t....." << digest->name << " field list..... ");
                    for (auto fieldListEntry : fieldList->sources) {
                        auto fieldInfo = phv.field(fieldListEntry->field);
                        if (fieldInfo)
                            LOG3("\t\t" << fieldInfo);
                        else
                            LOG3("\t\t" <<"-f?");
                    }
                }
            }
            return;
        }

        // For mirror digests, associate the mirror field with its field list,
        // which is used during constraint checks for bridge-metadata phv
        // allocation.
        LOG3(".....mirror fields in field list " << f->id << ":" << f->name);
        int fieldListIndex = 0;
        for (auto fieldList : digest->fieldLists) {
            LOG3("\t.....mirror metadata field list....." << fieldList);

            // The first two entries in the field list are both special and may
            // not be split between containers. The first indicates the mirror
            // session ID to the hardware. The second is used by the egress
            // parser to determine which field list a mirrored packet comes
            // from, and we need to be careful to ensure that it maintains the
            // expected layout exactly.
            if (!fieldList->sources.empty()) {
                auto* fieldInfo = phv.field(fieldList->sources[0]->field);
                if (fieldInfo) {
                    fieldInfo->set_no_split(true);
                    fieldInfo->set_is_marshaled(true);
                }
            }
            // XXX (Only for P4-14)
            if (BackendOptions().langVersion == CompilerOptions::FrontendVersion::P4_14 &&
                    fieldList->sources.size() > 1) {
                auto* fieldInfo = phv.field(fieldList->sources[1]->field);
                if (fieldInfo) fieldInfo->set_no_split(true);
            }

            // mirror_field_list fields are marked as is_marshaled, so that
            // the phv allocation will make sure that the allocation field can be
            // serialized without leaving any padding within the field.
            // TODO: One special case is that, on both gresses,
            // compiler_generated_meta.mirror_id must go to [H] and
            // compiler_generated_meta.mirror_source must go to [B].
            // This constraint is handled in the phv allocation.
            for (auto mirroredField : fieldList->sources) {
                PHV::Field* mirror = phv.field(mirroredField->field);
                if (mirror) {
                    if (mirror->metadata) {
                        mirror->mirror_field_list = {f, fieldListIndex};
                        mirror->set_exact_containers(true);
                        LOG3("\t\tSEtting mirrored metadata field to " << mirror);
                    } else {
                        mirror->set_exact_containers(true);
                        mirror->set_deparsed(true);
                        LOG3("\t\tSetting non metadata mirrored fields to " << mirror);
                    }
                } else {
                    BUG("\t\t mirror field does not exist: %1%", mirroredField->field);
                }
                fieldListIndex++;
            }
        }
    }

 public:
    explicit CollectPardeConstraints(PhvInfo& phv) : phv(phv) { }
};

/** The timestamp and version fields are located in a special part of the input
 * buffer, and PHV allocation needs to take care when allocating these fields:
 * Their start/end bits can't be placed too deep within a PHV container,
 * because that would cause the extractor to read off the end of the buffer.
 *
 * That is, both timestamp and version have both a validContainerStart range
 * (like extracted fields) but also a dual validContainerEnd range.
 *
 * However, rather than implement that for just two fields, this pass instead
 * sets the exact_containers requirement for these fields, sidestepping the
 * problem.
 */
class MarkTimestampAndVersion : public Inspector {
    PhvInfo& phv_i;

    void end_apply() {
        for (auto& f : phv_i) {
            cstring name = f.name;
            bool isTstamp = name.endsWith("global_tstamp") && f.is_intrinsic();
            bool isVersion = name.endsWith("global_ver") && f.is_intrinsic();
            if (isTstamp || isVersion) {
                LOG2("Setting exact_containers for " << f.name);
                f.set_exact_containers(true); } }
    }

 public:
    explicit MarkTimestampAndVersion(PhvInfo& phv) : phv_i(phv) { }
};

class MarkFieldAsBridged : public Inspector {
    PhvInfo& phv_i;

    cstring getOppositeGressFieldName(cstring name) {
        if (name.startsWith("ingress::")) {
            return ("egress::" + name.substr(9));
        } else if (name.startsWith("egress::")) {
            return ("ingress::" + name.substr(8));
        } else {
            BUG("Called getOppositeGressFieldName on unknown gress fieldname %1%", name);
            return cstring();
        }
    }

    void end_apply() {
        for (auto& f : phv_i) {
            if ((f.is_flexible() || f.metadata) && f.emitted()) {
                // ignore $constant generated from decaf, which is never bridged
                std::string f_name(f.name.c_str());
                if (f_name.find("$constant") != std::string::npos)
                    continue;
                f.bridged = true;
                LOG3("   marking field " << f << " as bridged");
            }
        }
    }

 public:
    explicit MarkFieldAsBridged(PhvInfo& phv) : phv_i(phv) { }
};

bool CollectExtractedTogetherFields::preorder(const IR::BFN::ParserState* state) {
    /* map a byte in ibuf to the corresponding set of metadata fields sourced from the byte. */
    ordered_map<int /* ibuf byte offset */, ordered_set<const PHV::Field*>> ibuf_to_fields[3];

    for (auto& statement : state->statements) {
        if (!statement->is<IR::BFN::Extract>())
            continue;
        auto extract = statement->to<IR::BFN::Extract>();
        BUG_CHECK(extract->dest, "Extract %1% does not have a destination",
                cstring::to_cstring(extract));
        BUG_CHECK(extract->dest->field, "Extract %1% does not have a destination field",
                cstring::to_cstring(extract));
        const PHV::Field* destField = phv_i.field(extract->dest->field);
        BUG_CHECK(destField, "Could not find destination field for extract %1%",
                cstring::to_cstring(extract));
        if (auto rval = extract->source->to<IR::BFN::PacketRVal>()) {
            auto offset = rval->range.lo / 8;
            if (auto mem = extract->dest->to<IR::BFN::FieldLVal>()) {
                auto destField = phv_i.field(mem->field);
                CHECK_NULL(destField);
                if (destField->metadata) {
                    ibuf_to_fields[destField->gress][offset].insert(destField);
                    LOG1("extracted " << destField << " from byte " << offset);
                }
            }
        }
    }

    // only generate extract_together constraint for egress metadata for now.
    auto& matrix = phv_i.getBridgedExtractedTogether();

    for (const auto& kv : ibuf_to_fields[EGRESS]) {
        for (const auto& f1 : kv.second) {
            for (const auto& f2 : kv.second) {
                if (!f1 || !f2) continue;
                if (f1->id == f2->id) continue;
                if (f1->padding || f2->padding) continue;
                LOG1("extract " << f1->name << " and " << f2->name << " together");
                matrix(f1->id, f2->id) = true;
            }
        }
    }

    return true;
}

/**
 *  If result of some arith ops goes into tempvar, then that tempvar can
 *  get allocated into wider container which makes the instruction alu
 *  work at different width than anticipated by the user. This poses
 *  problem for instructions like saturation where width of the operation
 *  affects the result.
 *
 *  Problem is fixed by constraining tempvar with solitary, exact and
 *  no_split which forces PHV allocation to allocate it into precise container.
 */
class ConstrainSatAddResultTempVars : public Inspector {
    PhvInfo& phv_i;

    bool needsConstraining(const IR::MAU::Instruction *ins) const {
        // If we allow slicing of int<X> and casting the result back to int
        // then subs and ssubs must be tested and possibly included here.
        return ins->name == "saddu" || ins->name == "sadds";
    }

 public:
    bool preorder(const IR::MAU::Instruction *ins) override {
        if (!needsConstraining(ins)) return false;

        auto expr = ins->operands.begin();
        if ((*expr)->is<IR::TempVar>()) {
            PHV::Field *f = phv_i.field((*expr)->to<IR::TempVar>());
            CHECK_NULL(f);
            f->set_exact_containers(true);
            f->set_solitary(PHV::SolitaryReason::ARCH);
            f->set_no_split(true);
        }

        return false;
    }

    explicit ConstrainSatAddResultTempVars(PhvInfo& phv) : phv_i(phv) { }
};

CollectPhvInfo::CollectPhvInfo(PhvInfo& phv) {
    addPasses({
        new ClearPhvInfo(phv),
        new CollectPhvFields(phv),
        new AllocatePOVBits(phv),
        new MapFieldToParserStates(phv),
        new CollectPardeConstraints(phv),
        new MarkFieldAsBridged(phv),
        new ComputeFieldAlignments(phv),
        new AddIntrinsicConstraints(phv),
        new MarkTimestampAndVersion(phv),
        new MarkPaddingAsDeparsed(phv),
        new CollectExtractedTogetherFields(phv),
        new ConstrainSatAddResultTempVars(phv)
    });
}
//
//
//***********************************************************************************
//
// output stream <<
//
//***********************************************************************************
//

std::ostream &PHV::operator<<(std::ostream &out, const PHV::AllocContext &ac) {
    out << "Context [ " << ac.typeStr();
    if (ac.is_table() && ac.table) out << " " << ac.table->name;
    out << " ]";
    return out;
}

std::ostream &PHV::operator<<(std::ostream &out, const PHV::Field &field) {
    out << field.id << ':' << field.name << '<' << field.size << '>';
    if (field.alignment)
        out << " ^" << field.alignment->align;
    else
        out << " ^x";
    if (field.validContainerRange_i != ZeroToMax())
        out << " ^" << field.validContainerRange_i;
    else
        out << " ^x";
    if (field.bridged) out << " bridge";
    if (field.metadata) out << " meta";
    if (field.is_intrinsic()) out << " intrinsic";
    if (field.is_digest()) out << " digest";
    if (field.mirror_field_list.member_field)
        out << " mirror%{"
            << field.mirror_field_list.member_field->id
            << ":" << field.mirror_field_list.member_field->name
            << "#" << field.mirror_field_list.field_list
            << "}%";
    if (field.pov) out << " pov";
    if (field.is_deparser_zero_candidate())
        out << " deparsed-zero";
    else if (field.deparsed())
        out << " deparsed";
    if (field.is_solitary()) out << " solitary" << field.getSolitaryConstraint();
    if (field.is_flexible()) out << " flexible";
    if (field.padding) out << " padding";
    if (field.overlayable) out << " overlayable";
    if (field.no_split()) {
        out << " no_split";
    } else if (field.no_split_ranges().size() > 0) {
        for (const auto& range : field.no_split_ranges()) {
            out << " no_split_at" << range; } }
    if (field.no_holes()) out << " no_holes";
    if (field.deparsed_bottom_bits()) out << " deparsed_bottom_bits";
    if (field.deparsed_to_tm()) out << " deparsed_to_tm";
    if (field.exact_containers()) out << " exact_containers";
    if (field.used_in_wide_arith()) out << " wide_arith";
    if (Device::phvSpec().hasContainerKind(PHV::Kind::mocha)) {
        if (field.is_mocha_candidate()) out << " mocha";
    }
    if (Device::phvSpec().hasContainerKind(PHV::Kind::dark)) {
        if (field.is_dark_candidate()) out << " dark";
    }
    if (field.is_checksummed()) out << " checksummed";
    if (field.same_container_group()) {
        out << " same_container_group";
    }
    for (auto &as : field.alloc_slice_i) {
        out << " ";
        if (as.field_slice().lo != 0 || as.width() != field.size) {
            out << as.field_slice().lo;
            if (as.width() > 1) out << ".." << as.field_slice().hi;
            out << ":"; }
        out << as.container();
        if (as.container_slice().lo != 0 || size_t(as.width()) != as.container().size()) {
            out << "(" << as.container_slice().lo;
            if (as.width() > 1) out << ".." << as.container_slice().hi;
            out << ")"; } }
    return out;
}

std::ostream &PHV::operator<<(std::ostream &out, const PHV::Field *fld) {
    if (fld) return out << *fld;
    return out << "(nullptr)";
}

// ordered_set Field*
std::ostream &operator<<(std::ostream &out, const ordered_set<PHV::Field *>& field_set) {
    for (auto &f : field_set) {
        out << f << std::endl;
    }
    return out;
}

// ordered_set const Field*
std::ostream &operator<<(std::ostream &out, const ordered_set<const PHV::Field *>& field_set) {
    for (auto &f : field_set) {
        out << f << std::endl;
    }
    return out;
}

std::ostream &operator<<(std::ostream &out, const PhvInfo &phv) {
    out << "++++++++++ All Fields name[size]{range} (" << phv.num_fields() << ") ++++++++++"
        << std::endl
        << std::endl;
    //
    for (auto& field : phv) {
         out << &field << std::endl;
    }
    return out;
}

std::ostream &operator<<(std::ostream &out, const PHV::FieldAccessType &op) {
    switch (op) {
        case PHV::FieldAccessType::NONE: out << "None"; break;
        case PHV::FieldAccessType::R: out << 'R'; break;
        case PHV::FieldAccessType::W: out << 'W'; break;
        case PHV::FieldAccessType::RW: out << "RW"; break;
        default: out << "<FieldAccessType " << int(op) << ">"; }
    return out;
}


std::ostream &operator<<(std::ostream& out, const PhvInfo::SameContainerAllocConstraint& c) {
    out << "SameContainerAllocConstraint:\n";
    for (const auto& set : c.same_byte_bits) {
        out << "{";
        std::string sep = "";
        for (const auto& e : set) {
            out << sep << e;
            sep = ", ";
        }
        out << "}\n";
    }
    return out;
}

std::ostream& operator<<(std::ostream& out,
                         const PhvInfo::SameContainerAllocConstraint::FieldBit& fb) {
    out << fb.first->name << "[" << fb.second << "]";
    return out;
}

namespace PHV {

std::ostream &operator<<(std::ostream &out, const PHV::FieldSlice& fs) {
    if (fs.field() == nullptr) {
        out << "-field-slice-of-null-field-ptr-";
        return out; }

    if (DBPrint::dbgetflags(out) & DBPrint::Brief) {
        out << fs.shortString();
        return out; }

    auto& field = *fs.field();
    out << field.name << "<" << field.size << ">";
    if (fs.alignment())
        out << " ^" << fs.alignment()->align;
    if (fs.validContainerRange() != ZeroToMax())
        out << " ^" << fs.validContainerRange();
    if (field.bridged) out << " bridge";
    if (field.metadata) out << " meta";
    if (field.is_intrinsic()) out << " intrinsic";
    if (field.mirror_field_list.member_field)
        out << " mirror%{"
            << field.mirror_field_list.member_field->id
            << ":" << field.mirror_field_list.member_field->name
            << "#" << field.mirror_field_list.field_list
            << "}%";
    if (field.pov) out << " pov";
    if (field.is_deparser_zero_candidate())
        out << " deparsed-zero";
    else if (field.deparsed())
        out << " deparsed";
    if (field.is_solitary()) out << " solitary";
    if (field.no_split()) out << " no_split";
    if (field.no_holes()) out << " no_holes";
    if (field.deparsed_bottom_bits()) out << " deparsed_bottom_bits";
    if (field.deparsed_to_tm()) out << " deparsed_to_tm";
    if (field.exact_containers()) out << " exact_containers";
    if (field.used_in_wide_arith()) out << " wide_arith";
    if (Device::phvSpec().hasContainerKind(PHV::Kind::mocha)) {
        if (field.is_mocha_candidate()) out << " mocha";
    }
    if (Device::phvSpec().hasContainerKind(PHV::Kind::dark)) {
        if (field.is_dark_candidate()) out << " dark";
    }
    if (field.same_container_group()) {
        out << " same_container_group";
    }
    out << " [" << fs.range().lo << ":" << fs.range().hi << "]";
    return out;
}

std::ostream &operator<<(std::ostream &out, const PHV::FieldSlice* fs) {
    if (fs)
        out << *fs;
    else
        out << "-null-field-slice ";
    return out;
}

std::ostream& operator<<(std::ostream& out, const PackingLayout& p) {
    out << "[";
    std::string sep = "";
    for (const auto& v : p.layout) {
        out << sep;
        sep = ", ";
        if (v.is_fs()) {
            out << v.fs().first->name << " " << v.fs().second;
        } else {
            out << v.size();
        }
    }
    out << "]";
    return out;
}

std::ostream& operator<<(std::ostream& out, const PackingLayout* p) {
    out << *p;
    return out;
}

}   // namespace PHV

namespace std {

ostream &operator<<(ostream &out, const list<::PHV::Field *>& field_list) {
    for (auto &f : field_list) {
        out << f << std::endl;
    }
    return out;
}

ostream &operator<<(ostream &out, const set<const ::PHV::Field *>& field_list) {
    for (auto &f : field_list) {
        out << f << std::endl;
    }
    return out;
}

}  // namespace std

// for calling from the debugger
void dump(const PhvInfo &phv) { std::cout << phv; }
void dump(const PhvInfo *phv) { std::cout << *phv; }
void dump(const PHV::Field &f) { std::cout << f << std::endl; }
void dump(const PHV::Field *f) { std::cout << *f << std::endl; }

const IR::Node* PhvInfo::DumpPhvFields::apply_visitor(const IR::Node *n, const char *) {
    LOG1("");
    LOG1("--- PHV FIELDS -------------------------------------------");
    LOG1("");
    LOG1("P: used in parser/deparser");
    LOG1("M: used in MAU");
    LOG1("R: referenced anywhere");
    LOG1("D: is deparsed");
    LOG1("Alias source fields may have no labels");
    for (auto& f : phv) {
        LOG1("(" <<
              (uses.is_used_parde(&f) ? "P" : " ") <<
              (uses.is_used_mau(&f) ? "M" : " ") <<
              (uses.is_referenced(&f) ? "R" : " ") <<
              (uses.is_deparsed(&f) ? "D" : " ") << ") " << /*f.externalName()*/
              f << " (" << f.gress << ")") ; }
    LOG1("");

    LOG1("--- PHV FIELDS WIDTH HISTOGRAM----------------------------");
    LOG1("\nINGRESS:");
    generate_field_histogram(INGRESS);
    LOG1("\nEGRESS:");
    generate_field_histogram(EGRESS);
    LOG1("\n");
    return n;
}

void PhvInfo::DumpPhvFields::generate_field_histogram(gress_t gress) const {
    if (!LOGGING(1)) return;
    std::map<int, size_t> size_distribution;
    size_t total_bits = 0;
    size_t total_fields = 0;
    for (auto& field : phv) {
        if (field.gress == gress) {
            // Only report histogram for fields that aren't dead code eliminated
            if (uses.is_referenced(&field) || uses.is_deparsed(&field) || uses.is_used_mau(&field)
                    || uses.is_used_parde(&field)) {
                size_distribution[field.size] += 1;
                total_bits += field.size;
                total_fields++; } } }
    // Print histogram
    LOG1("Fields to be allocated: " << total_fields);
    LOG1("Bits to be allocated: " << total_bits);
    for (auto entry : size_distribution) {
        std::stringstream row;
        row << entry.first << "\t";
        for (size_t i = 0; i < entry.second; i++)
            row << "x";
        row << " (" << entry.second << ")";
        LOG1(row.str()); }
}

bool PhvInfo::SameContainerAllocConstraint::same_container(const PHV::FieldSlice& a,
                                                           const PHV::FieldSlice& b) const {
    const FieldBit a_lo{a.field(), a.range().lo};
    const FieldBit a_hi{a.field(), a.range().hi};
    const FieldBit b_lo{b.field(), b.range().lo};
    const FieldBit b_hi{b.field(), b.range().hi};
    std::initializer_list<const FieldBit*> bits{&a_lo, &a_hi, &b_lo, &b_hi};
    for (const auto* b : bits) {
        if (!same_byte_bits.contains(*b)) {
            return false;
        }
    }
    return same_byte_bits.find(a_lo) == same_byte_bits.find(b_hi) ||
           same_byte_bits.find(a_hi) == same_byte_bits.find(b_lo);
}

PhvInfo::ContainterToSliceMap
PhvInfo::getContainerToSlicesMap(std::function<bool(const PHV::Field*)> *f,
                        std::function<bool(const PHV::AllocSlice*)> *s) const {
    ContainterToSliceMap cont_to_slices;
    for (const PHV::Field& field : *this) {
        if (f && (*f)(&field)) continue;
        const auto alloc_slices = get_alloc(&field);
        for (const auto& alloc_slice : alloc_slices) {
            if (s && (*s)(&alloc_slice)) continue;
            cont_to_slices[alloc_slice.container()].push_back(alloc_slice);
            LOG5("Container: " << alloc_slice.container()
                               << " --> slice: " << alloc_slice);
        }
    }
    return cont_to_slices;
}
