#pragma once

#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_cycles.h>
#include <rte_lcore.h>
#include <rte_mbuf.h>
#include <rte_byteorder.h>

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <getopt.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>

#include <sys/stat.h>
#include <netinet/in.h>

#include <pcapplusplus/Packet.h>
#include <pcapplusplus/PacketUtils.h>
#include <pcapplusplus/DpdkDevice.h>
#include <pcapplusplus/DpdkDeviceList.h>
#include <pcapplusplus/PcapFileDevice.h>
#include <pcapplusplus/SystemUtils.h>
#include <pcapplusplus/Logger.h>

#include <pcapplusplus/DpdkDevice.h>
#include <pcapplusplus/TcpLayer.h>
#include <pcapplusplus/TablePrinter.h>
#include <pcapplusplus/IPv4Layer.h>
#include <pcapplusplus/UdpLayer.h>
#include <pcapplusplus/Logger.h>

#include <torch/torch.h>
#include <torch/script.h>

#include "../common.hpp"

using namespace std;
using namespace pcpp;

// #define TEST_PARSER_EXTRACT_ALL
// #define FILL_BATCH
#define TEST_BURST
#define FLOW_TIME_RECORD
#define CUDA
#define YATC

namespace Model {
    using nic_queue_id_t = uint16_t;
    using nic_port_id_t = uint16_t;
    using cpu_core_id_t = uint16_t;
    using mem_pool_size_t = uint32_t;
    using parser_queue_assign_t = map<DpdkDevice *, vector<nic_queue_id_t> > ;
    using packet_type_size_t = uint8_t;
    
    const uint8_t METADATA_NUM_PER_FLOW = 5;

    struct DpdkConfig final {

        cpu_core_id_t core_id;

        parser_queue_assign_t nic_queue_list;

        DpdkConfig() : core_id(MAX_NUM_OF_CORES + 1) {}
        virtual ~DpdkConfig() {}
        DpdkConfig & operator=(const DpdkConfig &) = delete;
        DpdkConfig(const DpdkConfig &) = delete;

        void add_nic_queue(parser_queue_assign_t::key_type k, parser_queue_assign_t::mapped_type v) {
            nic_queue_list.emplace(k, v);
        }

        void add_nic_queue(parser_queue_assign_t::value_type p) {
            nic_queue_list.insert(p);
        }

    };

    struct Flow5Tuple final {

        uint32_t src_address;
        uint32_t dst_address;
        uint16_t proto_code;
        uint16_t src_port;
        uint16_t dst_port;

        Flow5Tuple() {};

        explicit Flow5Tuple(uint32_t sa, uint32_t da, uint16_t t, uint16_t sp, uint16_t dp):
                src_address(sa), dst_address(da),  proto_code(t), src_port(sp), dst_port(dp) {}
        
        virtual ~Flow5Tuple() {};
        Flow5Tuple & operator=(const Flow5Tuple &) = default;
        Flow5Tuple(const Flow5Tuple &) = default;

        bool operator == (const Flow5Tuple &t) const {
            return (src_address == t.src_address && dst_address == t.dst_address && proto_code == t.proto_code && 
                src_port == t.src_port && dst_port == t.dst_port) || (dst_address == t.src_address && 
                src_address == t.dst_address && proto_code == t.proto_code && dst_port == t.src_port && src_port == t.dst_port);
        };
    };

    struct PacketInfo final {
        Flow5Tuple id;
        double_t receive_time;
        uint64_t parser_seq;
        uint16_t length;
        packet_type_size_t pkt_type;

        PacketInfo() {}

        explicit PacketInfo(Flow5Tuple id, double_t rt, uint64_t parser_seq, 
                uint16_t length, packet_type_size_t pkt_type) : id(id), receive_time(rt), 
                parser_seq(parser_seq), length(length), pkt_type(pkt_type) {}

        virtual ~PacketInfo() {}
        PacketInfo & operator=(const PacketInfo &) = default;
        PacketInfo(const PacketInfo &) = default;
    };
#ifdef FLOW_TIME_RECORD
    struct FlowTimeRecordSimple final {
        uint64_t parser_seq[METADATA_NUM_PER_FLOW];
        double_t metadata_in_pool_timestamp[METADATA_NUM_PER_FLOW];
        double_t batch_in_analyzer_timestamp;
        double_t result_out_analyzer_timestamp;
        uint8_t recorded_pkt_num;
        FlowTimeRecordSimple() {recorded_pkt_num = 0;}
        void push_no_check(uint64_t seq, double_t ts) {
            parser_seq[recorded_pkt_num] = seq;
            metadata_in_pool_timestamp[recorded_pkt_num] = ts;
            recorded_pkt_num += 1;
        }
    };

    struct FlowTimeRecordComplex final {
        uint64_t parser_seq[METADATA_NUM_PER_FLOW];
        double_t rx_timestamp[METADATA_NUM_PER_FLOW];
        double_t tx_timestamp[METADATA_NUM_PER_FLOW];
        double_t metadata_in_pool_timestamp[METADATA_NUM_PER_FLOW];
        double_t batch_in_analyzer_timestamp[METADATA_NUM_PER_FLOW];
        double_t result_out_analyzer_timestamp[METADATA_NUM_PER_FLOW];
        double_t result_in_buffer_timestamp[METADATA_NUM_PER_FLOW];
        uint8_t recorded_pkt_num;
        FlowTimeRecordComplex() {recorded_pkt_num = 0;}
        void update(const FlowTimeRecordSimple simple, const double_t ts) {
            for (uint8_t i = recorded_pkt_num; i < simple.recorded_pkt_num; i++) {
                parser_seq[i] = simple.parser_seq[i];
                metadata_in_pool_timestamp[i] = simple.metadata_in_pool_timestamp[i];
                batch_in_analyzer_timestamp[i] = simple.batch_in_analyzer_timestamp;
                result_out_analyzer_timestamp[i] = simple.result_out_analyzer_timestamp;
                result_in_buffer_timestamp[i] = ts;
            }
            recorded_pkt_num = simple.recorded_pkt_num;
        }
    };
#endif
    struct PacketMetadata final { // MODEL
        uint8_t hdr[80];
        uint8_t payload[240];
        uint16_t hdr_len;
        uint16_t payload_len;

        PacketMetadata() {};

        explicit PacketMetadata(void* pkt_hdr_start, uint16_t pkt_hdr_len, 
                void* pkt_payload_start, uint16_t pkt_payload_len) {
            hdr_len = min<uint16_t>(pkt_hdr_len, 80);
            memcpy(hdr, pkt_hdr_start, hdr_len);
            payload_len = min<uint16_t>(pkt_payload_len, 240);
            memcpy(payload, pkt_payload_start, payload_len);
        }

        virtual ~PacketMetadata() {};
        PacketMetadata & operator=(const PacketMetadata &) = default;
        PacketMetadata(const PacketMetadata &) = default;
    };
    

    class hash_Flow5Tuple {
    
    public:
        uint32_t operator ()(const Flow5Tuple &t) const {
            return t.src_address ^ t.dst_address ^ t.proto_code ^ t.src_port ^ t.dst_port;
        }
    };

    
    template <typename T> class my_ring {
    private:
        volatile unsigned head;
        volatile unsigned tail;
        unsigned sz;
        T *q;
    public:
        // _sz should be 2^k-1
        bool init(unsigned _sz) {
            sz = _sz;
            if (sz & (sz + 1)) {
                q = nullptr;
                return false;
            }
            q = new(std::nothrow) T[sz + 1];
            head = tail = 0;
            return q != nullptr;
        }
        void destory() {
            // printf("destory\n");
            delete q;
        }
        inline unsigned count() {
            return (tail - head) & sz;
        }
        inline bool full() {
            return ((tail - head) & sz) == sz;
        }
        inline bool empty() {
            return ((tail - head) & sz) == 0;
        }
        inline unsigned remain() {
            return sz - ((tail - head) & sz);
        }
        unsigned enqueue(T ele) {
            if (full()) return 0;
            q[tail] = ele;
            tail = (tail + 1) & sz;
            return 1;
        }
        unsigned dequeue(T ele[], unsigned cnt) {
            unsigned tmp = count();
            if (tmp < cnt) return 0;
            int part1 = sz + 1 - head;
            
            if (cnt > part1) {
                memcpy(ele, q + head, part1 * sizeof(T));
                memcpy(ele + part1, q, (cnt - part1) * sizeof(T));
            } else {
                memcpy(ele, q + head, cnt * sizeof(T));
            }
            head = (head + cnt) & sz;
            return cnt;
        }
        unsigned enqueue_bulk(T ele[], unsigned cnt) {
            unsigned tmp = count();
            if (cnt + tmp > sz) return 0;
            
            int part1 = sz + 1 - tail;
            if (cnt > part1) {
                memcpy(q + tail, ele, part1 * sizeof(T));
                memcpy(q, ele + part1, (cnt - part1) * sizeof(T));
            } else {
                memcpy(q + tail, ele, cnt * sizeof(T));
            }

            tail = (tail + cnt) & sz;
            return cnt;
        }
    };

    struct XorShift32 final {
    private:
        uint32_t x;
    public:
        XorShift32(uint32_t init) : x(init) {}
        uint32_t get() {
            x ^= x << 13;
            x ^= x >> 17;
            x ^= x << 5;
            return x;
        }
    };
}