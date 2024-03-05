action act_t_esc(bit<8> t_esc) {
    eg_md.t_esc = t_esc;
}

table tab_t_esc {
    size = 1;
    actions = {
        act_t_esc;
    }
    default_action = act_t_esc(60);
}

Register<bit<8>, bit<32>>(MAX_FLOW_NUM) r_ambiguous_cnt;

RegisterAction<bit<8>, bit<32>, bit<8>>(r_ambiguous_cnt) ra_ambiguous_cnt = {
    void apply(inout bit<8> mem_cell, out bit<8> need_escalation) {
        if (mem_cell + 1 < eg_md.t_esc) {
            // if (eg_md.timeout == 1) {
            if (eg_md.code[0:0] == 1) {
                mem_cell = 0;
            } else {
                mem_cell = mem_cell |+| 1;
            }
            need_escalation = 0; // if timeout, this value won't be used
        } else {
            // if (eg_md.timeout == 1) {
            if (eg_md.code[0:0] == 1) {
                mem_cell = 0;
            } else {
                mem_cell = mem_cell |+| 1;
            }
            need_escalation = 1; // if timeout, this value won't be used
        }
    }};

action act_ambiguous_cnt() {
    eg_md.need_escalation = ra_ambiguous_cnt.execute(eg_md.idx);
}

table tab_ambiguous_cnt {
    size = 1;
    actions = {
        act_ambiguous_cnt;
    }
    const default_action = act_ambiguous_cnt();
}