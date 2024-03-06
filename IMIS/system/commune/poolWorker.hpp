#pragma once

#include "../common.hpp"
#include "dpdkCommon.hpp"
#include "parserWorker.hpp"
#include "analyzerWorker.hpp"

namespace Model {
    struct Flow5Tuple;
    struct PacketInfo;
    struct PacketMetadata;
#ifdef FLOW_TIME_RECORD
    struct FlowTimeRecordSimple;
#endif
    class ParserWorkerThread;
    class AnalyzerWorkerThread;
    
    struct PoolConfigParam final {

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

        string log_dir = "";

        // preprocess for inference
        size_t batch_size = 256;

        static const size_t MAX_PKT_METADATA_NUM_FETCH_FROM_PARSER = (1 << 16);
        size_t pkt_metadata_num_fetch_from_parser = (1 << 8) - 1;

        PoolConfigParam() = default;
        virtual ~PoolConfigParam() {}
        PoolConfigParam & operator=(const PoolConfigParam &) = delete;
        PoolConfigParam(const PoolConfigParam &) = delete;

        auto inline display_params() const -> void {
            printf("[Model PoolConfigParam Configuration]\n");

            printf("Batch size per call by Analyzer: %ld\n", batch_size);

            printf("Memory related param:\n");
            
            printf("Number of pkt metadata fetch from parser per loop: %ld\n",
                    pkt_metadata_num_fetch_from_parser);

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

    static const map<string, PoolConfigParam::verbose_type> pool_verbose_mode_map = {
        {"tracing", 	PoolConfigParam::verbose_type::TRACING},
        {"data_flow", 	PoolConfigParam::verbose_type::DATA_FLOW},
        {"time_detail", PoolConfigParam::verbose_type::TIME_DETAIL},
        {"memory", 	    PoolConfigParam::verbose_type::MEMORY},
        {"summarizing", PoolConfigParam::verbose_type::SUMMARY},
        {"init", 		PoolConfigParam::verbose_type::INIT},
        {"complete", 	PoolConfigParam::verbose_type::ALL}
    };

    class PoolWorkerThread final : public pcpp::DpdkWorkerThread {

        friend class DeviceConfig;

    private:
        shared_ptr<PoolConfigParam> p_pool_config;

        // pthread barrier
        pthread_barrier_t *p_start_barrier, *p_stop_barrier;
        
        // Core Id assigned by DPDK
        cpu_core_id_t m_core_id;

        // Indicator of stop
        volatile bool m_stop = false;

        // The registered ParserWorker
        shared_ptr<ParserWorkerThread> p_parser = nullptr;
        // The registered AnalyzerWorker
        shared_ptr<AnalyzerWorkerThread> p_analyzer = nullptr;

        size_t m_new_metadata_num = 0;
        shared_ptr<pair<PacketInfo, PacketMetadata>[]> metadata_from_parser;
        
        typedef struct FlowRecord {

            vector< vector<float> > metadata_sequence;

            PacketInfo info;
            uint8_t metadata_num;
            bool closed;
            bool paused;
#ifdef FLOW_TIME_RECORD
            FlowTimeRecordSimple time_record;
#endif
            FlowRecord() {}
            FlowRecord(PacketInfo info) : info(info), metadata_num(0), closed(false), paused(false) {}
        } FlowRecord;
        unordered_map<Flow5Tuple, FlowRecord, hash_Flow5Tuple> flow_record;

        struct FlowPrioCmp {
            bool operator() (pair<uint64_t, Flow5Tuple> a, pair<uint64_t, Flow5Tuple> b) {
                return a.first > b.first;
            }
        };

        priority_queue<pair<uint64_t, Flow5Tuple>, vector<pair<uint64_t, Flow5Tuple> >, FlowPrioCmp> flow_priority;

        struct PoolBatchRecord {
            double_t start_time;
            double_t end_time;
            int size;
            int closed_item;
            int paused_item;
            PoolBatchRecord(double_t start_time, double_t end_time, int size, int closed_item, int paused_item):
                    start_time(start_time), end_time(end_time), size(size), closed_item(closed_item), paused_item(paused_item) {}
        };
        vector<PoolBatchRecord> batch_record;
        
        // for statistics
        // items in pool
        uint64_t metadata_num_in_pool = 0;
        // throughput
        uint64_t interval_fetch_metadata_num = 0;
        uint64_t interval_inpool_metadata_num = 0;
        uint64_t interval_outpool_metadata_num = 0;
        uint64_t interval_tx_metadata_num = 0;
        uint64_t interval_pass_metadata_num = 0;
        // time distribution
        double_t interval_fetch_time = 0;
        double_t interval_inpool_time = 0;
        double_t interval_outpool_time = 0;
        double_t interval_loop_time = 0;
        // outpool batch
        uint64_t interval_outpool_batch_size = 0;
        // function calls
        size_t interval_fetch_entrance_count = 0;
        size_t interval_inpool_entrance_count = 0;
        size_t interval_outpool_entrance_count = 0;
        size_t interval_batch_num = 0;

        // Copy per-packet properties from registered ParserWorkers
        void fetch_from_parser(const shared_ptr<ParserWorkerThread> pt);
        void out_pool();
        void in_pool();

    public:
        volatile bool call_by_analyzer = false;
        volatile bool ready_for_analyzer = false;
        void bind_parser(shared_ptr<ParserWorkerThread> _p) { p_parser = _p; };
        void bind_analyzer(shared_ptr<AnalyzerWorkerThread> _p) { p_analyzer = _p; };
        
        PoolWorkerThread(pthread_barrier_t *p_start_barrier, pthread_barrier_t *p_stop_barrier) : 
                p_start_barrier(p_start_barrier), p_stop_barrier(p_stop_barrier) { }

        PoolWorkerThread(pthread_barrier_t *p_start_barrier, pthread_barrier_t *p_stop_barrier, const json & _j) : 
                p_start_barrier(p_start_barrier), p_stop_barrier(p_stop_barrier) {
            configure_via_json(_j);
        }

        virtual ~PoolWorkerThread() {}
        PoolWorkerThread & operator=(const PoolWorkerThread &) = delete;
        PoolWorkerThread(const PoolWorkerThread &) = delete;

        virtual bool run(uint32_t coreId) override;

        virtual void stop() override {
            LOGF("Pool on core # %d stop.", getCoreId());
            m_stop = true;
            if (p_pool_config->verbose_mode & PoolConfigParam::verbose_type::SUMMARY) {
                if (p_pool_config->log_dir != "") {
                    print_batch_record();
                }
            }
        }

        virtual uint32_t getCoreId() const override { return m_core_id; }

        auto configure_via_json(const json & jin) -> bool;

        auto print_batch_record() -> bool {
            string file_name = p_pool_config->log_dir + "/Pool_on_core_" + to_string(m_core_id) + "_batch_record.log";
            std::ofstream fout(file_name, ios::out);
            if (fout.is_open()) {
                fout << batch_record.size() << std::endl;
                for (auto record : batch_record) {
                    fout << std::fixed << setprecision(4) << record.start_time << " " << record.end_time << " " << record.size 
                            << " " << record.closed_item << " " << record.paused_item << std::endl;
                }
                fout.close();
                LOGF("Pool on core # %d save batch record to %s.\n", (int)getCoreId(), file_name.c_str());
                return true;
            } else {
                WARNF("Pool on core # %d unable to open file to log batch record.", (int)getCoreId());
                return false;
            }
        }
        
    };
}