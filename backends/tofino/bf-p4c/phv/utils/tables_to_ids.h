/**
 * Copyright (C) 2024 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License.  You may obtain a copy
 * of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed
 * under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations under the License.
 *
 *
 * SPDX-License-Identifier: Apache-2.0
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
