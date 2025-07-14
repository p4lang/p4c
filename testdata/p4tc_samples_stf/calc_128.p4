/* -*- P4_16 -*- */

/*
 * P4 Calculator
 *
 * This program implements a simple protocol. It can be carried over Ethernet
 * (Ethertype 0x1234).
 *
 * The Protocol header looks like this:
 *
 *   0   1   2   3   4   5   6   7   9  10  11  12   13  14  15
 * +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
 * | P | 4 | V |Op |            Operand A                      |
 * +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
 * |      ...      |            Operand B                      |
 * +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
 * |      ...      |            Result
 * +---------------+---+---+---+---+---+---+---+---+---+---+---+
 * |      ...      |
 * +---------------+
 *
 * P is an ASCII Letter 'P' (0x50)
 * 4 is an ASCII Letter '4' (0x34)
 * V, stands for Version, is currently 0.1 (0x01)
 * Op is an operation to Perform:
 *   '+' (0x2b) Result = OperandA + OperandB
 *   '-' (0x2d) Result = OperandA - OperandB
 *   '*' (0x2a) Result = OperandA * OperandB
 *   '&' (0x26) Result = OperandA & OperandB
 *   '|' (0x7c) Result = OperandA | OperandB
 *   '^' (0x5e) Result = OperandA ^ OperandB
 *
 * The device receives a packet, performs the requested operation, fills in the
 * result and sends the packet back out of the same port it came in on, while
 * swapping the source and destination addresses.
 *
 * If an unknown operation is specified or the header is not valid, the packet
 * is dropped
 */

#include <core.p4>
#include <tc/pna.p4>

struct metadata_t {
    /* In our case it is empty */
}

/*
 * Define the headers the program will recognize
 */

/*
 * Standard ethernet header
 */
header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

/*
 * This is a custom protocol header for the calculator. We'll use
 * ethertype 0x1234 for is (see parser)
 */
#define P4CALC_ETYPE 0x1234
#define P4CALC_P 0x50   // 'P'
#define P4CALC_4 0x34   // '4'
#define P4CALC_VER 0x01   // v0.1
#define P4CALC_PLUS 0x2b   // '+'
#define P4CALC_MINUS 0x2d   // '-'
#define P4CALC_MUL 0x2a   // '*'
#define P4CALC_AND  0x26   // '&'
#define P4CALC_OR   0x7c   // '|'
#define P4CALC_CARET 0x5e   // '^'

header p4calc_t {
    bit<8>  p;
    bit<8>  four;
    bit<8>  ver;
    bit<8>  op;
    bit<128> operand_a;
    bit<128> operand_b;
    bit<128> res;
}

/*
 * All headers, used in the program needs to be assembed into a single struct.
 * We only need to declare the type, but there is no need to instantiate it,
 * because it is done "by the architecture", i.e. outside of P4 functions
 */
struct headers_t {
    ethernet_t   ethernet;
    p4calc_t     p4calc;
}

/*************************************************************************
 ***********************  P A R S E R  ***********************************
 *************************************************************************/
parser MainParserImpl(
    packet_in pkt,
    out   headers_t  hdr,
    inout metadata_t meta,
    in    pna_main_parser_input_metadata_t istd)
{

    state start {
        pkt.extract(hdr.ethernet);
        transition select(hdr.ethernet.etherType) {
            P4CALC_ETYPE : check_p4calc;
            default      : accept;
        }
    }

    state check_p4calc {
        transition select(pkt.lookahead<p4calc_t>().p,
        pkt.lookahead<p4calc_t>().four,
        pkt.lookahead<p4calc_t>().ver) {
            (P4CALC_P, P4CALC_4, P4CALC_VER) : parse_p4calc;
            default                          : accept;
        }
    }

    state parse_p4calc {
        pkt.extract(hdr.p4calc);
        transition accept;
    }
}

/*************************************************************************
 **********************  M A I N    C O N T R O L ************************
 *************************************************************************/
control MainControlImpl(
    inout headers_t  hdr,
    inout metadata_t meta,
    in    pna_main_input_metadata_t  istd,
    inout pna_main_output_metadata_t ostd)
{

    action send_back(bit<128> result) {
        bit<48> tmp;

        /* Put the result back in */
        hdr.p4calc.res = result;

        /* Swap the MAC addresses */
        tmp = hdr.ethernet.dstAddr;
        hdr.ethernet.dstAddr = hdr.ethernet.srcAddr;
        hdr.ethernet.srcAddr = tmp;
        /* Send the packet back to the port it came from */
        send_to_port(istd.input_port);
    }

    action operation_add() {
        send_back(hdr.p4calc.operand_a + hdr.p4calc.operand_b);
    }

    action operation_sub() {
        send_back(hdr.p4calc.operand_a - hdr.p4calc.operand_b);
    }

    action operation_mul() {
        send_back(hdr.p4calc.operand_a * hdr.p4calc.operand_b);
    }

    action operation_and() {
        send_back(hdr.p4calc.operand_a & hdr.p4calc.operand_b);
    }

    action operation_or() {
        send_back(hdr.p4calc.operand_a | hdr.p4calc.operand_b);
    }

    action operation_xor() {
        send_back(hdr.p4calc.operand_a ^ hdr.p4calc.operand_b);
    }

    action operation_drop() {
        drop_packet();
    }

    table calculate {
        key = {
            hdr.p4calc.op        : exact @name("op");
        }
        actions = {
            operation_add;
            operation_sub;
            operation_mul;
            operation_and;
            operation_or;
            operation_xor;
            operation_drop;
        }
        const default_action = operation_drop();
        const entries = {
            P4CALC_PLUS : operation_add();
            P4CALC_MINUS : operation_sub();
            P4CALC_MUL : operation_mul();
            P4CALC_AND  : operation_and();
            P4CALC_OR   : operation_or();
            P4CALC_CARET: operation_xor();
        }
    }


    apply {
        if (hdr.p4calc.isValid()) {
            calculate.apply();
        } else {
            operation_drop();
        }
    }
}

/*************************************************************************
 ***********************  D E P A R S E R  *******************************
 *************************************************************************/
control MainDeparserImpl(
    packet_out pkt,
    inout    headers_t hdr,
    in    metadata_t meta,
    in    pna_main_output_metadata_t ostd)
{
    apply {
        pkt.emit(hdr.ethernet);
        pkt.emit(hdr.p4calc);
    }
}

/*************************************************************************
 ******************************  P N A  **********************************
 *************************************************************************/
PNA_NIC(
    MainParserImpl(),
    MainControlImpl(),
    MainDeparserImpl()
    ) main;
