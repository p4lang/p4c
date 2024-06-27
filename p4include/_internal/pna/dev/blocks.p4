// BEGIN:Programmable_blocks
parser MainParserT<MH, MM>(
    packet_in pkt,
    out   MH main_hdr,
    inout MM main_user_meta,
    in    pna_main_parser_input_metadata_t istd);

control MainControlT<MH, MM>(
    inout MH main_hdr,
    inout MM main_user_meta,
    in    pna_main_input_metadata_t  istd,
    inout pna_main_output_metadata_t ostd);

control MainDeparserT<MH, MM>(
    packet_out pkt,
    inout MH main_hdr,
    in    MM main_user_meta,
    in    pna_main_output_metadata_t ostd);

package PNA_NIC<MH, MM>(
    MainParserT<MH, MM> main_parser,
    MainControlT<MH, MM> main_control,
    MainDeparserT<MH, MM> main_deparser);
// END:Programmable_blocks
