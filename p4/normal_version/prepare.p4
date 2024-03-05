// CRCPolynomial(T coeff, bool reversed, bool msb, bool extended, T init, T xor)

// crc_32c
CRCPolynomial<bit<32>>(32w0x1EDC6F41,
                        true,
                        false,
                        false,
                        32w0x00000000,
                        32w0xFFFFFFFF
                        ) poly_id;
@symmetric("hdr.ipv4.src_addr", "hdr.ipv4.dst_addr")
@symmetric("ig_md.src_port", "ig_md.dst_port")
Hash<bit<32>>(HashAlgorithm_t.CUSTOM, poly_id) hash_id; // crc_32c

action act_get_id() {
    ig_md.id = hash_id.get(
        {hdr.ipv4.src_addr, hdr.ipv4.dst_addr,
        ig_md.src_port, ig_md.dst_port, hdr.ipv4.protocol});
}

@stage(0)
table tab_get_id {
    size = 1;
    actions = { act_get_id; }
    const default_action = act_get_id;
}

// crc_32
CRCPolynomial<bit<32>>(32w0x04C11DB7,
                        true,
                        false,
                        false,
                        32w0x00000000,
                        32w0xFFFFFFFF
                        ) poly_idx;
@symmetric("hdr.ipv4.src_addr", "hdr.ipv4.dst_addr")
@symmetric("ig_md.src_port", "ig_md.dst_port")
Hash<bit<32>>(HashAlgorithm_t.CUSTOM, poly_idx) hash_idx; // crc_32

action act_get_idx() {
    ig_md.idx[MAX_FLOW_NUM_WIDTH-1:0] = hash_idx.get({hdr.ipv4.src_addr, hdr.ipv4.dst_addr,
        ig_md.src_port, ig_md.dst_port, hdr.ipv4.protocol})[MAX_FLOW_NUM_WIDTH-1:0];
}

@stage(0)
table tab_get_idx {
    size = 1;
    actions = { act_get_idx; }
    const default_action = act_get_idx;
}

action act_get_timestamp() {
    ig_md.timestamp_65536ns = ig_prsr_md.global_tstamp[47:16];
    ig_md.timestamp_1ns = ig_prsr_md.global_tstamp[31:0];
}

@stage(0)
table tab_get_timestamp {
    size = 1;
    actions = { act_get_timestamp; }
    const default_action = act_get_timestamp;
}
