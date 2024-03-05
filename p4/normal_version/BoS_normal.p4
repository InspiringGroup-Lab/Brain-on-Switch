/* -*- P4_16 -*- */

// We indexed the packet, embedding vectors, hidden states, GRU tables, and classes from 0 in the code

#include <core.p4>

@pa_no_overlay("ingress", "ig_intr_md_for_dprsr.drop_ctl")
#include <tna.p4>

#include "headers.p4"
#include "util.p4"
#include "parsers.p4"
#include "per_packet.p4"

control SwitchIngress(
        inout header_t hdr,
        inout ig_metadata_t ig_md,
        in ingress_intrinsic_metadata_t ig_intr_md,
        in ingress_intrinsic_metadata_from_parser_t ig_prsr_md,
        inout ingress_intrinsic_metadata_for_deparser_t ig_dprsr_md,
        inout ingress_intrinsic_metadata_for_tm_t ig_tm_md) {
    
    action set_egr_mir_ses(MirrorId_t egr_mir_ses) {
        ig_md.egr_mir_ses = egr_mir_ses;
    }

    @max_actions(1)
    table tab_set_egr_mir_ses {
        key = {
            ig_intr_md.ingress_port[8:7]: exact @name("ig_pipe");
        }
        actions = {
            set_egr_mir_ses;
        }
        idle_timeout = false;
        default_action = set_egr_mir_ses(0);
    }

    action drop() {
        ig_dprsr_md.drop_ctl = 3w1;
    }

    action forward(PortId_t port) {
        ig_tm_md.ucast_egress_port = port;
    }

    @max_actions(2)
    table tab_execute_forward_action {
        key = {
            ig_md.use_rnn: ternary @name("ig_use_rnn");
            ig_md.use_per_packet: ternary @name("ig_use_per_packet");
            ig_md.collision: ternary @name("ig_collision");
            ig_intr_md.ingress_port: ternary @name("ig_port");
            ig_md.stored_escalation_flag: ternary @name("ig_stored_escalation_flag");
        }
        actions = {
            forward; drop;
        }
        idle_timeout = false;
        default_action = drop();
    }

    action act_write_code(bit<8> code) {
        ig_md.code = code;
    }

    table tab_write_code {
        key = {
            ig_md.timeout: exact;
            ig_md.collision: exact;
            ig_md.use_per_packet: exact;
            ig_md.use_rnn: exact;
        }
        actions = {
            act_write_code;
        }
        size = 16;
        const entries = {
            (0, 0, 0, 0): act_write_code(0);
            (1, 0, 0, 0): act_write_code(1);
            (0, 1, 0, 0): act_write_code(2);
            (1, 1, 0, 0): act_write_code(3);
            (0, 0, 1, 0): act_write_code(4);
            (1, 0, 1, 0): act_write_code(5);
            (0, 1, 1, 0): act_write_code(6);
            (1, 1, 1, 0): act_write_code(7);
            (0, 0, 0, 1): act_write_code(8);
            (1, 0, 0, 1): act_write_code(9);
            (0, 1, 0, 1): act_write_code(10);
            (1, 1, 0, 1): act_write_code(11);
            (0, 0, 1, 1): act_write_code(12);
            (1, 0, 1, 1): act_write_code(13);
            (0, 1, 1, 1): act_write_code(14);
            (1, 1, 1, 1): act_write_code(15);
        }
        const default_action = act_write_code(0);
        idle_timeout = false;
    }

    #include "prepare.p4"

    #include "flow_manager.p4"

    #include "last_timestamp.p4"

    #include "embedding.p4"

    #include "pkt_cnt.p4"

    #include "mem_and_swap.p4"

    #include "ig_gru.p4"

    #include "ig_global_cnt.p4"

    #include "ig_stored_escalation_flag.p4"

    apply {
        if (hdr.ipv4.isValid()) {
            ig_dprsr_md.drop_ctl = 0;

            tab_set_egr_mir_ses.apply();
            
            tab_embed_length.apply(); // stage 0

            tab_get_timestamp.apply(); // stage 0
            tab_get_id.apply(); // stage 0
            tab_get_idx.apply(); // stage 0

            tab_ig_global_pkt_cnt.apply(); // stage 0
            if (hdr.recir_md.isValid()) {
                // tab_ig_global_recir_pkt_cnt.apply(); // DEBUG only
            }
            
            tab_flow_info.apply(); // stage 1

            tab_global_collision_escalation_threshold.apply(); // stage 1

            if (!hdr.recir_md.isValid()) {
                if (ig_md.predicate == 2) { // collision
                    ig_md.collision = 1;
                } else {
                    ig_md.collision = 0;
                    tab_pkt_cnt_initial_8.apply(); // stage 2
                    tab_last_timestamp_1ns.apply(); // stage 2
                    tab_pkt_cnt_mod_7.apply(); // stage 2
                }

                if (ig_md.predicate >= 4) { // timeout
                    ig_md.timeout = 1; // stage 2
                } else {
                    ig_md.timeout = 0; // stage 2
                }

                tab_global_collision_escalation_cnt.apply(); // stage 2
            } else {
                drop();
            }

            if (ig_md.predicate >= 4) { // timeout
                // should be normal packet (i.e., not recirculated)
                ig_md.new_escalation_flag = ESCALATION_NO;
                ig_md.stored_escalation_flag_update = true;
            } else {
                if (ig_md.predicate == 1) { // no collision
                    if (hdr.recir_md.isValid()) {
                        ig_md.stored_escalation_flag_update = true;
                    }
                }
            }

            tab_stored_escalation_flag.apply(); // stage 3

            if (!hdr.recir_md.isValid()) {
                hdr.bridged_md.setValid();
                hdr.bridged_md.hidden_state3 = 0;
                if (ig_md.collision == 0) { // stage 3
                    ig_md.use_per_packet = 0;
                    if (ig_md.pkt_cnt_initial_8 == 7) {
                        ig_md.use_rnn = 1;
                    } else {
                        ig_md.use_rnn = 0;
                    }
                } else {
                    ig_md.use_rnn = 0;
                    if (ig_md.collision_escalation_predicate == 8) {
                        // imis
                        ig_md.use_per_packet = 0;
                    } else {
                        // perpacket
                        ig_md.use_per_packet = 1;
                    }
                }
                
                tab_execute_forward_action.apply(); // stage 3~12

                tab_write_code.apply(); // stage 3~12

                if (ig_md.collision == 0) {
                    if (ig_md.pkt_cnt_initial_8 == 0) {
                        ig_md.interval_1ns = 0; // stage 3
                    } else {
                        ig_md.interval_1ns = ig_md.timestamp_1ns - ig_md.last_timestamp_1ns; // stage 3
                    }
                }
            }

            if (!hdr.recir_md.isValid()) {
                if (ig_md.collision == 0) {
                    tab_embed_interval.apply(); // stage 4
                    tab_embed_fc.apply(); // stage 5

                    tab_bin0.apply();
                    tab_bin1.apply();
                    tab_bin2.apply();
                    tab_bin3.apply();
                    tab_bin4.apply();
                    tab_bin5.apply();
                    tab_bin6.apply();
                    if (ig_md.use_rnn == 1) {
                        tab_swap.apply();
                        tab_gru0_gru1.apply();
                        tab_gru2.apply();
                        tab_gru3.apply();
                    }
                }
                
                hdr.bridged_md.pkt_type = PKT_TYPE_NORMAL;
                hdr.bridged_md.egr_mir_ses = ig_md.egr_mir_ses;
                // hdr.bridged_md.hidden_state3 = ig_md.hidden_state3; // move into tab_gru3
                hdr.bridged_md.ev4 = ig_md.ev4;
                hdr.bridged_md.ev5 = ig_md.ev5;
                hdr.bridged_md.ev6 = ig_md.ev6;
                hdr.bridged_md.ev7 = ig_md.ev7;

                hdr.bridged_md.id = ig_md.id;
                hdr.bridged_md.idx = ig_md.idx;
                
                hdr.bridged_md.code = ig_md.code;
                hdr.bridged_md.stored_escalation_flag = ig_md.stored_escalation_flag;
            }
        } else {
            ig_dprsr_md.drop_ctl = 1;
        }
    }
}


control SwitchEgress(
        inout header_t hdr,
        inout eg_metadata_t eg_md,
        in egress_intrinsic_metadata_t eg_intr_md,
        in egress_intrinsic_metadata_from_parser_t eg_intr_md_from_prsr,
        inout egress_intrinsic_metadata_for_deparser_t eg_intr_dprs_md,
        inout egress_intrinsic_metadata_for_output_port_t eg_intr_oport_md) {
    
    PerPacket() per_packet;
    
    #include "eg_gru.p4"

    #include "classify.p4"

    // #include "eg_global_cnt.p4"

    #include "ambiguous_cnt.p4"

    action set_mirror() {
        eg_md.pkt_type = PKT_TYPE_MIRROR;
        eg_intr_dprs_md.mirror_type = MIRROR_TYPE_E2E;
    }

    apply {
        if (hdr.ipv4.isValid()) {
            // tab_eg_global_pkt_cnt.apply(); // DEBUG only
            if (hdr.mirror_md.isValid()) {
                hdr.ethernet.ether_type = ETHERTYPE_RECIRCULATE;
                hdr.recir_md.setValid();
                hdr.recir_md._unused = 0;
                // tab_eg_global_to_recir_pkt_cnt.apply(); // DEBUG only
            } else {
                // if (eg_md.timeout == 1) {
                if (eg_md.code[0:0] == 1) { // timeout
                    eg_md.tmp_dif1 = 0;
                    eg_md.tmp_dif2 = 0;
                } else { // set to any none-zero value
                    eg_md.tmp_dif1 = 1;
                    eg_md.tmp_dif2 = 1;
                }

                // if (eg_md.collision == 0) {
                if (eg_md.code[1:1] == 0) {
                    tab_intermediate_result_cnt.apply();
                }
                // if (eg_md.use_rnn == 1) {
                if (eg_md.code[3:3] == 1) {
                    tab_gru4.apply();
                    tab_gru5.apply();
                    tab_gru6.apply();
                    tab_gru7_output.apply();

                    tab_confidence_threshold.apply();
                    // the 8-th packet will get result == 0 to reset the following counters
                    tab_cpr0.apply();
                    tab_cpr1.apply();
                    tab_cpr2.apply();
                    tab_cpr3.apply();
                    tab_cpr4.apply();
                    tab_cpr5.apply();

                    tab_cmp012.apply();
                    tab_cmp345.apply();
                    tab_cmp012345.apply();

                    tab_t_esc.apply();
                }
                if (eg_md.tmp_dif1 == 0) { // when timeout || !choose 012 || (choose 012 && low_confidence)
                    if (eg_md.tmp_dif2 == 0) { // when timeout || !choose 345 || (choose 345 && low_confidence)
                        tab_ambiguous_cnt.apply();
                    }
                }
                // if (eg_md.use_rnn == 1) {
                if (eg_md.code[3:3] == 1) {
                    if (eg_md.stored_escalation_flag == ESCALATION_NO) {
                        if (eg_md.need_escalation == 1) {
                            set_mirror();
                            // tab_eg_global_to_mirror_pkt_cnt.apply(); // DEBUG only
                        }
                    }
                }
                // if (eg_md.use_rnn == 0) {
                if (eg_md.code[3:3] == 0) {
                    // Add per-packet model here
                    per_packet.apply(_, hdr.ipv4.ihl, eg_md.tcp_data_offset, hdr.ipv4.total_len, 
                            hdr.ipv4.ttl, hdr.ipv4.diffserv, eg_md.per_packet_class);
                }
                // write the classification result, this is only an example
                // if (eg_md.use_rnn == 1) {
                if (eg_md.code[3:3] == 1) {
                    hdr.ipv4.diffserv = 4w0 ++ eg_md.final_class;
                } else {
                    hdr.ipv4.diffserv = 4w0 ++ eg_md.per_packet_class;
                }
            }
        } else {
            eg_intr_dprs_md.drop_ctl = 1;
        }
    }
}

Pipeline(
        SwitchIngressParser(),
        SwitchIngress(),
        SwitchIngressDeparser(),

        SwitchEgressParser(),
        SwitchEgress(),
        SwitchEgressDeparser()) pipe;

Switch(pipe) main;
