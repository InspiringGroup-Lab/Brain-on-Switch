#include "parserWorker.hpp"

using namespace Model;

static inline auto __get_double_ts() -> double_t {
    struct timeval ts;
    gettimeofday(&ts, nullptr);
    return ts.tv_sec + ts.tv_usec*(1e-6);
}


bool ParserWorkerThread::run(uint32_t core_id) {
	m_core_id = core_id;
	LOGF("Helloworld from parser on core %2d", core_id);
	if (p_parser_config == nullptr) {
		pthread_barrier_wait(p_start_barrier);
		FATAL_ERROR("NULL parser configuration parameters.");
	}

	// if no DPDK devices were assigned to this worker/core don't enter the main loop and exit
	if (p_dpdk_config->nic_queue_list.size() == 0) {
		WARNF("NO NIC queue bind for parser on core %2d.", core_id);
		pthread_barrier_wait(p_start_barrier);
		return false;
	}
	
	if (!parser_to_pool_pkt_metadata_queue.init(p_parser_config->parser_to_pool_pkt_metadata_arr_size)) {
		FATAL_ERROR("error");
		pthread_barrier_wait(p_start_barrier);
		FATAL_ERROR("parser_to_pool_pkt_metadata_queue: bad allocation.");
	}

	if (!parser_to_buffer_pkt_queue.init(p_parser_config->parser_to_buffer_pkt_arr_size)) {
		FATAL_ERROR("error");
		pthread_barrier_wait(p_start_barrier);
		FATAL_ERROR("parser_to_buffer_pkt_queue: bad allocation.");
	}

	if (p_parser_config->verbose_mode & ParserConfigParam::verbose_type::INIT) {
		LOGF("Parser on core # %2d ready.", core_id);
	}

	pthread_barrier_wait(p_start_barrier);

	if (p_parser_config->verbose_mode & ParserConfigParam::verbose_type::INIT) {
		LOGF("Parser on core # %2d start.", core_id);
	}
	m_stop = false;

    thread verbose_stat(&ParserWorkerThread::verbose_tracing_thread, this);
    verbose_stat.detach();

	const uint16_t eth_hdr_len = sizeof(struct rte_ether_hdr);
	const uint16_t eth_type_offset = eth_hdr_len - 2;
	const uint16_t ip_hdr_offset = eth_hdr_len;
	const uint16_t ip_tos_offset = ip_hdr_offset + 1;
	const uint16_t ip_len_offset = ip_hdr_offset + 2;
	const uint16_t ip_protocol_offset = ip_hdr_offset + 9;
	const uint16_t ip_src_offset = ip_hdr_offset + 12;
	const uint16_t ip_dst_offset = ip_hdr_offset + 16;
#ifdef TEST_BURST
	const uint32_t test_src_ip = p_parser_config->test_src_ip;
    const uint32_t test_dst_ip = p_parser_config->test_dst_ip;
    const uint16_t old_src_port_min = p_parser_config->old_src_port_min;
    const uint16_t old_src_port_max = p_parser_config->old_src_port_max;
    const uint16_t new_dst_port_min = p_parser_config->new_dst_port_min;
    const uint16_t new_dst_port_max = p_parser_config->new_dst_port_max;
#endif
	m_parser_seq = 0;
	
	parser_start_time = get_time_spec();

	// main loop, runs until be told to stop
	while (!m_stop) {
		// go over all DPDK devices configured for this worker/core
		for (parser_queue_assign_t::iterator iter = p_dpdk_config->nic_queue_list.begin();
				iter != p_dpdk_config->nic_queue_list.end(); iter++) {
			DpdkDevice* dev = iter->first;
			int dev_id = dev->getDeviceId();
			int max_receive_burst = p_parser_config->max_receive_burst;
			// for each DPDK device go over all RX queues configured for this worker/core
			for (vector<nic_queue_id_t>::iterator iter2 = iter->second.begin(); 
					iter2 != iter->second.end(); 
					iter2++) {
				
				bool to_pool_queue_full = false;
				bool to_buffer_queue_full = false;
				struct rte_mbuf *bufs[max_receive_burst];
				
				// receive packets from network on the specified DPDK device and RX queue
				const uint16_t packetsReceived = rte_eth_rx_burst(dev_id, *iter2, bufs, max_receive_burst);
				
				// record the time when the pkt is received
				double_t rt = __get_double_ts();

				acquire_semaphore();
				// iterate all of the packets and parse the metadata
				for (uint16_t i = 0; i < packetsReceived; i++) {
					if (likely(i < packetsReceived - 1))
						rte_prefetch0(rte_pktmbuf_mtod(bufs[i + 1], void *));

					uint64_t *metadata_seq_ptr = (uint64_t *)((char *) bufs[i]->buf_addr);
					double_t *metadata_timestamp_ptr = 
							(double_t *)((char *) bufs[i]->buf_addr + sizeof(decltype(*metadata_seq_ptr)));
					packet_type_size_t *metadata_cls_ptr = (uint8_t *)((char *) bufs[i]->buf_addr
							+ sizeof(decltype(*metadata_seq_ptr)) + sizeof(decltype(*metadata_timestamp_ptr)));

					char *ptr = (char *) bufs[i]->buf_addr + bufs[i]->data_off;

					// check IP
					uint16_t eth_type = *((uint16_t*)(ptr + eth_type_offset));
					if (unlikely(eth_type != 8)) {
						cout << "no IPv4 packet!!!!" << endl;
						rte_pktmbuf_free(bufs[i]);
						continue;
					}
					uint8_t ip_hdr_len = ((*(ptr + ip_hdr_offset)) & 0x0f) << 2;
					uint8_t tos = *(ptr + ip_tos_offset);
					uint16_t ip_len = ntohs(*((uint16_t*)(ptr + ip_len_offset)));
					uint32_t src_addr = *((uint32_t*)(ptr + ip_src_offset));
					uint32_t dst_addr = *((uint32_t*)(ptr + ip_dst_offset));
					uint8_t type_code = *(ptr + ip_protocol_offset);
					
					// check TCP / UDP
					if (unlikely(type_code != 6 && type_code != 17)) {
						cout << "no TCP / UDP packet!!!!" << endl;
						rte_pktmbuf_free(bufs[i]);
						continue;
					}
					
					uint16_t l4_hdr_offset = ip_hdr_offset + ip_hdr_len;
					uint16_t src_port = ntohs(*((uint16_t*)(ptr + l4_hdr_offset)));
					uint16_t dst_port = ntohs(*((uint16_t*)(ptr + l4_hdr_offset + 2)));

#ifdef TEST_BURST
					if (unlikely(!(src_addr == test_src_ip && dst_addr == test_dst_ip &&
							((dst_port == 0 && old_src_port_min <= src_port && src_port <= old_src_port_max) || 
							(src_port == 0 && new_dst_port_min <= dst_port && dst_port <= new_dst_port_max))))) {
						printf("find(%X,%X,%u,%u)\n", src_addr, dst_addr, src_port, dst_port);
						rte_pktmbuf_free(bufs[i]);
						continue;
					}
#endif
					
					uint8_t l4_hdr_len = 0;
					uint16_t payload_offset = 0;
					uint16_t payload_len = 0;

					if (likely(type_code == 6)) {
						// TCP
						l4_hdr_len = (*(ptr + l4_hdr_offset + 12) & 0xf0);
						payload_offset = l4_hdr_offset + l4_hdr_len;
						payload_len = ip_len - ip_hdr_len - l4_hdr_len;
					} else {
					// else if (likely(type_code == 17))
						// UDP
						if ((dst_port == 4500 || src_port == 4500) && 
								(ip_len - ip_hdr_len - 8 >= 4) && 
								(*((uint32_t*)(ptr + l4_hdr_offset + 8)) != 0)) {
							// NAT-T: UDP + ESP
							// RFC 3948
							l4_hdr_len = 16;
							payload_offset = l4_hdr_offset + l4_hdr_len;
							payload_len = ip_len - ip_hdr_len - l4_hdr_len;
						} else {
							// other UDP
							l4_hdr_len = 8;
							payload_offset = l4_hdr_offset + l4_hdr_len;
							payload_len = ip_len - ip_hdr_len - l4_hdr_len;
						}
					}
					

					m_parser_seq += 1;

					*metadata_seq_ptr = m_parser_seq;
					*metadata_timestamp_ptr = rt;
					*metadata_cls_ptr = tos;

					// give metadata to pool and/or analyzer
					Flow5Tuple id(src_addr, dst_addr, type_code, src_port, dst_port);

					// for statistics
					++ parsed_pkt_num[dev_id];
					parsed_pkt_len[dev_id] += ip_len;

					const auto info = PacketInfo(id, rt, m_parser_seq, ip_len, tos);

					auto iter = flow_packet_num.find(id);

					if (unlikely(iter == flow_packet_num.end())) {
						flow_packet_num.insert(make_pair(id, 1));

						const auto metadata = PacketMetadata(ptr + ip_hdr_offset, payload_offset - ip_hdr_offset, 
								ptr + payload_offset, payload_len); // MODEL

						if (unlikely(!parser_to_pool_pkt_metadata_queue.enqueue(make_pair(info, metadata)))) {
							to_pool_queue_full = true;
							// WARNF("Parser on core # %2d: queue to pool reach max.", (int) this->getCoreId());
						}
					} else {
#ifndef TEST_PARSER_EXTRACT_ALL
						if (iter->second < METADATA_NUM_PER_FLOW) {
#endif
							iter->second += 1;

							const auto metadata = PacketMetadata(ptr + ip_hdr_offset, payload_offset - ip_hdr_offset, 
									ptr + payload_offset, payload_len); // MODEL

							if (unlikely(!parser_to_pool_pkt_metadata_queue.enqueue(make_pair(info, metadata)))) {
								to_pool_queue_full = true;
								// WARNF("Parser on core # %2d: queue to pool reach max.", (int) this->getCoreId());
							}
#ifndef TEST_PARSER_EXTRACT_ALL
						}
#endif
					}

					// rte_pktmbuf_free(bufs[i]);
					// give pkt to buffer
					if (unlikely(!parser_to_buffer_pkt_queue.enqueue(make_pair(info, bufs[i])))) {
						to_buffer_queue_full = true;
						// WARNF("Parser on core # %2d: queue to buffer reach max.", (int) this->getCoreId());
						rte_pktmbuf_free(bufs[i]);
					}
				}
				release_semaphore();
				if (to_pool_queue_full) {
					WARNF("Parser on core # %2d: queue to pool reach max.", (int) this->getCoreId());
				}
				if (to_buffer_queue_full) {
					WARNF("Parser on core # %2d: queue to buffer reach max.", (int) this->getCoreId());
				}
			}
		}
	}
	
	pthread_barrier_wait(p_stop_barrier);
	
	parser_to_pool_pkt_metadata_queue.destory();
	parser_to_buffer_pkt_queue.destory();
	return true;
}

void ParserWorkerThread::verbose_tracing_thread() {
	while (!m_stop) {
		if (p_parser_config->verbose_mode & ParserConfigParam::verbose_type::TRACING) {
			if (p_parser_config->verbose_mode & ParserConfigParam::verbose_type::DATA_FLOW) {
                stringstream ss;
				ss << "Parser on core # " << setw(2) << m_core_id << ": ";
				size_t index = 0;
				for (parser_queue_assign_t::const_iterator iter = cbegin(p_dpdk_config->nic_queue_list); 
						iter != cend(p_dpdk_config->nic_queue_list); iter ++) {
					ss << "DPDK Port" << setw(2) << iter->first->getDeviceId();
					ss << " [" << setw(6) << setprecision(4) 
							<< ((double) parsed_pkt_num[index] / 1e6) / p_parser_config->verbose_interval 
							<< " Mpps / ";

					ss << setw(6) << setprecision(4) 
							<< ((double) parsed_pkt_len[index] / (1e9 / 8) / 1.024 / 1.024 / 1.024) 
							/ p_parser_config->verbose_interval << " Gbps]\t";
					
					sum_parsed_pkt_num[index] += parsed_pkt_num[index];
					sum_parsed_pkt_len[index] += parsed_pkt_len[index];
					
					parsed_pkt_num[index] = 0;
					parsed_pkt_len[index] = 0;
					index ++;
				}
				const string &str2 = ss.str();
				LOGF("%s", str2.c_str());
            }
			if (p_parser_config->verbose_mode & ParserConfigParam::verbose_type::MEMORY) {
                LOGF("Parser on core # %2d: metadata_queue size: %u", m_core_id, parser_to_pool_pkt_metadata_queue.count());
				LOGF("Parser on core # %2d: pkt_queue size: %u", m_core_id, parser_to_buffer_pkt_queue.count());
            }
		} else {
			size_t index = 0;
			for (parser_queue_assign_t::const_iterator iter = cbegin(p_dpdk_config->nic_queue_list); 
					iter != cend(p_dpdk_config->nic_queue_list); iter ++) {
				sum_parsed_pkt_num[index] += parsed_pkt_num[index];
				sum_parsed_pkt_len[index] += parsed_pkt_len[index];
				
				parsed_pkt_num[index] = 0;
				parsed_pkt_len[index] = 0;
				index ++;
			}
		}
		sleep(p_parser_config->verbose_interval);
	}
}


void ParserWorkerThread::verbose_final() const {
	if (p_parser_config->verbose_mode & ParserConfigParam::verbose_type::SUMMARY) {
		stringstream ss;
		ss << "[Performance Statistic] Parser on core # " << setw(2) << m_core_id << ": ";
		ss << " Runtime: " << setw(5) << setprecision(3) << (parser_end_time - parser_start_time) << "s\n";
		size_t index = 0;
		for (parser_queue_assign_t::const_iterator iter = cbegin(p_dpdk_config->nic_queue_list); 
				iter != cend(p_dpdk_config->nic_queue_list); iter ++) {
			double_t _device_overall_packet_speed = 
					((double) sum_parsed_pkt_num[index] / 1e6)
					/ (parser_end_time - parser_start_time);

			double_t _device_overall_byte_speed = 
					((double) sum_parsed_pkt_len[index] / (1e9 / 8))
					/ (parser_end_time - parser_start_time);

			ss << "DPDK Port" << setw(2) << iter->first->getDeviceId();
			ss << " [" << setw(5) << setprecision(3) << _device_overall_packet_speed << " Mpps / ";
			ss << setw(5) << setprecision(3) << _device_overall_byte_speed << " Gbps]\t";
			
			index ++;
		}
		ss << endl;
		printf("%s", ss.str().c_str());
	} else {
		size_t index = 0;
		for (parser_queue_assign_t::const_iterator iter = cbegin(p_dpdk_config->nic_queue_list); 
				iter != cend(p_dpdk_config->nic_queue_list); iter ++) {
			double_t _device_overall_packet_speed = 
					((double) sum_parsed_pkt_num[index] / 1e6)
					/ (parser_end_time - parser_start_time);

			double_t _device_overall_byte_speed = 
					((double) sum_parsed_pkt_len[index] / (1e9 / 8))
					/ (parser_end_time - parser_start_time);

			index ++;
		}
	}
}


auto ParserWorkerThread::get_overall_performance() const -> pair<double_t, double_t> {
	if (!m_stop) {
		WARNF("Parsing not finsih, DO NOT collect result.");
		return {0, 0};
	}
	size_t index = 0;
	double_t thread_overall_num = 0, thread_overall_len = 0;
	for (parser_queue_assign_t::const_iterator iter = cbegin(p_dpdk_config->nic_queue_list); 
		iter != cend(p_dpdk_config->nic_queue_list); iter ++) {
			double_t _device_overall_packet_speed = 
					((double) sum_parsed_pkt_num[index] / 1e6)
					/ (parser_end_time - parser_start_time);

			double_t _device_overall_byte_speed = 
					((double) sum_parsed_pkt_len[index] / (1e9 / 8) 
					/ 1.024 / 1.024 / 1.024) / (parser_end_time - parser_start_time);

			thread_overall_num += _device_overall_packet_speed;
			thread_overall_len += _device_overall_byte_speed;
			index ++;
		}
	return {thread_overall_num, thread_overall_len};
}


auto ParserWorkerThread::configure_via_json(const json & jin) -> bool {
	if (p_parser_config != nullptr) {
		WARNF("Parser configuration overlap.");
		return false;
	}

	p_parser_config = make_shared<ParserConfigParam>();
	if (p_parser_config == nullptr) {
		WARNF("Parser configuration paramerter bad allocation.");
		return false;
	}

	try {
		// memory parameters
		if (jin.count("max_receive_burst")) {
			p_parser_config->max_receive_burst = 
					static_cast<decltype(p_parser_config->max_receive_burst)>(jin["max_receive_burst"]);
			if (p_parser_config->max_receive_burst > ParserConfigParam::RECEIVE_BURST_LIM) {
				FATAL_ERROR("Max receive burst exceed the length of maximum buffer size.");
			}
		}
		if (jin.count("parser_to_pool_pkt_metadata_arr_size")) {
			p_parser_config->parser_to_pool_pkt_metadata_arr_size = 
					static_cast<decltype(p_parser_config->parser_to_pool_pkt_metadata_arr_size)>
					(jin["parser_to_pool_pkt_metadata_arr_size"]);
			if (p_parser_config->parser_to_pool_pkt_metadata_arr_size > 
					ParserConfigParam::PARSER_TO_POOL_PKT_META_ARR_LIM) {
				FATAL_ERROR("Parser to Pool packet metadata array exceed.");
			}
		}
		if (jin.count("parser_to_buffer_pkt_arr_size")) {
			p_parser_config->parser_to_buffer_pkt_arr_size = 
					static_cast<decltype(p_parser_config->parser_to_buffer_pkt_arr_size)>
					(jin["parser_to_buffer_pkt_arr_size"]);
			if (p_parser_config->parser_to_buffer_pkt_arr_size > ParserConfigParam::PARSER_TO_BUFFER_PKT_ARR_LIM) {
				FATAL_ERROR("Parser to Buffer packet array exceed.");
			}
		}
		// verbose parameters
		if (jin.count("verbose_mode")) {
            const auto & _mode_array = jin["verbose_mode"];
			p_parser_config->verbose_mode = 0;

			for (auto _j_mode : _mode_array) {
                if (parser_verbose_mode_map.count(_j_mode) != 0) {
                    p_parser_config->verbose_mode |= parser_verbose_mode_map.at(_j_mode);
                } else {
                    WARNF("Unknown verbose mode: %s", static_cast<string>(_j_mode).c_str());
                    throw logic_error("Parse error Json tag: verbose_mode\n");
                }
            }
		}
		if (jin.count("verbose_interval")) {
			p_parser_config->verbose_interval = 
					static_cast<decltype(p_parser_config->verbose_interval)>(jin["verbose_interval"]);
		}
#ifdef TEST_BURST
		if (jin.count("test_src_ip")) {
			string str = jin["test_src_ip"];
			p_parser_config->test_src_ip = stoi(str, 0, 16);
		} else {
			FATAL_ERROR("test_src_ip unset");
			throw logic_error("test_src_ip not set when TEST_BURST defined\n");
		}
		if (jin.count("test_dst_ip")) {
			string str = jin["test_dst_ip"];
			p_parser_config->test_dst_ip = stoi(str, 0, 16);
		} else {
			FATAL_ERROR("test_dst_ip unset");
			throw logic_error("test_dst_ip not set when TEST_BURST defined\n");
		}
		if (jin.count("old_src_port_min")) {
			p_parser_config->old_src_port_min = 
					static_cast<decltype(p_parser_config->old_src_port_min)>(jin["old_src_port_min"]);
		} else {
			FATAL_ERROR("old_src_port_min unset");
			throw logic_error("old_src_port_min not set when TEST_BURST defined\n");
		}
		if (jin.count("old_src_port_max")) {
			p_parser_config->old_src_port_max = 
					static_cast<decltype(p_parser_config->old_src_port_max)>(jin["old_src_port_max"]);
		} else {
			FATAL_ERROR("old_src_port_max unset");
			throw logic_error("old_src_port_max not set when TEST_BURST defined\n");
		}
		if (jin.count("new_dst_port_min")) {
			p_parser_config->new_dst_port_min = 
					static_cast<decltype(p_parser_config->new_dst_port_min)>(jin["new_dst_port_min"]);
		} else {
			FATAL_ERROR("new_dst_port_min unset");
			throw logic_error("new_dst_port_min not set when TEST_BURST defined\n");
		}
		if (jin.count("new_dst_port_max")) {
			p_parser_config->new_dst_port_max = 
					static_cast<decltype(p_parser_config->new_dst_port_max)>(jin["new_dst_port_max"]);
		} else {
			FATAL_ERROR("new_dst_port_max unset");
			throw logic_error("new_dst_port_max not set when TEST_BURST defined\n");
		}
#endif
	} catch (exception & e) {
		WARN(e.what());
		return false;
	}
	return true;
}

