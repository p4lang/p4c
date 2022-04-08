### Added by Abe starting April 6 2022; these tests are all expected to fail until Abe`s branch with driver improvements is merged.

set(XFAIL driver_inputs_test_1
          driver_inputs_test_2
          driver_inputs_test_3
          driver_inputs_test_4)

set_tests_properties(driver_inputs_test_1 PROPERTIES LABELS "XFAIL")
set_tests_properties(driver_inputs_test_2 PROPERTIES LABELS "XFAIL")
set_tests_properties(driver_inputs_test_3 PROPERTIES LABELS "XFAIL")
set_tests_properties(driver_inputs_test_4 PROPERTIES LABELS "XFAIL")

p4c_add_xfail_reason("driver" "fail" driver_inputs_test_1)
p4c_add_xfail_reason("driver" "fail" driver_inputs_test_2)
p4c_add_xfail_reason("driver" "fail" driver_inputs_test_3)
p4c_add_xfail_reason("driver" "fail" driver_inputs_test_4)
