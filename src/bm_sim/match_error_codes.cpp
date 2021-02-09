/* Copyright 2021 VMware, Inc.
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
 * Antonin Bas
 *
 */

#include <bm/bm_sim/_assert.h>
#include <bm/bm_sim/match_error_codes.h>

namespace bm {

const char *match_error_code_to_string(MatchErrorCode code) {
  switch (code) {
    case MatchErrorCode::SUCCESS:
      return "SUCCESS";
    case MatchErrorCode::TABLE_FULL:
      return "TABLE_FULL";
    case MatchErrorCode::INVALID_HANDLE:
      return "INVALID_HANDLE";
    case MatchErrorCode::EXPIRED_HANDLE:
      return "EXPIRED_HANDLE";
    case MatchErrorCode::COUNTERS_DISABLED:
      return "COUNTERS_DISABLED";
    case MatchErrorCode::METERS_DISABLED:
      return "METERS_DISABLED";
    case MatchErrorCode::AGEING_DISABLED:
      return "AGEING_DISABLED";
    case MatchErrorCode::INVALID_TABLE_NAME:
      return "INVALID_TABLE_NAME";
    case MatchErrorCode::INVALID_ACTION_NAME:
      return "INVALID_ACTION_NAME";
    case MatchErrorCode::WRONG_TABLE_TYPE:
      return "WRONG_TABLE_TYPE";
    case MatchErrorCode::INVALID_MBR_HANDLE:
      return "INVALID_MBR_HANDLE";
    case MatchErrorCode::MBR_STILL_USED:
      return "MBR_STILL_USED";
    case MatchErrorCode::MBR_ALREADY_IN_GRP:
      return "MBR_ALREADY_IN_GRP";
    case MatchErrorCode::MBR_NOT_IN_GRP:
      return "MBR_NOT_IN_GRP";
    case MatchErrorCode::INVALID_GRP_HANDLE:
      return "INVALID_GRP_HANDLE";
    case MatchErrorCode::GRP_STILL_USED:
      return "GRP_STILL_USED";
    case MatchErrorCode::EMPTY_GRP:
      return "EMPTY_GRP";
    case MatchErrorCode::DUPLICATE_ENTRY:
      return "DUPLICATE_ENTRY";
    case MatchErrorCode::BAD_MATCH_KEY:
      return "BAD_MATCH_KEY";
    case MatchErrorCode::INVALID_METER_OPERATION:
      return "INVALID_METER_OPERATION";
    case MatchErrorCode::DEFAULT_ACTION_IS_CONST:
      return "DEFAULT_ACTION_IS_CONST";
    case MatchErrorCode::DEFAULT_ENTRY_IS_CONST:
      return "DEFAULT_ENTRY_IS_CONST";
    case MatchErrorCode::NO_DEFAULT_ENTRY:
      return "NO_DEFAULT_ENTRY";
    case MatchErrorCode::INVALID_ACTION_PROFILE_NAME:
      return "INVALID_ACTION_PROFILE_NAME";
    case MatchErrorCode::NO_ACTION_PROFILE_SELECTION:
      return "NO_ACTION_PROFILE_SELECTION";
    case MatchErrorCode::IMMUTABLE_TABLE_ENTRIES:
      return "IMMUTABLE_TABLE_ENTRIES";
    case MatchErrorCode::BAD_ACTION_DATA:
      return "BAD_ACTION_DATA";
    case MatchErrorCode::ERROR:
      return "UNKNOWN_ERROR";
  }
  _BM_UNREACHABLE("Enum value not handled in switch statement");
}

}  // namespace bm
