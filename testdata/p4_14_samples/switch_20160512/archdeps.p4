/*
archdeps.p4
*/

#ifndef _ARCH_DEPS_H

#define _ARCH_DEPS_H

#define ingress_input_port standard_metadata.ingress_port
#define ingress_egress_port standard_metadata.egress_spec
#define egress_egress_port standard_metadata.egress_port
#define intrinsic_mcast_grp intrinsic_metadata.mcast_grp
#define egress_egress_rid intrinsic_metadata.egress_rid

#endif

