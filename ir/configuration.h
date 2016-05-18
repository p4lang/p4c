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

#ifndef IR_CONFIGURATION_H_
#define IR_CONFIGURATION_H_

namespace P4CConfiguration {

// Maximum width supported for a bit-field or integer
const int MaximumWidthSupported = 2048;
// Maximum size for a header stack array
const int MaximumArraySize = 256;

}

#endif /* IR_CONFIGURATION_H_ */
