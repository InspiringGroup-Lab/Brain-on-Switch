Register<bit<32>, _>(1) r_ig_global_pkt_cnt;
RegisterAction<bit<32>, _, bit<32>>(r_ig_global_pkt_cnt) ra_ig_global_pkt_cnt = {
    void apply(inout bit<32> mem_cell, out bit<32> cnt) {
        cnt = mem_cell;
        mem_cell = mem_cell + 1;
    } };
action act_ig_global_pkt_cnt() {
    ig_md.global_pkt_cnt = ra_ig_global_pkt_cnt.execute(0)[7:0];
}
table tab_ig_global_pkt_cnt {
    size = 1;
    actions = { act_ig_global_pkt_cnt; }
    const default_action = act_ig_global_pkt_cnt;
}

action act_global_collision_escalation_threshold(bit<8> threshold) {
    ig_md.global_collision_escalation_threshold = threshold;
}

table tab_global_collision_escalation_threshold {
    size = 256;
    key = {
        ig_md.global_pkt_cnt: exact;
    }
    actions = {
        act_global_collision_escalation_threshold;
    }
}

Register<bit<8>, _>(1) r_global_collision_escalation_cnt;
RegisterAction<bit<8>, _, bit<4>>(r_global_collision_escalation_cnt) ra_global_collision_escalation_cnt = {
    void apply(inout bit<8> mem_cell, out bit<4> result) {
        result = this.predicate(ig_md.predicate == 2, mem_cell < ig_md.global_collision_escalation_threshold);
        if (mem_cell < ig_md.global_collision_escalation_threshold) {
            if (ig_md.predicate == 2) { // collision
                mem_cell = mem_cell + 1; // IMIS, result=8
            } // else result=4, nothing
        } else {
            mem_cell = ig_md.global_collision_escalation_threshold;
            // result=2, perpacket
            // result=1, nothing
        }
    } };
action act_global_collision_escalation_cnt() {
    ig_md.collision_escalation_predicate = ra_global_collision_escalation_cnt.execute(0);
}
table tab_global_collision_escalation_cnt {
    size = 1;
    actions = { act_global_collision_escalation_cnt; }
    const default_action = act_global_collision_escalation_cnt;
}

Register<bit<32>, _>(1) r_ig_global_recir_pkt_cnt;
RegisterAction<bit<32>, _, void>(r_ig_global_recir_pkt_cnt) ra_ig_global_recir_pkt_cnt = {
    void apply(inout bit<32> mem_cell) {
        mem_cell = mem_cell + 1;
    } };
action act_ig_global_recir_pkt_cnt() {
    ra_ig_global_recir_pkt_cnt.execute(0);
}
table tab_ig_global_recir_pkt_cnt {
    size = 1;
    actions = { act_ig_global_recir_pkt_cnt; }
    const default_action = act_ig_global_recir_pkt_cnt;
}
