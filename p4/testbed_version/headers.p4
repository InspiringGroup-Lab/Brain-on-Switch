/* -*- P4_16 -*- */

/*******************************************************************************
 * BAREFOOT NETWORKS CONFIDENTIAL & PROPRIETARY
 *
 * Copyright (c) Intel Corporation
 * SPDX-License-Identifier: CC-BY-ND-4.0
 */

#ifndef _HEADERS_
#define _HEADERS_

typedef bit<48> mac_addr_t;
typedef bit<32> ipv4_addr_t;
typedef bit<128> ipv6_addr_t;
typedef bit<12> vlan_id_t;

typedef bit<16> ether_type_t;
const ether_type_t ETHERTYPE_IPV4 = 16w0x0800;
const ether_type_t ETHERTYPE_ARP = 16w0x0806;
const ether_type_t ETHERTYPE_IPV6 = 16w0x86dd;
const ether_type_t ETHERTYPE_VLAN = 16w0x8100;

typedef bit<8> ip_protocol_t;
const ip_protocol_t IP_PROTOCOLS_ICMP = 1;
const ip_protocol_t IP_PROTOCOLS_TCP = 6;
const ip_protocol_t IP_PROTOCOLS_UDP = 17;


header ethernet_h {
    mac_addr_t dst_addr;
    mac_addr_t src_addr;
    bit<16> ether_type;
}

header ipv4_h {
    bit<4> version;
    bit<4> ihl;
    bit<8> diffserv;
    bit<16> total_len;
    bit<16> identification;
    bit<3> flags;
    bit<13> frag_offset;
    bit<8> ttl;
    bit<8> protocol;
    bit<16> hdr_checksum;
    ipv4_addr_t src_addr;
    ipv4_addr_t dst_addr;
}

header tcp_h {
    bit<16> src_port;
    bit<16> dst_port;
    bit<32> seq_no;
    bit<32> ack_no;
    bit<4> data_offset;
    bit<4> res;
    bit<8> flags;
    bit<16> window;
    bit<16> checksum;
    bit<16> urgent_ptr;
}

header udp_h {
    bit<16> src_port;
    bit<16> dst_port;
    bit<16> hdr_length;
    bit<16> checksum;
}

// ================================ // TODO: check
// speedup = 1x
// original timeout = 256ms = 256000000ns
// original precision = 16384ns

// embedding ipd is using 16384ns precision
// 256000000 / 16384 = 15625, so 16384 entries is ok
// [25:14] of timestamp_1ns

// timeout_65536ns is 256000000 / 65536 = 3906
#define TIMEOUT 3906

#define MAX_FLOW_NUM 65536
#define MAX_FLOW_NUM_WIDTH 16

const PortId_t pipe_aux_recir_port = 196;

typedef bit<8>  pkt_type_t;
const pkt_type_t PKT_TYPE_NORMAL = 1;
const pkt_type_t PKT_TYPE_MIRROR = 2;

#if __TARGET_TOFINO__ == 1
typedef bit<3> mirror_type_t;
#else
typedef bit<4> mirror_type_t;
#endif
const mirror_type_t MIRROR_TYPE_I2E = 1;
const mirror_type_t MIRROR_TYPE_E2E = 2;

const ether_type_t ETHERTYPE_RECIRCULATE = 16w0xffff;

const bit<8> ESCALATION_YES = 8w0xff;
const bit<8> ESCALATION_NO = 8w0x00;

header bridged_md_h { // total: 24 bytes = 8 * 24 bits = 196 bits
    bit<8> pkt_type;
    @padding bit<8> _pd;

    @padding bit<(16-MIRROR_ID_WIDTH)> __pd;
    MirrorId_t egr_mir_ses;

    bit<16> hidden_state3;
    
    @padding bit<2> _pd4;
    bit<6> ev4;
    @padding bit<2> _pd5;
    bit<6> ev5;
    @padding bit<2> _pd6;
    bit<6> ev6;
    @padding bit<2> _pd7;
    bit<6> ev7;

    bit<32> id;
    bit<32> idx;

    bit<8> code; // 1: timeout, 2: collision, 4: use_per_packet, 8: use_rnn
    bit<8> stored_escalation_flag;
}

header mirror_h {
    pkt_type_t pkt_type;
    bit<8> _unused;
}

header recir_h { // total: 1 byte = 8 bits
    bit<8> _unused;
}

header look_ahead_h {
    pkt_type_t pkt_type;
    bit<8> _unused;
}


header aux_md_h {
    @padding bit<(16-PORT_ID_WIDTH)> _pd0;
    PortId_t egress_port;
    @padding bit<5> _pd1;
    bit<3> ground_truth;
    @padding bit<5> _pd2;
    bit<3> pred;
    bit<8> code; // 1: timeout, 2: collision, 4: use_per_packet, 8: use_rnn
    bit<8> stored_escalation_flag;
    /*
    stored_escalation_flag==ESCALATION_YES => IMIS
    otherwise:
    collision==0 && use_rnn=0 => first 7 packets
    collision==0 && use_rnn=1 => rnn
    collision==1 && use_per_packet=0 => IMIS (perpacket)
    collision==1 && use_per_packet=1 => perpacket
    */
}

struct header_t {
    look_ahead_h normal_md;
    mirror_h mirror_md; // not used
    bridged_md_h bridged_md;
    aux_md_h aux_md;
    ethernet_h ethernet;
    recir_h recir_md;
    ipv4_h ipv4;
    tcp_h tcp;
    udp_h udp;
}

struct empty_header_t {}

struct empty_metadata_t {}

struct flow_info {
    bit<32> timestamp;
    bit<32> id;
}

@pa_container_size("ingress", "ig_md.embedded_interval", 8)
struct ig_metadata_t {
    bit<16> src_port;
    bit<16> dst_port;

    bit<8> pkt_cnt_mod_7;
    bit<8> pkt_cnt_initial_8;

    bit<6> tmp0;
    bit<6> tmp1;
    bit<6> tmp2;
    bit<6> tmp3;
    bit<6> tmp4;
    bit<6> tmp5;
    bit<6> tmp6;

    bit<6> ev0;
    bit<6> ev1;
    bit<6> ev2;
    bit<6> ev3;
    bit<6> ev4;
    bit<6> ev5;
    bit<6> ev6;
    bit<6> ev7;

    bit<16> hidden_state1;
    bit<16> hidden_state2;
    bit<16> hidden_state3;

    bit<32> timestamp_65536ns;
    bit<32> timestamp_1ns;
    bit<32> id;
    bit<32> idx;

    bit<4> predicate;
    bit<1> timeout;
    bit<1> collision;
    bit<1> use_rnn;
    bit<1> use_per_packet;
    bit<8> code;
    bit<8> stored_escalation_flag;

    bool stored_escalation_flag_update;
    bit<8> new_escalation_flag;

    bit<10> embedded_length;
    bit<8> embedded_interval;

    bit<32> last_timestamp_1ns;
    bit<32> interval_1ns;

    bit<4> collision_escalation_predicate;
    bit<8> global_pkt_cnt;
    bit<8> global_collision_escalation_threshold;

    MirrorId_t egr_mir_ses;
}

@pa_container_size("egress", "eg_md.hidden_state4", 16)
@pa_container_size("egress", "eg_md.hidden_state5", 16)
@pa_container_size("egress", "eg_md.hidden_state6", 16)
struct eg_metadata_t {
    bit<6> ev4;
    bit<6> ev5;
    bit<6> ev6;
    bit<6> ev7;

    bit<16> hidden_state3;
    bit<16> hidden_state4;
    bit<16> hidden_state5;
    bit<16> hidden_state6;

    bit<8> intermediate_result_cnt; // number of rnn intermediate result - 1

    bit<16> cpr0;
    bit<16> cpr1;
    bit<16> cpr2;
    bit<16> cpr3;
    bit<16> cpr4;
    bit<16> cpr5;

    bit<16> t_conf0;
    bit<16> t_conf1;
    bit<16> t_conf2;
    bit<16> t_conf3;
    bit<16> t_conf4;
    bit<16> t_conf5;

    bit<16> pr0;
    bit<16> pr1;
    bit<16> pr2;
    bit<16> pr3;
    bit<16> pr4;
    bit<16> pr5;

    bit<16> tmp_cpr1;
    bit<4> tmp_class1;
    bit<16> tmp_dif1;
    bit<16> tmp_cpr2;
    bit<4> tmp_class2;
    bit<16> tmp_dif2;

    bit<4> final_class;

    bit<8> stored_escalation_flag;

    MirrorId_t egr_mir_ses;
    pkt_type_t pkt_type;
    bit<32> id;
    bit<32> idx;
    bit<8> code;

    bit<8> ambiguous_cnt;
    bit<8> t_esc;
    bit<8> need_escalation;

    bit<16> src_port;
    bit<16> dst_port;
    bit<4> tcp_data_offset;

    bit<4> per_packet_class;
}

#endif /* _HEADERS_ */
