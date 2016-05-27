/* Copyright 2013-present Barefoot Networks, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * Antonin Bas (antonin@barefootnetworks.com)
 *
 */

#ifndef BM_BM_SIM_MATCH_ERROR_CODES_H_
#define BM_BM_SIM_MATCH_ERROR_CODES_H_

namespace bm {

enum class MatchErrorCode {
  SUCCESS = 0,
  TABLE_FULL,
  INVALID_HANDLE,
  EXPIRED_HANDLE,
  COUNTERS_DISABLED,
  METERS_DISABLED,
  AGEING_DISABLED,
  INVALID_TABLE_NAME,
  INVALID_ACTION_NAME,
  WRONG_TABLE_TYPE,
  INVALID_MBR_HANDLE,
  MBR_STILL_USED,
  MBR_ALREADY_IN_GRP,
  MBR_NOT_IN_GRP,
  INVALID_GRP_HANDLE,
  GRP_STILL_USED,
  EMPTY_GRP,
  DUPLICATE_ENTRY,
  BAD_MATCH_KEY,
  INVALID_METER_OPERATION,
  DEFAULT_ACTION_IS_CONST,
  DEFAULT_ENTRY_IS_CONST,
  NO_DEFAULT_ENTRY,
  ERROR,
};

}  // namespace bm

#endif  // BM_BM_SIM_MATCH_ERROR_CODES_H_
