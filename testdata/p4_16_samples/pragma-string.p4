/*
Copyright 2013-present Barefoot Networks, Inc.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

/* P4TEST_IGNORE_STDERR */

@pragma name "original"
const bit b = 1;

@pragma name "string \" with \" quotes"
const bit c = 1;

@pragma name "string with
newline"
const bit d = 1;

@pragma name "string with quoted \
newline"
const bit e = 1;

@pragma name "8-bit string ⟶"
const bit f = 1;
