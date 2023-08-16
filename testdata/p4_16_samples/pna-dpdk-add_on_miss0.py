#!/usr/bin/env python3

# Copyright 2023 Intel Corporation
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# Andy Fingerhut, andy.fingerhut@gmail.com

import logging
import pprint
import queue
import time

import ptf
import ptf.testutils as tu
from ptf.base_tests import BaseTest
import p4runtime_sh.shell as sh
import p4runtime_sh.utils as shutils
import p4runtime_sh.p4runtime as p4rt


# Bsed on https://github.com/jafingerhut/p4-guide/blob/master/ipdk/23.01/add_on_miss0/ptf-tests/ptf-test1.py


logger = logging.getLogger(None)
ch = logging.StreamHandler()
ch.setLevel(logging.INFO)
# create formatter and add it to the handlers
formatter = logging.Formatter('%(asctime)s - %(name)s - %(levelname)s - %(message)s')
ch.setFormatter(formatter)
logger.addHandler(ch)

pp = pprint.PrettyPrinter(indent=4)


class AbstractIdleTimeoutTest(BaseTest):
    def setUp(self):
        # Setting up PTF dataplane
        self.dataplane = ptf.dataplane_instance
        self.dataplane.flush()

        logging.info("AbstractIdleTimeoutTest.setUp()")
        grpc_addr = tu.test_param_get("grpcaddr")
        if grpc_addr is None:
            grpc_addr = 'localhost:9559'
        certs_dir = '/usr/share/stratum/certs'
        root_certificate = certs_dir + '/ca.crt'
        private_key = certs_dir + '/client.key'
        certificate_chain = certs_dir + '/client.crt'
        # Default to insecure
        ssl_opts = p4rt.SSLOptions(True, root_certificate, certificate_chain,
                                   private_key)
        sh.setup(device_id=1,
                 grpc_addr=grpc_addr,
                 election_id=(0, 1),
                 ssl_options=ssl_opts)

    def tearDown(self):
        logging.info("IdleTimeoutTest.tearDown()")
        sh.teardown()

    def runTestImpl(self):
        self.setupCtrlPlane()
        logger.info("Sending Packet ...")
        self.sendPacket()
        logger.info("Verifying Packet ...")
        self.verifyPackets()

#############################################################
# Define a few small helper functions that help construct
# parameters for the table_add() method.
#############################################################

def entry_count(table_name):
    te = sh.TableEntry(table_name)
    n = 0
    for x in te.read():
        n = n + 1
    return n

def init_key_from_read_tableentry(read_te):
    new_te = sh.TableEntry(read_te.name)
    # This is only written to work for tables where all key fields are
    # match_kind exact.
    for f in read_te.match._fields:
        new_te.match[f] = '%d' % (int.from_bytes(read_te.match[f].exact.value, 'big'))
    return new_te

def delete_all_entries(tname):
    te = sh.TableEntry(tname)
    for e in te.read():
        d = init_key_from_read_tableentry(e)
        d.delete()

def add_ipv4_host_entry_action_send(ipv4_addr_str, port_int):
    te = sh.TableEntry('ipv4_host')(action='send')
    te.match['dst_addr'] = '%s' % (ipv4_addr_str)
    te.action['port'] = '%d' % (port_int)
    te.insert()


class OneEntryTest(AbstractIdleTimeoutTest):

    def sendPacket(self):
        in_dmac = 'ee:30:ca:9d:1e:00'
        in_smac = 'ee:cd:00:7e:70:00'
        ip_src_addr = '1.1.1.1'
        ip_dst_addr = '2.2.2.2'
        ig_port = 0
        sport = 59597
        dport = 7503
        pkt_in = tu.simple_tcp_packet(eth_src=in_smac, eth_dst=in_dmac,
                                      ip_src=ip_src_addr, ip_dst=ip_dst_addr,
                                      tcp_sport=sport, tcp_dport=dport)
        
        # Send in a first packet that should experience a miss on
        # table ct_tcp_table, cause a new entry to be added by the
        # data plane with a 30-second expiration time, and be
        # forwarded with a change to its source MAC address that the
        # add_on_miss0.p4 program uses to indicate that a miss
        # occurred.
        logging.info("Sending packet #1")
        tu.send_packet(self, ig_port, pkt_in)
        first_pkt_time = time.time()

    def verifyPackets(self):
        in_dmac = 'ee:30:ca:9d:1e:00'
        in_smac = 'ee:cd:00:7e:70:00'
        ip_src_addr = '1.1.1.1'
        ip_dst_addr = '2.2.2.2'
        sport = 59597
        dport = 7503
        # add_on_miss0.p4 replaces least significant 8 bits of source
        # MAC address with 0xf1 on a hit of table ct_tcp_table, or
        # 0xa5 on a miss.
        out_smac_for_miss = in_smac[:-2] + 'a5'
        eg_port = 1
        exp_pkt_for_miss = \
            tu.simple_tcp_packet(eth_src=out_smac_for_miss, eth_dst=in_dmac,
                                 ip_src=ip_src_addr, ip_dst=ip_dst_addr,
                                 tcp_sport=sport, tcp_dport=dport)

        # Check hit not tested for now for simplicity
        # out_smac_for_hit = in_smac[:-2] + 'f1'
        # exp_pkt_for_hit = \
        #     tu.simple_tcp_packet(eth_src=out_smac_for_hit, eth_dst=in_dmac,
        #                          ip_src=ip_src_addr, ip_dst=ip_dst_addr,
        #                          tcp_sport=sport, tcp_dport=dport)

        
        tu.verify_packets(self, exp_pkt_for_miss, [eg_port])
        logging.info("    packet experienced a miss in ct_tcp_table as expected")

    def setupCtrlPlane(self):
        ig_port = 0
        eg_port = 1

        ip_src_addr = '1.1.1.1'
        ip_dst_addr = '2.2.2.2'

        logging.info("Attempting to delete all entries in ipv4_host")
        delete_all_entries('ipv4_host')
        logging.info("Attempting to add entries to ipv4_host")
        add_ipv4_host_entry_action_send(ip_src_addr, ig_port)
        add_ipv4_host_entry_action_send(ip_dst_addr, eg_port)
        logging.info("Now ipv4_host contains %d entries"
                     "" % (entry_count('ipv4_host')))

    def runTest(self):
        self.runTestImpl()
        

        
        
       
