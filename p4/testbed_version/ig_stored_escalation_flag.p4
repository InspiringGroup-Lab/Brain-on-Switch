Register<bit<8>, bit<32>>(MAX_FLOW_NUM) r_stored_escalation_flag;

RegisterAction<bit<8>, bit<32>, bit<8>>(r_stored_escalation_flag) ra_stored_escalation_flag = {
    void apply(inout bit<8> mem_cell, out bit<8> stored_escalation_flag) {
        if (ig_md.stored_escalation_flag_update) {
            mem_cell = ig_md.new_escalation_flag;
        }
        stored_escalation_flag = mem_cell;
    } };

action act_stored_escalation_flag() {
    ig_md.stored_escalation_flag = ra_stored_escalation_flag.execute(ig_md.idx);
}

@stage(3)
table tab_stored_escalation_flag {
    size = 1;
    actions = {
        act_stored_escalation_flag;
    }
    const default_action = act_stored_escalation_flag();
}