/*
struct flow_info {
    bit<32> timestamp;
    bit<32> id;
}
*/

Register<flow_info, bit<32>>(MAX_FLOW_NUM) r_flow_info;

RegisterAction<flow_info, bit<32>, bit<4>>(r_flow_info) ra_flow_info = {
    void apply(inout flow_info mem_cell, out bit<4> result) {
        result = this.predicate(ig_md.id != mem_cell.id, ig_md.timestamp_65536ns - mem_cell.timestamp > TIMEOUT);
        if (ig_md.timestamp_65536ns - mem_cell.timestamp > TIMEOUT) {
            if (ig_md.id != mem_cell.id) {
                mem_cell.timestamp = ig_md.timestamp_65536ns;
                mem_cell.id = ig_md.id;
                //result = 8; // timeout & replace
            } else {
                mem_cell.timestamp = ig_md.timestamp_65536ns;
                //result = 4; // timeout & reset
            }
        } else {
            if (ig_md.id != mem_cell.id) {
                //result = 2; // skip & per-packet
            } else {
                mem_cell.timestamp = ig_md.timestamp_65536ns;
                //result = 1; // normal rnn
            }
        }
    } };

action act_flow_info() {
    ig_md.predicate = ra_flow_info.execute(ig_md.idx);
}

@stage(1)
table tab_flow_info {
    size = 1;
    actions = { act_flow_info; }
    const default_action = act_flow_info;
}