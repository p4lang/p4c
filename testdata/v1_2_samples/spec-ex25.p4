match_kind { exact }

typedef bit<48> EthernetAddress;

extern tbl {}

control c(bit x)
{
    action Set_dmac(EthernetAddress dmac) 
    {}

    action drop() {}
    
    table unit()
    {
        key = { x : exact; }

#if 0
        entries = {
            32w0x0A_00_00_01 => drop();
            32w0x0A_00_00_02 => Set_dmac(dmac = (EthernetAddress)48w0x11_22_33_44_55_66);
            32w0x0B_00_00_03 => Set_dmac(dmac = (EthernetAddress)48w0x11_22_33_44_55_77);
            32w0x0B_00_00_00 &&& 32w0xFF_00_00_00 => drop();
        }
#endif
        
        actions = {
            Set_dmac;
            drop;
        }
        
        default_action = Set_dmac((EthernetAddress)48w0xAA_BB_CC_DD_EE_FF);
        
        implementation = tbl();
    }

    apply {}
}
