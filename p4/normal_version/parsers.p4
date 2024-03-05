/* -*- P4_16 -*- */

/*******************************************************************************
 * BAREFOOT NETWORKS CONFIDENTIAL & PROPRIETARY
 *
 * Copyright (c) Intel Corporation
 * SPDX-License-Identifier: CC-BY-ND-4.0
 */

#ifndef _PARSERS_
#define _PARSERS_

#include "headers.p4"
#include "util.p4"

// ---------------------------------------------------------------------------
// Ingress parser
// ---------------------------------------------------------------------------

parser SwitchIngressParser(
        packet_in pkt,
        out header_t hdr,
        out ig_metadata_t ig_md,
        out ingress_intrinsic_metadata_t ig_intr_md) {
    
    TofinoIngressParser() tofino_parser;
    
    state start {
        ig_md.src_port = 0;
        ig_md.dst_port = 0;

        ig_md.pkt_cnt_mod_7 = 0;
        ig_md.pkt_cnt_initial_8 = 0;

        ig_md.tmp0 = 0;
        ig_md.tmp1 = 0;
        ig_md.tmp2 = 0;
        ig_md.tmp3 = 0;
        ig_md.tmp4 = 0;
        ig_md.tmp5 = 0;
        ig_md.tmp6 = 0;

        ig_md.ev0 = 0;
        ig_md.ev1 = 0;
        ig_md.ev2 = 0;
        ig_md.ev3 = 0;
        ig_md.ev4 = 0;
        ig_md.ev5 = 0;
        ig_md.ev6 = 0;
        ig_md.ev7 = 0;
        
        ig_md.hidden_state1 = 0;
        ig_md.hidden_state2 = 0;
        ig_md.hidden_state3 = 0;
        
        ig_md.timestamp_65536ns = 0;
        ig_md.timestamp_1ns = 0;
        ig_md.id = 0;
        ig_md.idx = 0;

        ig_md.predicate = 0;
        ig_md.timeout = 0;
        ig_md.collision = 0;
        ig_md.use_rnn = 0;
        ig_md.use_per_packet = 0;

        ig_md.code = 0;
        ig_md.stored_escalation_flag = 0;
        ig_md.stored_escalation_flag_update = false;
        ig_md.new_escalation_flag = 0;
        
        ig_md.embedded_length = 0;
        ig_md.embedded_interval = 0;

        ig_md.last_timestamp_1ns = 0;
        ig_md.interval_1ns = 0;

        ig_md.collision_escalation_predicate = 0;
        ig_md.global_pkt_cnt = 0;
        ig_md.global_collision_escalation_threshold = 0;

        ig_md.egr_mir_ses = 0;

        tofino_parser.apply(pkt, ig_intr_md);
        transition parse_ethernet;
    }

    state parse_ethernet {
        pkt.extract(hdr.ethernet);
        
        transition select(hdr.ethernet.ether_type) {
            ETHERTYPE_IPV4: parse_ipv4;
            ETHERTYPE_RECIRCULATE: parse_recirculate;
            default: reject;
        }
    }
    state parse_recirculate {
        pkt.extract(hdr.recir_md);
        ig_md.new_escalation_flag = ESCALATION_YES;
        transition parse_ipv4;
    }

    state parse_ipv4 {
        pkt.extract(hdr.ipv4);
        transition select(hdr.ipv4.protocol) {
            IP_PROTOCOLS_TCP: parse_tcp;
            IP_PROTOCOLS_UDP: parse_udp;
            default: reject;
        }
    }

    state parse_tcp {
        pkt.extract(hdr.tcp);
        ig_md.src_port = hdr.tcp.src_port;
        ig_md.dst_port = hdr.tcp.dst_port;
        transition accept;
    }

    state parse_udp {
        pkt.extract(hdr.udp);
        ig_md.src_port = hdr.udp.src_port;
        ig_md.dst_port = hdr.udp.dst_port;
        transition accept;
    }
}

// ---------------------------------------------------------------------------
// Ingress Deparser
// ---------------------------------------------------------------------------

control SwitchIngressDeparser(
        packet_out pkt,
        inout header_t hdr,
        in ig_metadata_t ig_md,
        in ingress_intrinsic_metadata_for_deparser_t ig_dprsr_md) {

    apply {
        pkt.emit(hdr.bridged_md);
        pkt.emit(hdr.ethernet);
        pkt.emit(hdr.ipv4);
        pkt.emit(hdr.tcp);
        pkt.emit(hdr.udp);
    }
}

// ---------------------------------------------------------------------------
// Egress parser
// ---------------------------------------------------------------------------


parser SwitchEgressParser(
        packet_in pkt,
        out header_t hdr,
        out eg_metadata_t eg_md,
        out egress_intrinsic_metadata_t eg_intr_md) {
    TofinoEgressParser() tofino_parser;
    
    state start {
        tofino_parser.apply(pkt, eg_intr_md);

        eg_md.ev4 = 0;
        eg_md.ev5 = 0;
        eg_md.ev6 = 0;
        eg_md.ev7 = 0;

        eg_md.hidden_state3 = 0;
        eg_md.hidden_state4 = 0;
        eg_md.hidden_state5 = 0;
        eg_md.hidden_state6 = 0;

        eg_md.intermediate_result_cnt = 0;

        eg_md.cpr0 = 0;
        eg_md.cpr1 = 0;
        eg_md.cpr2 = 0;
        eg_md.cpr3 = 0;
        eg_md.cpr4 = 0;
        eg_md.cpr5 = 0;
        
        eg_md.t_conf0 = 0;
        eg_md.t_conf1 = 0;
        eg_md.t_conf2 = 0;
        eg_md.t_conf3 = 0;
        eg_md.t_conf4 = 0;
        eg_md.t_conf5 = 0;

        eg_md.pr0 = 0;
        eg_md.pr1 = 0;
        eg_md.pr2 = 0;
        eg_md.pr3 = 0;
        eg_md.pr4 = 0;
        eg_md.pr5 = 0;

        eg_md.tmp_cpr1 = 0;
        eg_md.tmp_class1 = 0;
        eg_md.tmp_dif1 = 0;

        eg_md.tmp_cpr2 = 0;
        eg_md.tmp_class2 = 0;
        eg_md.tmp_dif2 = 0;

        eg_md.final_class = 0;
        
        eg_md.stored_escalation_flag = 0;

        eg_md.egr_mir_ses = 0;
        eg_md.pkt_type = 0;

        eg_md.id = 0;
        eg_md.idx = 0;
        eg_md.code = 0;

        eg_md.ambiguous_cnt = 0;
        eg_md.t_esc = 0;
        eg_md.need_escalation = 0;

        eg_md.src_port = 0;
        eg_md.dst_port = 0;

        eg_md.tcp_data_offset = 0;

        eg_md.per_packet_class = 0;

        transition parse_metadata;
    }
    
    state parse_metadata {
        look_ahead_h look_ahead_md = pkt.lookahead<look_ahead_h>();
        transition select(look_ahead_md.pkt_type) {
            PKT_TYPE_NORMAL : parse_bridged_md;
            PKT_TYPE_MIRROR : parse_mirror_md;
            default : reject;
        }
    }

    state parse_bridged_md {
        pkt.extract(hdr.bridged_md);
        eg_md.egr_mir_ses = hdr.bridged_md.egr_mir_ses;
        eg_md.hidden_state3 = hdr.bridged_md.hidden_state3;
        eg_md.ev4 = hdr.bridged_md.ev4;
        eg_md.ev5 = hdr.bridged_md.ev5;
        eg_md.ev6 = hdr.bridged_md.ev6;
        eg_md.ev7 = hdr.bridged_md.ev7;
        
        eg_md.id = hdr.bridged_md.id;
        eg_md.idx = hdr.bridged_md.idx;
        eg_md.code = hdr.bridged_md.code;
        eg_md.stored_escalation_flag = hdr.bridged_md.stored_escalation_flag;
        transition parse_ethernet;
    }

    state parse_mirror_md {
        pkt.extract(hdr.mirror_md);
        transition parse_ethernet;
    }

    state parse_ethernet {
        pkt.extract(hdr.ethernet);
        
        transition select(hdr.ethernet.ether_type) {
            ETHERTYPE_IPV4: parse_ipv4;
            default:        reject;
        }
    }

    state parse_ipv4 {
        pkt.extract(hdr.ipv4);
        transition select(hdr.ipv4.protocol) {
            IP_PROTOCOLS_TCP: parse_tcp;
            IP_PROTOCOLS_UDP: parse_udp;
            default: reject;
        }
    }

    state parse_tcp {
        pkt.extract(hdr.tcp);
        eg_md.src_port = hdr.tcp.src_port;
        eg_md.dst_port = hdr.tcp.dst_port;
        eg_md.tcp_data_offset = hdr.tcp.data_offset;
        transition accept;
    }

    state parse_udp {
        pkt.extract(hdr.udp);
        eg_md.src_port = hdr.udp.src_port;
        eg_md.dst_port = hdr.udp.dst_port;
        transition accept;
    }
}

// ---------------------------------------------------------------------------
// Egress Deparser
// ---------------------------------------------------------------------------

control SwitchEgressDeparser(
        packet_out pkt,
        inout header_t hdr,
        in eg_metadata_t eg_md,
        in egress_intrinsic_metadata_for_deparser_t eg_intr_dprs_md) {

    Mirror() mirror;
    Checksum() ipv4_checksum;

    apply {
        hdr.ipv4.hdr_checksum = ipv4_checksum.update(
                {hdr.ipv4.version,
                 hdr.ipv4.ihl,
                 hdr.ipv4.diffserv,
                 hdr.ipv4.total_len,
                 hdr.ipv4.identification,
                 hdr.ipv4.flags,
                 hdr.ipv4.frag_offset,
                 hdr.ipv4.ttl,
                 hdr.ipv4.protocol,
                 hdr.ipv4.src_addr,
                 hdr.ipv4.dst_addr});
    
        if (eg_intr_dprs_md.mirror_type == MIRROR_TYPE_E2E) {
            mirror.emit<mirror_h>(eg_md.egr_mir_ses, {eg_md.pkt_type, 0});
        }
        pkt.emit(hdr.ethernet);
        pkt.emit(hdr.recir_md);
        pkt.emit(hdr.ipv4);
        pkt.emit(hdr.tcp);
        pkt.emit(hdr.udp);
    }
}

#endif /* _PARSERS_ */