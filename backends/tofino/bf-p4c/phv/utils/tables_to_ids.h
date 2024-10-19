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

#ifndef BACKENDS_TOFINO_BF_P4C_PHV_UTILS_TABLES_TO_IDS_H_
#define BACKENDS_TOFINO_BF_P4C_PHV_UTILS_TABLES_TO_IDS_H_

class MapTablesToIDs : public Inspector {
 private:
    ordered_map<const IR::MAU::Table *, unsigned> tableToIDs;
    ordered_map<unsigned, const IR::MAU::Table *> idToTables;
    unsigned lastID = 0;

    profile_t init_apply(const IR::Node *root) override {
        tableToIDs.clear();
        idToTables.clear();
        lastID = 0;
        return Inspector::init_apply(root);
    }

    bool preorder(const IR::MAU::Table *tbl) override {
        BUG_CHECK(tableToIDs.count(tbl) == 0, "Table %1% already assigned ID %2%", tbl->name,
                  tableToIDs.at(tbl));
        BUG_CHECK(idToTables.count(lastID) == 0, "ID %1% already assigned to table %2%", lastID,
                  idToTables.at(lastID));
        tableToIDs[tbl] = lastID;
        idToTables[lastID] = tbl;
        ++lastID;
        return true;
    }

 public:
    const ordered_map<const IR::MAU::Table *, unsigned> &getTableToIDs() const {
        return tableToIDs;
    }

    const ordered_map<unsigned, const IR::MAU::Table *> &getIDsToTables() const {
        return idToTables;
    }

    unsigned getIDForTable(const IR::MAU::Table *tbl) const {
        BUG_CHECK(tableToIDs.count(tbl), "No ID assigned to table %1%", tbl->name);
        return tableToIDs.at(tbl);
    }

    const IR::MAU::Table *getTableForID(unsigned id) const {
        BUG_CHECK(idToTables.count(id), "ID %1% not assigned to any table", id);
        return idToTables.at(id);
    }
};

#endif /* BACKENDS_TOFINO_BF_P4C_PHV_UTILS_TABLES_TO_IDS_H_ */
