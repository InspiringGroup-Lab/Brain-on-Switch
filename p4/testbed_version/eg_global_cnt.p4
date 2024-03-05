Register<bit<32>, _>(1) r_eg_global_pkt_cnt;
RegisterAction<bit<32>, _, void>(r_eg_global_pkt_cnt) ra_eg_global_pkt_cnt = {
    void apply(inout bit<32> mem_cell) {
        mem_cell = mem_cell + 1;
    } };
action act_eg_global_pkt_cnt() {
    ra_eg_global_pkt_cnt.execute(0);
}
table tab_eg_global_pkt_cnt {
    size = 1;
    actions = { act_eg_global_pkt_cnt; }
    const default_action = act_eg_global_pkt_cnt;
}

Register<bit<32>, _>(1) r_eg_global_to_mirror_pkt_cnt;
RegisterAction<bit<32>, _, void>(r_eg_global_to_mirror_pkt_cnt) ra_eg_global_to_mirror_pkt_cnt = {
    void apply(inout bit<32> mem_cell) {
        mem_cell = mem_cell + 1;
    } };
action act_eg_global_to_mirror_pkt_cnt() {
    ra_eg_global_to_mirror_pkt_cnt.execute(0);
}
table tab_eg_global_to_mirror_pkt_cnt {
    size = 1;
    actions = { act_eg_global_to_mirror_pkt_cnt; }
    const default_action = act_eg_global_to_mirror_pkt_cnt;
}
