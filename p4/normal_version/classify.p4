// When pkt_cnt_initial_8=0, the return value is meaningless. 
Register<bit<8>, bit<32>>(MAX_FLOW_NUM) r_intermediate_result_cnt;
RegisterAction<bit<8>, bit<32>, bit<8>>(r_intermediate_result_cnt) ra_intermediate_result_cnt = {
    void apply(inout bit<8> mem_cell, out bit<8> cpr) {
        cpr = mem_cell;
        // if (eg_md.use_rnn == 0) {
        if (eg_md.code[3:3] == 0) {
        // if (eg_md.pkt_cnt_initial_8 < 7) {
            mem_cell = 0;
        } else {
            mem_cell = mem_cell + 1;
        }
    } };
action act_intermediate_result_cnt() {
    eg_md.intermediate_result_cnt[6:0] = ra_intermediate_result_cnt.execute(eg_md.idx)[6:0];
}
table tab_intermediate_result_cnt {
    size = 1;
    actions = { act_intermediate_result_cnt; }
    const default_action = act_intermediate_result_cnt;
}

action act_confidence_threshold(bit<16> t_conf0, bit<16> t_conf1, bit<16> t_conf2, 
        bit<16> t_conf3, bit<16> t_conf4, bit<16> t_conf5) {
    eg_md.t_conf0 = t_conf0;
    eg_md.t_conf1 = t_conf1;
    eg_md.t_conf2 = t_conf2;
    eg_md.t_conf3 = t_conf3;
    eg_md.t_conf4 = t_conf4;
    eg_md.t_conf5 = t_conf5;
}

table tab_confidence_threshold {
    size = 128;
    key = {
        eg_md.intermediate_result_cnt: exact;
    }
    actions = {
        act_confidence_threshold;
    }
    idle_timeout = false;
    const default_action = act_confidence_threshold(0, 0, 0, 0, 0, 0);
}

Register<bit<16>, bit<32>>(MAX_FLOW_NUM) r_cpr0;
RegisterAction<bit<16>, bit<32>, bit<16>>(r_cpr0) ra_cpr0 = {
    void apply(inout bit<16> mem_cell, out bit<16> cpr) {
        if (eg_md.intermediate_result_cnt == 0) {
            mem_cell = eg_md.pr0;
        } else {
            mem_cell = mem_cell + eg_md.pr0;
        }
        cpr = mem_cell;
    } };
action act_cpr0() {
    eg_md.cpr0 = ra_cpr0.execute(eg_md.idx);
}
table tab_cpr0 {
    size = 1;
    actions = { act_cpr0; }
    const default_action = act_cpr0;
}

Register<bit<16>, bit<32>>(MAX_FLOW_NUM) r_cpr1;
RegisterAction<bit<16>, bit<32>, bit<16>>(r_cpr1) ra_cpr1 = {
    void apply(inout bit<16> mem_cell, out bit<16> cpr) {
        if (eg_md.intermediate_result_cnt == 0) {
            mem_cell = eg_md.pr1;
        } else {
            mem_cell = mem_cell + eg_md.pr1;
        }
        cpr = mem_cell;
    } };
action act_cpr1() {
    eg_md.cpr1 = ra_cpr1.execute(eg_md.idx);
}
table tab_cpr1 {
    size = 1;
    actions = { act_cpr1; }
    const default_action = act_cpr1;
}

Register<bit<16>, bit<32>>(MAX_FLOW_NUM) r_cpr2;
RegisterAction<bit<16>, bit<32>, bit<16>>(r_cpr2) ra_cpr2 = {
    void apply(inout bit<16> mem_cell, out bit<16> cpr) {
        if (eg_md.intermediate_result_cnt == 0) {
            mem_cell = eg_md.pr2;
        } else {
            mem_cell = mem_cell + eg_md.pr2;
        }
        cpr = mem_cell;
    } };
action act_cpr2() {
    eg_md.cpr2 = ra_cpr2.execute(eg_md.idx);
}
table tab_cpr2 {
    size = 1;
    actions = { act_cpr2; }
    const default_action = act_cpr2;
}

Register<bit<16>, bit<32>>(MAX_FLOW_NUM) r_cpr3;
RegisterAction<bit<16>, bit<32>, bit<16>>(r_cpr3) ra_cpr3 = {
    void apply(inout bit<16> mem_cell, out bit<16> cpr) {
        if (eg_md.intermediate_result_cnt == 0) {
            mem_cell = eg_md.pr3;
        } else {
            mem_cell = mem_cell + eg_md.pr3;
        }
        cpr = mem_cell;
    } };
action act_cpr3() {
    eg_md.cpr3 = ra_cpr3.execute(eg_md.idx);
}
table tab_cpr3 {
    size = 1;
    actions = { act_cpr3; }
    const default_action = act_cpr3;
}

Register<bit<16>, bit<32>>(MAX_FLOW_NUM) r_cpr4;
RegisterAction<bit<16>, bit<32>, bit<16>>(r_cpr4) ra_cpr4 = {
    void apply(inout bit<16> mem_cell, out bit<16> cpr) {
        if (eg_md.intermediate_result_cnt == 0) {
            mem_cell = eg_md.pr4;
        } else {
            mem_cell = mem_cell + eg_md.pr4;
        }
        cpr = mem_cell;
    } };
action act_cpr4() {
    eg_md.cpr4 = ra_cpr4.execute(eg_md.idx);
}
table tab_cpr4 {
    size = 1;
    actions = { act_cpr4; }
    const default_action = act_cpr4;
}

Register<bit<16>, bit<32>>(MAX_FLOW_NUM) r_cpr5;
RegisterAction<bit<16>, bit<32>, bit<16>>(r_cpr5) ra_cpr5 = {
    void apply(inout bit<16> mem_cell, out bit<16> cpr) {
        if (eg_md.intermediate_result_cnt == 0) {
            mem_cell = eg_md.pr5;
        } else {
            mem_cell = mem_cell + eg_md.pr5;
        }
        cpr = mem_cell;
    } };
action act_cpr5() {
    eg_md.cpr5 = ra_cpr5.execute(eg_md.idx);
}
table tab_cpr5 {
    size = 1;
    actions = { act_cpr5; }
    const default_action = act_cpr5;
}

action act_choose0() {
    eg_md.tmp_cpr1 = eg_md.cpr0;
    eg_md.tmp_class1 = 0;
    eg_md.tmp_dif1 = eg_md.cpr0 |-| eg_md.t_conf0;
}

action act_choose1() {
    eg_md.tmp_cpr1 = eg_md.cpr1;
    eg_md.tmp_class1 = 1;
    eg_md.tmp_dif1 = eg_md.cpr1 |-| eg_md.t_conf1;
}

action act_choose2() {
    eg_md.tmp_cpr1 = eg_md.cpr2;
    eg_md.tmp_class1 = 2;
    eg_md.tmp_dif1 = eg_md.cpr2 |-| eg_md.t_conf2;
}

action act_choose012() {
    eg_md.final_class = eg_md.tmp_class1;
    eg_md.tmp_dif2 = 0;
}

action act_choose3() {
    eg_md.tmp_dif2 = eg_md.cpr3 |-| eg_md.t_conf3;
    eg_md.tmp_cpr2 = eg_md.cpr3;
    eg_md.tmp_class2 = 3;
}

action act_choose4() {
    eg_md.tmp_dif2 = eg_md.cpr4 |-| eg_md.t_conf4;
    eg_md.tmp_cpr2 = eg_md.cpr4;
    eg_md.tmp_class2 = 4;
}

action act_choose5() {
    eg_md.tmp_dif2 = eg_md.cpr5 |-| eg_md.t_conf5;
    eg_md.tmp_cpr2 = eg_md.cpr5;
    eg_md.tmp_class2 = 5;
}

action act_choose345() {
    eg_md.final_class = eg_md.tmp_class2;
    eg_md.tmp_dif1 = 0;
}

table tab_cmp012 {
    size = 363;
    key = {
        eg_md.cpr0:ternary;
        eg_md.cpr1:ternary;
        eg_md.cpr2:ternary;
    }
    actions = { act_choose0; act_choose1; act_choose2; }
    const entries = {
        #include "tab_cmp012_entries.p4"
    }
}

table tab_cmp345 {
    size = 363;
    key = {
        eg_md.cpr3:ternary;
        eg_md.cpr4:ternary;
        eg_md.cpr5:ternary;
    }
    actions = { act_choose3; act_choose4; act_choose5; }
    const entries = {
        #include "tab_cmp345_entries.p4"
    }
}

table tab_cmp012345 {
    size = 22;
    key = {
        eg_md.tmp_cpr1:ternary;
        eg_md.tmp_cpr2:ternary;
    }
    actions = { act_choose012; act_choose345; }
    const entries = {
        #include "tab_cmp012345_entries.p4"
    }
}