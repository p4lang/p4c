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

/// A P4CConfiguration is a set of parameters to the compiler that cannot be changed via user
/// options. Implementations should be singleton classes.
class P4CConfiguration {
 public:
    /// Maximum width supported for a bit field or integer.
    virtual int maximumWidthSupported() const = 0;

    /// Maximum size for a header stack array.
    virtual int maximumArraySize() const = 0;
};

class DefaultP4CConfiguration : public P4CConfiguration {
 public:
    int maximumWidthSupported() const { return 2048; }
    int maximumArraySize() const { return 256; }

    /// @return the singleton instance.
    static const DefaultP4CConfiguration &get() {
        static DefaultP4CConfiguration instance;
        return instance;
    }

 protected:
    DefaultP4CConfiguration() {}
};

#endif /* IR_CONFIGURATION_H_ */
