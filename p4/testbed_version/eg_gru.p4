action act_gru4(bit<9> h) {
    eg_md.hidden_state4[8:0] = h;
}
@ways(1)
@max_actions(1)
table tab_gru4 {
    size = 32768;
    key = {
        eg_md.hidden_state3[8:0]: exact @name("h3");
        eg_md.ev4: exact @name("ev4");
    }
    actions = {
        act_gru4;
    }
    const default_action = act_gru4(0);
    idle_timeout = false;
}

action act_gru5(bit<9> h) {
    eg_md.hidden_state5[8:0] = h;
}
@ways(1)
@max_actions(1)
table tab_gru5 {
    size = 32768;
    key = {
        eg_md.hidden_state4[8:0]: exact @name("h4");
        eg_md.ev5: exact @name("ev5");
    }
    actions = {
        act_gru5;
    }
    const default_action = act_gru5(0);
    idle_timeout = false;
}

action act_gru6(bit<9> h) {
    eg_md.hidden_state6[8:0] = h;
}
@ways(1)
@max_actions(1)
table tab_gru6 {
    size = 32768;
    key = {
        eg_md.hidden_state5[8:0]: exact @name("h5");
        eg_md.ev6: exact @name("ev6");
    }
    actions = {
        act_gru6;
    }
    const default_action = act_gru6(0);
    idle_timeout = false;
}

action act_gru7_output(bit<4> pr0, bit<4> pr1, bit<4> pr2, bit<4> pr3,
        bit<4> pr4, bit<4> pr5) {
    eg_md.pr0 = 12w0 ++ pr0;
    eg_md.pr1 = 12w0 ++ pr1;
    eg_md.pr2 = 12w0 ++ pr2;
    eg_md.pr3 = 12w0 ++ pr3;
    eg_md.pr4 = 12w0 ++ pr4;
    eg_md.pr5 = 12w0 ++ pr5;
}
@ways(1)
@max_actions(1)
table tab_gru7_output {
    size = 32768;
    key = {
        eg_md.hidden_state6[8:0]: exact @name("h6");
        eg_md.ev7: exact @name("ev7");
    }
    actions = {
        act_gru7_output;
    }
    const default_action = act_gru7_output(0, 0, 0, 0, 0, 0);
    idle_timeout = false;
}
