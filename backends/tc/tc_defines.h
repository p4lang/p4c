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

#define DEFAULT_TABLE_ENTRIES 2048
#define DEFAULT_KEY_MASK 8
#define PORTID_BITWIDTH 32
#define DEFAULT_KEY_ID 1
#define DEFAULT_PIPELINE_ID 1
#define DEFAULT_METADATA_ID 1
#define BITWIDTH 32

// data types supported
#define BIT_TYPE 0
#define DEV_TYPE 1
#define MACADDR_TYPE 2
#define IPV4_TYPE 3
#define IPV6_TYPE 4
#define BE16_TYPE 5
#define BE32_TYPE 6
#define BE64_TYPE 7

#define PARAM_INDEX_0 0
#define PARAM_INDEX_1 1
#define PARAM_INDEX_2 2
#define PARAM_INDEX_3 3

#define SET 1
#define RESET 0

// PNA parser metadata fields
#define PARSER_RECIRCULATED 0
#define PARSER_INPUT_PORT 1

// PNA input metadata fields
#define INPUT_RECIRCULATED 0
#define INPUT_TIMESTAMP 1
#define INPUT_PARSER_ERROR 2
#define INPUT_CLASS_OF_SERVICE 3
#define INPUT_INPUT_PORT 4

// PNA output metadata fields
#define OUTPUT_CLASS_OF_SERVICE 0

// kernel metadata fields
#define UNDEFINED 0
#define UNSUPPORTED 1
#define SKBREDIR 2
#define SKBIIF 3
#define SKBTSTAMP 4
#define SKBPRIO 5

#define MAX_PNA_PARSER_META 2
#define MAX_PNA_INPUT_META 5
#define MAX_PNA_OUTPUT_META 1

#define TABLEDEFAULT 0
#define TABLEONLY 1
#define DEFAULTONLY 2

#define EXACT_TYPE 0
#define LPM_TYPE 1
#define TERNARY_TYPE 2

#endif /* BACKENDS_TC_TC_DEFINES_H_ */
