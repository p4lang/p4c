#include <core.p4>
#include <tc/pna.p4>

struct metadata_t {
    @tc_type("ipv4") bit<32> src;
    @tc_type("ipv4") bit<32> dst;
    bool    push;
}

header ethernet_t {
    @tc_type("macaddr") bit<48> dstAddr;
    @tc_type("macaddr") bit<48> srcAddr;
    bit<16> etherType;
}

header ipv4_t {
    bit<4>  version;
    bit<4>  ihl;
    bit<8>  diffserv;
    bit<16> totalLen;
    bit<16> identification;
    bit<3>  flags;
    bit<13> fragOffset;
    bit<8>  ttl;
    bit<8>  protocol;
    bit<16> hdrChecksum;
    @tc_type("ipv4") bit<32> srcAddr;
    @tc_type("ipv4") bit<32> dstAddr;
}

struct headers_t {
    ethernet_t   ethernet;
    ipv4_t       outer;
    ipv4_t       inner;
}

#define ETHERTYPE_IPV4 0x0800
#define IPPROTO_IPV4 0x4

/***********************  P A R S E R  **************************/
parser Parser(
        packet_in pkt,
        out   headers_t  hdr,
        inout metadata_t meta,
        in    pna_main_parser_input_metadata_t istd)
{
    state start {
        transition parse_ethernet;
    }

    state parse_ethernet {
        pkt.extract(hdr.ethernet);
        transition select(hdr.ethernet.etherType) {
            ETHERTYPE_IPV4: parse_ipv4;
            default: reject;
        }
    }

    state parse_ipv4 {
        pkt.extract(hdr.outer);
        transition select(hdr.outer.protocol) {
            IPPROTO_IPV4: parse_ipv4_inner;
            default: accept;
        }
    }

    state parse_ipv4_inner {
        pkt.extract(hdr.inner);
        transition accept;
    }
}

void ip_ttl_dec(InternetChecksum chk, inout ipv4_t ip)
{
      chk.clear();

      chk.set_state(~ip.hdrChecksum);

      chk.subtract({ ip.ttl, ip.protocol });

      ip.ttl = ip.ttl - 1;
      chk.add({ ip.ttl, ip.protocol });

      ip.hdrChecksum = chk.get();
}

void ip_checksum(InternetChecksum chk, inout ipv4_t ip)
{
   chk.clear();
   chk.add({
      /* 16-bit word 0 */ ip.version, ip.ihl, ip.diffserv,
      /* 16-bit word 1 */ ip.totalLen,
      /* 16-bit word 2 */ ip.identification,
      /* 16-bit word 3 */ ip.flags, ip.fragOffset,
      /* 16-bit word 4 */ ip.ttl, ip.protocol,
      /* 16-bit word 5 skip ip.hdrChecksum, */
      /* 16-bit words 6-7 */ ip.srcAddr,
      /* 16-bit words 8-9 */ ip.dstAddr
   });
   ip.hdrChecksum = chk.get();
}

void ipip_push(inout headers_t hdr, in metadata_t meta)
{
   hdr.inner = hdr.outer;
   hdr.outer.srcAddr = meta.src;
   hdr.outer.dstAddr = meta.dst;
   hdr.outer.ttl = 64;
   hdr.outer.protocol = 4; /* IPIP */
   /* Assume MTU can accomodate +20 bytes */
   hdr.outer.totalLen = hdr.outer.totalLen + 20;
   hdr.outer.hdrChecksum = 0;
}

/***************** M A T C H - A C T I O N  *********************/
control Main(
    inout headers_t  hdr,
    inout metadata_t meta,
    in pna_main_input_metadata_t  istd,
    inout pna_main_output_metadata_t ostd
)
{
   action set_ipip(@tc_type("ipv4") bit<32> src, @tc_type("ipv4") bit<32> dst, @tc_type("dev") PortId_t port) {
      meta.src = src;
      meta.dst = dst;
      meta.push = true;
      send_to_port(port);
   }

   action set_nh(@tc_type("macaddr") bit<48> dmac, @tc_type("dev") PortId_t port) {
      hdr.ethernet.dstAddr = dmac;
      send_to_port(port);
   }

   action drop() {
      drop_packet();
   }

   table fwd_table {
      key = {
         istd.input_port : exact @tc_type("dev") @name("port");
      }
      actions = {
         set_ipip;
         set_nh;
         drop;
      }
      default_action = drop;
   }

   apply {
      if (hdr.outer.isValid()) { /* applies to both ipip and plain ip */
         fwd_table.apply(); /* lookup based on incoming netdev */
         if (hdr.inner.isValid()) { /* incoming packet ipip */
            /* Pop the ipip header by invalidating outer header */
            hdr.outer.setInvalid();
         }
      }
   }
}

/*********************  D E P A R S E R  ************************/
control Deparser(
    packet_out pkt,
    inout    headers_t hdr,
    in    metadata_t meta,
    in    pna_main_output_metadata_t ostd)
{
    InternetChecksum() chk;

    apply {
        pkt.emit(hdr.ethernet);
	if (meta.push && hdr.outer.isValid()) {
               /* Push the ipip header */
               ipip_push(hdr, meta);
               ip_checksum(chk, hdr.outer);
               ip_ttl_dec(chk, hdr.inner);
	}
        pkt.emit(hdr.outer);
        pkt.emit(hdr.inner);
    }
}

/************ F I N A L   P A C K A G E ******************************/
PNA_NIC(
    Parser(),
    Main(),
    Deparser()
) main;
