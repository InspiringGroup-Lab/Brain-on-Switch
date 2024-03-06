#pragma once

#include "../common.hpp"
#include "dpdkCommon.hpp"
#include "parserWorker.hpp"
#include "poolWorker.hpp"

namespace Model {
    struct PacketMetadata;
#ifdef FLOW_TIME_RECORD
    struct FlowTimeRecordSimple;
#endif
    class ParserWorkerThread;
    class PoolWorkerThread;
    class DeviceConfig;

    struct AnalyzerConfigParam final {

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

        int keep_warm_batch_size = 64;

        string log_dir = "";
        string model_load_path = "../model/classifier/classifier.pt";

        static const size_t ANALYZER_TO_BUFFER_RESULT_ARR_LIM = (1 << 15);
        size_t analyzer_to_buffer_result_arr_size = (1 << 12) - 1;

        AnalyzerConfigParam() = default;
        virtual ~AnalyzerConfigParam() {}
        AnalyzerConfigParam & operator=(const AnalyzerConfigParam &) = delete;
        AnalyzerConfigParam(const AnalyzerConfigParam &) = delete;

        auto inline display_params() const -> void {
            printf("[Model AnalyzerConfigParam Configuration]\n");

            printf("Memory related param:\n");
            printf("Analyzer to Buffer result buffer size: %ld\n",
                    analyzer_to_buffer_result_arr_size);

            stringstream ss;
            ss << "Verbose mode: {";
            if (verbose_mode & INIT) ss << "Init,";
            if (verbose_mode & TRACING) ss << "Tracing,";
            if (verbose_mode & SUMMARY) ss << "Summary,";
            ss << "}";
            printf("%s (Interval %4.2lfs)\n\n", ss.str().c_str(), verbose_interval);

            printf("Keep warm batch size: %d\n", keep_warm_batch_size);

            printf("Load path of model: %s\n", model_load_path.c_str());
        }
    };

    static const map<string, AnalyzerConfigParam::verbose_type> analyzer_verbose_mode_map = {
        {"tracing", 	AnalyzerConfigParam::verbose_type::TRACING},
        {"data_flow", 	AnalyzerConfigParam::verbose_type::DATA_FLOW},
        {"time_detail", AnalyzerConfigParam::verbose_type::TIME_DETAIL},
        {"memory", 	    AnalyzerConfigParam::verbose_type::MEMORY},
        {"summarizing", AnalyzerConfigParam::verbose_type::SUMMARY},
        {"init", 		AnalyzerConfigParam::verbose_type::INIT},
        {"complete", 	AnalyzerConfigParam::verbose_type::ALL}
    };

    class AnalyzerWorkerThread final : public pcpp::DpdkWorkerThread {

        friend class DeviceConfig;

    private:
        shared_ptr<AnalyzerConfigParam> p_analyzer_config;

        // pthread barrier
        pthread_barrier_t *p_start_barrier, *p_stop_barrier;

        // Indicator of stop
        volatile bool m_stop = false;
        
        // Core Id assigned by DPDK
        cpu_core_id_t m_core_id;

        uint8_t magic = 0;

        // statistics
        size_t analyze_entrance_count = 0;
        size_t warmup_entrance_count = 0;
        size_t inference_entrence_count = 0;
        // time of each step
        double_t interval_lock_time = 0;
        double_t interval_classifier_time = 0;
        double_t interval_syn_time = 0;
        double_t interval_warmup_time = 0;
        // batch size
        uint64_t interval_batch_size = 0;

        struct AnalyzerBatchRecord {
            double_t start_time;
            double_t end_time;
            int size;
            AnalyzerBatchRecord(double_t start_time, double_t end_time, int size):
                    start_time(start_time), end_time(end_time), size(size) {}
        };
        vector<AnalyzerBatchRecord> batch_record;

        // The registered PoolWorker
        shared_ptr<PoolWorkerThread> p_pool;

        torch::jit::script::Module model;

        void inference();

    public:
        vector<float> input_vec_c;
        
#ifdef FLOW_TIME_RECORD
        vector<pair<PacketInfo, FlowTimeRecordSimple> > input_item_c;
#else
        vector<PacketInfo> input_item_c;
#endif
        int input_num_c = 0;
#ifdef FLOW_TIME_RECORD
        my_ring<pair<PacketInfo, FlowTimeRecordSimple> > analyzer_to_buffer_result_queue;
#else
        my_ring<PacketInfo> analyzer_to_buffer_result_queue;
#endif
        void bind_pool(const shared_ptr<PoolWorkerThread> _p) {
            p_pool = _p;
        }

        AnalyzerWorkerThread(pthread_barrier_t *p_start_barrier, pthread_barrier_t *p_stop_barrier,
                const shared_ptr<AnalyzerConfigParam> p_p = nullptr) : 
                p_start_barrier(p_start_barrier), p_stop_barrier(p_stop_barrier), p_analyzer_config(p_p) {}

        AnalyzerWorkerThread(pthread_barrier_t *p_start_barrier, pthread_barrier_t *p_stop_barrier, const json & _j) : 
                p_start_barrier(p_start_barrier), p_stop_barrier(p_stop_barrier) { configure_via_json(_j); }

        virtual ~AnalyzerWorkerThread() {}
        AnalyzerWorkerThread & operator=(const AnalyzerWorkerThread &) = delete;
        AnalyzerWorkerThread(const AnalyzerWorkerThread &) = delete;

        virtual bool run(uint32_t coreId) override;

        virtual uint32_t getCoreId() const override { return m_core_id; }

        // Config from json file
        auto configure_via_json(const json & jin) -> bool;

        virtual void stop() override {
            LOGF("Analyzer on core # %d stop.", getCoreId());
            m_stop = true;
            LOGF("Analyzer on core # %2d magic=%d", getCoreId(), magic);
            if (p_analyzer_config->verbose_mode & AnalyzerConfigParam::verbose_type::SUMMARY) {
                if (p_analyzer_config->log_dir != "") {
                    print_batch_record();
                }
            }
        }

        auto print_batch_record() -> bool {
            string file_name = p_analyzer_config->log_dir + "/Analyzer_on_core_" + to_string(m_core_id) + "_batch_record.log";
            std::ofstream fout(file_name, ios::out);
            if (fout.is_open()) {
                fout << batch_record.size() << std::endl;
                for (auto record : batch_record) {
                    fout << std::fixed << setprecision(4) << record.start_time << " " << record.end_time << " " << record.size << std::endl;
                }
                fout.close();
                LOGF("Analyzer on core # %d save batch record to %s.\n", (int)getCoreId(), file_name.c_str());
                return true;
            } else {
                WARNF("Analyzer on core # %d unable to open file to log batch record.", (int)getCoreId());
                return false;
            }
        }

    };
}