#include "headers.p4"
/* Only an example from NetBeacon */
/* Set your models/parameters HERE */

#define FEAT1_ENCODE_WIDTH 2
#define FEAT4_ENCODE_WIDTH 8
#define FEAT5_ENCODE_WIDTH 144
#define FEAT7_ENCODE_WIDTH 64
#define FEAT9_ENCODE_WIDTH 24

#define NUM_FEAT1_ENCODE_ENTRY 2
#define NUM_FEAT4_ENCODE_ENTRY 8
#define NUM_FEAT5_ENCODE_ENTRY 256
#define NUM_FEAT7_ENCODE_ENTRY 90
#define NUM_FEAT9_ENCODE_ENTRY 30
#define NUM_PKT_TREE_ENTRY 390

struct per_packet_metadata_t {
    bit<FEAT1_ENCODE_WIDTH> feat1_encode;
    bit<FEAT4_ENCODE_WIDTH> feat4_encode;
    bit<FEAT5_ENCODE_WIDTH> feat5_encode;
    bit<FEAT7_ENCODE_WIDTH> feat7_encode;
    bit<FEAT9_ENCODE_WIDTH> feat9_encode;
}

control PerPacket(out per_packet_metadata_t md,
        in bit<4> ihl,
        in bit<4> tcp_data_offset,
        in bit<16> ip_len,
        in bit<8> ttl,
        in bit<8> diffserv,
        out bit<4> per_packet_class
        ) {
    
    action feat1_hit(bit<FEAT1_ENCODE_WIDTH> ind) {md.feat1_encode = ind;}
    table Feat1 {
        key = {ihl: ternary;}
        actions = {feat1_hit;}
        size = NUM_FEAT1_ENCODE_ENTRY; 
        const default_action = feat1_hit(0);
    }
    action feat4_hit(bit<FEAT4_ENCODE_WIDTH> ind) {md.feat4_encode = ind;}
    table Feat4 {
        key = {tcp_data_offset: ternary;}
        actions = {feat4_hit;}
        size = NUM_FEAT4_ENCODE_ENTRY; 
        const default_action = feat4_hit(0);
    }
    action feat5_hit(bit<FEAT5_ENCODE_WIDTH> ind) {md.feat5_encode = ind;}
    table Feat5 {
        key = {ip_len: ternary;}
        actions = {feat5_hit;}
        size = NUM_FEAT5_ENCODE_ENTRY;
        const default_action = feat5_hit(0);
    }
    action feat7_hit(bit<FEAT7_ENCODE_WIDTH> ind) {md.feat7_encode = ind;}
    table Feat7 {
        key = {ttl: ternary;}
        actions = {feat7_hit;}
        size = NUM_FEAT7_ENCODE_ENTRY; 
        const default_action = feat7_hit(0);
    }
    action feat9_hit(bit<FEAT9_ENCODE_WIDTH> ind) {md.feat9_encode = ind;}
    table Feat9 {
        key = {diffserv: ternary;}
        actions = {feat9_hit;}
        size = NUM_FEAT9_ENCODE_ENTRY;
        const default_action = feat9_hit(0);
    }

    action Set_Class_Pkt_Tree(bit<4> result) {
        per_packet_class = result;
    }
    table Pkt_Tree {
        key = {
            md.feat1_encode: ternary;
            md.feat4_encode: ternary;
            md.feat5_encode: ternary;
            md.feat7_encode: ternary;
            md.feat9_encode: ternary;
        }
        actions = {
            Set_Class_Pkt_Tree;
        }
        size = NUM_PKT_TREE_ENTRY;
        const default_action = Set_Class_Pkt_Tree(0);
    }
    
    apply {
        Feat1.apply();
        Feat4.apply();
        Feat5.apply();
        Feat7.apply();
        Feat9.apply();
        Pkt_Tree.apply();
    }
}
