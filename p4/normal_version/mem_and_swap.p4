Register<bit<8>, bit<32>>(MAX_FLOW_NUM) r_bin0;
RegisterAction<bit<8>, bit<32>, bit<8>>(r_bin0) ra_bin0 = {
    void apply(inout bit<8> mem_cell, out bit<8> bin) {
        bin = mem_cell;
        if (ig_md.pkt_cnt_mod_7 == 0) {
            mem_cell = 2w0 ++ ig_md.ev7;
        }
    } };
action act_bin0() {
    ig_md.tmp0 = ra_bin0.execute(ig_md.idx)[6-1:0];
}
table tab_bin0 {
    size = 1;
    actions = { act_bin0; }
    const default_action = act_bin0;
}

Register<bit<8>, bit<32>>(MAX_FLOW_NUM) r_bin1;
RegisterAction<bit<8>, bit<32>, bit<8>>(r_bin1) ra_bin1 = {
    void apply(inout bit<8> mem_cell, out bit<8> bin) {
        bin = mem_cell;
        if (ig_md.pkt_cnt_mod_7 == 1) {
            mem_cell = 2w0 ++ ig_md.ev7;
        }
    } };
action act_bin1() {
    ig_md.tmp1 = ra_bin1.execute(ig_md.idx)[6-1:0];
}
table tab_bin1 {
    size = 1;
    actions = { act_bin1; }
    const default_action = act_bin1;
}

Register<bit<8>, bit<32>>(MAX_FLOW_NUM) r_bin2;
RegisterAction<bit<8>, bit<32>, bit<8>>(r_bin2) ra_bin2 = {
    void apply(inout bit<8> mem_cell, out bit<8> bin) {
        bin = mem_cell;
        if (ig_md.pkt_cnt_mod_7 == 2) {
            mem_cell = 2w0 ++ ig_md.ev7;
        }
    } };
action act_bin2() {
    ig_md.tmp2 = ra_bin2.execute(ig_md.idx)[6-1:0];
}
table tab_bin2 {
    size = 1;
    actions = { act_bin2; }
    const default_action = act_bin2;
}

Register<bit<8>, bit<32>>(MAX_FLOW_NUM) r_bin3;
RegisterAction<bit<8>, bit<32>, bit<8>>(r_bin3) ra_bin3 = {
    void apply(inout bit<8> mem_cell, out bit<8> bin) {
        bin = mem_cell;
        if (ig_md.pkt_cnt_mod_7 == 3) {
            mem_cell = 2w0 ++ ig_md.ev7;
        }
    } };
action act_bin3() {
    ig_md.tmp3 = ra_bin3.execute(ig_md.idx)[6-1:0];
}
table tab_bin3 {
    size = 1;
    actions = { act_bin3; }
    const default_action = act_bin3;
}

Register<bit<8>, bit<32>>(MAX_FLOW_NUM) r_bin4;
RegisterAction<bit<8>, bit<32>, bit<8>>(r_bin4) ra_bin4 = {
    void apply(inout bit<8> mem_cell, out bit<8> bin) {
        bin = mem_cell;
        if (ig_md.pkt_cnt_mod_7 == 4) {
            mem_cell = 2w0 ++ ig_md.ev7;
        }
    } };
action act_bin4() {
    ig_md.tmp4 = ra_bin4.execute(ig_md.idx)[6-1:0];
}
table tab_bin4 {
    size = 1;
    actions = { act_bin4; }
    const default_action = act_bin4;
}

Register<bit<8>, bit<32>>(MAX_FLOW_NUM) r_bin5;
RegisterAction<bit<8>, bit<32>, bit<8>>(r_bin5) ra_bin5 = {
    void apply(inout bit<8> mem_cell, out bit<8> bin) {
        bin = mem_cell;
        if (ig_md.pkt_cnt_mod_7 == 5) {
            mem_cell = 2w0 ++ ig_md.ev7;
        }
    } };
action act_bin5() {
    ig_md.tmp5 = ra_bin5.execute(ig_md.idx)[6-1:0];
}
table tab_bin5 {
    size = 1;
    actions = { act_bin5; }
    const default_action = act_bin5;
}

Register<bit<8>, bit<32>>(MAX_FLOW_NUM) r_bin6;
RegisterAction<bit<8>, bit<32>, bit<8>>(r_bin6) ra_bin6 = {
    void apply(inout bit<8> mem_cell, out bit<8> bin) {
        bin = mem_cell;
        if (ig_md.pkt_cnt_mod_7 == 6) {
            mem_cell = 2w0 ++ ig_md.ev7;
        }
    } };
action act_bin6() {
    ig_md.tmp6 = ra_bin6.execute(ig_md.idx)[6-1:0];
}
table tab_bin6 {
    size = 1;
    actions = { act_bin6; }
    const default_action = act_bin6;
}

action act_swap0() {
    ig_md.ev0 = ig_md.tmp0;
    ig_md.ev1 = ig_md.tmp1;
    ig_md.ev2 = ig_md.tmp2;
    ig_md.ev3 = ig_md.tmp3;
    ig_md.ev4 = ig_md.tmp4;
    ig_md.ev5 = ig_md.tmp5;
    ig_md.ev6 = ig_md.tmp6;
}

action act_swap1() {
    ig_md.ev0 = ig_md.tmp1;
    ig_md.ev1 = ig_md.tmp2;
    ig_md.ev2 = ig_md.tmp3;
    ig_md.ev3 = ig_md.tmp4;
    ig_md.ev4 = ig_md.tmp5;
    ig_md.ev5 = ig_md.tmp6;
    ig_md.ev6 = ig_md.tmp0;
}

action act_swap2() {
    ig_md.ev0 = ig_md.tmp2;
    ig_md.ev1 = ig_md.tmp3;
    ig_md.ev2 = ig_md.tmp4;
    ig_md.ev3 = ig_md.tmp5;
    ig_md.ev4 = ig_md.tmp6;
    ig_md.ev5 = ig_md.tmp0;
    ig_md.ev6 = ig_md.tmp1;
}

action act_swap3() {
    ig_md.ev0 = ig_md.tmp3;
    ig_md.ev1 = ig_md.tmp4;
    ig_md.ev2 = ig_md.tmp5;
    ig_md.ev3 = ig_md.tmp6;
    ig_md.ev4 = ig_md.tmp0;
    ig_md.ev5 = ig_md.tmp1;
    ig_md.ev6 = ig_md.tmp2;
}

action act_swap4() {
    ig_md.ev0 = ig_md.tmp4;
    ig_md.ev1 = ig_md.tmp5;
    ig_md.ev2 = ig_md.tmp6;
    ig_md.ev3 = ig_md.tmp0;
    ig_md.ev4 = ig_md.tmp1;
    ig_md.ev5 = ig_md.tmp2;
    ig_md.ev6 = ig_md.tmp3;
}

action act_swap5() {
    ig_md.ev0 = ig_md.tmp5;
    ig_md.ev1 = ig_md.tmp6;
    ig_md.ev2 = ig_md.tmp0;
    ig_md.ev3 = ig_md.tmp1;
    ig_md.ev4 = ig_md.tmp2;
    ig_md.ev5 = ig_md.tmp3;
    ig_md.ev6 = ig_md.tmp4;
}

action act_swap6() {
    ig_md.ev0 = ig_md.tmp6;
    ig_md.ev1 = ig_md.tmp0;
    ig_md.ev2 = ig_md.tmp1;
    ig_md.ev3 = ig_md.tmp2;
    ig_md.ev4 = ig_md.tmp3;
    ig_md.ev5 = ig_md.tmp4;
    ig_md.ev6 = ig_md.tmp5;
}

@max_actions(7)
table tab_swap {
    size = 7;
    key = { ig_md.pkt_cnt_mod_7:exact; }
    actions = { act_swap0; act_swap1; act_swap2; act_swap3; act_swap4; act_swap5; act_swap6; }
    const entries = {
        (0):act_swap0(); (1):act_swap1(); (2):act_swap2(); (3):act_swap3(); (4):act_swap4(); (5):act_swap5(); (6):act_swap6();
    }
    const default_action = act_swap0();
}
