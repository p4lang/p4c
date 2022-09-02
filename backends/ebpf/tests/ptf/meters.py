#!/usr/bin/env python
# Copyright 2022-present Orange
# Copyright 2022-present Open Networking Foundation
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
from common import *
import math

PORT0 = 0
PORT1 = 1
PORT2 = 2
ALL_PORTS = [PORT0, PORT1, PORT2]


def get_meter_value_mask(with_spin_lock=True, add_hex_prefix=True):
    """
    This function is used to create a meter_value mask
    that hides last three meter_value fields:
    two timestamps and spin lock field.
    """
    full_value = "ffffffffffffffff"
    zero_value = "0000000000000000"
    mask = full_value  # pir_period

    # pir_unit_per_period, cir_period, cir_unit_per_period
    # pbs, cbs, pbs_left, cbs_left
    for i in range(0, 7):
        mask = mask + full_value

    # time_p, time_c, spin lock (optionally)
    for i in range(0, 3 if with_spin_lock else 2):
        mask = mask + zero_value

    return "0x" + mask if add_hex_prefix else mask


def convert_dec_to_hex_string(decimal_value):
    """
    This function converts a decimal value into 8-byte hex string
    and reverses the order of bytes.
    """
    hex_str = '{:016x}'.format(decimal_value)
    reversed_hex = []
    index = len(hex_str)
    while index > 0:
        reversed_hex += hex_str[index - 2:index].capitalize() + ' '
        index = index - 2
    return ''.join(reversed_hex)


def convert_rate(rate):
    """
    This function converts provided pir or cir rate (byte/s or packet/s) into
    period and unit_per_period.
    These values are stored in the Meter state.
    """
    NS_IN_S = 1e9
    METER_PERIOD_MIN = 100

    if rate == 0:
        unit_per_period = 0
        period = 0
        return int(period), int(unit_per_period)

    period = NS_IN_S / rate

    if period >= METER_PERIOD_MIN:
        unit_per_period = 1
    else:
        unit_per_period = math.ceil(METER_PERIOD_MIN / period)
        period = (NS_IN_S * unit_per_period) / rate

    return int(period), int(unit_per_period)


def build_meter_value(pir, cir,
                      pbs, cbs,
                      pbs_left, cbs_left, add_spin_lock=True):
    """
    This function builds a hex string with the values that are stored in the Meter state.
    :param pir: peak information rate in byte/s or packet/s
    :param cir: committed information rate in byte/s or packet/s
    :param pbs: peak bucket size in bytes or packets
    :param cbs: committed bucket size in bytes or packets
    :param pbs_left: peak bucket size left in bytes or packets
    :param cbs_left: committed bucket size left in bytes or packets
    :param add_spin_lock: indicates if add spin lock field at the end of meter value.
    The spin lock field is always placed at the end of meter entry or table entry with direct meter
    :return:
    """
    pir_period, pir_unit_per_period = convert_rate(pir)
    cir_period, cir_unit_per_period = convert_rate(cir)

    meter_value = [convert_dec_to_hex_string(pir_period),
                   convert_dec_to_hex_string(pir_unit_per_period),
                   convert_dec_to_hex_string(cir_period),
                   convert_dec_to_hex_string(cir_unit_per_period),
                   convert_dec_to_hex_string(pbs),
                   convert_dec_to_hex_string(cbs),
                   convert_dec_to_hex_string(pbs_left),
                   convert_dec_to_hex_string(cbs_left),
                   # Last three fields are not being checked
                   convert_dec_to_hex_string(0),
                   convert_dec_to_hex_string(0)]

    if add_spin_lock:
        meter_value.append(convert_dec_to_hex_string(0))

    return ''.join(meter_value)


class MeterPSATest(P4EbpfTest):
    """
    Test Meter used in control block. Type BYTES.
    Send 100 B packet and verify that the number of tokes decreases by 100.
    """

    p4_file_path = "p4testdata/meters.p4"

    def runTest(self):
        pkt = testutils.simple_ip_packet()
        # cir, pir -> 2 Mb/s -> 250000 byte/s, cbs, pbs -> bs (10 ms) -> 2500 B -> 09 C4
        # period 4000 ns -> 0F A0, 1 B per period -> 01
        self.meter_update(name="ingress_meter1", index=0,
                          pir=250000, pbs=2500, cir=250000, cbs=2500)
        testutils.send_packet(self, PORT0, pkt)
        testutils.verify_packet(self, pkt, PORT1)

        # Expecting pbs_left, cbs_left 2500 B - 100 B = 2400 B -> 09 60
        meter_value = build_meter_value(pir=250000, cir=250000, pbs=2500,
                                        pbs_left=2400, cbs=2500, cbs_left=2400)
        self.verify_map_entry(name="ingress_meter1", key="hex 00",
                              expected_value=meter_value, mask=get_meter_value_mask())


class MeterColorAwarePSATest(P4EbpfTest):
    """
    Test color-aware Meter used in control block. Type BYTES. Pre coloured with YELLOW.
    Send 100 B packet and verify that the number of tokes decreases by 100 in a Peak bucket only.
    """

    p4_file_path = "p4testdata/meters-color-aware.p4"

    def runTest(self):
        pkt = testutils.simple_ip_packet()
        # cir, pir -> 2 Mb/s -> 250000 byte/s, cbs, pbs -> bs (10 ms) -> 2500 B
        self.meter_update(name="ingress_meter1", index=0,
                          pir=250000, pbs=2500, cir=250000, cbs=2500)
        testutils.send_packet(self, PORT0, pkt)
        testutils.verify_packet(self, pkt, PORT1)
        # Expecting pbs_left, 2500 - 100 B = 2400 B, cbs_left stays 2500 B
        meter_value = build_meter_value(pir=250000, cir=250000, pbs=2500,
                                        pbs_left=2400, cbs=2500, cbs_left=2500)
        self.verify_map_entry(name="ingress_meter1", key="hex 00",
                              expected_value=meter_value,
                              mask=get_meter_value_mask())


class MeterActionPSATest(P4EbpfTest):
    """
    Test Meter used in action. Type BYTES.
    Send 100 B packet and verify that the number of tokes decreases by 100.
    """

    p4_file_path = "p4testdata/meters-action.p4"

    def runTest(self):
        pkt = testutils.simple_ip_packet()

        # cir, pir -> 10 Mb/s -> 1,25 MB/s, cbs, pbs -> bs (10 ms) -> 6250 B
        self.meter_update(name="ingress_meter1", index=0,
                          pir=1250000, pbs=6250, cir=1250000, cbs=6250)
        self.table_add(table="ingress_tbl_fwd", key=[4], action=1, data=[5])

        testutils.send_packet(self, PORT0, pkt)
        testutils.verify_packet(self, pkt, PORT1)
        # Expecting pbs_left, cbs_left 6250 B - 100 B = 6150 B
        meter_value = build_meter_value(pir=1250000, cir=1250000, pbs=6250,
                                        pbs_left=6150, cbs=6250, cbs_left=6150)
        self.verify_map_entry(name="ingress_meter1", key="hex 00",
                              expected_value=meter_value,
                              mask=get_meter_value_mask())


class MeterPacketsPSATest(P4EbpfTest):
    """
    Test Meter used in control block. Type PACKETS.
    Send 1 packet and verify that the number of tokes decreases by 1.
    """

    p4_file_path = "p4testdata/meters-packets.p4"

    def runTest(self):
        pkt = testutils.simple_ip_packet()
        # cir, pir -> 100 packets/s, bs -> 10 packets
        self.meter_update(name="ingress_meter1", index=0,
                          pir=100, pbs=10, cir=100, cbs=10)
        testutils.send_packet(self, PORT0, pkt)
        testutils.verify_packet(self, pkt, PORT1)
        # Expecting pbs_left, cbs_left 10 - 1 = 9
        meter_value = build_meter_value(pir=100, cir=100, pbs=10,
                                        pbs_left=9, cbs=10, cbs_left=9)
        self.verify_map_entry(name="ingress_meter1", key="hex 00",
                              expected_value=meter_value,
                              mask=get_meter_value_mask())


class DirectMeterPSATest(P4EbpfTest):
    """
    Test Direct Meter. Type BYTES.
    Send 100 B packet and verify that the number of tokes decreases by 100.
    """

    p4_file_path = "p4testdata/meters-direct.p4"

    def runTest(self):
        pkt = testutils.simple_ip_packet()

        # cir, pir -> 10 Mb/s, cbs, pbs -> bs (10 ms) -> 6250 B
        self.table_add(table="ingress_tbl_fwd", key=[4], action=1, data="5",
                       meters={"ingress_meter1": {"pir": 1250000, "pbs": 6250, "cir": 1250000, "cbs": 6250}})

        testutils.send_packet(self, PORT0, pkt)
        testutils.verify_packet(self, pkt, PORT1)
        # Expecting pbs_left, cbs_left 6250 B - 100 B = 6150 B
        meter_value = build_meter_value(pir=1250000, cir=1250000, pbs=6250,
                                        pbs_left=6150, cbs=6250, cbs_left=6150)
        expected_value = "hex 01 00 00 00 05 00 00 00 " + meter_value
        self.verify_map_entry(name="ingress_tbl_fwd", key="hex 04 00 00 00",
                              expected_value=expected_value,
                              mask=get_meter_value_mask())


class DirectMeterColorAwarePSATest(P4EbpfTest):
    """
    Test color-aware Direct Meter. Type BYTES. Pre coloured with YELLOW.
    Send 100 B packet and verify that the number of tokes decreases by 100 in a Peak bucket only.
    """

    p4_file_path = "p4testdata/meters-direct-color-aware.p4"

    def runTest(self):
        pkt = testutils.simple_ip_packet()

        # cir, pir -> 10 Mb/s, cbs, pbs -> bs (10 ms) -> 6250 B
        self.table_add(table="ingress_tbl_fwd", key=[4], action=1, data="5",
                       meters={"ingress_meter1": {"pir": 1250000, "pbs": 6250, "cir": 1250000, "cbs": 6250}})

        testutils.send_packet(self, PORT0, pkt)
        testutils.verify_packet(self, pkt, PORT1)
        # Expecting pbs_left 6250 B - 100 B = 6150 B, cbs_left unchanged 6250 B
        meter_value = build_meter_value(pir=1250000, cir=1250000, pbs=6250,
                                        pbs_left=6150, cbs=6250, cbs_left=6250)
        expected_value = "hex 01 00 00 00 05 00 00 00 " + meter_value
        self.verify_map_entry(name="ingress_tbl_fwd", key="hex 04 00 00 00",
                              expected_value=expected_value,
                              mask=get_meter_value_mask())


class DirectAndIndirectMeterPSATest(P4EbpfTest):
    """
    Test Direct Meter and Indirect Meter together. Type BYTES.
    Send 100 B packet and verify that the number of tokes decreases by 100.
    """

    p4_file_path = "p4testdata/meters-direct-and-indirect.p4"

    def runTest(self):
        pkt = testutils.simple_ip_packet()

        # cir, pir -> 10 Mb/s, cbs, pbs -> bs (10 ms) -> 6250 B
        self.table_add(table="ingress_tbl_fwd", key=[4], action=1, data="5",
                       meters={"ingress_direct_meter": {"pir": 1250000, "pbs": 6250, "cir": 1250000, "cbs": 6250}})

        # cir, pir -> 10 Mb/s, cbs, pbs -> bs (10 ms) -> 6250 B
        self.meter_update(name="ingress_indirect_meter", index=0, pir=1250000, pbs=6250, cir=1250000, cbs=6250)

        testutils.send_packet(self, PORT0, pkt)
        testutils.verify_packet(self, pkt, PORT1)
        # Expecting pbs_left, cbs_left 6250 B - 100 B = 6150 B
        meter_value = build_meter_value(pir=1250000, cir=1250000, pbs=6250,
                                        pbs_left=6150, cbs=6250, cbs_left=6150)
        # action id | egress_port | meter_value
        expected_value = "hex 01 00 00 00 05 00 00 00 " + meter_value
        self.verify_map_entry(name="ingress_tbl_fwd", key="hex 04 00 00 00",
                              expected_value=expected_value,
                              mask=get_meter_value_mask())

        # Expecting pbs_left, cbs_left 6250 B - 100 B = 6150 B
        meter_value = build_meter_value(pir=1250000, cir=1250000, pbs=6250,
                                        pbs_left=6150, cbs=6250, cbs_left=6150)
        expected_value = "hex " + meter_value
        self.verify_map_entry(name="ingress_indirect_meter", key="hex 00",
                              expected_value=expected_value,
                              mask=get_meter_value_mask())


class DirectAndIndirectActionMeterPSATest(DirectAndIndirectMeterPSATest):
    """
    Test Direct Meter and Indirect Meter together
    and execute Indirect Meter in single action.
    """

    p4_file_path = "p4testdata/meters-direct-and-indirect-single-action.p4"


class DirectTwoMetersPSATest(P4EbpfTest):
    """
    Test two Direct Meters in one table. Type BYTES.
    Send 100 B packet and verify that the number of tokes decreases by 100.
    """

    p4_file_path = "p4testdata/meters-two-direct.p4"

    def runTest(self):
        pkt = testutils.simple_ip_packet()

        # cir, pir -> 10 Mb/s, cbs, pbs -> bs (10 ms) -> 6250 B
        self.table_add(table="ingress_tbl_fwd", key=[4], action=1, data=[5],
                       meters={"ingress_meter1": {"pir": 1250000, "pbs": 6250, "cir": 1250000, "cbs": 6250},
                               "ingress_meter2": {"pir": 1250000, "pbs": 6250, "cir": 1250000, "cbs": 6250}})

        testutils.send_packet(self, PORT0, pkt)
        testutils.verify_packet(self, pkt, PORT1)
        # Expecting pbs_left, cbs_left 6250 B - 100 B = 6150 B
        meter_1_value = build_meter_value(pir=1250000, cir=1250000, pbs=6250,
                                          pbs_left=6150, cbs=6250, cbs_left=6150,
                                          add_spin_lock=False)
        meter_2_value = build_meter_value(pir=1250000, cir=1250000, pbs=6250,
                                          pbs_left=6150, cbs=6250, cbs_left=6150,
                                          add_spin_lock=True)
        expected_value = "hex 01 00 00 00 05 00 00 00 " + meter_1_value + meter_2_value
        self.verify_map_entry(name="ingress_tbl_fwd", key="hex 04 00 00 00",
                              expected_value=expected_value,
                              mask=get_meter_value_mask(with_spin_lock=False) +
                                   get_meter_value_mask(add_hex_prefix=False))


class DirectAndCounterMeterPSATest(P4EbpfTest):
    """
    Test Direct Meter with Direct Counter.
    Send 100 B packet and verify that the number of tokes decreases by 100.
    """

    p4_file_path = "p4testdata/meters-direct-and-counter.p4"

    def runTest(self):
        pkt = testutils.simple_ip_packet()

        # cir, pir -> 10 Mb/s, cbs, pbs -> bs (10 ms) -> 6250 B
        self.table_add(table="ingress_tbl_fwd", key=[4], action=1, data=[5],
                       counters={"ingress_counter1": {"packets": 1}},
                       meters={"ingress_meter1": {"pir": 1250000, "pbs": 6250, "cir": 1250000, "cbs": 6250}})

        testutils.send_packet(self, PORT0, pkt)
        testutils.verify_packet(self, pkt, PORT1)
        # Expecting pbs_left, cbs_left 6250 B - 100 B = 6150 B
        meter_value = build_meter_value(pir=1250000, cir=1250000, pbs=6250,
                                        pbs_left=6150, cbs=6250, cbs_left=6150)
        # action id | egress_port | counter packets and padding | meter_value
        expected_value = "hex 01 00 00 00 05 00 00 00 02 00 00 00 00 00 00 00 " + meter_value
        self.verify_map_entry(name="ingress_tbl_fwd", key="hex 04 00 00 00",
                              expected_value=expected_value,
                              mask=get_meter_value_mask())
