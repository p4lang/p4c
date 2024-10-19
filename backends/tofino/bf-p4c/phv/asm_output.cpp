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

#include <random>
#include <set>

#include "bf-p4c-options.h"
#include "bf-p4c/common/asm_output.h"
#include "bf-p4c/common/field_defuse.h"
#include "bf-p4c/device.h"
#include "bf-p4c/phv/phv.h"
#include "bf-p4c/phv/phv_fields.h"
#include "lib/stringref.h"

PhvAsmOutput::PhvAsmOutput(const PhvInfo &p, const FieldDefUse &defuse,
                           const TableSummary &tbl_summary,
                           const LiveRangeReport *live_range_report, bool have_ghost)
    : phv(p),
      defuse(defuse),
      tbl_summary(tbl_summary),
      live_range_report(live_range_report),
      have_ghost(have_ghost) {}

void PhvAsmOutput::getLiveRanges(LiveRangePerContainer &lr) const {
    const auto &livemap = live_range_report->get_livemap();
    for (const auto &kv : livemap) {
        auto f = kv.first;
        int min = live_range_report->get_max_stages(), max = -1;
        for (const auto &stg : kv.second) {
            if (stg.first < min)
                min = ((stg.second.isWrite() && !stg.second.isRead() && (stg.first != -1))
                           ? (stg.first + 1)
                           : stg.first);
            if (stg.first >= max) max = (stg.second.isWrite() ? (stg.first + 1) : stg.first);
        }
        f->foreach_alloc([&](const PHV::AllocSlice &alloc) {
            auto c = alloc.container();
            cstring fname = cstring::to_cstring(canon_name(f->externalName()));
            ordered_set<cstring> mutex_fields;
            for (const auto *f2 : phv.fields_in_container(c)) {
                if (phv.isFieldMutex(f, f2)) {
                    cstring mfname = cstring::to_cstring(canon_name(f2->externalName()));
                    mutex_fields.insert(mfname);
                }
            }

            auto fldUse = alloc.getEarliestLiveness().second;
            int alloc_min = (fldUse.isWrite() && !fldUse.isRead())
                                ? alloc.getEarliestLiveness().first + 1
                                : alloc.getEarliestLiveness().first;
            fldUse = alloc.getLatestLiveness().second;
            int alloc_max = fldUse.isWrite() ? alloc.getLatestLiveness().first + 1
                                             : alloc.getLatestLiveness().first;

            int newMin = min, newMax = max;
            // Skip POV bits as they are always used in deparser
            if (!f->pov) {
                // FIXME: Skip Tofino field slices, since slice information does
                // not reflect the correct deparser stage. The deparser stage
                // value is set through the table dependency graph based on min
                // stage calculation. After table placement, no. of stages can
                // be >= min stage. However the slices created for phv
                // allocation already have the min stage info which is
                // incorrect. This needs to be updated after table placement.
                // This needs revisiting if/when stage based phv allocation is
                // implemented for Tofino Ideally this check should go away once
                // slices have updated info on max stage.
                if (Device::currentDevice() != Device::TOFINO) {
                    // Defuse information coming through livemap does not
                    // account for stage based allocation in TOFINO2+
                    // architectures. This information is provided through
                    // min/max stage values in field slices. Here, we adjust the
                    // live range based on slice info
                    if (alloc_min > min) newMin = alloc_min;
                    if (alloc_max < max) newMax = alloc_max;
                }
            }

            // alt-phv-alloc mode does not have any bug of deparser stage like ^. Just use
            // slice live range is correct.
            if (BFNContext::get().options().alt_phv_alloc) {
                newMin = alloc_min;
                newMax = alloc_max;
            }

            FieldUse fuse = {
                fname,
                f->gress,
                newMin,
                newMax,
                (f->parsed() && (newMin == -1)),
                ((f->deparsed() || f->deparsed_to_tm()) && (newMax == Device::numStages())),
                mutex_fields};
            auto &fieldsInContainer = lr[f->gress][c];
            fieldsInContainer[fname].push_back(fuse);
            LOG5(" Slice : " << alloc << ", Parsed : " << f->parsed() << ", Deparsed : "
                             << f->deparsed() << ", Deparsed to tm: " << f->deparsed_to_tm()
                             << ", Deparsed bottom bits: " << f->deparsed_bottom_bits()
                             << ", New min stage : " << newMin << ", New max stage : " << newMax);
        });
    }
}

void emit_alloc(std::ostream &out, const PHV::AllocSlice &alloc, PHV::Field *f) {
    out << "  " << canon_name(f->externalName());
    if (alloc.field_slice().lo > 0 || alloc.width() < f->size)
        out << '.' << alloc.field_slice().lo << '-' << alloc.field_slice().hi;
    out << ": " << alloc.container();
    if (alloc.container_slice().lo > 0 ||
        alloc.container().size() != static_cast<size_t>(alloc.width())) {
        out << '(' << alloc.container_slice().lo;
        if (alloc.width() > 1) out << ".." << alloc.container_slice().hi;
        out << ')';
    }
    out << std::endl;
}

/* For testing assmbler's stage based allocation - takes the current PHV allocation
 * and writes it out as a set of random stage based allocation
 */
void emit_stage_alloc(std::ostream &out, const PHV::AllocSlice &alloc, PHV::Field *f) {
    out << "  " << canon_name(f->externalName());
    if (alloc.field_slice().lo > 0 || alloc.width() < f->size)
        out << '.' << alloc.field_slice().lo << '-' << alloc.field_slice().hi;

    std::set<int> stageset;
    std::random_device rd;
    std::mt19937 mteng(rd());
    std::uniform_int_distribution<> dist(0, Device::numStages());
    stageset.insert(0);
    stageset.insert(Device::numStages());
    for (int i = 0; i < dist(mteng); i++) {
        stageset.insert(dist(mteng));
    }

    out << ": " << " { ";
    int first = *(stageset.begin());
    stageset.erase(stageset.cbegin());
    bool prntc = true;
    for (auto it : stageset) {
        if (prntc)
            prntc = false;
        else
            out << ',';
        out << " stage " << first << ".." << it << ": " << alloc.container();
        if (alloc.container_slice().lo > 0 ||
            alloc.container().size() != static_cast<size_t>(alloc.width())) {
            out << '(' << alloc.container_slice().lo;
            if (alloc.width() > 1) out << ".." << alloc.container_slice().hi;
            out << ") ";
        }
        first = it + 1;
    }

    out << " } ";
    out << std::endl;
}

void emit_stage_phv_field(std::ostream &out, PHV::Field *field, const LiveRangeReport *lr_report) {
    ordered_map<le_bitrange, std::vector<PHV::AllocSlice>> fieldRangeToAllocMap;
    ordered_set<le_bitrange> minimum_alloc_ranges;

    // this foreach_alloc collects all base alloc slice field ranges. The definition of base field
    // ranges is that in a set of base field ranges S, every live range does not contain other field
    // ranges. And for every field range in a field F, there is a set of fields, which is a subset
    // of S, that can cover field F. For example, a field contains three field ranges [0-8], [0-6]
    // [7-8]. The base field ranges are [0-6], [7-8]
    field->foreach_alloc([&](const PHV::AllocSlice &alloc) {
        auto alloc_range = alloc.field_slice();
        LOG3("\tAlloSlice: " << alloc);
        ordered_set<le_bitrange> to_remove;
        for (const auto &range : minimum_alloc_ranges) {
            if (range.contains(alloc_range)) {
                to_remove.push_back(range);
            }
        }
        for (const auto &range : to_remove) {
            LOG3("\t Remove from minimum_alloc_ranges: " << range);
            minimum_alloc_ranges.erase(range);
        }
        minimum_alloc_ranges.push_back(alloc_range);
    });

    // this foreach_alloc tries to deal with cases like this:
    // field [0-8] from stage 0 to stage 3, field [0-6] from stage 4 to stage 11, field [7-8] from
    // stage 4 to stage 13. In bfa, `field [0-8] from stage 0 to stage 3` needs to be splitted to
    // field [0-6] from stage 0 to stage 3 and field [7-8] from stage 0 to stage 3.
    field->foreach_alloc([&](const PHV::AllocSlice &alloc) {
        for (const auto &range : minimum_alloc_ranges) {
            if (alloc.field_slice() == range) {
                fieldRangeToAllocMap[range].push_back(alloc);
            } else if (alloc.field_slice().contains(range)) {
                le_bitrange container_range(alloc.container_slice().lo + range.lo,
                                            alloc.container_slice().lo + range.hi);
                auto new_alloc =
                    new PHV::AllocSlice(field, alloc.container(), range, container_range);
                new_alloc->setLiveness(alloc.getEarliestLiveness(), alloc.getLatestLiveness());
                LOG3("\t  Creating new AllocSlice " << *new_alloc);
                fieldRangeToAllocMap[range].push_back(*new_alloc);
            }
        }
    });
    // No allocation for the field.
    if (fieldRangeToAllocMap.size() == 0) return;

    const auto &livemap = lr_report->get_livemap();
    int lr_min = lr_report->get_max_stages(), lr_max = -1;
    if (livemap.find(field) != livemap.end()) {
        for (const auto &stg : livemap.at(field)) {
            if (stg.first < lr_min)
                lr_min =
                    ((stg.second.isWrite() && !stg.second.isRead()) ? (stg.first + 1) : stg.first);
            if (stg.first > lr_max)
                lr_max = (field->deparsed() || field->deparsed_to_tm())
                             ? lr_report->get_max_stages()
                             : (stg.second.isWrite() ? (stg.first + 1) : stg.first);
        }
    } else {
        LOG5("WARNING: No livemap found for field " << field->name);
        lr_min = -1;
        lr_max = lr_report->get_max_stages();
    }

    for (auto kv : fieldRangeToAllocMap) {
        unsigned numAllocSlices = kv.second.size();
        unsigned alloc_num = 0;
        bool stageAllocReqd = false;
        out << "  " << canon_name(field->externalName());
        if (kv.first.lo > 0 || kv.first.size() < field->size)
            out << '.' << kv.first.lo << '-' << kv.first.hi;
        out << ": ";
        std::sort(kv.second.begin(), kv.second.end(),
                  [](const PHV::AllocSlice &lhs, const PHV::AllocSlice &rhs) {
                      if (lhs.getEarliestLiveness().first != rhs.getEarliestLiveness().first)
                          return lhs.getEarliestLiveness().first < rhs.getEarliestLiveness().first;
                      return lhs.getEarliestLiveness().second < rhs.getEarliestLiveness().second;
                  });
        for (auto &alloc : kv.second) {
            ++alloc_num;
            int min_stage, max_stage;
            // For Tofino (if alt-phv-alloc not enabled),
            // no need to look in slice's Liveness because
            // we do not support dark overlay (split liverange into
            // smaller liveranges) and also because liveness has not
            // been translated to placed-table stages.
            if (Device::currentDevice() != Device::TOFINO ||
                BFNContext::get().options().alt_phv_alloc) {
                if (alloc.getEarliestLiveness().first == -1)
                    min_stage = 0;
                else if (alloc.getEarliestLiveness().second.isWrite())
                    min_stage = alloc.getEarliestLiveness().first + 1;
                else
                    min_stage = alloc.getEarliestLiveness().first;

                if (alloc.getLatestLiveness().second.isWrite() &&
                    alloc.getLatestLiveness().first != lr_report->get_max_stages())
                    max_stage = alloc.getLatestLiveness().first + 1;
                else
                    max_stage = alloc.getLatestLiveness().first;
                if (min_stage < lr_min) min_stage = lr_min;
                if (max_stage > lr_max)
                    max_stage = ((lr_max == -1) ? 0 : ((lr_max < lr_min) ? lr_min : lr_max));
            } else {  // TOFINO device
                min_stage = ((lr_min == -1) ? 0 : lr_min);
                max_stage = ((lr_max == -1) ? 0 : ((lr_max < lr_min) ? lr_min : lr_max));
            }

            if (min_stage != 0 || max_stage != lr_report->get_max_stages()) {
                stageAllocReqd = true;
                if (alloc_num == 1) out << "{ ";
                if (min_stage != max_stage)
                    out << " stage " << min_stage << ".." << max_stage << ": " << alloc.container();
                else
                    out << " stage " << min_stage << ": " << alloc.container();
            } else {
                out << alloc.container();
            }
            bool containerSliceReqd =
                alloc.container_slice().lo > 0 ||
                alloc.container().size() != static_cast<size_t>(alloc.width());
            if (containerSliceReqd) {
                out << '(' << alloc.container_slice().lo;
                if (alloc.width() > 1) out << ".." << alloc.container_slice().hi;
                out << ')';
            }
            if (alloc_num != numAllocSlices) out << ',';
        }
        if (stageAllocReqd) out << " } ";
        out << std::endl;
    }
}

void emit_phv_field(std::ostream &out, PHV::Field *field, const LiveRangeReport *lrr) {
    if (Device::currentDevice() == Device::JBAY || BFNContext::get().options().alt_phv_alloc) {
        emit_stage_phv_field(out, field, lrr);
    } else if (Device::currentDevice() == Device::TOFINO) {
        emit_stage_phv_field(out, field, lrr);
    }
}

void PhvAsmOutput::emit_gress(std::ostream &out, gress_t gress) const {
    out << "phv " << gress << ":\n";
    // FIXME -- for now, all ghost PHV are allocated as ingress, so we just
    // FIXME -- duplicate the ingress phv
    if (gress == GHOST) gress = INGRESS;
    for (auto &f : phv) {
        if (f.gress == gress) {
            emit_phv_field(out, &f, live_range_report);
        }
    }

    if (BackendOptions().debugInfo) {
        out << "  " << "context_json:\n";
        // // Collect all fields used on a per container basis with required live
        // // range info for assembly. This info is used by P4i hence only output
        // // when debug flag is set.
        if (live_range_report) {
            LiveRangePerContainer lr;
            int maxStages = live_range_report->get_max_stages();
            getLiveRanges(lr);
            for (auto cf : lr[gress]) {
                out << "    " << cf.first << ":\n";
                for (auto f : cf.second) {
                    auto fuses = f.second;
                    auto fname = f.first;
                    for (auto fuse : fuses) {
                        auto live_start = fuse.get_live_start(maxStages);
                        auto live_end = fuse.get_live_end(maxStages);
                        out << "    - { name : " << fname << ", ";
                        out << "live_start : " << live_start << ", ";
                        out << "live_end : " << live_end << ", ";
                        out << "mutually_exclusive_with: [ ";
                        std::string sep = "";
                        for (auto mf : fuse.mutex_fields) {
                            out << sep << mf;
                            if (sep == "") sep = ", ";
                        }
                        out << " ] }" << std::endl;
                    }
                }
            }
        } else {
            warning("Live range information not generated in assembly/context.json");
        }
    }
}

std::ostream &operator<<(std::ostream &out, const PhvAsmOutput &phvasm) {
    phvasm.emit_gress(out, INGRESS);
    phvasm.emit_gress(out, EGRESS);
    if (phvasm.have_ghost) phvasm.emit_gress(out, GHOST);
    return out;
}
