action act_gru0_gru1(bit<9> h) {
    ig_md.hidden_state1[8:0] = h;
}
@ways(1)
@max_actions(1)
table tab_gru0_gru1 {
    size = 4096;
    key = {
        ig_md.ev0: exact @name("ev0");
        ig_md.ev1: exact @name("ev1");
    }
    actions = {
        act_gru0_gru1;
    }
    const default_action = act_gru0_gru1(0);
    idle_timeout = false;
}

action act_gru2(bit<9> h) {
    ig_md.hidden_state2[8:0] = h;
}
@ways(1)
@max_actions(1)
table tab_gru2 {
    size = 32768;
    key = {
        ig_md.hidden_state1[8:0]: exact @name("h1");
        ig_md.ev2: exact @name("ev2");
    }
    actions = {
        act_gru2;
    }
    const default_action = act_gru2(0);
    idle_timeout = false;
}

action act_gru3(bit<9> h) {
    ig_md.hidden_state3[8:0] = h;
    hdr.bridged_md.hidden_state3[8:0] = h;
}
@ways(1)
@max_actions(1)
table tab_gru3 {
    size = 32768;
    key = {
        ig_md.hidden_state2[8:0]: exact @name("h2");
        ig_md.ev3: exact @name("ev3");
    }
    actions = {
        act_gru3;
    }
    const default_action = act_gru3(0);
    idle_timeout = false;
}
