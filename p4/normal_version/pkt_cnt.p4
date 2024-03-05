Register<bit<8>, bit<32>>(MAX_FLOW_NUM) r_pkt_cnt_mod_7;

RegisterAction<bit<8>, bit<32>, bit<8>>(r_pkt_cnt_mod_7) ra_pkt_cnt_mod_7 = {
    void apply(inout bit<8> mem_cell, out bit<8> pkt_cnt_mod_7) {
        pkt_cnt_mod_7 = mem_cell;
        if (mem_cell == 6) {
            mem_cell = 0;
        } else {
            mem_cell = mem_cell + 1;
        }
    } };

action act_pkt_cnt_mod_7() {
    ig_md.pkt_cnt_mod_7 = ra_pkt_cnt_mod_7.execute(ig_md.idx);
}

table tab_pkt_cnt_mod_7 {
    size = 1;
    actions = { act_pkt_cnt_mod_7; }
    const default_action = act_pkt_cnt_mod_7;
}

Register<bit<8>, bit<32>>(MAX_FLOW_NUM) r_pkt_cnt_initial_8;

RegisterAction<bit<8>, bit<32>, bit<8>>(r_pkt_cnt_initial_8) ra_pkt_cnt_initial_8 = {
    void apply(inout bit<8> mem_cell, out bit<8> cnt8) {
        if (ig_md.predicate >= 4) { // timeout
            mem_cell = 0;
        } else {
            if (mem_cell < 7) {
                mem_cell = mem_cell + 1;
            }
        }
        cnt8 = mem_cell;
    } };

action act_pkt_cnt_initial_8() {
    ig_md.pkt_cnt_initial_8 = ra_pkt_cnt_initial_8.execute(ig_md.idx);
}

table tab_pkt_cnt_initial_8 {
    size = 1;
    actions = { act_pkt_cnt_initial_8; }
    const default_action = act_pkt_cnt_initial_8;
}