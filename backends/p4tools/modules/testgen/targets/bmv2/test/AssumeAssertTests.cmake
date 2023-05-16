p4tools_add_test_with_args(
  P4TEST "${CMAKE_CURRENT_LIST_DIR}/p4-programs/assert_assume_tests/bmv2_testgen_assert_1.p4"
  TAG "testgen-p4c-bmv2-assert" ALIAS "bmv2_testgen_assert_1.p4" DRIVER ${P4TESTGEN_DRIVER}
  TARGET "bmv2" ARCH "v1model" USE_ASSERT_MODE DISABLE_ASSUME_MODE TEST_ARGS "-I${P4C_BINARY_DIR}/p4include --test-backend STF --print-traces --max-tests 10 "
)

p4tools_add_test_with_args(
  P4TEST "${CMAKE_CURRENT_LIST_DIR}/p4-programs/assert_assume_tests/bmv2_testgen_assert_2.p4"
  TAG "testgen-p4c-bmv2-assert" ALIAS "bmv2_testgen_assert_2.p4" DRIVER ${P4TESTGEN_DRIVER}
  TARGET "bmv2" ARCH "v1model" USE_ASSERT_MODE DISABLE_ASSUME_MODE CHECK_EMPTY TEST_ARGS "-I${P4C_BINARY_DIR}/p4include --test-backend STF --print-traces --max-tests 10 "
)

p4tools_add_test_with_args(
  P4TEST "${CMAKE_CURRENT_LIST_DIR}/p4-programs/assert_assume_tests/bmv2_testgen_assert_assume_1.p4"
  TAG "testgen-p4c-bmv2-assert" ALIAS "bmv2_testgen_assert_assume_1.p4" DRIVER ${P4TESTGEN_DRIVER}
  TARGET "bmv2" ARCH "v1model" USE_ASSERT_MODE USE_ASSUME_MODE CHECK_EMPTY TEST_ARGS "-I${P4C_BINARY_DIR}/p4include --test-backend STF --print-traces --max-tests 10 "
)

p4tools_add_test_with_args(
  P4TEST "${CMAKE_CURRENT_LIST_DIR}/p4-programs/assert_assume_tests/bmv2_testgen_assert_assume_1.p4"
  TAG "testgen-p4c-bmv2-assert" ALIAS "bmv2_testgen_assert_assume_1_neg.p4" DRIVER ${P4TESTGEN_DRIVER}
  TARGET "bmv2" ARCH "v1model" USE_ASSERT_MODE DISABLE_ASSUME_MODE TEST_ARGS "-I${P4C_BINARY_DIR}/p4include --test-backend STF --print-traces --max-tests 10 "
)

p4tools_add_test_with_args(
  P4TEST "${CMAKE_CURRENT_LIST_DIR}/p4-programs/assert_assume_tests/bmv2_testgen_assert_assume_2.p4"
  TAG "testgen-p4c-bmv2-assert" ALIAS "bmv2_testgen_assert_assume_2.p4" DRIVER ${P4TESTGEN_DRIVER}
  TARGET "bmv2" ARCH "v1model" USE_ASSERT_MODE CHECK_EMPTY TEST_ARGS "-I${P4C_BINARY_DIR}/p4include --test-backend STF --print-traces --max-tests 10 "
)

p4tools_add_test_with_args(
  P4TEST "${CMAKE_CURRENT_LIST_DIR}/p4-programs/assert_assume_tests/bmv2_testgen_assert_assume_3.p4"
  TAG "testgen-p4c-bmv2-assert" ALIAS "bmv2_testgen_assert_assume_3.p4" DRIVER ${P4TESTGEN_DRIVER}
  TARGET "bmv2" ARCH "v1model" USE_ASSERT_MODE TEST_ARGS "-I${P4C_BINARY_DIR}/p4include --test-backend STF --print-traces --max-tests 10 "
)
