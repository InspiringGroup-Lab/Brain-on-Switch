#pragma once

#include "dpdkCommon.hpp"
#include "analyzerWorker.hpp"
#include "bufferWorker.hpp"
#include "parserWorker.hpp"
#include "poolWorker.hpp"

#define DISP_PARAM

namespace Model {

    class AnalyzerWorkerThread;
    class BufferWorkerThread;
    class ParserWorkerThread;
    class PoolWorkerThread;
    
    struct DeviceConfigParam final {

        // Number of NIC input and output queue
        nic_queue_id_t number_rx_queue = 8;
        nic_queue_id_t number_tx_queue = 8;
        mem_pool_size_t mbuf_pool_size = 4096 * 20 - 1;

        // DPDK CPU core config
        cpu_core_id_t core_num_for_analyzer = 8;
        cpu_core_id_t core_num_for_buffer = 8;
        cpu_core_id_t core_num_for_parser = 8;
        cpu_core_id_t core_num_for_pool = 8;
        cpu_core_id_t core_num = 33;

        vector<cpu_core_id_t> core_list_for_analyzer {1, 2, 3, 4, 5, 6, 7, 8};
        vector<cpu_core_id_t> core_list_for_buffer {9, 10, 11, 12, 13, 14, 15, 16};
        vector<cpu_core_id_t> core_list_for_parser {17, 18, 19, 20, 21, 22, 23, 24};
        vector<cpu_core_id_t> core_list_for_pool {25, 26, 29, 28, 29, 30, 31, 32};
        cpu_core_id_t master_core = 0;

        vector<nic_port_id_t> dpdk_port_vec;
        nic_port_id_t output_port = 0;

        vector<string> eal_param_vec {};

        auto inline display_params() const -> void {
            printf("[Model Device Configuration]\n");

            printf("Num. NIC RX queue: %d, Num. NIC TX queue: %d, NIC Mbuf pool size: %d.\n",
                    number_rx_queue, number_tx_queue, mbuf_pool_size);

            stringstream ss;
            ss << "Used NIC Port for DPDK: [";
            for (const auto & id : dpdk_port_vec) {
                ss << static_cast<int>(id) << ", ";
            }
            ss << "]" << endl;

            ss << "Used Output NIC Port for DPDK: ";
            ss << static_cast<int>(output_port) << endl;

            uint32_t i = 0;
            ss << "Eal params for DPDK: ";
            for (auto s : eal_param_vec)
                ss << s << " ";
            printf("%s\n", ss.str().c_str());
            
            printf("Num. Core packet parsing: %d, Num. Core analyze: %d. [Sum core used: %d]\n\n"
                    , core_num_for_analyzer, core_num_for_parser, core_num);
        }

        DeviceConfigParam() {}
        virtual ~DeviceConfigParam() {}
        DeviceConfigParam & operator=(const DeviceConfigParam &) = delete;
        DeviceConfigParam(const DeviceConfigParam &) = delete;
    
    };

    struct ThreadStateManagement final {

        bool stop = true;

        vector<shared_ptr<AnalyzerWorkerThread> > analyzer_worker_thread_vec;
        vector<shared_ptr<BufferWorkerThread> > buffer_worker_thread_vec;
        vector<shared_ptr<ParserWorkerThread> > parser_worker_thread_vec;
        vector<shared_ptr<PoolWorkerThread> > pool_worker_thread_vec;

        pthread_barrier_t *p_stop_barrier;

        ThreadStateManagement() = default;
        virtual ~ThreadStateManagement() {}
        ThreadStateManagement & operator=(const ThreadStateManagement &) = default;
        ThreadStateManagement(const ThreadStateManagement &) = default;

        ThreadStateManagement(const decltype(analyzer_worker_thread_vec) & _analyzer_vec,
                const decltype(buffer_worker_thread_vec) & _buffer_vec,
                const decltype(parser_worker_thread_vec) & _parser_vec,
                const decltype(pool_worker_thread_vec) & _pool_vec,
                pthread_barrier_t *p_stop_barrier) : 
                analyzer_worker_thread_vec(_analyzer_vec), 
                buffer_worker_thread_vec(_buffer_vec), 
                parser_worker_thread_vec(_parser_vec), 
                pool_worker_thread_vec(_pool_vec),
                stop(false),
                p_stop_barrier(p_stop_barrier) {}

    };

    class DeviceConfig final {
    
    private:

        using device_list_t = vector<DpdkDevice*>;
        using assign_queue_t = vector<shared_ptr<DpdkConfig> >;

        shared_ptr<const DeviceConfigParam> p_configure_param;

        // Show configuration details
        bool verbose = true;
        mutable bool dpdk_init_once = false;

        // 3 helper for do_init
        auto configure_dpdk_nic(const CoreMask mask_all_used_core, const uint8_t master_core) const -> device_list_t;

        auto assign_queue_to_parser(const device_list_t & dev_list, 
                const vector<SystemCore> & cores_parser) const -> assign_queue_t;

        auto create_worker_threads(const assign_queue_t & queue_assign,
                vector<shared_ptr<AnalyzerWorkerThread> > & analyzer_thread_vec,
                vector<shared_ptr<BufferWorkerThread> > & buffer_thread_vec,
                vector<shared_ptr<ParserWorkerThread> > & parser_thread_vec,
                vector<shared_ptr<PoolWorkerThread> > & pool_thread_vec,
                pthread_barrier_t *p_start_barrier, pthread_barrier_t *p_stop_barrier) -> bool;

        static void interrupt_callback(void* cookie);

        json j_cfg_analyzer;
        json j_cfg_buffer;
        json j_cfg_parser;
        json j_cfg_pool;

    public:
        
        // Default constructor
        explicit DeviceConfig() {
            LOGF("Device configure uses default parameters");
        }

        explicit DeviceConfig(const decltype(p_configure_param) _p): p_configure_param(_p) {
            LOGF("Device configure uses specific parameters");
        }

        explicit DeviceConfig(const json & jin) {
            if (configure_via_json(jin)) {
                LOGF("Device configure uses json");
            } else {
                LOGF("Json object invalid");
            }
        }

        virtual ~DeviceConfig() {};
        DeviceConfig & operator=(const DeviceConfig &) = delete;
        DeviceConfig(const DeviceConfig &) = delete;

        // List all of avaliable DPDK ports
        // void list_dpdk_ports() const;

        // Do init after all configures are done
        void do_init();

        // Config from json file
        auto configure_via_json(const json & jin) -> bool;
    };
}