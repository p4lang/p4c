/*
 * Copyright 2024 Intel Corp.
 * SPDX-FileCopyrightText: 2024 Intel Corp.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

enum HashAlgorithm_t {
    IDENTITY,
    RANDOM
}

@pure extern HashAlgorithm_t identity_hash(bool msb, bool extend);
@pure extern HashAlgorithm_t random_hash(bool msb, bool extend);

extern Hash<W> {
    /// Constructor
    /// @type_param W : width of the calculated hash.
    /// @param algo : The default algorithm used for hash calculation.
    Hash(HashAlgorithm_t algo);

    /// Compute the hash for the given data.
    /// @param data : The list of fields contributing to the hash.
    /// @return The hash value.
    W get<D>(in D data);
}

header h1_t {
    bit<32>     f1;
    bit<32>     f2;
    bit<32>     f3;
}

struct hdrs {
    h1_t        h1;
    bit<16>     hash;
}

control test(inout hdrs hdr) {
    apply {
        hdr.hash = Hash<bit<16>>(random_hash(false, false)).get(hdr.h1);
    }
}
