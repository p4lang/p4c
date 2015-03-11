import nnpy
import struct

def get_msg_type(msg):
    type_, = struct.unpack('i', msg[:4])
    return type_

class MSG_TYPES:
    (PACKET_IN, PACKET_OUT,
     PARSER_START, PARSER_DONE, PARSER_EXTRACT,
     DEPARSER_START, DEPARSER_DONE, DEPARSER_EMIT,
     CONDITION_EVAL, TABLE_HIT, TABLE_MISS,
     ACTION_EXECUTE) = range(12)

    @staticmethod
    def get_msg_class(type_):
        classes = {
            MSG_TYPES.PACKET_IN: PacketIn,
            MSG_TYPES.PACKET_OUT: PacketOut,
            MSG_TYPES.PARSER_START: ParserStart,
            MSG_TYPES.PARSER_DONE: ParserDone,
            MSG_TYPES.PARSER_EXTRACT: ParserExtract,
            MSG_TYPES.DEPARSER_START: DeparserStart,
            MSG_TYPES.DEPARSER_DONE: DeparserDone,
            MSG_TYPES.DEPARSER_EMIT: DeparserEmit,
            MSG_TYPES.CONDITION_EVAL: ConditionEval,
            MSG_TYPES.TABLE_HIT: TableHit,
            MSG_TYPES.TABLE_MISS: TableMiss,
            MSG_TYPES.ACTION_EXECUTE: ActionExecute,
        }
        return classes[type_]

    @staticmethod
    def get_str(type_):
        strs = {
            MSG_TYPES.PACKET_IN: "PACKET_IN",
            MSG_TYPES.PACKET_OUT: "PACKET_OUT",
            MSG_TYPES.PARSER_START: "PARSER_START",
            MSG_TYPES.PARSER_DONE: "PARSER_DONE",
            MSG_TYPES.PARSER_EXTRACT: "PARSER_EXTRACT",
            MSG_TYPES.DEPARSER_START: "DEPARSER_START",
            MSG_TYPES.DEPARSER_DONE: "DEPARSER_DONE",
            MSG_TYPES.DEPARSER_EMIT: "DEPARSER_EMIT",
            MSG_TYPES.CONDITION_EVAL: "CONDITION_EVAL",
            MSG_TYPES.TABLE_HIT: "TABLE_HIT",
            MSG_TYPES.TABLE_MISS: "TABLE_MISS",
            MSG_TYPES.ACTION_EXECUTE: "ACTION_EXECUTE",
        }
        return strs[type_]

class Msg(object):
    def __init__(self, msg):
        self.msg = msg

    def extract_hdr(self):
        struct_ = struct.Struct("iiQQQ")
        (_, self.switch_id,
         self.sig, self.id_, self.copy_id) = struct_.unpack_from(self.msg)
        return struct_.size

    def extract(self):
        bytes_extracted = self.extract_hdr()
        msg_remainder = self.msg[bytes_extracted:]
        return self.struct_.unpack(msg_remainder)

    def __str__(self):
        return "type: %s, switch_id: %d, sig: %d, id: %d, copy_id: %d" %\
            (self.type_str, self.switch_id,
             self.sig, self.id_, self.copy_id)

class PacketIn(Msg):
    def __init__(self, msg):
        super(PacketIn, self).__init__(msg)
        self.type_ = MSG_TYPES.PACKET_IN
        self.type_str = MSG_TYPES.get_str(self.type_)
        self.struct_ = struct.Struct("i")
        
    def extract(self):
        self.port_in, = super(PacketIn, self).extract()

    def __str__(self):
        return super(PacketIn, self).__str__() +\
            ", port_in: %d" % self.port_in

class PacketOut(Msg):
    def __init__(self, msg):
        super(PacketOut, self).__init__(msg)
        self.type_ = MSG_TYPES.PACKET_OUT
        self.type_str = MSG_TYPES.get_str(self.type_)
        self.struct_ = struct.Struct("i")
        
    def extract(self):
        self.port_out, = super(PacketOut, self).extract()

    def __str__(self):
        return super(PacketOut, self).__str__() +\
            ", port_out: %d" % self.port_out

class ParserStart(Msg):
    def __init__(self, msg):
        super(ParserStart, self).__init__(msg)
        self.type_ = MSG_TYPES.PARSER_START
        self.type_str = MSG_TYPES.get_str(self.type_)
        self.struct_ = struct.Struct("i")
        
    def extract(self):
        self.parser_id, = super(ParserStart, self).extract()

    def __str__(self):
        return super(ParserStart, self).__str__() +\
            ", parser_id: %d" % self.parser_id

class ParserDone(Msg):
    def __init__(self, msg):
        super(ParserDone, self).__init__(msg)
        self.type_ = MSG_TYPES.PARSER_DONE
        self.type_str = MSG_TYPES.get_str(self.type_)
        self.struct_ = struct.Struct("i")
        
    def extract(self):
        self.parser_id, = super(ParserDone, self).extract()

    def __str__(self):
        return super(ParserDone, self).__str__() +\
            ", parser_id: %d" % self.parser_id

class ParserExtract(Msg):
    def __init__(self, msg):
        super(ParserExtract, self).__init__(msg)
        self.type_ = MSG_TYPES.PARSER_EXTRACT
        self.type_str = MSG_TYPES.get_str(self.type_)
        self.struct_ = struct.Struct("i")
        
    def extract(self):
        self.header_id = super(ParserExtract, self).extract()

    def __str__(self):
        return super(ParserExtract, self).__str__() +\
            ", header_id: %d" % self.header_id

class DeparserStart(Msg):
    def __init__(self, msg):
        super(DeparserStart, self).__init__(msg)
        self.type_ = MSG_TYPES.DEPARSER_START
        self.type_str = MSG_TYPES.get_str(self.type_)
        self.struct_ = struct.Struct("i")
        
    def extract(self):
        self.deparser_id, = super(DeparserStart, self).extract()

    def __str__(self):
        return super(DeparserStart, self).__str__() +\
            ", deparser_id: %d" % self.deparser_id

class DeparserDone(Msg):
    def __init__(self, msg):
        super(DeparserDone, self).__init__(msg)
        self.type_ = MSG_TYPES.DEPARSER_DONE
        self.type_str = MSG_TYPES.get_str(self.type_)
        self.struct_ = struct.Struct("i")
        
    def extract(self):
        self.deparser_id, = super(DeparserDone, self).extract()

    def __str__(self):
        return super(DeparserDone, self).__str__() +\
            ", deparser_id: %d" % self.deparser_id

class DeparserEmit(Msg):
    def __init__(self, msg):
        super(DeparserEmit, self).__init__(msg)
        self.type_ = MSG_TYPES.DEPARSER_EMIT
        self.type_str = MSG_TYPES.get_str(self.type_)
        self.struct_ = struct.Struct("i")
        
    def extract(self):
         self.header_id = super(DeparserEmit, self).extract()

    def __str__(self):
        return super(DeparserEmit, self).__str__() +\
            ", header_id: %d" % self.header_id

class ConditionEval(Msg):
    def __init__(self, msg):
        super(ConditionEval, self).__init__(msg)
        self.type_ = MSG_TYPES.CONDITION_EVAL
        self.type_str = MSG_TYPES.get_str(self.type_)
        self.struct_ = struct.Struct("ii")
        
    def extract(self):
        self.condition_id, self.result = super(ConditionEval, self).extract()
        self.result = True if self.result != 0 else False

    def __str__(self):
        return super(ConditionEval, self).__str__() +\
            ", condition_id: %d, result: %s" % (self.condition_id, self.result)


class TableHit(Msg):
    def __init__(self, msg):
        super(TableHit, self).__init__(msg)
        self.type_ = MSG_TYPES.TABLE_HIT
        self.type_str = MSG_TYPES.get_str(self.type_)
        self.struct_ = struct.Struct("i")
        
    def extract(self):
        self.table_id = super(TableHit, self).extract()

    def __str__(self):
        return super(TableHit, self).__str__() +\
            ", table_id: %d" % self.table_id

class TableMiss(Msg):
    def __init__(self, msg):
        super(TableMiss, self).__init__(msg)
        self.type_ = MSG_TYPES.TABLE_MISS
        self.type_str = MSG_TYPES.get_str(self.type_)
        self.struct_ = struct.Struct("i")
        
    def extract(self):
        self.table_id = super(TableMiss, self).extract()

    def __str__(self):
        return super(TableMiss, self).__str__() +\
            ", table_id: %d" % self.table_id

class ActionExecute(Msg):
    def __init__(self, msg):
        super(ActionExecute, self).__init__(msg)
        self.type_ = MSG_TYPES.ACTION_EXECUTE
        self.type_str = MSG_TYPES.get_str(self.type_)
        self.struct_ = struct.Struct("i")
        
    def extract(self):
        super(ActionExecute, self).extract()

    def __str__(self):
        return super(ActionExecute, self).__str__()

def main():
    sub = nnpy.Socket(nnpy.AF_SP, nnpy.SUB)
    sub.connect('ipc:///tmp/test_bm.ipc')
    sub.setsockopt(nnpy.SUB, nnpy.SUB_SUBSCRIBE, '')
    
    while True:
        msg = sub.recv()
        msg_type = get_msg_type(msg)

        # try:
        p = MSG_TYPES.get_msg_class(msg_type)(msg)
        # except:
        #     print "Unknown msg type", msg_type
        #     continue
        p.extract()
        print p

if __name__ == "__main__":
    main()
