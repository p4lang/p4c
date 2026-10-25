/*
 * SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
 * Copyright 2013-present Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define FABRIC_ENABLE
#define EGRESS_FILTER
#define INT_EP_ENABLE
#define INT_TRANSIT_ENABLE
#define OUTER_PIM_BIDIR_OPTIMIZATION
#define PIM_BIDIR_OPTIMIZATION
#define SFLOW_ENABLE

#ifdef MULTICAST_DISABLE
#define L2_MULTICAST_DISABLE
#define L3_MULTICAST_DISABLE
#endif

// Defines for switchapi library
#ifdef URPF_DISABLE
#define P4_URPF_DISABLE
#endif

#ifdef IPV6_DISABLE
#define P4_IPV6_DISABLE
#endif

#ifdef MPLS_DISABLE
#define P4_MPLS_DISABLE
#endif

#ifdef MULTICAST_DISABLE
#define P4_MULTICAST_DISABLE
#endif

#ifdef L2_MULTICAST_DISABLE
#define P4_L2_MULTICAST_DISABLE
#endif

#ifdef L3_MULTICAST_DISABLE
#define P4_L3_MULTICAST_DISABLE
#endif

#ifdef TUNNEL_DISABLE
#define P4_TUNNEL_DISABLE
#endif

#ifdef STORM_CONTROL_DISABLE
#define P4_STORM_CONTROL_DISABLE
#endif

#ifdef IPSG_DISABLE
#define P4_IPSG_DISABLE
#endif

#ifdef ACL_DISABLE
#define P4_ACL_DISABLE
#endif

#ifdef QOS_DISABLE
#define P4_QOS_DISABLE
#endif

#ifdef STP_DISABLE
#define P4_STP_DISABLE
#endif

#ifdef L2_DISABLE
#define P4_L2_DISABLE
#endif

#ifdef L3_DISABLE
#define P4_L3_DISABLE
#endif

#ifdef IPV4_DISABLE
#define P4_IPV4_DISABLE
#endif

#ifdef STATS_DISABLE
#define P4_STATS_DISABLE
#endif

#ifdef EGRESS_FILTER
#define P4_EGRESS_FILTER
#endif

#ifdef INT_EP_ENABLE
#define INT_ENABLE
#define P4_INT_ENABLE
#define P4_INT_EP_ENABLE
#endif

#ifdef INT_TRANSIT_ENABLE
#define P4_INT_TRANSIT_ENABLE
#define INT_ENABLE
#define P4_INT_ENABLE
#endif

#ifdef SFLOW_ENABLE
#define P4_SFLOW_ENABLE
#endif

#ifdef METER_DISABLE
#define P4_METER_DISABLE
#endif
