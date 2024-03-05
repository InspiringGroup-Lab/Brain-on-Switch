Register<bit<32>, _>(1) r_aux_ig_global_pkt_cnt;
RegisterAction<bit<32>, _, void>(r_aux_ig_global_pkt_cnt) ra_aux_ig_global_pkt_cnt = {
    void apply(inout bit<32> mem_cell) {
        mem_cell = mem_cell + 1;
    } };
action act_aux_ig_global_pkt_cnt() {
    ra_aux_ig_global_pkt_cnt.execute(0);
}
table tab_aux_ig_global_pkt_cnt {
    size = 1;
    actions = { act_aux_ig_global_pkt_cnt; }
    const default_action = act_aux_ig_global_pkt_cnt;
}

Register<bit<32>, bit<3>>(8) r_global_rnn_ground_truth_cnt;
RegisterAction<bit<32>, bit<3>, void>(r_global_rnn_ground_truth_cnt) ra_global_rnn_ground_truth_cnt = {
    void apply(inout bit<32> mem_cell) {
        mem_cell = mem_cell + 1;
    } };
action act_global_rnn_ground_truth_cnt() {
    ra_global_rnn_ground_truth_cnt.execute(hdr.aux_md.ground_truth);
}
table tab_global_rnn_ground_truth_cnt {
    size = 1;
    actions = { act_global_rnn_ground_truth_cnt; }
    const default_action = act_global_rnn_ground_truth_cnt;
}
/*
Register<bit<32>, bit<3>>(8) r_global_rnn_pred_cnt;
RegisterAction<bit<32>, bit<3>, void>(r_global_rnn_pred_cnt) ra_global_rnn_pred_cnt = {
    void apply(inout bit<32> mem_cell) {
        mem_cell = mem_cell + 1;
    } };
action act_global_rnn_pred_cnt() {
    ra_global_rnn_pred_cnt.execute(hdr.aux_md.pred);
}
table tab_global_rnn_pred_cnt {
    size = 1;
    actions = { act_global_rnn_pred_cnt; }
    const default_action = act_global_rnn_pred_cnt;
}

Register<bit<32>, bit<3>>(8) r_global_rnn_tp_cnt;
RegisterAction<bit<32>, bit<3>, void>(r_global_rnn_tp_cnt) ra_global_rnn_tp_cnt = {
    void apply(inout bit<32> mem_cell) {
        mem_cell = mem_cell + 1;
    } };
action act_global_rnn_tp_cnt() {
    ra_global_rnn_tp_cnt.execute(hdr.aux_md.ground_truth);
}
table tab_global_rnn_tp_cnt {
    size = 1;
    actions = { act_global_rnn_tp_cnt; }
    const default_action = act_global_rnn_tp_cnt;
}
*/
Hash<bit<6>>(HashAlgorithm_t.IDENTITY) hash_result_for_rnn;
Register<bit<32>, bit<6>>(64) r_global_rnn_conf_mat_cnt;
RegisterAction<bit<32>, bit<6>, void>(r_global_rnn_conf_mat_cnt) ra_global_rnn_conf_mat_cnt = {
    void apply(inout bit<32> mem_cell) {
        mem_cell = mem_cell + 1;
    } };
action act_global_rnn_conf_mat_cnt() {
    ra_global_rnn_conf_mat_cnt.execute(hash_result_for_rnn.get({hdr.aux_md.ground_truth, hdr.aux_md.pred}));
}
table tab_global_rnn_conf_mat_cnt {
    size = 1;
    actions = { act_global_rnn_conf_mat_cnt; }
    const default_action = act_global_rnn_conf_mat_cnt;
}
/*
Register<bit<32>, bit<1>>(1) r_global_rnn_cnt;
RegisterAction<bit<32>, bit<1>, void>(r_global_rnn_cnt) ra_global_rnn_cnt = {
    void apply(inout bit<32> mem_cell) {
        mem_cell = mem_cell + 1;
    } };
action act_global_rnn_cnt() {
    ra_global_rnn_cnt.execute(0);
}
table tab_global_rnn_cnt {
    size = 1;
    actions = { act_global_rnn_cnt; }
    const default_action = act_global_rnn_cnt;
}
*/

Hash<bit<6>>(HashAlgorithm_t.IDENTITY) hash_result_for_perpacket;
Register<bit<32>, bit<6>>(64) r_global_perpacket_conf_mat_cnt;
RegisterAction<bit<32>, bit<6>, void>(r_global_perpacket_conf_mat_cnt) ra_global_perpacket_conf_mat_cnt = {
    void apply(inout bit<32> mem_cell) {
        mem_cell = mem_cell + 1;
    } };
action act_global_perpacket_conf_mat_cnt() {
    ra_global_perpacket_conf_mat_cnt.execute(hash_result_for_perpacket.get({hdr.aux_md.ground_truth, hdr.aux_md.pred}));
}
table tab_global_perpacket_conf_mat_cnt {
    size = 1;
    actions = { act_global_perpacket_conf_mat_cnt; }
    const default_action = act_global_perpacket_conf_mat_cnt;
}

Register<bit<32>, bit<3>>(8) r_global_perpacket_ground_truth_cnt;
RegisterAction<bit<32>, bit<3>, void>(r_global_perpacket_ground_truth_cnt) ra_global_perpacket_ground_truth_cnt = {
    void apply(inout bit<32> mem_cell) {
        mem_cell = mem_cell + 1;
    } };
action act_global_perpacket_ground_truth_cnt() {
    ra_global_perpacket_ground_truth_cnt.execute(hdr.aux_md.ground_truth);
}
table tab_global_perpacket_ground_truth_cnt {
    size = 1;
    actions = { act_global_perpacket_ground_truth_cnt; }
    const default_action = act_global_perpacket_ground_truth_cnt;
}

Register<bit<32>, bit<3>>(8) r_global_pre_analysis_ground_truth_cnt;
RegisterAction<bit<32>, bit<3>, void>(r_global_pre_analysis_ground_truth_cnt) ra_global_pre_analysis_ground_truth_cnt = {
    void apply(inout bit<32> mem_cell) {
        mem_cell = mem_cell + 1;
    } };
action act_global_pre_analysis_ground_truth_cnt() {
    ra_global_pre_analysis_ground_truth_cnt.execute(hdr.aux_md.ground_truth);
}
table tab_global_pre_analysis_ground_truth_cnt {
    size = 1;
    actions = { act_global_pre_analysis_ground_truth_cnt; }
    const default_action = act_global_pre_analysis_ground_truth_cnt;
}

Register<bit<32>, bit<3>>(8) r_global_IMIS_ground_truth_cnt;
RegisterAction<bit<32>, bit<3>, void>(r_global_IMIS_ground_truth_cnt) ra_global_IMIS_ground_truth_cnt = {
    void apply(inout bit<32> mem_cell) {
        mem_cell = mem_cell + 1;
    } };
action act_global_IMIS_ground_truth_cnt() {
    ra_global_IMIS_ground_truth_cnt.execute(hdr.aux_md.ground_truth);
}
table tab_global_IMIS_ground_truth_cnt {
    size = 1;
    actions = { act_global_IMIS_ground_truth_cnt; }
    const default_action = act_global_IMIS_ground_truth_cnt;
}

Register<bit<32>, bit<3>>(8) r_global_collision_IMIS_ground_truth_cnt;
RegisterAction<bit<32>, bit<3>, void>(r_global_collision_IMIS_ground_truth_cnt) ra_global_collision_IMIS_ground_truth_cnt = {
    void apply(inout bit<32> mem_cell) {
        mem_cell = mem_cell + 1;
    } };
action act_global_collision_IMIS_ground_truth_cnt() {
    ra_global_collision_IMIS_ground_truth_cnt.execute(hdr.aux_md.ground_truth);
}
table tab_global_collision_IMIS_ground_truth_cnt {
    size = 1;
    actions = { act_global_collision_IMIS_ground_truth_cnt; }
    const default_action = act_global_collision_IMIS_ground_truth_cnt;
}
