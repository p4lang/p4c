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

#include "remove_act_tables.h"

bool AnalyzeActionTables::preorder(IR::P4Control *control) { return false; }

bool AnalyzeActionTables::preorder(const IR::MAU::Table *t) { return false; }

bool AnalyzeActionTables::preorder(IR::P4Action *action) { return false; }

const IR::Node *DoRemoveActionTables::postorder(const IR::MAU::Table *t) { return t; }
