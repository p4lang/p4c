#!/usr/bin/env python3

# The test line is a list of input data in following forma:
# * Input BFA file to translate, relative from this folder (string)
# * BFA parameters (list)
# * Expected results (dictionary)
#   - name of the inspected match (string)
#   - results for 8 and 16 bit extractions  where the first one is for 8 bit
#     and second one for 16 bit extractions (tuple)
#

TEST_LIST = [
    ["../asm/network_tap.bfa", [], {
        "ingress,parse_inner_ipv6":         (68, 30),
        "ingress,parse_ipv6.$split_1":      (56, 18),
        "ingress,parse_inner_tcp":          (72, 34),
        "ingress,parse_volte,0x6*":         (68, 28),
        "egress,parse_gtp_base,0x01****":   (60, 36),
        "egress,parse_imsi":                (64, 38),
        "egress,parse_volte,0x6*":          (60, 36),
        "egress,parse_inner_tcp":           (64, 44),
        "egress,parse_inner_ipv6":          (64, 40),
        "egress,parse_ipv6":                (32, 18),
        "egress,parse_mpls_bos,0x6*":       (52, 26)}],
]
