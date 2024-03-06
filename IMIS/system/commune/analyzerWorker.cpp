#include "analyzerWorker.hpp"

using namespace Model;

static inline auto __get_double_ts() -> double_t {
    struct timeval ts;
    gettimeofday(&ts, nullptr);
    return ts.tv_sec + ts.tv_usec * (1e-6);
}


void AnalyzerWorkerThread::inference() {
    ++ inference_entrence_count;

    torch::NoGradGuard no_grad;
    //////////////////////////////////////////////
    double_t _t_lock_start = __get_double_ts();
    ////////////////////////////////////////////// get pool lock
    p_pool->ready_for_analyzer = false;
    // LOGF("Analyzer on core # %2d set ready_by_pool = false.", getCoreId());
    p_pool->call_by_analyzer = true;
    // LOGF("Analyzer on core # %2d set call_by_analyzer = true.", getCoreId());
    while (!m_stop && !p_pool->ready_for_analyzer) {}
    if (m_stop) return;
    // LOGF("Analyzer on core # %2d waken up.", getCoreId());
    //////////////////////////////////////////////
    interval_lock_time += __get_double_ts() - _t_lock_start;
    interval_batch_size += input_num_c;
    //////////////////////////////////////////////
    double_t _t_classifier_start = __get_double_ts();
    ////////////////////////////////////////////// classifier inference
    
    // std::this_thread::sleep_for(std::chrono::milliseconds(100));
    if (input_num_c > 0) {
        ++ analyze_entrance_count;
        AnalyzerBatchRecord new_record(_t_classifier_start, 0, input_num_c);
#ifdef FLOW_TIME_RECORD
        for (auto &item : input_item_c) {
            item.second.batch_in_analyzer_timestamp = _t_classifier_start;
        }
#endif

    #ifdef FILL_BATCH
        input_vec_c.resize(p_analyzer_config->keep_warm_batch_size * 40 * 40);
    #endif
    #ifdef CUDA
        torch::Tensor input_ten_c = torch::from_blob(input_vec_c.data(), {input_num_c, 1, 40, 40}, // MODEL
                at::kFloat).to(at::kCUDA);
    #else
        torch::Tensor input_ten_c = torch::from_blob(input_vec_c.data(), {input_num_c, 1, 40, 40}, // MODEL
                at::kFloat).to(at::kCPU);
    #endif

        vector<torch::jit::IValue> input_c;
        input_c.push_back(input_ten_c);
        LOGF("Analyzer on core # %2d start model.", getCoreId());

        double_t _t_before = __get_double_ts();
        at::Tensor result = model.forward(input_c).toTensor().to(at::kByte).to(at::kCPU).argmax(1);
        double_t _t_after = __get_double_ts();
        if (_t_after - _t_before > 2.0) {
            WARNF("Analyzer on core # %2d meet bad point %.3lf.", getCoreId(), _t_after - _t_before);
        }

        LOGF("Analyzer on core # %2d finish model.", getCoreId());
        
        int idx_c = 0;

        interval_classifier_time += __get_double_ts() - _t_classifier_start;
        double_t _t_syn_start = __get_double_ts();

        // give result to buffer
        // LOGF("Analyzer on core # %2d start output.", getCoreId());
        for (auto iter = input_item_c.begin(); iter != input_item_c.end(); ++iter, ++idx_c) {
#ifdef FLOW_TIME_RECORD
            iter->first.pkt_type = result[idx_c].item<uint8_t>();
            iter->second.result_out_analyzer_timestamp = _t_syn_start;
#else
            iter->pkt_type = result[idx_c].item<uint8_t>();
#endif       
            if (analyzer_to_buffer_result_queue.enqueue(*iter) == 0) {
                WARNF("Analyzer on core # %2d: analyzer_to_buffer_result_queue overflow.", getCoreId());
            }
        }

        // LOGF("Analyzer on core # %2d finish output.", getCoreId());
        double _t_tmp = __get_double_ts();
        interval_syn_time += _t_tmp - _t_syn_start;
        new_record.end_time = _t_tmp;
        batch_record.push_back(new_record);
    } else {
        // no input_num_c
        ++ warmup_entrance_count;
        double_t _t_warmup_start = __get_double_ts();
    #ifdef CUDA
        // keep warm
        torch::Tensor input_ten_c = torch::randn({p_analyzer_config->keep_warm_batch_size, 1, 40, 40}).to(at::kCUDA);
        vector<torch::jit::IValue> input_c;
        input_c.push_back(input_ten_c);
        at::Tensor result = model.forward(input_c).toTensor().to(at::kByte).to(at::kCPU).argmax(1);
        magic ^= result[0].item<uint8_t>();
    #endif
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        interval_warmup_time += __get_double_ts() - _t_warmup_start;
    }
}


bool AnalyzerWorkerThread::run(uint32_t coreId) {
    m_core_id = coreId;

    torch::NoGradGuard no_grad;

    if (p_pool == nullptr) {
        WARNF("Analyzer on core # %2d: No Pool thread bind.", coreId);
        pthread_barrier_wait(p_start_barrier);
        return false;
    }

    // Deserialize the ScriptModule from a file using torch::jit::load().
    model = torch::jit::load(p_analyzer_config->model_load_path);
#ifdef CUDA
    model.to(at::kCUDA);
#else
    model.to(at::kCPU);
#endif
    model.eval();

    if (!analyzer_to_buffer_result_queue.init(p_analyzer_config->analyzer_to_buffer_result_arr_size)) {
		FATAL_ERROR("error");
		pthread_barrier_wait(p_start_barrier);
		FATAL_ERROR("analyzer_to_buffer_result_queue: bad allocation.");
	}
    
    if (p_analyzer_config->verbose_mode & AnalyzerConfigParam::verbose_type::INIT) LOGF("Analyzer on core # %2d ready.", coreId);
    pthread_barrier_wait(p_start_barrier);
    if (p_analyzer_config->verbose_mode & AnalyzerConfigParam::verbose_type::INIT) LOGF("Analyzer on core # %2d start.", coreId);
    
    if (true) {
        for (int i = 0; i < 2; i++) {
            torch::Tensor input_ten_c = torch::zeros({256, 1, 40, 40}).to(at::kCUDA); // FIXME
            vector<torch::jit::IValue> input_c;
            input_c.push_back(input_ten_c);
            at::Tensor result = model.forward(input_c).toTensor().to(at::kByte).to(at::kCPU).argmax(1);
            magic ^= result[0].item<uint8_t>();
        }
    }
    
    m_stop = false;

    double_t __s = __get_double_ts();

    while(!m_stop) {
        // for performance statistic
        double_t __t = __get_double_ts();
        double_t __delta = (__t - __s);
        if ((p_analyzer_config->verbose_mode & AnalyzerConfigParam::verbose_type::TRACING)
                && __delta > p_analyzer_config->verbose_interval) {
            // cout << endl;

            if (p_analyzer_config->verbose_mode & AnalyzerConfigParam::verbose_type::MEMORY) {
                LOGF("Analyzer on core # %2d: result_queue size: %u", coreId, analyzer_to_buffer_result_queue.count());
            }

            if (analyze_entrance_count > 0) {
                auto mean_lock_time = interval_lock_time / analyze_entrance_count;
                auto mean_classifier_time = interval_classifier_time / analyze_entrance_count;
                auto mean_syn_time = interval_syn_time / analyze_entrance_count;
                auto mean_warmup_time = interval_warmup_time / analyze_entrance_count;
                auto mean_call_time = mean_lock_time + mean_classifier_time + mean_warmup_time + mean_syn_time;

                if (p_analyzer_config->verbose_mode & AnalyzerConfigParam::verbose_type::DATA_FLOW) {
                    LOGF("Analyzer on core # %2d: [Average batch size %ld, Average item process time %5.3lfms.] / call (%ld calls)",
                            coreId, interval_batch_size / analyze_entrance_count,
                            mean_call_time / (interval_batch_size / analyze_entrance_count) * 1e3,
                            analyze_entrance_count);
                }

                if (p_analyzer_config->verbose_mode & AnalyzerConfigParam::verbose_type::TIME_DETAIL) {
                    LOGF("Analyzer on core # %2d: Averaged time of Analyzing [Lock: %5.3lfs, "
                            "Classifier: %5.3lfs, Syn: %5.3lfs, Warmup %5.3lf, Total: %5.3lfs.]",
                            coreId, mean_lock_time, mean_classifier_time, mean_syn_time, mean_warmup_time, mean_call_time);
                }
            } else {
                LOGF("Analyzer on core # %2d: Warmup %5.3lf (%ld times)", coreId, interval_warmup_time, warmup_entrance_count);
            }

            analyze_entrance_count = 0;
            warmup_entrance_count = 0;
            interval_lock_time = 0;
            interval_classifier_time = 0;
            interval_syn_time = 0;
            interval_warmup_time = 0;
            interval_batch_size = 0;

            __s = __t;
        }

        // analyze action
        inference();
    }

    pthread_barrier_wait(p_stop_barrier);
    return true;
}


// Config from json file
auto AnalyzerWorkerThread::configure_via_json(const json & jin) -> bool {
    if (p_analyzer_config != nullptr) {
		WARNF("Analyzer configuration overlap.");
		return false;
	}

	p_analyzer_config = make_shared<AnalyzerConfigParam>();
	if (p_analyzer_config == nullptr) {
		WARNF("Analyzer configuration paramerter bad allocation.");
		return false;
	}

    try {
        // verbose parameters
        if (jin.count("verbose_mode")) {
            const auto & _mode_array = jin["verbose_mode"];
			p_analyzer_config->verbose_mode = 0;

			for (auto _j_mode : _mode_array) {
                if (analyzer_verbose_mode_map.count(_j_mode) != 0) {
                    p_analyzer_config->verbose_mode |= analyzer_verbose_mode_map.at(_j_mode);
                } else {
                    WARNF("Unknown verbose mode: %s", static_cast<string>(_j_mode).c_str());
                    throw logic_error("Parse error Json tag: verbose_mode\n");
                }
            }
		}

		if (jin.count("verbose_interval")) {
			p_analyzer_config->verbose_interval = 
				    static_cast<decltype(p_analyzer_config->verbose_interval)>(jin["verbose_interval"]);
		}
        
        // memory parameters
        if (jin.count("analyzer_to_buffer_result_arr_size")) {
			p_analyzer_config->analyzer_to_buffer_result_arr_size = 
				    static_cast<decltype(p_analyzer_config->analyzer_to_buffer_result_arr_size)>
                    (jin["analyzer_to_buffer_result_arr_size"]);
			if (p_analyzer_config->analyzer_to_buffer_result_arr_size
                    > AnalyzerConfigParam::ANALYZER_TO_BUFFER_RESULT_ARR_LIM) {
				FATAL_ERROR("Analyzer to Buffer result array exceed.");
			}
		}

        if (jin.count("keep_warm_batch_size")) {
            p_analyzer_config->keep_warm_batch_size = 
                    static_cast<decltype(p_analyzer_config->keep_warm_batch_size)>(jin["keep_warm_batch_size"]);
        } else WARNF("keep_warm_batch_size not found, use default.");

        // for output log
        if (jin.count("log_dir")) {
            p_analyzer_config->log_dir = 
                    static_cast<decltype(p_analyzer_config->log_dir)>(jin["log_dir"]);
        } else WARNF("log_dir not found, no log.");
        /////////////////////////////////////////// Critical Paramerters
        if (jin.count("model_load_path")) {
            p_analyzer_config->model_load_path = 
                    static_cast<decltype(p_analyzer_config->model_load_path)>(jin["model_load_path"]);
        } else {
            FATAL_ERROR("Load path of model not found!");
        }
        ///////////////////////////////////////////
    } catch (exception & e) {
        WARN(e.what());
        return false;
    }

    return true;
}