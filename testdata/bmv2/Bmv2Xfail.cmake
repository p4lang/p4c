# SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
# SPDX-FileCopyrightText: 2026 The P4 Language Consortium
# SPDX-License-Identifier: Apache-2.0

set (XFAIL_TESTS
  # This test defines two lpm keys for a table.
  # Even though the P4 spec allows it, it doesn't really make sense in a switch
  # This test uses a feature currently unsupported in the BMv2 back-end.
  testdata/bmv2/v1model/issue907-bmv2.p4
  # This test uses a table graph that is not implementable in BMv2
  testdata/bmv2/v1model/issue986-bmv2.p4
  # These tests use a table key with type 'error'
  testdata/bmv2/v1model/issue1062-bmv2.p4
  # This test uses an undefined extern
  testdata/bmv2/v1model/issue1193-bmv2.p4
  # This test uses an incorrect model
  testdata/core/issue1205-bmv2.p4
  # These psa tests are not ready to run on bmv2 yet
  testdata/shared/psa/psa-example-digest-bmv2.p4
  testdata/shared/psa/psa-example-parser-checksum.p4 # error: error.NoError: unsupported exact key expression
  testdata/shared/psa/psa-meter4.p4 # Compiler Bug: backends/bmv2/portable_common/portable.cpp:574: Null info
  testdata/shared/psa/psa-meter5.p4 # Compiler Bug: backends/bmv2/portable_common/portable.cpp:574: Null info
  testdata/shared/psa/psa-table-hit-miss.p4 # error: Program can not be implemented on this target since it
                                               # contains a path from table MyIC.tbl back to itself
  # This reads stack.next
  testdata/bmv2/v1model/issue692-bmv2.p4
  # These test use computations in the verify/update checksum controls - unsupported
  testdata/bmv2/v1model/issue1765-bmv2.p4
  testdata/bmv2/v1model/issue1765-1-bmv2.p4
  # Next three use unknown externs
  testdata/bmv2/v1model/issue1882-bmv2.p4
  testdata/bmv2/v1model/issue1882-1-bmv2.p4
  testdata/bmv2/v1model/issue2664-bmv2.p4
  # This test tries to hash (via the hash extern) a value that isn't a tuple - unsupported
  testdata/bmv2/v1model/hashing-non-tuple-bmv2.p4
  # Similarly, this test hashes on a bit input.
  testdata/bmv2/v1model/issue584-1-bmv2.p4
  # These pna tests are not ready to run on bmv2 yet
  testdata/shared/pna/pna-example-SelectByDirection.p4  # error: SelectByDirection(...): not supported
  testdata/shared/pna/pna-example-SelectByDirection1.p4  # error: SelectByDirection(...): not supported
  testdata/shared/pna/pna-example-SelectByDirection2.p4  # error: SelectByDirection(...): not supported
  testdata/shared/pna/pna-example-header-union.p4  # Error -11
  testdata/shared/pna/pna-example-header-union1.p4  # Error -11
  testdata/shared/pna/pna-example-pass-2.p4  # Conditional execution in actions not supported
  testdata/shared/pna/pna-example-tcp-connection-tracking.p4  # Conditional execution in actions not supported
  testdata/shared/pna/pna-extract-local-header.p4  # error: IfStatement: not supported within a deparser on this target
)
if (SUPPORT_P4_14)
  list(APPEND XFAIL_TESTS
    # so we allow it to fail on BMv2.
    testdata/p4_14/issue60.p4
    # compiler claims (incorrectly?) that c2 has mulitple successors, so is not supported
    testdata/p4_14/issue-1426.p4
    # This test also uses a custom extern
    testdata/p4_14/issue604.p4
  )
endif()
