#pragma once

#include "../common.hpp"
#include "dpdkCommon.hpp"
#include "parserWorker.hpp"
#include "analyzerWorker.hpp"

namespace Model {
    struct PacketInfo;
#ifdef FLOW_TIME_RECORD
    struct FlowTimeRecordSimple;
    struct FlowTimeRecordComplex;
#endif
    class ParserWorkerThread;
    class AnalyzerWorkerThread;

    struct BufferConfigParam final {

        using verbose_mode_t = uint8_t;
        enum verbose_type : verbose_mode_t {
            NONE 	    = 0x0,
            INIT 	    = 0x1,
            SUMMARY     = 0x2,
            TRACING     = 0x4,
            MEMORY      = 0x8,
            DATA_FLOW   = 0x10,
            TIME_DETAIL = 0x20,
            ALL 	    = 0x3f
        };

        double_t verbose_interval = 5.0;
        verbose_mode_t verbose_mode = NONE;
        
        static const size_t MAX_PKT_NUM_FETCH_FROM_PARSER = (1 << 16);
        size_t pkt_num_fetch_from_parser = (1 << 10) - 1;
        static const size_t MAX_RESULT_NUM_FETCH_FROM_ANALYZER = (1 << 16);
        size_t result_num_fetch_from_analyzer = (1 << 9) - 1;
        static const size_t MAX_BUFFER_SIZE_PER_FLOW = (1 << 26);
        size_t buffer_size_per_flow = (1 << 10) - 1;

        double_t buffer_timeout = 0;

        string log_dir = "";

        BufferConfigParam() = default;
        virtual ~BufferConfigParam() {}
        BufferConfigParam & operator=(const BufferConfigParam &) = delete;
        BufferConfigParam(const BufferConfigParam &) = delete;

        auto inline display_params() const -> void {
            printf("[Model BufferConfigParam Configuration]\n");

            printf("Number of pkts fetch from parser per loop: %ld\n", pkt_num_fetch_from_parser);
            printf("Number of results fetch from analyzer per loop: %ld\n", result_num_fetch_from_analyzer);
            
            printf("Memory related param:\n");
            printf("Buffer size per flow: %ld\n", buffer_size_per_flow);

            printf("Buffer timeout: %.3lf\n", buffer_timeout);

            stringstream ss;
            ss << "Verbose mode: {";
            if (verbose_mode & INIT) ss << "Init,";
            if (verbose_mode & TRACING) ss << "Tracing,";
            if (verbose_mode & SUMMARY) ss << "Summary,";
            if (verbose_mode & MEMORY) ss << "Memory,";
            if (verbose_mode & DATA_FLOW) ss << "Data flow,";
            if (verbose_mode & TIME_DETAIL) ss << "Time detail,";
            ss << "}";
            printf("%s (Interval %4.2lfs)\n\n", ss.str().c_str(), verbose_interval);
        }
    };

    static const map<string, BufferConfigParam::verbose_type> buffer_verbose_mode_map = {
        {"tracing", 	BufferConfigParam::verbose_type::TRACING},
        {"data_flow", 	BufferConfigParam::verbose_type::DATA_FLOW},
        {"time_detail", BufferConfigParam::verbose_type::TIME_DETAIL},
        {"memory", 	    BufferConfigParam::verbose_type::MEMORY},
        {"summarizing", BufferConfigParam::verbose_type::SUMMARY},
        {"init", 		BufferConfigParam::verbose_type::INIT},
        {"complete", 	BufferConfigParam::verbose_type::ALL}
    };

    class BufferWorkerThread final : public pcpp::DpdkWorkerThread {

        friend class DeviceConfig;
        
    private:
        shared_ptr<BufferConfigParam> p_buffer_config;
        
        // pthread barrier
        pthread_barrier_t *p_start_barrier, *p_stop_barrier;
        
        // Core Id assigned by DPDK
        cpu_core_id_t m_core_id;

        // Indicator of stop
        volatile bool m_stop = false;
        
        // The registered Analyzer
        shared_ptr<AnalyzerWorkerThread> p_analyzer = nullptr;
        // The registered Parser
        shared_ptr<ParserWorkerThread> p_parser = nullptr;

        nic_queue_id_t tx_queue_id = 0;
        nic_port_id_t tx_port_id = 0;
#ifdef TEST_BURST
        size_t m_record_num = 0;
        double_t records[100000000];
#endif
        // unordered_set<Flow5Tuple, hash_Flow5Tuple> flow_set;

        size_t m_new_result_num = 0;
        size_t m_new_pkt_num = 0;
#ifdef FLOW_TIME_RECORD
        shared_ptr<pair<PacketInfo, FlowTimeRecordSimple>[]> result_from_analyzer;
#else
        shared_ptr<PacketInfo[]> result_from_analyzer;
#endif
        shared_ptr<pair<PacketInfo, rte_mbuf *>[]> pkt_from_parser;

        typedef struct FlowBuffer {
            queue<pair<PacketInfo, rte_mbuf *> > pkt_queue;
            uint32_t pkt_num_in_queue;
            uint64_t analyzed_parser_seq;
            packet_type_size_t pkt_type;
#ifdef FLOW_TIME_RECORD
            FlowTimeRecordComplex time_record;
            uint8_t time_record_idx;
#endif
            FlowBuffer() {
                pkt_queue = queue<pair<PacketInfo, rte_mbuf *> >();
                pkt_num_in_queue = 0;
                analyzed_parser_seq = 0;
                pkt_type = 0;
            }
            auto front() {
                return pkt_queue.front();
            }
            void pop() {
                pkt_queue.pop();
                pkt_num_in_queue -= 1;
            }
            void push(pair<PacketInfo, rte_mbuf *> pkt) {
                pkt_queue.push(pkt);
                pkt_num_in_queue += 1;
            }
        } FlowBuffer;
        // Records table for flow sequences
        unordered_map<Flow5Tuple, FlowBuffer, hash_Flow5Tuple> flow_buffer;

        // for statistics
        // items in buffer
        uint64_t pkt_num_in_buffer = 0;

        uint64_t interval_max_pkt_num_in_buffer = 0;
        uint64_t interval_max_pkt_num_in_queue = 0;

        // throughput
        uint64_t interval_fetch_pkt_num = 0;
        uint64_t interval_fetch_result_num = 0;

        uint64_t interval_send_pkt_num = 0;
        uint64_t interval_send_pkt_len = 0;
        uint64_t interval_drop_pkt_num = 0;
        uint64_t interval_pass_pkt_num = 0;

        // time distribution
        double_t interval_fetch_pkt_time = 0;
        double_t interval_fetch_result_time = 0;
        double_t interval_send_pkt_time = 0;
        double_t interval_loop_time = 0;

        size_t interval_fetch_pkt_entrance_count = 0;
        size_t interval_fetch_result_entrance_count = 0;
        size_t interval_send_pkt_entrance_count = 0;

        // waiting time for pkts
        double_t interval_send_pkt_waiting_time = 0;

        // Get packet from registered ParserWorker
        void fetch_pkt_from_parser(const shared_ptr<ParserWorkerThread> pt);
        // Get result from registered AnalyzerWorker
        void fetch_result_from_analyzer(const shared_ptr<AnalyzerWorkerThread> pt);
        // Send packets with result from registered AnalyzerWorker
        void flush_buffer_with_new_result();
        // Send single packet
        bool send(rte_mbuf *mbuf, PacketInfo info, double_t t);

    public:
        void bind_parser(const shared_ptr<ParserWorkerThread> _p) { p_parser = _p; }
        void bind_tx_device(const nic_port_id_t _port_id, const nic_queue_id_t _queue_id) {
            tx_port_id = _port_id;
            tx_queue_id = _queue_id;
        }
        void bind_analyzer(const shared_ptr<AnalyzerWorkerThread> _p) {
            p_analyzer = _p;
        }
        
        BufferWorkerThread(pthread_barrier_t *p_start_barrier, pthread_barrier_t *p_stop_barrier): 
                p_start_barrier(p_start_barrier), p_stop_barrier(p_stop_barrier) { }

        BufferWorkerThread(pthread_barrier_t *p_start_barrier, pthread_barrier_t *p_stop_barrier, 
                const json & _j): p_start_barrier(p_start_barrier), p_stop_barrier(p_stop_barrier) {
            configure_via_json(_j);
        }

        virtual ~BufferWorkerThread() {}
        BufferWorkerThread & operator=(const BufferWorkerThread &) = delete;
        BufferWorkerThread(const BufferWorkerThread &) = delete;

        virtual bool run(uint32_t coreId) override;

        virtual void stop() override {
            LOGF("Buffer on core # %d stop.", getCoreId());
            m_stop = true;

            if (p_buffer_config->verbose_mode & BufferConfigParam::verbose_type::SUMMARY) {
                if (p_buffer_config->log_dir != "") {
#ifdef TEST_BURST
                    print_long_record();
#endif
#ifdef FLOW_TIME_RECORD
                    print_time_detail();
#endif
                } else {
                    WARNF("Buffer on core # %d: no log_dir specified.", (int)getCoreId());
                }
            }
        }

        virtual uint32_t getCoreId() const override { return m_core_id; }

        auto configure_via_json(const json & jin) -> bool;
#ifdef TEST_BURST
        auto print_long_record() -> bool {
            string file_name = p_buffer_config->log_dir + "/Buffer_on_core_" + to_string(m_core_id) + ".bin";
            std::ofstream fout(file_name, ios::out | ios::binary);
            if (fout.is_open()) {
                int num = (int)m_record_num;
                fout.write(reinterpret_cast<char*>(&num), sizeof(num));
                fout.write(reinterpret_cast<char*>(records), sizeof(records));
                fout.close();
                LOGF("Buffer on core # %d save long result to %s.\n", (int)getCoreId(), file_name.c_str());
                return true;
            } else {
                WARNF("Buffer on core # %d unable to open file to log long result.", (int)getCoreId());
                return false;
            }
        }
#endif
#ifdef FLOW_TIME_RECORD
        auto print_time_detail() -> bool {
            string file_name = p_buffer_config->log_dir + "/Buffer_on_core_" + to_string(m_core_id) + "_time_detail.log";
            std::ofstream fout(file_name, ios::out);
            if (fout.is_open()) {

                int num = 0;
#ifdef TEST_BURST
                for (auto &kv : flow_buffer) { // test only
                    if (kv.first.dst_port != 0) {
                        num += 1;
                    }
                }
#else
                num = flow_buffer.size();
#endif
                fout << num << std::endl;
                for (auto &kv : flow_buffer) {
#ifdef TEST_BURST
                    if (kv.first.dst_port != 0) { // test only
#endif
                        int cnt = (int)kv.second.time_record_idx;
                        FlowTimeRecordComplex &time_record = kv.second.time_record;
                        fout << cnt << std::endl;
                        for (int i = 0; i < cnt; i++) {
                            fout << time_record.metadata_in_pool_timestamp[i] - time_record.rx_timestamp[i] << " " 
                                    << time_record.batch_in_analyzer_timestamp[i] - time_record.metadata_in_pool_timestamp[i] << " "
                                    << time_record.result_out_analyzer_timestamp[i] - time_record.batch_in_analyzer_timestamp[i] << " " 
                                    << time_record.result_in_buffer_timestamp[i] - time_record.result_out_analyzer_timestamp[i] << " "
                                    << time_record.tx_timestamp[i] - time_record.result_in_buffer_timestamp[i] << std::endl;
                        }
#ifdef TEST_BURST
                    }
#endif
                }
                fout.close();
                LOGF("Buffer on core # %d save time detail to %s.\n", (int)getCoreId(), file_name.c_str());
                return true;
            } else {
                WARNF("Buffer on core # %d unable to open file to log time detail.", (int)getCoreId());
                return false;
            }
        }
#endif
    };
}