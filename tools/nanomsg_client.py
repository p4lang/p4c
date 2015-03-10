import nnpy
import struct

def get_msg_type(msg):
    type_, = struct.unpack('i', msg[:4])
    return type_

class MSG_TYPES:
    (PACKET_IN, TABLE_HIT) = range(2)

    @staticmethod
    def get_msg_class(type_):
        classes = {
            MSG_TYPES.PACKET_IN: PacketIn,
            MSG_TYPES.TABLE_HIT: TableHit,
        }
        return classes[type_]

class Msg(object):
    def __init__(self, msg):
        self.msg = msg

    def extract(self):
        struct_ = struct.Struct("iiQQQ")
        (_, self.switch_id,
         self.sig, self.id_, self.copy_id) = struct_.unpack_from(self.msg)
        return struct_.size

    def __str__(self):
        return "type: %s, switch_id: %d, sig: %d, id: %d, copy_id: %d" %\
            (self.type_str, self.switch_id,
             self.sig, self.id_, self.copy_id)

class PacketIn(Msg):
    def __init__(self, msg):
        super(PacketIn, self).__init__(msg)
        self.type_ = 0
        self.type_str = "PACKET_IN"
        
    def extract(self):
        return super(PacketIn, self).extract()

    def __str__(self):
        return super(PacketIn, self).__str__()

class TableHit(Msg):
    def __init__(self, msg):
        super(TableHit, self).__init__(msg)
        self.type_ = 0
        self.type_str = "TABLE_HIT"
        
    def extract(self):
        bytes_extracted = super(TableHit, self).extract()
        msg_remainder = self.msg[bytes_extracted:]
        struct_ = struct.Struct("i")
        self.table_id, = struct_.unpack(msg_remainder)
        return bytes_extracted + struct_.size

    def __str__(self):
        return super(TableHit, self).__str__() +\
            ", table_id: %d" % self.table_id

def main():
    sub = nnpy.Socket(nnpy.AF_SP, nnpy.SUB)
    sub.connect('ipc:///tmp/test_bm.ipc')
    sub.setsockopt(nnpy.SUB, nnpy.SUB_SUBSCRIBE, '')
    
    while True:
        msg = sub.recv()
        msg_type = get_msg_type(msg)

        try:
            p = MSG_TYPES.get_msg_class(msg_type)(msg)
        except:
            print "Unknown msg type", msg_type
            continue
        p.extract()
        print p

if __name__ == "__main__":
    main()
