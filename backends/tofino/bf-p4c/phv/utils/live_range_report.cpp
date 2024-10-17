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

#include "bf-p4c/phv/analysis/meta_live_range.h"
#include "bf-p4c/common/table_printer.h"
#include "bf-p4c/phv/utils/live_range_report.h"

std::map<int, PHV::FieldUse> LiveRangeReport::processUseDefSet(
        const FieldDefUse::LocPairSet& defuseSet,
        PHV::FieldUse usedef) const {
    const int DEPARSER = maxStages;
    const int PARSER = -1;
    std::map<int, PHV::FieldUse> fieldMap;
    for (const FieldDefUse::locpair use : defuseSet) {
        const IR::BFN::Unit* use_unit = use.first;
        if (use_unit->is<IR::BFN::ParserState>() || use_unit->is<IR::BFN::Parser>() ||
            use_unit->is<IR::BFN::GhostParser>()) {
            auto* ps = use_unit->to<IR::BFN::ParserState>();
            cstring use_location;
            if (!ps) {
                use_location = " to parser"_cs;
            } else {
                use_location = " to parser state " + ps->name;
                // Ignore initialization in parser.
                if (use.second->is<ImplicitParserInit>()) {
                    LOG4("\t  Ignoring initialization in " << use_location);
                    continue;
                }
            }
            fieldMap[PARSER] |= usedef;
            LOG4("\tAssign " << usedef << use_location);
        } else if (use_unit->is<IR::BFN::Deparser>()) {
            fieldMap[DEPARSER] |= usedef;
            LOG4("\tAssign " << usedef << " to deparser");
        } else if (use_unit->is<IR::MAU::Table>()) {
            const auto* t = use_unit->to<IR::MAU::Table>();
            LOG4("\tTable " << t->name);
            auto stages = alloc.stages(t);
            for (auto stage : stages) {
                fieldMap[stage] |= usedef;
                LOG4("\tAssign " << usedef << " to stage " << stage);
            }
            if (t->is_always_run_action()) {
                fieldMap[t->stage()] |= usedef;
                LOG4("\tAssign " << usedef << " to stage " << t->stage());
            }
        } else {
            if (!use_unit->to<IR::BFN::GhostParser>())
                BUG("Unknown unit encountered %1%", use_unit->toString());
        }
    }
    return fieldMap;
}

void LiveRangeReport::setFieldLiveMap(const PHV::Field* f) {
    auto usemap = processUseDefSet(defuse.getAllUses(f->id), PHV::FieldUse(READ));
    auto defmap = processUseDefSet(defuse.getAllDefs(f->id), PHV::FieldUse(WRITE));

    auto update_usemap = [&]() {
        int min = maxStages + 1;
        int max = -2;
        for (auto kv : usemap) {
            min = std::min(kv.first, min);
            max = std::max(kv.first, max);
        }
        if (min != maxStages + 1 && max != -2) {
            LOG4("Min-max for " << f->name << " : [" << min << ", " << max << "]");
            for (int i = min; i <= max; i++)
                usemap[i] |= PHV::FieldUse(LIVE);
        }
    };

    auto update_livemap = [&](const PHV::Field *a) {
        if (!f) return;
        if (livemap.count(a)) {
            auto &fuse = livemap[a];
            usemap.insert(fuse.begin(), fuse.end());
        }
        update_usemap();
        livemap[a] = usemap;
    };

    // Combine the maps into a single map.
    for (auto kv : defmap)
        usemap[kv.first] |= kv.second;

    // If alias present, update usemap and alias field in livemap
    if (f->aliasSource) {
        auto a = f->aliasSource;
        update_livemap(a);
        aliases[a] = f;
    }
    // If field present in alias map, update usemap and the field in livemap
    // This is to ensure field is always updated and is agnostic to the
    // traversal order
    if (aliases.count(f)) {
        update_livemap(aliases[f]);
    }

    // For no alias just update the usemap
    if (!f->aliasSource && aliases.count(f) == 0)
        update_usemap();

    livemap[f] = usemap;
}

cstring LiveRangeReport::printFieldLiveness() {
    std::stringstream ss;
    ss << std::endl << "Live Ranges for PHV Fields:" << std::endl;
    std::vector<std::string> headers;
    headers.push_back("Field");
    headers.push_back("Bit Size");
    headers.push_back("P");
    for (int i = 0; i < maxStages; i++)
        headers.push_back(std::to_string(i));
    headers.push_back("D");
    TablePrinter tp(ss, headers, TablePrinter::Align::LEFT);
    for (auto kv : livemap) {
        std::vector<std::string> row;
        // Add field name.
        row.push_back(std::string(kv.first->name));
        row.push_back(std::to_string(kv.first->size));
        for (int i = -1; i <= maxStages; i++) {
            if (kv.second.count(i)) {
                PHV::FieldUse use_type(kv.second.at(i));
                row.push_back(std::string(use_type.toString()));
                // Skip aliased fields for bit calculation
                if (aliases.count(kv.first)) continue;
                if (kv.second.at(i).isRead())
                    stageToReadBits[i] += kv.first->size;
                if (kv.second.at(i).isWrite())
                    stageToWriteBits[i] += kv.first->size;
                if (kv.second.at(i).isLive())
                    stageToLiveBits[i] += kv.first->size;
            } else {
                row.push_back("");
            }
        }
        tp.addRow(row);
    }
    tp.print();
    return ss.str();
}

std::vector<std::string> LiveRangeReport::createStatRow(
        std::string title,
        const std::map<int, int>& data) const {
    std::vector<std::string> rv;
    rv.push_back(title);
    for (int i = -1; i <= maxStages; i++) {
        if (data.count(i))
            rv.push_back(std::to_string(data.at(i)));
        else
            rv.push_back(std::to_string(0));
    }
    return rv;
}

cstring LiveRangeReport::printBitStats() const {
    std::stringstream ss;
    ss << std::endl << "Number of bits per stage:" << std::endl;
    std::vector<std::string> headers;
    headers.push_back("Statistic");
    headers.push_back("P");
    for (int i = 0; i < maxStages; i++)
        headers.push_back(std::to_string(i));
    headers.push_back("D");
    TablePrinter tp(ss, headers, TablePrinter::Align::LEFT);
    tp.addRow(createStatRow("Bits Read", stageToReadBits));
    tp.addRow(createStatRow("Bits Written", stageToWriteBits));
    tp.addRow(createStatRow("Bits Live", stageToLiveBits));
    tp.print();
    return ss.str();
}

cstring LiveRangeReport::printAliases() const {
    std::stringstream ss;
    ss << std::endl << "Field Aliases:" << std::endl;
    std::vector<std::string> headers;
    headers.push_back("Field");
    headers.push_back("Alias");
    TablePrinter tp(ss, headers, TablePrinter::Align::LEFT);
    for (auto a : aliases) {
        std::vector<std::string> row;
        row.push_back(a.first->name.c_str());
        row.push_back(a.second->name.c_str());
        tp.addRow(row);
    }
    tp.print();
    return ss.str();
}

Visitor::profile_t LiveRangeReport::init_apply(const IR::Node* root) {
    stageToReadBits.clear();
    stageToWriteBits.clear();
    stageToAccessBits.clear();
    stageToLiveBits.clear();
    livemap.clear();
    aliases.clear();

    int maxStagesInAlloc = alloc.maxStages();
    int maxDeviceStages = Device::numStages();
    maxStages = (maxStagesInAlloc > maxDeviceStages) ? maxStagesInAlloc : maxDeviceStages;
    for (const PHV::Field& f : phv) {
        bool only_tphv_allocation = true;
        f.foreach_alloc([&](const PHV::AllocSlice& slice) {
            if (!slice.container().is(PHV::Kind::tagalong))
                only_tphv_allocation = false;
        });
        if (only_tphv_allocation) continue;
        setFieldLiveMap(&f);
    }
    LOG1(printFieldLiveness());
    LOG1(printBitStats());
    LOG1(printAliases());
    return Inspector::init_apply(root);
}
