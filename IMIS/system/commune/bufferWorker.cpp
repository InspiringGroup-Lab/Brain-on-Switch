#include "bufferWorker.hpp"

using namespace Model;

static inline auto __get_double_ts() -> double_t {
    struct timeval ts;
    gettimeofday(&ts, nullptr);
    return ts.tv_sec + ts.tv_usec * (1e-6);
}

bool BufferWorkerThread::send(rte_mbuf *mbuf, PacketInfo info, double_t t) {
    const uint16_t eth_hdr_len = sizeof(struct rte_ether_hdr);
	const uint16_t ip_hdr_offset = eth_hdr_len;
	const uint16_t ip_tos_offset = ip_hdr_offset + 1;
    char *ptr = (char *) mbuf->buf_addr + mbuf->data_off;
    *(ptr + ip_tos_offset) = info.pkt_type;
    double waiting_time = t - info.receive_time;
    interval_send_pkt_waiting_time += waiting_time;
    interval_send_pkt_len += info.length;
    interval_send_pkt_num += 1;
    
    // Collect waiting_time
#ifdef TEST_BURST
    if (likely(info.id.dst_port != 0)) { // remove background flow, test only
        if (likely(m_record_num < 100000000)) {
            records[m_record_num++] = waiting_time;
        }
    }
#endif
    if (unlikely(rte_eth_tx_burst(tx_port_id, tx_queue_id, &mbuf, 1) == 0)) {
        WARNF("ERROR: rte_eth_tx_burst return false in BufferWorkerThread::send");
        rte_pktmbuf_free(mbuf);
        return false;
    } else return true;
}

void BufferWorkerThread::fetch_pkt_from_parser(const shared_ptr<ParserWorkerThread> pt) {
    interval_fetch_pkt_entrance_count ++;
    
    size_t copy_len  = 0;
    int tot = pt->parser_to_buffer_pkt_queue.count();
    interval_max_pkt_num_in_queue = max<uint64_t>(interval_max_pkt_num_in_queue, tot);

    if (tot >= p_buffer_config->pkt_num_fetch_from_parser) {
        copy_len = p_buffer_config->pkt_num_fetch_from_parser;
    } else copy_len = tot;

    interval_fetch_pkt_num += copy_len;
    pt->parser_to_buffer_pkt_queue.dequeue(pkt_from_parser.get(), copy_len);

    double_t _t = 0;
    for (int i = 0; i < copy_len; i++) {
        if (unlikely((i & 63) == 0)) {
            _t = __get_double_ts();
        }

        PacketInfo info = pkt_from_parser[i].first;
        rte_mbuf *mbuf = pkt_from_parser[i].second;

        if (likely(i + 1 < copy_len)) {
            rte_prefetch0(rte_pktmbuf_mtod(pkt_from_parser[i + 1].second, void *));
        }

        // flow_set.insert(info.id);

        auto iter = flow_buffer.find(info.id);
        if (unlikely(iter == flow_buffer.end())) {
            // new flow
            FlowBuffer _buf;
            _buf.push(pkt_from_parser[i]);
            flow_buffer.insert(make_pair(info.id, _buf));
            pkt_num_in_buffer += 1;
        } else if (iter->second.pkt_num_in_queue == 0) {
            // passed flow
            if (info.parser_seq <= iter->second.analyzed_parser_seq) {
                info.pkt_type = iter->second.pkt_type;
                send(mbuf, info, _t);
                // HERE
                interval_pass_pkt_num += 1;
            } else {
                iter->second.push(pkt_from_parser[i]);
                pkt_num_in_buffer += 1;
            }
        } else {
            // existing flow
            if (unlikely(iter->second.pkt_num_in_queue == p_buffer_config->buffer_size_per_flow)) {
                send(iter->second.front().second, iter->second.front().first, _t);
                iter->second.pop();
                interval_drop_pkt_num += 1;
                pkt_num_in_buffer -= 1;
                WARNF("Buffer on core # %2d drop pkts", getCoreId());
#ifdef FLOW_TIME_RECORD
                assert(false);
#endif
            }
            iter->second.push(pkt_from_parser[i]);
            pkt_num_in_buffer += 1;
        }
    }
    m_new_pkt_num = copy_len;
}

void BufferWorkerThread::fetch_result_from_analyzer(const shared_ptr<AnalyzerWorkerThread> pt) {
    interval_fetch_result_entrance_count ++;
    size_t copy_len  = 0;
    int tot = pt->analyzer_to_buffer_result_queue.count();
    if (tot > 0) {
        if (tot >= p_buffer_config->result_num_fetch_from_analyzer) {
            copy_len = p_buffer_config->result_num_fetch_from_analyzer;
        } else copy_len = tot;
        interval_fetch_result_num += copy_len;
        pt->analyzer_to_buffer_result_queue.dequeue(result_from_analyzer.get(), copy_len);
    }
    m_new_result_num = copy_len;
}

void BufferWorkerThread::flush_buffer_with_new_result() {
    interval_send_pkt_entrance_count ++;
    interval_max_pkt_num_in_buffer = max(interval_max_pkt_num_in_buffer, pkt_num_in_buffer);

    const size_t pkt_num_fetch_from_parser = p_buffer_config->pkt_num_fetch_from_parser;

    size_t flushed_pkt_num = 0;
    // const double alpha = 2;
    
    for (int i = 0; i < m_new_result_num; i++) {
        /*if (flushed_pkt_num >= alpha * pkt_num_fetch_from_parser) {
            int cnt = flushed_pkt_num / (alpha * pkt_num_fetch_from_parser);
            for (int j = 0; j < cnt; j++) {
                fetch_pkt_from_parser(p_parser);
                if (m_new_pkt_num < pkt_num_fetch_from_parser) {
                    break;
                }
            }
            flushed_pkt_num = 0;
        }*/
#ifdef FLOW_TIME_RECORD
        PacketInfo result_info = result_from_analyzer[i].first;
        FlowTimeRecordSimple result_time_record = result_from_analyzer[i].second;
        double ts = __get_double_ts();
#else
        PacketInfo result_info = result_from_analyzer[i];
#endif
        auto iter = flow_buffer.find(result_info.id);
        if (likely(iter == flow_buffer.end())) {
            // new flow
            FlowBuffer _buf;
            _buf.pkt_type = result_info.pkt_type;
            _buf.analyzed_parser_seq = result_info.parser_seq;
#ifdef FLOW_TIME_RECORD
            _buf.time_record.update(result_time_record, ts);
#endif
            flow_buffer.insert(make_pair(result_info.id, _buf));
        } else {
            // existing flow
            FlowBuffer &buffer = iter->second;
            buffer.pkt_type = result_info.pkt_type;
            buffer.analyzed_parser_seq = result_info.parser_seq;
#ifdef FLOW_TIME_RECORD
            FlowTimeRecordComplex &time_record = buffer.time_record;
            uint8_t &time_record_idx = buffer.time_record_idx;
            time_record.update(result_time_record, ts);
#endif
            int cnt = 0;
            double_t _t = 0;
            while (buffer.pkt_num_in_queue > 0) {
                if (unlikely(cnt & 63) == 0) {
                    _t = __get_double_ts();
                }
                cnt += 1;
                pair<PacketInfo, rte_mbuf *> pkt = buffer.front();
                PacketInfo &pkt_info = pkt.first;

                if (pkt_info.parser_seq <= buffer.analyzed_parser_seq) {
                    pkt_info.pkt_type = buffer.pkt_type;
                    buffer.pop();
                    if (likely(buffer.pkt_num_in_queue > 0)) {
                        rte_prefetch0(rte_pktmbuf_mtod(buffer.front().second, void *));
                    }
                    // HERE
#ifdef FLOW_TIME_RECORD

                    if (time_record_idx < time_record.recorded_pkt_num) {
                        assert(time_record.parser_seq[time_record_idx] == pkt_info.parser_seq);
                        time_record.rx_timestamp[time_record_idx] = pkt_info.receive_time;
                        time_record.tx_timestamp[time_record_idx] = _t;
                        time_record_idx += 1;
                    }
#endif
                    send(pkt.second, pkt_info, _t);
                    pkt_num_in_buffer -= 1;
                    flushed_pkt_num += 1;
                } else {
                    break;
                }
            }
        }
    }
    m_new_result_num = 0;
}

bool BufferWorkerThread::run(uint32_t coreId) {
    m_core_id = coreId;

    if (p_parser == nullptr) {
        WARNF("Buffer on core # %2d: No Parser thread bind.", coreId);
        pthread_barrier_wait(p_start_barrier);
        return false;
    }

    if (p_analyzer == nullptr) {
        WARNF("Buffer on core # %2d: No Analyzer thread bind.", coreId);
        pthread_barrier_wait(p_start_barrier);
        return false;
    }

    pkt_from_parser = shared_ptr<pair<PacketInfo, rte_mbuf *>[]>
            (new pair<PacketInfo, rte_mbuf *>[p_buffer_config->pkt_num_fetch_from_parser](), 
			std::default_delete<pair<PacketInfo, rte_mbuf *>[]>());
    if (pkt_from_parser == nullptr) {
        WARNF("Buffer on core # %2d: pkt_from_parser array: bad allocation", coreId);
        pthread_barrier_wait(p_start_barrier);
        return false;
    }
#ifdef FLOW_TIME_RECORD
    result_from_analyzer = shared_ptr<pair<PacketInfo, FlowTimeRecordSimple>[]>
            (new pair<PacketInfo, FlowTimeRecordSimple>[p_buffer_config->result_num_fetch_from_analyzer](), 
			std::default_delete<pair<PacketInfo, FlowTimeRecordSimple>[]>());
#else
    result_from_analyzer = shared_ptr<PacketInfo[]>(new PacketInfo[p_buffer_config->result_num_fetch_from_analyzer](), 
			std::default_delete<PacketInfo[]>());
#endif
    if (result_from_analyzer == nullptr) {
        WARNF("Buffer on core # %2d: result_from_analyzer array: bad allocation", coreId);
        pthread_barrier_wait(p_start_barrier);
        return false;
    }

    if (p_buffer_config->verbose_mode & BufferConfigParam::verbose_type::INIT)
        LOGF("Buffer on core # %2d ready.", coreId);
    pthread_barrier_wait(p_start_barrier);
    if (p_buffer_config->verbose_mode & BufferConfigParam::verbose_type::INIT)
        LOGF("Buffer on core # %2d start.", coreId);
    m_stop = false;

    double_t __s = __get_double_ts();

    while (!m_stop) {
        double_t _t_loop_start = __get_double_ts();
        // for performance statistic
        double_t __t = __get_double_ts();
        double_t __delta = (__t - __s);
        if ((p_buffer_config->verbose_mode & BufferConfigParam::verbose_type::TRACING)
                && __delta > p_buffer_config->verbose_interval) {
            interval_max_pkt_num_in_buffer = max(interval_max_pkt_num_in_buffer, pkt_num_in_buffer);
            interval_max_pkt_num_in_queue = max<uint64_t>(interval_max_pkt_num_in_queue, p_parser->parser_to_buffer_pkt_queue.count());

            if (p_buffer_config->verbose_mode & BufferConfigParam::verbose_type::MEMORY) {
                // LOGF("Buffer on core # %2d: Flow Num: %ld", coreId, flow_set.size());
                LOGF("Buffer on core # %2d: [ max items in buffer %ld, items in buffer %ld, dropped items %ld.] "
                        "(interval: %5.3lfs)", coreId, interval_max_pkt_num_in_buffer, pkt_num_in_buffer, 
                        interval_drop_pkt_num, __delta);
                LOGF("Buffer on core # %2d: max items in parser_to_buffer_pkt_queue %ld", coreId, interval_max_pkt_num_in_queue);
            }

            if (p_buffer_config->verbose_mode & BufferConfigParam::verbose_type::DATA_FLOW) {
                LOGF("Buffer on core # %2d: Throughput [ in-buffer speed %5.3lf mpps, "
                        "out-buffer speed %5.3lf mpps (%5.3lf Gbps), pass speed %5.3lf mpps, fetch result speed %5.3lf *10^3/s]", 
                        coreId, interval_fetch_pkt_num / __delta / 1e6, interval_send_pkt_num / __delta / 1e6, 
                        interval_send_pkt_len * 8 / __delta / 1e9, interval_pass_pkt_num / __delta / 1e6,
                        interval_fetch_result_num / __delta / 1e3);
                if (interval_send_pkt_num > 0) {
                    // TEST only, should use LOGF()
                    WARNF("Buffer on core # %2d: [ Average waiting time for pkts %5.3lfs.] (%ld pkts)", 
                            coreId, interval_send_pkt_waiting_time / interval_send_pkt_num, interval_send_pkt_num);
                } else LOGF("Buffer on core # %2d: No out-buffer packet.", coreId);
            }
            
            if (p_buffer_config->verbose_mode & BufferConfigParam::verbose_type::TIME_DETAIL) {
                LOGF("Buffer on core # %2d: Time distribution [ in-buffer %5.3lfs (%ld calls), "
                        "fetch-result: %5.3lfs (%ld calls), out-buffer: %5.3lfs (%ld calls) total %5.3lfs.]", 
                        coreId, interval_fetch_pkt_time, interval_fetch_pkt_entrance_count, interval_fetch_result_time, 
                        interval_fetch_result_entrance_count, interval_send_pkt_time, 
                        interval_send_pkt_entrance_count, interval_loop_time);
            }

            if (p_buffer_config->verbose_mode & BufferConfigParam::verbose_type::SUMMARY) {
#ifdef TEST_BURST
                LOGF("Buffer on core # %2d: record num = %ld]", 
                        coreId, m_record_num);
#endif
            }

            interval_max_pkt_num_in_buffer = pkt_num_in_buffer;
            interval_max_pkt_num_in_queue = p_parser->parser_to_buffer_pkt_queue.count();
            
            // throughput
            interval_fetch_pkt_num = 0;
            interval_fetch_result_num = 0;

            interval_send_pkt_num = 0;
            interval_send_pkt_len = 0;
            interval_drop_pkt_num = 0;
            interval_pass_pkt_num = 0;

            // time distribution
            interval_fetch_pkt_time = 0;
            interval_fetch_result_time = 0;
            interval_send_pkt_time = 0;
            interval_loop_time = 0;

            interval_fetch_pkt_entrance_count = 0;
            interval_fetch_result_entrance_count = 0;
            interval_send_pkt_entrance_count = 0;

            // waiting time for pkts
            interval_send_pkt_num = 0;
            interval_send_pkt_waiting_time = 0;

            __s = __t;
        }
        
        double_t _t_fetch_pkt_start = __get_double_ts();
        // fetch per-packets properties from ParserWorkers
        fetch_pkt_from_parser(p_parser);
        interval_fetch_pkt_time += __get_double_ts() - _t_fetch_pkt_start;

        double_t _t_fetch_result_start = __get_double_ts();
        fetch_result_from_analyzer(p_analyzer);
        interval_fetch_result_time += __get_double_ts() - _t_fetch_result_start;
        if (m_new_result_num > 0) {
            double_t _t_send_pkt_start = __get_double_ts();
            flush_buffer_with_new_result();
            interval_send_pkt_time += __get_double_ts() - _t_send_pkt_start;
        }

        interval_loop_time += __get_double_ts() - _t_loop_start;
    }
    pthread_barrier_wait(p_stop_barrier);
    return true;
}


auto BufferWorkerThread::configure_via_json(const json & jin) -> bool {
    if (p_buffer_config != nullptr) {
		WARNF("Buffer configuration overlap.");
		return false;
	}

	p_buffer_config = make_shared<BufferConfigParam>();
	if (p_buffer_config == nullptr) {
		WARNF("Buffer configuration paramerter bad allocation.");
		return false;
	}

    try {
        // verbose parameters
        if (jin.count("verbose_mode")) {
            const auto & _mode_array = jin["verbose_mode"];
			p_buffer_config->verbose_mode = 0;

			for (auto _j_mode : _mode_array) {
                if (buffer_verbose_mode_map.count(_j_mode) != 0) {
                    p_buffer_config->verbose_mode |= buffer_verbose_mode_map.at(_j_mode);
                } else {
                    WARNF("Unknown verbose mode: %s", static_cast<string>(_j_mode).c_str());
                    throw logic_error("Parse error Json tag: verbose_mode\n");
                }
            }
		}

		if (jin.count("verbose_interval")) {
			p_buffer_config->verbose_interval = 
				    static_cast<decltype(p_buffer_config->verbose_interval)>(jin["verbose_interval"]);
		}
        
        if (jin.count("buffer_timeout")) {
            p_buffer_config->buffer_timeout = 
                    static_cast<decltype(p_buffer_config->buffer_timeout)>(jin["buffer_timeout"]);
        } else WARNF("Critical tag not found: buffer_timeout, use default. (unsupported now)");

        // for output log
        if (jin.count("log_dir")) {
            p_buffer_config->log_dir = 
                    static_cast<decltype(p_buffer_config->log_dir)>(jin["log_dir"]);
        } else WARNF("log_dir not found, no log.");
        /////////////////////////////////////////// Critical Paramerters
        // for memory allocation
        if (jin.count("pkt_num_fetch_from_parser")) {
            p_buffer_config->pkt_num_fetch_from_parser = 
                    static_cast<decltype(p_buffer_config->pkt_num_fetch_from_parser)>(jin["pkt_num_fetch_from_parser"]);
            if (p_buffer_config->pkt_num_fetch_from_parser > BufferConfigParam::MAX_PKT_NUM_FETCH_FROM_PARSER)
                FATAL_ERROR("pkt_num_fetch_from_parser exceed.");
        } else WARNF("Critical tag not found: pkt_num_fetch_from_parser, use default.");

        if (jin.count("result_num_fetch_from_analyzer")) {
            p_buffer_config->result_num_fetch_from_analyzer = 
                    static_cast<decltype(p_buffer_config->result_num_fetch_from_analyzer)>(jin["result_num_fetch_from_analyzer"]);
            if (p_buffer_config->result_num_fetch_from_analyzer > BufferConfigParam::MAX_RESULT_NUM_FETCH_FROM_ANALYZER)
                FATAL_ERROR("result_num_fetch_from_analyzer exceed.");
        } else WARNF("Critical tag not found: result_num_fetch_from_analyzer, use default.");

        if (jin.count("buffer_size_per_flow")) {
            p_buffer_config->buffer_size_per_flow = 
                    static_cast<decltype(p_buffer_config->buffer_size_per_flow)>(jin["buffer_size_per_flow"]);
            if (p_buffer_config->buffer_size_per_flow > BufferConfigParam::MAX_BUFFER_SIZE_PER_FLOW) 
                FATAL_ERROR("buffer_size_per_flow exceed.");
        } else WARNF("Critical tag not found: buffer_size_per_flow, use default.");
        ///////////////////////////////////////////
    } catch (exception & e) {
        WARN(e.what());
        return false;
    }

    return true;
}