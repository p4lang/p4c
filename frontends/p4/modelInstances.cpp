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

#include "fromv1.0/v1model.h"

/* These must be in the same compiliation unit to ensure that P4CoreLibrary::instance
 * is initialized before V1Model::instance */
namespace P4V1 {

V1Model V1Model::instance;
const char *V1Model::versionInitial = "20180101";
const char *V1Model::versionCurrent = "20200408";

}  // namespace P4V1
