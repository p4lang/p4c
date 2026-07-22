/*
 * SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
 * Copyright 2013-present Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef IR_CONFIGURATION_H_
#define IR_CONFIGURATION_H_

namespace P4 {

/// A P4CConfiguration is a set of parameters to the compiler that cannot be changed via user
/// options. Implementations should be singleton classes.
class P4CConfiguration {
 public:
    virtual ~P4CConfiguration() = default;

    /// Maximum width supported for a bit field or integer.
    virtual int maximumWidthSupported() const = 0;

    /// Maximum size for a header stack array.
    virtual int maximumArraySize() const = 0;
};

class DefaultP4CConfiguration : public P4CConfiguration {
 public:
    int maximumWidthSupported() const override { return 2048; }
    int maximumArraySize() const override { return 256; }

    /// @return the singleton instance.
    static const DefaultP4CConfiguration &get() {
        static DefaultP4CConfiguration instance;
        return instance;
    }

 protected:
    DefaultP4CConfiguration() {}
};

}  // namespace P4

#endif /* IR_CONFIGURATION_H_ */
