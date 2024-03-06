#include "poolWorker.hpp"


using namespace Model;


static inline auto __get_double_ts() -> double_t {
    struct timeval ts;
    gettimeofday(&ts, nullptr);
    return ts.tv_sec + ts.tv_usec * (1e-6);
}

void PoolWorkerThread::fetch_from_parser(const shared_ptr<ParserWorkerThread> pt) {
    // LOGF("Pool on core # %2d start start fetch.", getCoreId());
    ++ interval_fetch_entrance_count;
    int tot = pt->parser_to_pool_pkt_metadata_queue.count();
    size_t copy_len = 0;
    if (tot > 0) {
        if (tot >= p_pool_config->pkt_metadata_num_fetch_from_parser) {
            copy_len = p_pool_config->pkt_metadata_num_fetch_from_parser;
        } else copy_len = tot;
        interval_fetch_metadata_num += copy_len;
        pt->parser_to_pool_pkt_metadata_queue.dequeue(metadata_from_parser.get(), copy_len);
    }
    m_new_metadata_num = copy_len;
    // LOGF("Pool on core # %2d start finish fetch, new_metadata_num=%ld.", getCoreId(), m_new_metadata_num);
}


void PoolWorkerThread::out_pool() {
    // LOGF("Pool on core # %2d start out_pool.", getCoreId());
    ++ interval_outpool_entrance_count;

    //////////////////////////////////////////////
    double_t _t_outpool_start = __get_double_ts();
    ////////////////////////////////////////////// get batch from pool

    size_t batch_size = p_pool_config->batch_size;

    // get input batch for classifier
    vector<float> input_vec_c;
    input_vec_c.reserve(batch_size * 1 * 40 * 40); // MODEL
    
#ifdef FLOW_TIME_RECORD
    vector<pair<PacketInfo, FlowTimeRecordSimple> > input_item_c;
#else
    vector<PacketInfo> input_item_c;
#endif
    int input_num_c = 0;

    vector<uint16_t> sz_vec;

    int paused_item = 0;
    int closed_item = 0;

    while (input_num_c < batch_size) {
        if (unlikely(flow_priority.empty())) {
            break;
        }
        Flow5Tuple id = flow_priority.top().second;
        flow_priority.pop();

        auto iter = flow_record.find(id);

        if (unlikely(iter == flow_record.end())) {
            FATAL_ERROR("Flow5Tuple found in flow_priority missing in flow_record");
            continue;
        }

        input_num_c += 1;
        // CHOOSE MODE
        PacketInfo info(iter->second.info);
        if (likely(iter->second.metadata_num == METADATA_NUM_PER_FLOW)) {
            // CHOOSE MODE
            // info.parser_seq = 0xffffffffffffffffULL;
            info.parser_seq |= 0x7000000000000000ULL;
        }
        // LOGF("### POOL send (%u,%u) with %d pkts, seq=%lu to Analyzer", info.id.src_port, 
        //         info.id.dst_port, iter->second.metadata_num, info.parser_seq);
#ifdef FLOW_TIME_RECORD
        input_item_c.push_back(make_pair(info, iter->second.time_record));
#else
        input_item_c.push_back(info);
#endif
        for (int i = 0; i < iter->second.metadata_num; i++) {

            input_vec_c.insert(input_vec_c.end(), iter->second.metadata_sequence[i].begin(),
                iter->second.metadata_sequence[i].end());

        }

        input_vec_c.resize(input_num_c * METADATA_NUM_PER_FLOW * 320, 0);

        interval_tx_metadata_num += iter->second.metadata_num;
        if (unlikely(iter->second.metadata_num < METADATA_NUM_PER_FLOW)) {
            // CHOOSE MODE
            paused_item += 1;
            iter->second.paused = true;
            // metadata_num_in_pool -= iter->second.metadata_num;
            // iter->second.closed = true;
        } else {
            closed_item += 1;
            interval_outpool_metadata_num += iter->second.metadata_num;
            metadata_num_in_pool -= iter->second.metadata_num;
            iter->second.closed = true;
        }
    }
    assert(input_vec_c.size() == input_num_c * 1 * 40 * 40); // MODEL
    interval_outpool_batch_size += input_num_c;

    p_analyzer->input_vec_c = input_vec_c;
    p_analyzer->input_item_c = input_item_c;
    p_analyzer->input_num_c = input_num_c;

    call_by_analyzer = false;
    ready_for_analyzer = true;

    double_t _t_tmp = __get_double_ts();

    if (input_num_c > 0) {
        interval_batch_num += 1;
        batch_record.push_back(PoolBatchRecord(_t_outpool_start, _t_tmp, input_num_c, closed_item, paused_item));
    }

    interval_outpool_time += _t_tmp - _t_outpool_start;
    // LOGF("Pool on core # %2d finish out_pool input_num_c=%d.", getCoreId(), input_num_c);
}


void PoolWorkerThread::in_pool() {
    // LOGF("Pool on core # %2d start in_pool.", getCoreId());
    ++ interval_inpool_entrance_count;
    double_t _t_inpool_start = __get_double_ts();

    for (size_t i = 0; i < m_new_metadata_num; i++) {
        PacketInfo info = metadata_from_parser[i].first;
        PacketMetadata metadata = metadata_from_parser[i].second;
        Flow5Tuple id = info.id;
        auto iter = flow_record.find(id);

        if (likely(iter != flow_record.end())) {
            if (iter->second.closed) {
                interval_pass_metadata_num += 1;
            } else if (iter->second.metadata_num == METADATA_NUM_PER_FLOW) {
                interval_pass_metadata_num += 1;
            } else {
                interval_inpool_metadata_num += 1;

                vector<float> _v1;
                _v1.reserve(320);
                for (int j = 0; j < metadata.hdr_len; j++) {
                    _v1.push_back(metadata.hdr[j]);
                }
                _v1.resize(80, 0);
                for (int j = 0; j < metadata.payload_len; j++) {
                    _v1.push_back(metadata.payload[j]);
                }
                _v1.resize(240, 0);
                iter->second.metadata_sequence.push_back(_v1);

                iter->second.metadata_num += 1;
                iter->second.info.parser_seq = info.parser_seq;
                if (unlikely(iter->second.paused)) {
                    iter->second.paused = false;
                    flow_priority.push(make_pair(info.parser_seq, id));
                }
#ifdef FLOW_TIME_RECORD
                iter->second.time_record.push_no_check(info.parser_seq, __get_double_ts());
#endif
                metadata_num_in_pool += 1;
            }
        } else {
            flow_record.insert(make_pair(id, FlowRecord(info)));
            interval_inpool_metadata_num += 1;

            vector<float> _v1;
            _v1.reserve(320);
            for (int j = 0; j < metadata.hdr_len; j++) {
                _v1.push_back(metadata.hdr[j]);
            }
            _v1.resize(80, 0);
            for (int j = 0; j < metadata.payload_len; j++) {
                _v1.push_back(metadata.payload[j]);
            }
            _v1.resize(240, 0);
            flow_record[id].metadata_sequence.push_back(_v1);

            flow_record[id].metadata_num += 1;
#ifdef FLOW_TIME_RECORD
            flow_record[id].time_record.push_no_check(info.parser_seq, __get_double_ts());
#endif
            metadata_num_in_pool += 1;
            flow_priority.push(make_pair(info.parser_seq, id));
        }
    }
    m_new_metadata_num = 0;
    interval_inpool_time += __get_double_ts() - _t_inpool_start;
    // LOGF("Pool on core # %2d finish in_pool.", getCoreId());
}

bool PoolWorkerThread::run(uint32_t coreId) {
    m_core_id = coreId;

    if (p_parser == nullptr) {
        WARN("Pool on core # %2d: No Parser thread bind.", coreId);
        pthread_barrier_wait(p_start_barrier);
        return false;
    }

    if (p_analyzer == nullptr) {
        WARN("Pool on core # %2d: No Analyzer thread bind.", coreId);
        pthread_barrier_wait(p_start_barrier);
        return false;
    }

    metadata_from_parser = shared_ptr<pair<PacketInfo, PacketMetadata>[]>
            (new pair<PacketInfo, PacketMetadata>[p_pool_config->pkt_metadata_num_fetch_from_parser](), 
			std::default_delete<pair<PacketInfo, PacketMetadata>[]>());
    if (metadata_from_parser == nullptr) {
        WARN("Pool on core # %2d: metadata_from_parser: bad allocation", coreId);
        pthread_barrier_wait(p_start_barrier);
        return false;
    }

    if (p_pool_config->verbose_mode & PoolConfigParam::verbose_type::INIT)
        LOGF("Pool on core # %2d ready.", coreId);
    pthread_barrier_wait(p_start_barrier);
    if (p_pool_config->verbose_mode & PoolConfigParam::verbose_type::INIT)
        LOGF("Pool on core # %2d start.", coreId);
    m_stop = false;

    double_t __s = __get_double_ts();
    size_t loop_count = 0;

    while (!m_stop) {
        double_t _t_loop_start = __get_double_ts();
        loop_count += 1;
        // for performance statistic
        double_t __t = __get_double_ts();
        double_t __delta = (__t - __s);
        if ((p_pool_config->verbose_mode & PoolConfigParam::verbose_type::TRACING)
                && __delta > p_pool_config->verbose_interval) {

            if (p_pool_config->verbose_mode & PoolConfigParam::verbose_type::MEMORY) {
                LOGF("Pool on core # %2d: [ items in pool %ld ] (interval: %5.3lfs)", 
                        coreId, metadata_num_in_pool, __delta);
            }

            if (p_pool_config->verbose_mode & PoolConfigParam::verbose_type::DATA_FLOW) {
                LOGF("Pool on core # %2d: Throughput [ fetch speed %5.3lf /s, inpool speed %5.3lf /s, "
                        "outpool speed %5.3lf /s, tx speed %5.3lf /s, pass metadata speed %5.3lf /s ]", 
                        coreId, interval_fetch_metadata_num / __delta, interval_inpool_metadata_num / __delta, 
                        interval_outpool_metadata_num / __delta, interval_tx_metadata_num / __delta,  
                        interval_pass_metadata_num / __delta);
                if (interval_batch_num > 0) {
                    LOGF("Pool on core # %2d: Batching average sizes [ batch size: %.2f. ] (%ld batches / %ld calls by analyzer)", 
                            coreId, (float) interval_outpool_batch_size / interval_batch_num, interval_batch_num,
                            interval_outpool_entrance_count);
                } else LOGF("Pool on core # %2d: No batch to analyzer.", coreId);
            }

            if (p_pool_config->verbose_mode & PoolConfigParam::verbose_type::TIME_DETAIL) {
                LOGF("Pool on core # %2d: Time distribution [ fetch %5.3lfs (%ld calls), inpool: %5.3lfs (%ld calls), "
                        "outpool %5.3lfs (%ld calls), total %5.3lfs.] (%ld loops)", 
                        coreId, interval_fetch_time, interval_fetch_entrance_count, interval_inpool_time, 
                        interval_inpool_entrance_count, interval_outpool_time, interval_outpool_entrance_count,
                        interval_loop_time, loop_count);
            }

            interval_fetch_metadata_num = 0;
            interval_inpool_metadata_num = 0;
            interval_outpool_metadata_num = 0;
            interval_tx_metadata_num = 0;
            interval_pass_metadata_num = 0;

            interval_loop_time = 0;
            interval_fetch_time = 0;
            interval_inpool_time = 0;
            interval_outpool_time = 0;

            interval_outpool_batch_size = 0;
            interval_batch_num = 0;

            interval_fetch_entrance_count = 0;
            interval_inpool_entrance_count = 0;
            interval_outpool_entrance_count = 0;

            __s = __t;
        }
        
        double_t _t_fetch_start = __get_double_ts();
        fetch_from_parser(p_parser);
        interval_fetch_time += __get_double_ts() - _t_fetch_start;

        double_t _t_inpool_start = __get_double_ts();
        in_pool();
        interval_inpool_time += __get_double_ts() - _t_inpool_start;
        
        if (call_by_analyzer) {
            double_t _t_outpool_start = __get_double_ts();
            out_pool();
            interval_outpool_time += __get_double_ts() - _t_outpool_start;
        }
        
        interval_loop_time += __get_double_ts() - _t_loop_start;
    }

    pthread_barrier_wait(p_stop_barrier);
    return true;
}


auto PoolWorkerThread::configure_via_json(const json & jin) -> bool {
    if (p_pool_config != nullptr) {
		WARNF("Pool configuration overlap.");
		return false;
	}

	p_pool_config = make_shared<PoolConfigParam>();
	if (p_pool_config == nullptr) {
		WARNF("Pool configuration paramerter bad allocation.");
		return false;
	}

    try {
        // verbose parameters
        if (jin.count("verbose_mode")) {
            const auto & _mode_array = jin["verbose_mode"];
			p_pool_config->verbose_mode = 0;

			for (auto _j_mode : _mode_array) {
                if (pool_verbose_mode_map.count(_j_mode) != 0) {
                    p_pool_config->verbose_mode |= pool_verbose_mode_map.at(_j_mode);
                } else {
                    WARNF("Unknown verbose mode: %s", static_cast<string>(_j_mode).c_str());
                    throw logic_error("Parse error Json tag: verbose_mode\n");
                }
            }
		}
		if (jin.count("verbose_interval")) {
			p_pool_config->verbose_interval = 
				    static_cast<decltype(p_pool_config->verbose_interval)>(jin["verbose_interval"]);
		}

        // for output log
        if (jin.count("log_dir")) {
            p_pool_config->log_dir = 
                    static_cast<decltype(p_pool_config->log_dir)>(jin["log_dir"]);
        } else WARNF("log_dir not found, no log.");
        /////////////////////////////////////////// Critical Paramerters
        // for memory allocation
        if (jin.count("pkt_metadata_num_fetch_from_parser")) {
            p_pool_config->pkt_metadata_num_fetch_from_parser = 
                    static_cast<decltype(p_pool_config->pkt_metadata_num_fetch_from_parser)>
                    (jin["pkt_metadata_num_fetch_from_parser"]);
            if (p_pool_config->pkt_metadata_num_fetch_from_parser 
                    > PoolConfigParam::MAX_PKT_METADATA_NUM_FETCH_FROM_PARSER)
                FATAL_ERROR("pkt_metadata_num_fetch_from_parser size exceed.");
        } else WARNF("Critical tag not found: pkt_metadata_num_fetch_from_parser, use default.");
        if (jin.count("batch_size")) {
            p_pool_config->batch_size = static_cast<decltype(p_pool_config->batch_size)>(jin["batch_size"]);
        } else WARNF("Critical tag not found: batch_size, use default.");
        ///////////////////////////////////////////
    } catch (exception & e) {
        WARN(e.what());
        return false;
    }

    return true;
}