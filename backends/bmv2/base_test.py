# This file was copied and then modified from the file
# testlib/base_test.py in the https://github.com/jafingerhut/p4-guide/
# repository. This file in turn was a modified version of
# https://github.com/p4lang/PI/blob/ec6865edc770b42f22fea15e6da17ca58a83d3a6/proto/ptf/base_test.py.

import logging
import queue
import re
import sys
import threading
import time
from collections import Counter
from functools import partialmethod, wraps
from pathlib import Path

import google.protobuf.text_format
import grpc
import ptf
from google.rpc import code_pb2, status_pb2
from p4.config.v1 import p4info_pb2
from p4.v1 import p4runtime_pb2, p4runtime_pb2_grpc
from ptf import config
from ptf import testutils as ptfutils
from ptf.base_tests import BaseTest

FILE_DIR = Path(__file__).resolve().parent
TOOLS_PATH = FILE_DIR.joinpath("../../tools")
sys.path.append(str(TOOLS_PATH))
import testutils


def stringify(n, length=0):
    """Take a non-negative integer 'n' as the first parameter, and a
    non-negative integer 'length' in units of _bytes_ as the second
    parameter (it defaults to 0 if not provided).  Return a string
    with binary contents expected by the Python P4Runtime client
    operations.  If 'n' does not fit in 'length' bytes, it is
    represented in the fewest number of bytes it does fit into without
    loss of precision.  It always returns a string at least one byte
    long, even if n=length=0."""
    assert isinstance(length, int)
    assert length >= 0
    assert isinstance(n, int)
    assert n >= 0
    if length == 0 and n == 0:
        length = 1
    else:
        n_size_bits = n.bit_length()
        n_size_bytes = (n_size_bits + 7) // 8
        if n_size_bytes > length:
            length = n_size_bytes
    s = n.to_bytes(length, byteorder="big")
    return s


# Used to indicate that the gRPC error Status object returned by the server has
# an incorrect format.
class P4RuntimeErrorFormatException(Exception):
    def __init__(self, message):
        super().__init__(message)


# Used to iterate over the p4.Error messages in a gRPC error Status object
class P4RuntimeErrorIterator:
    def __init__(self, grpc_error):
        assert grpc_error.code() == grpc.StatusCode.UNKNOWN
        self.grpc_error = grpc_error

        error = None
        # The gRPC Python package does not have a convenient way to access the
        # binary details for the error: they are treated as trailing metadata.
        for meta in self.grpc_error.trailing_metadata():
            if meta[0] == "grpc-status-details-bin":
                error = status_pb2.Status()
                error.ParseFromString(meta[1])
                break
        if error is None:
            raise P4RuntimeErrorFormatException("No binary details field")

        if len(error.details) == 0:
            raise P4RuntimeErrorFormatException(
                "Binary details field has empty Any details repeated field"
            )
        self.errors = error.details
        self.idx = 0

    def __iter__(self):
        return self

    def __next__(self):
        while self.idx < len(self.errors):
            p4_error = p4runtime_pb2.Error()
            one_error_any = self.errors[self.idx]
            if not one_error_any.Unpack(p4_error):
                raise P4RuntimeErrorFormatException("Cannot convert Any message to p4.Error")
            if p4_error.canonical_code == code_pb2.OK:
                continue
            v = self.idx, p4_error
            self.idx += 1
            return v
        raise StopIteration


# P4Runtime uses a 3-level message in case of an error during the processing of
# a write batch. This means that if we do not wrap the grpc.RpcError inside a
# custom exception, we can end-up with a non-helpful exception message in case
# of failure as only the first level will be printed. In this custom exception
# class, we extract the nested error message (one for each operation included in
# the batch) in order to print error code + user-facing message.  See P4 Runtime
# documentation for more details on error-reporting.
class P4RuntimeWriteException(Exception):
    def __init__(self, grpc_error):
        assert grpc_error.code() == grpc.StatusCode.UNKNOWN
        super().__init__()
        self.errors = []
        error_iterator = P4RuntimeErrorIterator(grpc_error)
        for error_tuple in error_iterator:
            self.errors.append(error_tuple)

    def __str__(self):
        message = "Error(s) during Write:\n"
        for idx, p4_error in self.errors:
            code_name = code_pb2._CODE.values_by_number[p4_error.canonical_code].name
            message += f"\t* At index {idx}: {code_name}, '{p4_error.message}'\n"
        return message

    def as_list_of_dicts(self):
        lst = []
        for idx, p4_error in self.errors:
            code_name = code_pb2._CODE.values_by_number[p4_error.canonical_code].name
            lst.append(
                {
                    "index": idx,
                    "code": p4_error.code,
                    "canonical_code": p4_error.canonical_code,
                    "code_name": code_name,
                    "details": p4_error.details,
                    "message": p4_error.message,
                    "space": p4_error.space,
                }
            )
        return lst


# Strongly inspired from _AssertRaisesContext in Python's unittest module
class _AssertP4RuntimeErrorContext:
    """A context manager used to implement the assertP4RuntimeError method."""

    def __init__(self, test_case, error_code=None, msg_regexp=None):
        self.failureException = test_case.failureException
        self.error_code = error_code
        self.msg_regexp = msg_regexp
        self.exception = None

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_value, tb):
        if exc_type is None:
            try:
                exc_name = self.expected.__name__
            except AttributeError:
                exc_name = str(self.msg_regexp)
            raise self.failureException(f"{exc_name} not raised")
        if not issubclass(exc_type, P4RuntimeWriteException):
            # let unexpected exceptions pass through
            return False
        self.exception = exc_value  # store for later retrieval

        if self.error_code is None:
            return True

        expected_code_name = code_pb2._CODE.values_by_number[self.error_code].name
        # guaranteed to have at least one element
        _, p4_error = exc_value.errors[0]
        code_name = code_pb2._CODE.values_by_number[p4_error.canonical_code].name
        if p4_error.canonical_code != self.error_code:
            # not the expected error code
            raise self.failureException(
                f"Invalid P4Runtime error code: expected {expected_code_name} but got {code_name}"
            )

        if self.msg_regexp is None:
            return True

        if not self.msg_regexp.search(p4_error.message):
            raise self.failureException(
                f"Invalid P4Runtime error msg: '{self.msg_regexp.pattern}' does not match"
                f" '{p4_error.message}'"
            )
        return True


# This code is common to all tests. setUp() is invoked at the beginning of the
# test and tearDown is called at the end, no matter whether the test passed /
# failed / errored.
class P4RuntimeTest(BaseTest):
    def setUp(self):
        BaseTest.setUp(self)
        self.device_id = 0

        # Setting up PTF dataplane
        self.dataplane = ptf.dataplane_instance
        self.dataplane.flush()

        self.p4info_obj_map = {}
        self._swports = []
        for _, port, _ in config["interfaces"]:
            self._swports.append(port)

        grpc_addr = ptfutils.test_param_get("grpcaddr")
        if grpc_addr is None:
            grpc_addr = "0.0.0.0:28000"

        self.channel = grpc.insecure_channel(grpc_addr)
        self.stub = p4runtime_pb2_grpc.P4RuntimeStub(self.channel)

        proto_txt_path = ptfutils.test_param_get("p4info")
        testutils.log.info(f"Reading p4info from {proto_txt_path}")
        self.p4info = p4info_pb2.P4Info()
        with open(proto_txt_path, "rb") as fin:
            google.protobuf.text_format.Merge(fin.read(), self.p4info)

        self.import_p4info_names()

        # used to store write requests sent to the P4Runtime server, useful for
        # autocleanup of tests (see definition of autocleanup decorator below)
        self._reqs = []

        self.set_up_stream()

    def updateConfig(self):
        """
        Performs a SetForwardingPipelineConfig on the device with provided
        P4Info and binary device config.

        Prerequisite: setUp() method has been called first to create
        the channel.
        """
        request = p4runtime_pb2.SetForwardingPipelineConfigRequest()
        request.device_id = self.device_id
        config = request.config
        # TBD: It seems like there should be a way to do this without
        # reading the P4info file again, instead getting the data from
        # self.p4info as saved when executing the setUp() method
        # above.  The following commented-out assignment does not work.
        # config.p4info = self.p4info
        proto_txt_path = ptfutils.test_param_get("p4info")
        with open(proto_txt_path, "r", encoding="utf-8") as fin:
            google.protobuf.text_format.Merge(fin.read(), config.p4info)
        config_path = ptfutils.test_param_get("config")
        testutils.log.info(f"Reading config (compiled P4 program) from {config_path}")
        with open(config_path, "rb") as config_f:
            config.p4_device_config = config_f.read()
        request.action = p4runtime_pb2.SetForwardingPipelineConfigRequest.VERIFY_AND_COMMIT
        try:
            response = self.stub.SetForwardingPipelineConfig(request)
            logging.debug(f"Response {response}")
        except Exception as e:
            logging.error("Error during SetForwardingPipelineConfig")
            logging.error(str(e))
            return False
        return True

    # In order to make writing tests easier, we accept any suffix that uniquely
    # identifies the object among p4info objects of the same type.
    def import_p4info_names(self):
        suffix_count = Counter()
        for obj_type in [
            "tables",
            "action_profiles",
            "actions",
            "counters",
            "direct_counters",
            "controller_packet_metadata",
            "meters",
            "direct_meters",
        ]:
            for obj in getattr(self.p4info, obj_type):
                pre = obj.preamble
                suffix = None
                for s in reversed(pre.name.split(".")):
                    suffix = s if suffix is None else s + "." + suffix
                    key = (obj_type, suffix)
                    self.p4info_obj_map[key] = obj
                    suffix_count[key] += 1
        for key, c in list(suffix_count.items()):
            if c > 1:
                del self.p4info_obj_map[key]

    def set_up_stream(self):
        self.stream_out_q = queue.Queue()
        self.stream_in_q = queue.Queue()

        def stream_req_iterator():
            while True:
                p = self.stream_out_q.get()
                if p is None:
                    break
                yield p

        def stream_recv(stream):
            for p in stream:
                now = time.time()
                logging.debug(
                    f"stream_recv received at time {now} and stored stream msg in stream_in_q: {p}"
                )
                self.stream_in_q.put({"time": now, "message": p})

        self.stream = self.stub.StreamChannel(stream_req_iterator())
        self.stream_recv_thread = threading.Thread(target=stream_recv, args=(self.stream,))
        self.stream_recv_thread.start()

        self.handshake()

    def handshake(self):
        req = p4runtime_pb2.StreamMessageRequest()
        arbitration = req.arbitration
        arbitration.device_id = self.device_id
        # TODO(antonin): we currently allow 0 as the election id in P4Runtime;
        # if this changes we will need to use an election id > 0 and update the
        # Write message to include the election id
        # election_id = arbitration.election_id
        # election_id.high = 0
        # election_id.low = 1
        self.stream_out_q.put(req)

        logging.debug("handshake() checking whether arbitration msg received from server")
        rep = self.get_stream_packet("arbitration", timeout=2)
        if rep is None:
            self.fail("Failed to establish handshake")

    def tearDown(self):
        self.tear_down_stream()
        BaseTest.tearDown(self)

    def tear_down_stream(self):
        self.stream_out_q.put(None)
        self.stream_recv_thread.join()

    def get_packet_in(self, timeout=1):
        msg = self.get_stream_packet("packet", timeout)
        if msg is None:
            self.fail("Packet in not received")
            return None
        else:
            return msg.packet

    def serializable_enum_dict(self, name):
        p4info = self.p4info
        type_info = p4info.type_info
        logging.debug(f"serializable_enum_dict: data{type_info.serializable_enums[name]}")
        name_to_int = {}
        int_to_name = {}
        for member in type_info.serializable_enums[name].members:
            name = member.name
            int_val = int.from_bytes(member.value, byteorder="big")
            name_to_int[name] = int_val
            int_to_name[int_val] = name
        logging.debug(
            f"serializable_enum_dict: name='{name}'"
            f" name_to_int={name_to_int} int_to_name={int_to_name}"
        )
        return name_to_int, int_to_name

    def controller_packet_metadata_dict_key_id(self, name):
        cpm_info = self.get_controller_packet_metadata(name)
        assert cpm_info is not None
        ret = {}
        for md in cpm_info.metadata:
            ret[md.id] = {"id": md.id, "name": md.name, "bitwidth": md.bitwidth}
        return ret

    def controller_packet_metadata_dict_key_name(self, name):
        cpm_info = self.get_controller_packet_metadata(name)
        assert cpm_info is not None
        ret = {}
        for md in cpm_info.metadata:
            ret[md.name] = {"id": md.id, "name": md.name, "bitwidth": md.bitwidth}
        return ret

    def decode_packet_in_metadata(self, packet):
        pktin_info = self.controller_packet_metadata_dict_key_id("packet_in")
        pktin_field_to_val = {}
        for md in packet.metadata:
            md_id_int = md.metadata_id
            md_val_int = int.from_bytes(md.value, byteorder="big")
            assert md_id_int in pktin_info
            md_field_info = pktin_info[md_id_int]
            pktin_field_to_val[md_field_info["name"]] = md_val_int
        ret = {"metadata": pktin_field_to_val, "payload": packet.payload}
        logging.debug(f"decode_packet_in_metadata: ret={ret}")
        return ret

    def verify_packet_in(self, exp_pktinfo, received_pktinfo):
        if received_pktinfo != exp_pktinfo:
            logging.error("PacketIn packet received:")
            logging.error(received_pktinfo)
            logging.error("PacketIn packet expected:")
            logging.error(exp_pktinfo)
            assert received_pktinfo == exp_pktinfo

    def get_stream_packet(self, type_, timeout=1):
        """Get and return the next stream_in packet that has type
        equal to type_.  If type_ is None, then get the next stream_in
        packet regardless of its type.  If type_ is not None, this
        method reads and discards any messages it finds with a
        different type.  If no appropriate message to be returned is
        found within the 'timeout' value (in seconds), return None."""
        start = time.time()
        try:
            while True:
                remaining = timeout - (time.time() - start)
                if remaining < 0:
                    break
                msginfo = self.stream_in_q.get(timeout=remaining)
                logging.debug(f"get_stream_packet dequeuing msg from stream_in_q:{msginfo}")
                if type_ is None or msginfo["message"].HasField(type_):
                    return msginfo["message"]
                logging.debug(
                    "get_stream_packet msg={msginfo} has no field type_={type_} so discarding"
                )
        except:
            # Timeout expired.
            pass
        return None

    def get_stream_packet2(self, type_, timeout=1):
        """Like get_stream_packet, except it returns two values.  The first is
        a dictionary with the key 'message' having a value that is the
        message, and a key 'time' having the value of time.time() when
        that message was received and stored in an internal queue
        where it waits to be retrieved by get_stream_packet or this
        method.  The second return value is a list of dictionaries,
        each with the same keys, containing all of the received
        messages that were skipped over in order to get t othe message
        with the desired type_ value.  As for get_stream_packet, the
        first return value is None if no stream_in message is
        retrieved within the specified timeout."""
        start = time.time()
        skipped_msginfos = []
        try:
            while True:
                remaining = timeout - (time.time() - start)
                if remaining < 0:
                    return None, skipped_msginfos
                msginfo = self.stream_in_q.get(timeout=remaining)
                logging.debug(f"get_stream_packet dequeuing msginfo from stream_in_q:{msginfo}")
                if type_ is None or msginfo["message"].HasField(type_):
                    return msginfo, skipped_msginfos
                logging.debug(
                    f"get_stream_packet msginfo['message'] has no field type_={type_} so discarding"
                )
                skipped_msginfos.append(msginfo)
        except:
            # Timeout expired.
            pass
        return None

    def encode_packet_out_metadata(self, pktout_dict):
        ret = p4runtime_pb2.PacketOut()
        ret.payload = pktout_dict["payload"]
        pktout_info = self.controller_packet_metadata_dict_key_name("packet_out")
        for k, v in pktout_dict["metadata"].items():
            md = ret.metadata.add()
            md.metadata_id = pktout_info[k]["id"]
            # I am not sure, but it seems that perhaps some code after
            # this point expects the bytes array of the values of the
            # controller metadata fields to be the full width of the
            # field, not the abbreviated version that omits leading 0
            # bytes that most P4Runtime API messages expect.
            bitwidth = pktout_info[k]["bitwidth"]
            bytewidth = (bitwidth + 7) // 8
            # logging.debug("dbg encode k=%s v=%s bitwidth=%s bytewidth=%s"
            #              "" % (k, v, bitwidth, bytewidth))
            md.value = stringify(v, bytewidth)
        return ret

    def send_packet_out(self, packet):
        packet_out_req = p4runtime_pb2.StreamMessageRequest()
        packet_out_req.packet.CopyFrom(packet)
        self.stream_out_q.put(packet_out_req)

    def swports(self, idx):
        if idx >= len(self._swports):
            self.fail(f"Index {idx} is out-of-bound of port map")
            return None
        return self._swports[idx]

    def get_obj(self, obj_type, name):
        key = (obj_type, name)
        return self.p4info_obj_map.get(key, None)

    def get_obj_id(self, obj_type, name):
        obj = self.get_obj(obj_type, name)
        if obj is None:
            return None
        return obj.preamble.id

    def get_param_id(self, action_name, name):
        a = self.get_obj("actions", action_name)
        if a is None:
            return None
        for p in a.params:
            if p.name == name:
                return p.id
        return None

    def get_mf_by_name(self, table_name, field_name):
        t = self.get_obj("tables", table_name)
        if t is None:
            return None
        for mf in t.match_fields:
            if mf.name == field_name:
                return mf
        return None

    # These are attempts at convenience functions aimed at making writing
    # P4Runtime PTF tests easier.

    class MF:
        def __init__(self, name):
            self.name = name

    class Exact(MF):
        def __init__(self, name, v):
            super(P4RuntimeTest.Exact, self).__init__(name)
            assert isinstance(v, int)
            self.v = v

        def add_to(self, mf_id, mf_bitwidth, mk):
            mf = mk.add()
            mf.field_id = mf_id
            mf.exact.value = stringify(self.v)

    class Lpm(MF):
        def __init__(self, name, v, pLen):
            super(P4RuntimeTest.Lpm, self).__init__(name)
            assert isinstance(v, int)
            assert isinstance(pLen, int)
            self.v = v
            self.pLen = pLen

        def add_to(self, mf_id, mf_bitwidth, mk):
            if self.pLen == 0:
                # P4Runtime API requires omitting fields that are
                # completely wildcard.
                return
            mf = mk.add()
            mf.field_id = mf_id
            mf.lpm.prefix_len = self.pLen

            # P4Runtime now has strict rules regarding ternary
            # matches: in the case of LPM, trailing bits in the value
            # (after prefix) must be set to 0.
            prefix_mask = ((1 << self.pLen) - 1) << (mf_bitwidth - self.pLen)
            mf.lpm.value = stringify(self.v & prefix_mask)

    class Ternary(MF):
        def __init__(self, name, v, mask):
            super(P4RuntimeTest.Ternary, self).__init__(name)
            assert isinstance(v, int)
            assert isinstance(mask, int)
            self.v = v
            self.mask = mask

        def add_to(self, mf_id, mf_bitwidth, mk):
            if self.mask == 0:
                # P4Runtime API requires omitting fields that are
                # completely wildcard.
                return
            mf = mk.add()
            mf.field_id = mf_id
            # P4Runtime now has strict rules regarding ternary matches: in the
            # case of Ternary, "don't-care" bits in the value must be set to 0
            masked_val_int = self.v & self.mask
            mf.ternary.value = stringify(masked_val_int)
            mf.ternary.mask = stringify(self.mask)

    class Range(MF):
        def __init__(self, name, min_val, max_val):
            super(P4RuntimeTest.Range, self).__init__(name)
            assert isinstance(min_val, int)
            assert isinstance(max_val, int)
            self.min_val = min_val
            self.max_val = max_val

        def add_to(self, mf_id, mf_bitwidth, mk):
            # P4Runtime API requires omitting fields that are
            # completely wildcard, i.e. the range is [0, 2^W-1] for a
            # field with type bit<W>.
            max_val_for_bitwidth = (1 << mf_bitwidth) - 1
            if (self.min_val == 0) and (self.max_val == max_val_for_bitwidth):
                return
            mf = mk.add()
            mf.field_id = mf_id
            mf.range.low = stringify(self.min_val)
            mf.range.high = stringify(self.max_val)

    class Optional(MF):
        def __init__(self, name, val, exact_match):
            super(P4RuntimeTest.Optional, self).__init__(name)
            assert isinstance(val, int)
            assert isinstance(exact_match, bool)
            self.val = val
            self.exact_match = exact_match

        def add_to(self, mf_id, mf_bitwidth, mk):
            if not self.exact_match:
                return
            mf = mk.add()
            mf.field_id = mf_id
            mf.optional.value = stringify(self.val)

    # Sets the match key for a p4::TableEntry object. mk needs to be an iterable
    # object of MF instances
    def set_match_key(self, table_entry, t_name, mk):
        for mf in mk:
            mf_obj = self.get_mf_by_name(t_name, mf.name)
            mf.add_to(mf_obj.id, mf_obj.bitwidth, table_entry.match)

    def set_action(self, action, a_name, params):
        action_id = self.get_action_id(a_name)
        if action_id is None:
            self.fail(
                "Failed to get id of action '{}' - perhaps the action name is misspelled?"
            ).format(a_name)
        action.action_id = action_id
        for p_name, v in params:
            param = action.params.add()
            param.param_id = self.get_param_id(a_name, p_name)
            param.value = stringify(v)

    # Sets the action & action data for a p4::TableEntry object. params needs to
    # be an iterable object of 2-tuples (<param_name>, <value>).
    def set_action_entry(self, table_entry, a_name, params):
        self.set_action(table_entry.action.action, a_name, params)

    def set_one_shot_profile(self, table_entry, a_name, params):
        action_profile_action = (
            table_entry.action.action_profile_action_set.action_profile_actions.add()
        )
        action_id = self.get_action_id(a_name)
        if action_id is None:
            self.fail(
                "Failed to get id of action '{}' - perhaps the action name is misspelled?"
            ).format(a_name)
        action_profile_action.action.action_id = action_id
        for p_name, v in params:
            param = action_profile_action.action.params.add()
            param.param_id = self.get_param_id(a_name, p_name)
            param.value = stringify(v)
        action_profile_action.weight = 1

    def _write(self, req):
        try:
            return self.stub.Write(req, timeout=2)
        except grpc.RpcError as e:
            if e.code() != grpc.StatusCode.UNKNOWN:
                raise e
            raise P4RuntimeWriteException(e) from e

    def write_request(self, req, store=True):
        rep = self._write(req)
        if store:
            self._reqs.append(req)
        return rep

    #
    # Convenience functions to build and send P4Runtime write requests
    #

    def _push_update_member(self, req, ap_name, mbr_id, a_name, params, update_type):
        update = req.updates.add()
        update.type = update_type
        ap_member = update.entity.action_profile_member
        ap_member.action_profile_id = self.get_ap_id(ap_name)
        ap_member.member_id = mbr_id
        self.set_action(ap_member.action, a_name, params)

    def push_update_add_member(self, req, ap_name, mbr_id, a_name, params):
        self._push_update_member(req, ap_name, mbr_id, a_name, params, p4runtime_pb2.Update.INSERT)

    def send_request_add_member(self, ap_name, mbr_id, a_name, params):
        req = p4runtime_pb2.WriteRequest()
        req.device_id = self.device_id
        self.push_update_add_member(req, ap_name, mbr_id, a_name, params)
        return req, self.write_request(req)

    def push_update_modify_member(self, req, ap_name, mbr_id, a_name, params):
        self._push_update_member(req, ap_name, mbr_id, a_name, params, p4runtime_pb2.Update.MODIFY)

    def send_request_modify_member(self, ap_name, mbr_id, a_name, params):
        req = p4runtime_pb2.WriteRequest()
        req.device_id = self.device_id
        self.push_update_modify_member(req, ap_name, mbr_id, a_name, params)
        return req, self.write_request(req, store=False)

    def push_update_add_group(self, req, ap_name, grp_id, grp_size=32, mbr_ids=[]):
        update = req.updates.add()
        update.type = p4runtime_pb2.Update.INSERT
        ap_group = update.entity.action_profile_group
        ap_group.action_profile_id = self.get_ap_id(ap_name)
        ap_group.group_id = grp_id
        ap_group.max_size = grp_size
        for mbr_id in mbr_ids:
            member = ap_group.members.add()
            member.member_id = mbr_id

    def send_request_add_group(self, ap_name, grp_id, grp_size=32, mbr_ids=[]):
        req = p4runtime_pb2.WriteRequest()
        req.device_id = self.device_id
        self.push_update_add_group(req, ap_name, grp_id, grp_size, mbr_ids)
        return req, self.write_request(req)

    def push_update_set_group_membership(self, req, ap_name, grp_id, mbr_ids=[]):
        update = req.updates.add()
        update.type = p4runtime_pb2.Update.MODIFY
        ap_group = update.entity.action_profile_group
        ap_group.action_profile_id = self.get_ap_id(ap_name)
        ap_group.group_id = grp_id
        for mbr_id in mbr_ids:
            member = ap_group.members.add()
            member.member_id = mbr_id

    def send_request_set_group_membership(self, ap_name, grp_id, mbr_ids=[]):
        req = p4runtime_pb2.WriteRequest()
        req.device_id = self.device_id
        self.push_update_set_group_membership(req, ap_name, grp_id, mbr_ids)
        return req, self.write_request(req, store=False)

    #
    # for all add_entry function, use mk == None for default entry
    #
    # TODO(antonin): The current P4Runtime reference implementation on p4lang
    # does not support resetting the default entry (i.e. a DELETE operation on
    # the default entry), which is why we make sure not to include it in the
    # list used for autocleanup, by passing store=False to write_request calls.
    #

    def push_update_add_entry_to_action(self, req, t_name, mk, a_name, params, priority=None):
        update = req.updates.add()
        update.type = p4runtime_pb2.Update.INSERT
        table_entry = update.entity.table_entry
        table_entry.table_id = self.get_table_id(t_name)
        if mk is not None:
            self.set_match_key(table_entry, t_name, mk)
        else:
            table_entry.is_default_action = True
            update.type = p4runtime_pb2.Update.MODIFY
        if priority:
            table_entry.priority = priority
        self.set_action_entry(table_entry, a_name, params)

    def send_request_add_entry_to_action(self, t_name, mk, a_name, params, priority=None):
        req = p4runtime_pb2.WriteRequest()
        req.device_id = self.device_id
        self.push_update_add_entry_to_action(req, t_name, mk, a_name, params, priority)
        return req, self.write_request(req, store=mk is not None)

    def check_table_name_and_key(self, table_name_and_key):
        assert isinstance(table_name_and_key, tuple)
        assert len(table_name_and_key) == 2

    def check_action_name_and_params(self, action_name_and_params):
        assert isinstance(action_name_and_params, tuple)
        assert len(action_name_and_params) == 2

    def make_table_entry(
        self, table_name_and_key, action_name_and_params, priority=None, options=None
    ):
        self.check_table_name_and_key(table_name_and_key)
        table_name = table_name_and_key[0]
        key = table_name_and_key[1]
        self.check_action_name_and_params(action_name_and_params)
        action_name = action_name_and_params[0]
        action_params = action_name_and_params[1]

        table_entry = p4runtime_pb2.TableEntry()
        table_entry.table_id = self.get_table_id(table_name)
        if key is not None:
            self.set_match_key(table_entry, table_name, key)
        else:
            table_entry.is_default_action = True
        if priority:
            table_entry.priority = priority
        if options is not None:
            if "idle_timeout_ns" in options:
                table_entry.idle_timeout_ns = options["idle_timeout_ns"]
            if "elapsed_ns" in options:
                table_entry.time_since_last_hit.elapsed_ns = options["elapsed_ns"]
            if "metadata" in options:
                table_entry.metadata = options["metadata"]
            if "oneshot" in options and options["oneshot"]:
                self.set_one_shot_profile(table_entry, action_name, action_params)
                return table_entry
        self.set_action_entry(table_entry, action_name, action_params)
        return table_entry

    # A shorter name for send_request_add_entry_to_action, and also
    # bundles up table name and key into one tuple, and action name
    # and params into another tuple, for the convenience of the caller
    # using some helper functions that create these tuples.
    def table_add(self, table_name_and_key, action_name_and_params, priority=None, options=None):
        table_entry = self.make_table_entry(
            table_name_and_key, action_name_and_params, priority, options
        )
        testutils.log.info(f"table_add: table_entry={table_entry}")
        req = p4runtime_pb2.WriteRequest()
        req.device_id = self.device_id
        update = req.updates.add()
        update.type = p4runtime_pb2.Update.INSERT
        update.entity.table_entry.CopyFrom(table_entry)
        key = table_name_and_key[1]
        if key is None:
            update.type = p4runtime_pb2.Update.MODIFY
        testutils.log.info(f"table_add: req={req}")
        return req, self.write_request(req, store=key is not None)

    def pre_add_mcast_group(self, mcast_grp_id, port_instance_pair_list):
        """When a packet is sent from ingress to the packet buffer with
        v1model architecture standard_metadata field "mcast_grp" equal
        to `mcast_grp_id`, configure the (egress_port, instance)
        places to which the packet will be copied.

        The first parameter is the `mcast_grp_id` value.

        The second is a list of 2-tuples.  The first element of each
        2-tuple is the egress port to which the copy should be sent,
        and the second is the "replication id", also called
        "egress_rid" in the P4_16 v1model architecture
        standard_metadata struct, or "instance" in the P4_16 PSA
        architecture psa_egress_input_metadata_t struct.  That value
        can be useful if you want to send multiple copies of the same
        packet out of the same output port, but want each one to be
        processed differently during egress processing.  If you want
        that, put multiple pairs with the same egress port in the
        replication list, but each with a different value of
        "replication id".
        """
        assert isinstance(mcast_grp_id, int)
        assert isinstance(port_instance_pair_list, list)
        for x in port_instance_pair_list:
            assert isinstance(x, tuple)
            assert len(x) == 2
            assert isinstance(x[0], int)
            assert isinstance(x[1], int)
        req = p4runtime_pb2.WriteRequest()
        req.device_id = self.device_id
        update = req.updates.add()
        update.type = p4runtime_pb2.Update.INSERT
        pre_entry = update.entity.packet_replication_engine_entry
        mc_grp_entry = pre_entry.multicast_group_entry
        mc_grp_entry.multicast_group_id = mcast_grp_id
        for x in port_instance_pair_list:
            replica = mc_grp_entry.replicas.add()
            replica.egress_port = x[0]
            replica.instance = x[1]
        return req, self.write_request(req, store=False)

    def insert_pre_clone_session(self, session_id, ports, cos=0, packet_length_bytes=0):
        req = self.get_new_write_request()
        update = req.updates.add()
        update.type = p4runtime_pb2.Update.INSERT
        pre_entry = update.entity.packet_replication_engine_entry
        clone_entry = pre_entry.clone_session_entry
        clone_entry.session_id = session_id
        clone_entry.class_of_service = cos
        clone_entry.packet_length_bytes = packet_length_bytes
        for port in ports:
            replica = clone_entry.replicas.add()
            replica.egress_port = port
            replica.instance = 1
        return req, self.write_request(req)

    def response_dump_helper(self, request):
        for response in self.stub.Read(request, timeout=2):
            yield response

    def make_counter_read_request(self, counter_name, direct=False):
        req = p4runtime_pb2.ReadRequest()
        req.device_id = self.device_id
        entity = req.entities.add()
        if direct:
            counter = entity.direct_counter_entry
            counter_obj = self.get_obj("direct_counters", counter_name)
            counter.table_entry.table_id = counter_obj.direct_table_id
        else:
            counter = entity.counter_entry
            counter.counter_id = self.get_counter_id(counter_name)
        return req, counter

    def counter_dump_data(self, counter_name, direct=False):
        req, _ = self.make_counter_read_request(counter_name, direct)
        if direct:
            exp_one_of = "direct_counter_entry"
        else:
            exp_one_of = "counter_entry"
        counter_entries = []
        for response in self.response_dump_helper(req):
            for entity in response.entities:
                assert entity.WhichOneof("entity") == exp_one_of
                if direct:
                    entry = entity.direct_counter_entry
                else:
                    entry = entity.counter_entry
                counter_entries.append(entry)
        return counter_entries

    def meter_write(self, meter_name, index, meter_config):
        req = self.get_new_write_request()
        update = req.updates.add()
        update.type = p4runtime_pb2.Update.MODIFY
        entity = update.entity
        meter_write = entity.meter_entry
        meter_write.meter_id = self.get_meter_id(meter_name)
        meter_write.index.index = index
        meter_write.config.cir = meter_config.cir
        meter_write.config.cburst = meter_config.cburst
        meter_write.config.pir = meter_config.pir
        meter_write.config.pburst = meter_config.pburst
        return req, self.write_request(req)

    def direct_meter_write(self, meter_config, table_id, table_entry):
        req = self.get_new_write_request()
        update = req.updates.add()
        update.type = p4runtime_pb2.Update.MODIFY
        entity = update.entity
        meter_write = entity.direct_meter_entry
        meter_write.table_entry.table_id = table_id
        if table_entry is None:
            meter_write.table_entry.is_default_action = True
        else:
            meter_write.table_entry.CopyFrom(table_entry)
        meter_write.config.cir = meter_config.cir
        meter_write.config.cburst = meter_config.cburst
        meter_write.config.pir = meter_config.pir
        meter_write.config.pburst = meter_config.pburst
        return req, self.write_request(req)

    def make_table_read_request(self, table_name):
        req = p4runtime_pb2.ReadRequest()
        req.device_id = self.device_id
        entity = req.entities.add()
        table = entity.table_entry
        table.table_id = self.get_table_id(table_name)
        return req, table

    def make_table_read_request_by_id(self, table_id):
        req = p4runtime_pb2.ReadRequest()
        req.device_id = self.device_id
        entity = req.entities.add()
        table = entity.table_entry
        table.table_id = table_id
        return req, table

    def table_dump_data(self, table_name):
        req, table = self.make_table_read_request(table_name)
        table_entries = []
        for response in self.response_dump_helper(req):
            for entity in response.entities:
                # testutils.log.info('entity.WhichOneof("entity")="%s"'
                #      '' % (entity.WhichOneof('entity')))
                assert entity.WhichOneof("entity") == "table_entry"
                entry = entity.table_entry
                table_entries.append(entry)
                # testutils.log.info(entry)
                # testutils.log.info('----')

        # Now try to get the default action.  I say 'try' because as
        # of 2019-Mar-21, this is not yet implemented in the open
        # source simple_switch_grpc implementation, and in that case
        # the code above will catch the exception and return 'None'
        # for the table_default_entry value.
        table_default_entry = None
        req, table = self.make_table_read_request(table_name)
        table.is_default_action = True
        try:
            for response in self.response_dump_helper(req):
                for entity in response.entities:
                    # testutils.log.info('entity.WhichOneof("entity")="%s"'
                    #      '' % (entity.WhichOneof('entity')))
                    assert entity.WhichOneof("entity") == "table_entry"
                    entry = entity.table_entry
                    table_default_entry = entity
        except grpc._channel._Rendezvous as e:
            testutils.log.info("Caught exception:")
            testutils.log.info(e)

        return table_entries, table_default_entry

    def push_update_add_entry_to_member(self, req, t_name, mk, mbr_id):
        update = req.updates.add()
        update.type = p4runtime_pb2.Update.INSERT
        table_entry = update.entity.table_entry
        table_entry.table_id = self.get_table_id(t_name)
        if mk is not None:
            self.set_match_key(table_entry, t_name, mk)
        else:
            table_entry.is_default_action = True
        table_entry.action.action_profile_member_id = mbr_id

    def send_request_add_entry_to_member(self, t_name, mk, mbr_id):
        req = p4runtime_pb2.WriteRequest()
        req.device_id = self.device_id
        self.push_update_add_entry_to_member(req, t_name, mk, mbr_id)
        return req, self.write_request(req, store=mk is not None)

    def push_update_add_entry_to_group(self, req, t_name, mk, grp_id):
        update = req.updates.add()
        update.type = p4runtime_pb2.Update.INSERT
        table_entry = update.entity.table_entry
        table_entry.table_id = self.get_table_id(t_name)
        if mk is not None:
            self.set_match_key(table_entry, t_name, mk)
        else:
            table_entry.is_default_action = True
        table_entry.action.action_profile_group_id = grp_id

    def send_request_add_entry_to_group(self, t_name, mk, grp_id):
        req = p4runtime_pb2.WriteRequest()
        req.device_id = self.device_id
        self.push_update_add_entry_to_group(req, t_name, mk, grp_id)
        return req, self.write_request(req, store=mk is not None)

    # iterates over all requests in reverse order; if they are INSERT updates,
    # replay them as DELETE updates; this is a convenient way to clean-up a lot
    # of switch state
    def undo_write_requests(self, reqs):
        updates = []
        for req in reversed(reqs):
            for update in reversed(req.updates):
                if update.type == p4runtime_pb2.Update.INSERT:
                    updates.append(update)
        new_req = p4runtime_pb2.WriteRequest()
        new_req.device_id = self.device_id
        for update in updates:
            update.type = p4runtime_pb2.Update.DELETE
            new_req.updates.add().CopyFrom(update)
        return self._write(new_req)

    def assertP4RuntimeError(self, code=None, msg_regexp=None):
        if msg_regexp is not None:
            msg_regexp = re.compile(msg_regexp)
        context = _AssertP4RuntimeErrorContext(self, code, msg_regexp)
        return context

    def get_new_write_request(self):
        req = p4runtime_pb2.WriteRequest()
        req.device_id = self.device_id
        election_id = req.election_id
        election_id.high = 0
        election_id.low = 0
        return req


# Add p4info object and object id "getters" for each object type; these are just
# wrappers around P4RuntimeTest.get_obj and P4RuntimeTest.get_obj_id.
# For example: get_table(x) and get_table_id(x) respectively call
# get_obj("tables", x) and get_obj_id("tables", x)
for obj_type, nickname in [
    ("tables", "table"),
    ("action_profiles", "ap"),
    ("actions", "action"),
    ("counters", "counter"),
    ("direct_counters", "direct_counter"),
    ("meters", "meter"),
    ("direct_meters", "direct_meter"),
    ("controller_packet_metadata", "controller_packet_metadata"),
]:
    name = "_".join(["get", nickname])
    setattr(P4RuntimeTest, name, partialmethod(P4RuntimeTest.get_obj, obj_type))
    name = "_".join(["get", nickname, "id"])
    setattr(P4RuntimeTest, name, partialmethod(P4RuntimeTest.get_obj_id, obj_type))


# this decorator can be used on the runTest method of P4Runtime PTF tests
# when it is used, the undo_write_requests will be called at the end of the test
# (irrespective of whether the test was a failure, a success, or an exception
# was raised). When this is used, all write requests must be performed through
# one of the send_request_* convenience functions, or by calling write_request;
# do not use stub.Write directly!
# most of the time, it is a great idea to use this decorator, as it makes the
# tests less verbose. In some circumstances, it is difficult to use it, in
# particular when the test itself issues DELETE request to remove some
# objects. In this case you will want to do the cleanup yourself (in the
# tearDown function for example); you can still use undo_write_request which
# should make things easier.
# because the PTF test writer needs to choose whether or not to use autocleanup,
# it seems more appropriate to define a decorator for this rather than do it
# unconditionally in the P4RuntimeTest tearDown method.
def autocleanup(f):
    @wraps(f)
    def handle(*args, **kwargs):
        test = args[0]
        assert isinstance(test, P4RuntimeTest)
        try:
            return f(*args, **kwargs)
        finally:
            test.undo_write_requests(test._reqs)

    return handle


# Copied update_config from the https://github.com/p4lang/PI
# repository in file proto/ptf/ptf_runner.py, then modified it
# slightly:


def update_config(config_path, p4info_path, grpc_addr, device_id):
    """
    Performs a SetForwardingPipelineConfig on the device with provided
    P4Info and binary device config
    """
    channel = grpc.insecure_channel(grpc_addr)
    stub = p4runtime_pb2_grpc.P4RuntimeStub(channel)
    testutils.log.info("Sending P4 config from file %s with P4info %s", config_path, p4info_path)
    request = p4runtime_pb2.SetForwardingPipelineConfigRequest()
    request.device_id = device_id
    config = request.config
    with open(p4info_path, "r", encoding="utf-8") as p4info_f:
        google.protobuf.text_format.Merge(p4info_f.read(), config.p4info)
    with open(config_path, "rb") as config_f:
        config.p4_device_config = config_f.read()
    request.action = p4runtime_pb2.SetForwardingPipelineConfigRequest.VERIFY_AND_COMMIT
    try:
        response = stub.SetForwardingPipelineConfig(request)
        logging.debug("Response %s", response)
    except Exception as e:
        logging.error("Error during SetForwardingPipelineConfig")
        logging.error(e)
        return False
    return True
