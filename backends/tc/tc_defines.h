/*
Copyright (C) 2023 Intel Corporation

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing,
software distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions
and limitations under the License.
*/

#ifndef BACKENDS_TC_TC_DEFINES_H_
#define BACKENDS_TC_TC_DEFINES_H_

namespace TC {

inline constexpr auto DEFAULT_TABLE_ENTRIES = 2048;
inline constexpr auto DEFAULT_KEY_MASK = 8;
inline constexpr auto PORTID_BITWIDTH = 32;
inline constexpr auto DEFAULT_KEY_ID = 1;
inline constexpr auto DEFAULT_PIPELINE_ID = 1;
inline constexpr auto DEFAULT_METADATA_ID = 1;
inline constexpr auto BITWIDTH = 32;

// Supported data types.
inline constexpr auto BIT_TYPE = 0;
inline constexpr auto DEV_TYPE = 1;
inline constexpr auto MACADDR_TYPE = 2;
inline constexpr auto IPV4_TYPE = 3;
inline constexpr auto IPV6_TYPE = 4;
inline constexpr auto BE16_TYPE = 5;
inline constexpr auto BE32_TYPE = 6;
inline constexpr auto BE64_TYPE = 7;

inline constexpr auto PARAM_INDEX_0 = 0;
inline constexpr auto PARAM_INDEX_1 = 1;
inline constexpr auto PARAM_INDEX_2 = 2;
inline constexpr auto PARAM_INDEX_3 = 3;

inline constexpr auto SET = 1;
inline constexpr auto RESET = 0;

// PNA parser metadata fields.
inline constexpr auto PARSER_RECIRCULATED = 0;
inline constexpr auto PARSER_INPUT_PORT = 1;

// PNA input metadata fields.
inline constexpr auto INPUT_RECIRCULATED = 0;
inline constexpr auto INPUT_TIMESTAMP = 1;
inline constexpr auto INPUT_PARSER_ERROR = 2;
inline constexpr auto INPUT_CLASS_OF_SERVICE = 3;
inline constexpr auto INPUT_INPUT_PORT = 4;

// PNA output metadata fields.
inline constexpr auto OUTPUT_CLASS_OF_SERVICE = 0;

// Kernel metadata fields.
inline constexpr auto UNDEFINED = 0;
inline constexpr auto UNSUPPORTED = 1;
inline constexpr auto SKBREDIR = 2;
inline constexpr auto SKBIIF = 3;
inline constexpr auto SKBTSTAMP = 4;
inline constexpr auto SKBPRIO = 5;

inline constexpr auto MAX_PNA_PARSER_META = 2;
inline constexpr auto MAX_PNA_INPUT_META = 5;
inline constexpr auto MAX_PNA_OUTPUT_META = 1;

inline constexpr auto TABLEDEFAULT = 0;
inline constexpr auto TABLEONLY = 1;
inline constexpr auto DEFAULTONLY = 2;

inline constexpr auto EXACT_TYPE = 0;
inline constexpr auto LPM_TYPE = 1;
inline constexpr auto TERNARY_TYPE = 2;
}  // namespace TC

#endif /* BACKENDS_TC_TC_DEFINES_H_ */
