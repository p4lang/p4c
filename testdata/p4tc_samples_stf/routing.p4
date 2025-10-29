/* -*- P4_16 -*- */

#include <core.p4>
#include <tc/pna.p4>

typedef bit<48> macaddr_t;

struct metadata_t {}

header ethernet_t {
    @tc_type("macaddr") macaddr_t dstAddr;
    @tc_type("macaddr") macaddr_t srcAddr;
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
    ipv4_t       ip;
}

#define ETHERTYPE_IPV4 0x0800

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
        pkt.extract(hdr.ip);
        transition accept;
    }
}

void ip_ttl_dec(InternetChecksum chk, inout ipv4_t ip) {

   /* Decrement ttl
    * HC' = (~HC) + ~m + m'
    */

   chk.clear();
   chk.set_state(~ip.hdrChecksum);

   chk.subtract({ ip.ttl, ip.protocol });
   ip.ttl = ip.ttl - 1;
   chk.add({ ip.ttl, ip.protocol });

   ip.hdrChecksum = chk.get();
}

/***************** M A T C H - A C T I O N  *********************/

control Main(
    inout headers_t  hdr,
    inout metadata_t meta,
    in    pna_main_input_metadata_t  istd,
    inout pna_main_output_metadata_t ostd
)
{
   bit<32> nh_index;

   action drop() {
      drop_packet();
   }

   action set_nh(@tc_type("macaddr") bit<48> dmac, @tc_type("dev") PortId_t port) {
      hdr.ethernet.dstAddr = dmac;
      send_to_port(port);
   }

   table nh_table {
      key = {
         nh_index : exact;
      }
      actions = {
         drop;
         set_nh;
      }
      default_action = drop;
   }

   action set_nhid(bit<32> index) {
      nh_index = index;
   }

   table fib_table {
      key = {
         hdr.ip.dstAddr : lpm @tc_type("ipv4") @name("prefix");
      }
      actions = {
         set_nhid;
      }
      /* 0 is the default route */
      default_action = set_nhid(0);
   }

   apply {
      if (hdr.ip.isValid() && hdr.ip.ttl > 1) {
            fib_table.apply();
            nh_table.apply();
      } else {
           drop_packet();
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
        ip_ttl_dec(chk, hdr.ip);
        pkt.emit(hdr.ip);
    }
}

/************ F I N A L   P A C K A G E ******************************/
PNA_NIC(
    Parser(),
    Main(),
    Deparser()
) main;
