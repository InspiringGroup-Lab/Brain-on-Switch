action act_embed_length(bit<10> len) {
    ig_md.embedded_length = len;
}

@max_actions(1)
table tab_embed_length {
    size = 1501;
    key = {
        hdr.ipv4.total_len[15:0]: exact @name("ip_len");
    }
    actions = {
        act_embed_length;
    }
    default_action = act_embed_length(0);
    idle_timeout = false;
}

action act_embed_interval(bit<8> interval) {
    ig_md.embedded_interval = interval;
}

@stage(4)
@max_actions(1)
table tab_embed_interval {
    size = 16384; // TODO: check
    key = {
        ig_md.interval_1ns[27:14]: exact @name("ig_md.interval_16384ns"); // TODO: check
    }
    actions = {
        act_embed_interval;
    }
    default_action = act_embed_interval(0xff);
    idle_timeout = false;
}

action act_embed_fc(bit<6> ev) {
    ig_md.ev7 = ev;
}

@stage(5)
@max_actions(1)
table tab_embed_fc {
    size = 262144;
    key = {
        ig_md.embedded_length: exact;
        ig_md.embedded_interval: exact;
    }
    actions = {
        act_embed_fc;
    }
    const default_action = act_embed_fc(0);
    idle_timeout = false;
}
