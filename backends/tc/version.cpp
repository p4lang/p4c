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
#include "version.h"

// Return the version string
std::string version_string() {
    std::string version = std::to_string(MAJORVERSION) + "." + std::to_string(MINORVERSION) + ".";
    version += std::to_string(BUILDVERSION) + "." + std::to_string(FIXVERSION);
    return version;
}

unsigned int version_num() {
    return (((MAJORVERSION) << 24) + ((MINORVERSION) << 16) + ((BUILDVERSION) << 8) + (FIXVERSION));
}
