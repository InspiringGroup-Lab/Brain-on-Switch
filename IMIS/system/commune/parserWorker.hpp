#pragma once

#include "../common.hpp"
#include "dpdkCommon.hpp"
#include "deviceConfig.hpp"
#include "analyzerWorker.hpp"

using namespace std;
using namespace pcpp;

namespace Model {
    class DeviceConfig;

    struct ParserConfigParam final {

        using verbose_mode_t = uint8_t;
        enum verbose_type : verbose_mode_t {
            NONE 	    = 0x0,
            INIT 	    = 0x1,
            SUMMARY     = 0x2,
            TRACING     = 0x4,
            MEMORY      = 0x8,
            DATA_FLOW   = 0x10,
            ALL 	    = 0x1f
        };

        double_t verbose_interval = 5.0;
        verbose_mode_t verbose_mode = NONE;

        static const size_t PARSER_TO_POOL_PKT_META_ARR_LIM = (1 << 26);
        size_t parser_to_pool_pkt_metadata_arr_size = (1 << 22) - 1;
        static const size_t PARSER_TO_BUFFER_PKT_ARR_LIM = (1 << 26);
        size_t parser_to_buffer_pkt_arr_size = (1 << 20) - 1;
        static const size_t RECEIVE_BURST_LIM = (1 << 16);
        size_t max_receive_burst = 64;
#ifdef TEST_BURST
        uint32_t test_src_ip = 0;
        uint32_t test_dst_ip = 0;
        uint16_t old_src_port_min = 1;
        uint16_t old_src_port_max = 1;
        uint16_t new_dst_port_min = 1;
        uint16_t new_dst_port_max = 1;
#endif

        ParserConfigParam() = default;
        virtual ~ParserConfigParam() {}
        ParserConfigParam & operator=(const ParserConfigParam &) = delete;
        ParserConfigParam(const ParserConfigParam &) = delete;

        auto inline display_params() const -> void {
            printf("[Model Parser Configuration]\n");

            printf("Memory related param:\n");
            printf("Maximum receive burst: %ld, Parser to Pool metadata buffer size: %ld\n, Parser to Buffer packet buffer size: %ld\n",
                    max_receive_burst, parser_to_pool_pkt_metadata_arr_size, parser_to_buffer_pkt_arr_size);

            stringstream ss;
            ss << "Verbose mode: {";
            if (verbose_mode & INIT) ss << "Init,";
            if (verbose_mode & TRACING) ss << "Tracing,";
            if (verbose_mode & SUMMARY) ss << "Summary,";
            ss << "}";
            printf("%s (Interval %4.2lfs)\n\n", ss.str().c_str(), verbose_interval);
        }
    };

    static const map<string, ParserConfigParam::verbose_type> parser_verbose_mode_map = {
        {"tracing", 	ParserConfigParam::verbose_type::TRACING},
        {"data_flow", 	ParserConfigParam::verbose_type::DATA_FLOW},
        {"memory", 	    ParserConfigParam::verbose_type::MEMORY},
        {"summarizing", ParserConfigParam::verbose_type::SUMMARY},
        {"init", 		ParserConfigParam::verbose_type::INIT},
        {"complete", 	ParserConfigParam::verbose_type::ALL}
    };

    class ParserWorkerThread final : public DpdkWorkerThread {

        friend class DeviceConfig;

    private:

        const shared_ptr<DpdkConfig> p_dpdk_config;
        shared_ptr<ParserConfigParam> p_parser_config;

        pthread_barrier_t *p_start_barrier, *p_stop_barrier;

        volatile bool m_stop = false;

        uint64_t m_parser_seq = 0;

        cpu_core_id_t m_core_id;

        // statistical variables
        mutable vector<uint64_t> parsed_pkt_len;
        mutable vector<uint64_t> parsed_pkt_num;
        mutable vector<uint64_t> sum_parsed_pkt_num;
        mutable vector<uint64_t> sum_parsed_pkt_len;
        mutable double_t parser_start_time, parser_end_time;

        void verbose_final() const;
        void verbose_tracing_thread();

        // Read-Write exclution for per-packet Metadata
        mutable sem_t semaphore;
        void inline acquire_semaphore() const {
            sem_wait(&semaphore);
        }
        void inline release_semaphore() const {
            sem_post(&semaphore);
        }

    public:

        // Collect the per-packets metadata
        my_ring<pair<PacketInfo, PacketMetadata> > parser_to_pool_pkt_metadata_queue;

        my_ring<pair<PacketInfo, rte_mbuf *> > parser_to_buffer_pkt_queue;

        unordered_map<Flow5Tuple, uint8_t, hash_Flow5Tuple> flow_packet_num;

        ParserWorkerThread(const shared_ptr<DpdkConfig> p_d,
                pthread_barrier_t *p_start_barrier,
                pthread_barrier_t *p_stop_barrier,
                const json & j_p): 
                p_dpdk_config(p_d),
                p_start_barrier(p_start_barrier),
                p_stop_barrier(p_stop_barrier) {
            
            if (p_dpdk_config == nullptr) {
                FATAL_ERROR("NULL dpdk configuration for parser.");
            }

            sem_init(&semaphore, 0, 1);

            if (j_p.size()) {
                configure_via_json(j_p);
            }

            sum_parsed_pkt_num.resize(p_d->nic_queue_list.size(), 0);
            sum_parsed_pkt_len.resize(p_d->nic_queue_list.size(), 0);
            parsed_pkt_len.resize(p_d->nic_queue_list.size(), 0);
            parsed_pkt_num.resize(p_d->nic_queue_list.size(), 0);
        }

        ParserWorkerThread(const shared_ptr<DpdkConfig> p_d, 
                pthread_barrier_t *p_start_barrier,
                pthread_barrier_t *p_stop_barrier,
                const shared_ptr<ParserConfigParam> p_p = nullptr
                ): 
                p_dpdk_config(p_d), p_parser_config(p_p),
                p_start_barrier(p_start_barrier),
                p_stop_barrier(p_stop_barrier) {

            if (p_dpdk_config == nullptr) {
                FATAL_ERROR("dpdk configuration not found for parser.");
            }

            sem_init(&semaphore, 0, 1);

            sum_parsed_pkt_num.resize(p_d->nic_queue_list.size(), 0);
            sum_parsed_pkt_len.resize(p_d->nic_queue_list.size(), 0);
            parsed_pkt_len.resize(p_d->nic_queue_list.size(), 0);
            parsed_pkt_num.resize(p_d->nic_queue_list.size(), 0);
        }

        virtual ~ParserWorkerThread() {}
        ParserWorkerThread & operator=(const ParserWorkerThread&) = delete;
        ParserWorkerThread(const ParserWorkerThread&) = delete;

        auto get_overall_performance() const -> pair<double_t, double_t>;

        virtual bool run(uint32_t core_id) override;

        virtual void stop() override {
            LOGF("Parser on core # %d stop.", getCoreId());
            m_stop = true;
            parser_end_time = get_time_spec();
            size_t index = 0;
            for (parser_queue_assign_t::const_iterator ite = cbegin(p_dpdk_config->nic_queue_list); 
                    ite != cend(p_dpdk_config->nic_queue_list); ite ++) {
                sum_parsed_pkt_num[index] += parsed_pkt_num[index];
                sum_parsed_pkt_len[index] += parsed_pkt_len[index];
                
                parsed_pkt_num[index] = 0;
                parsed_pkt_len[index] = 0;
                index ++;
            }
            verbose_final();
        }

        virtual uint32_t getCoreId() const override {
            return m_core_id;
        }

        auto configure_via_json(const json & jin) -> bool;
    };
}
