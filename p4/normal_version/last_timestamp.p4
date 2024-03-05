Register<bit<32>, bit<32>>(MAX_FLOW_NUM) r_last_timestamp_1ns;

RegisterAction<bit<32>, bit<32>, bit<32>>(r_last_timestamp_1ns) ra_last_timestamp_1ns = {
    void apply(inout bit<32> mem_cell, out bit<32> ts) {
        ts = mem_cell;
        mem_cell = ig_md.timestamp_1ns;
    } };

action act_last_timestamp_1ns() {
    ig_md.last_timestamp_1ns = ra_last_timestamp_1ns.execute(ig_md.idx);
}

@stage(2)
table tab_last_timestamp_1ns {
    size = 1;
    actions = { act_last_timestamp_1ns; }
    const default_action = act_last_timestamp_1ns;
}
