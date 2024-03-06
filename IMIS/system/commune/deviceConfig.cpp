#include "deviceConfig.hpp"

using namespace Model;
using namespace pcpp;


static inline auto __get_double_ts() -> double_t {
    struct timeval ts;
    gettimeofday(&ts, nullptr);
    return ts.tv_sec + ts.tv_usec*(1e-6);
}


auto DeviceConfig::assign_queue_to_parser(const device_list_t & dev_list, 
		const vector<SystemCore> & cores_parser) const -> assign_queue_t {
	if (verbose) {
		LOGF("Assign NIC queue to packet parsering threads.");
	}

	nic_queue_id_t total_number_que = 0;
	using nic_queue_rep_t = vector<pair<device_list_t::value_type, nic_queue_id_t> >;
	nic_queue_rep_t que_to_use;
	for (const auto p_dev: dev_list) {
		for (nic_queue_id_t i = 0; i < p_configure_param->number_rx_queue; i ++ ) {
			que_to_use.push_back({p_dev, i});
		}
		total_number_que += p_configure_param->number_rx_queue;
	}

	nic_queue_rep_t temp(que_to_use);
	que_to_use.clear();
	for (nic_queue_id_t i = 0; i < p_configure_param->number_rx_queue; i ++) {
		for(nic_port_id_t j = 0; j < dev_list.size(); j ++) {
			que_to_use.push_back(temp[j * p_configure_param->number_rx_queue + i]);
		}
	}
	nic_queue_id_t num_rx_queues_percore = total_number_que / cores_parser.size();
	nic_queue_id_t rx_queues_remainder = total_number_que % cores_parser.size();

	nic_queue_rep_t::const_iterator ite_to_assign = que_to_use.cbegin();
	assign_queue_t _assignment;
	for (const auto & _core: cores_parser) {
		printf("Using core %d for parsering.\n", _core.Id);

		auto _config = make_shared<DpdkConfig>();
		_config->core_id = _core.Id;
		for (nic_queue_id_t index = 0; index < num_rx_queues_percore; index++) {
			if (ite_to_assign == que_to_use.cend()) {
				break;
			}
			_config->nic_queue_list[ite_to_assign->first].push_back(ite_to_assign->second);
			ite_to_assign ++;
		}

		if (rx_queues_remainder > 0 && (ite_to_assign != que_to_use.cend())) {
			_config->nic_queue_list[ite_to_assign->first].push_back(ite_to_assign->second);
			ite_to_assign ++;

			rx_queues_remainder --;
		}

		_assignment.push_back(_config);
		printf("Core configuration:\n");
		for (const auto & ref: _config->nic_queue_list) {
			printf("\t DPDK device#%d: ", ref.first->getDeviceId());
			for (const auto v: ref.second) {
				printf("\t RX-Queue#%d;  ", v);
			}
			printf("\n");
		}
		if (_config->nic_queue_list.size() == 0) {
			printf("\t None\n");
		}
	}
	return _assignment;
}

auto DeviceConfig::create_worker_threads(const assign_queue_t & queue_assign,
		vector<shared_ptr<AnalyzerWorkerThread> > & analyzer_thread_vec,
		vector<shared_ptr<BufferWorkerThread> > & buffer_thread_vec,
		vector<shared_ptr<ParserWorkerThread> > & parser_thread_vec,
		vector<shared_ptr<PoolWorkerThread> > & pool_thread_vec,
		pthread_barrier_t *p_start_barrier, pthread_barrier_t *p_stop_barrier) -> bool {

#ifdef DISP_PARAM
	if (verbose) {
		p_configure_param->display_params();
	}
#endif

	for (size_t i = 0; i < p_configure_param->core_num_for_parser; i++) {
		const auto p_new_parser = make_shared<ParserWorkerThread>(queue_assign[i], p_start_barrier, p_stop_barrier);
		if (p_new_parser == nullptr) {
			return false;
		}

		parser_thread_vec.push_back(p_new_parser);

		if (j_cfg_parser.size() != 0) {
			p_new_parser->configure_via_json(j_cfg_parser);
		}
	}

#ifdef DISP_PARAM
	if (verbose) {
		parser_thread_vec[0]->p_parser_config->display_params();
	}
#endif

	for (size_t i = 0; i < p_configure_param->core_num_for_pool; i++) {
		const auto p_new_pool = make_shared<PoolWorkerThread>(p_start_barrier, p_stop_barrier);
		if (p_new_pool == nullptr) {
			return false;
		}
		pool_thread_vec.push_back(p_new_pool);

		if (j_cfg_pool.size() != 0) {
			p_new_pool->configure_via_json(j_cfg_pool);
		}
		p_new_pool->bind_parser(parser_thread_vec[i]);
	}

#ifdef DISP_PARAM
	if (verbose) {
		pool_thread_vec[0]->p_pool_config->display_params();
	}
#endif

	// bind the ParserWorkers to the AnalyzerWorker
	for (cpu_core_id_t i = 0; i < p_configure_param->core_num_for_analyzer; i ++) {
		const auto p_new_analyzer = make_shared<AnalyzerWorkerThread>(p_start_barrier, p_stop_barrier);
		if (p_new_analyzer == nullptr) {
			return false;
		}
		analyzer_thread_vec.push_back(p_new_analyzer);
		
		p_new_analyzer->bind_pool(pool_thread_vec[i]);
		pool_thread_vec[i]->bind_analyzer(p_new_analyzer);

		if (j_cfg_analyzer.size() != 0) {
			p_new_analyzer->configure_via_json(j_cfg_analyzer);
		}
	}

#ifdef DISP_PARAM
	if (verbose) {
		analyzer_thread_vec[0]->p_analyzer_config->display_params();
	}
#endif

	for (size_t i = 0; i < p_configure_param->core_num_for_buffer; i++) {
		const auto p_new_buffer = make_shared<BufferWorkerThread>(p_start_barrier, p_stop_barrier);
		if (p_new_buffer == nullptr) {
			return false;
		}
		buffer_thread_vec.push_back(p_new_buffer);

		p_new_buffer->bind_parser(parser_thread_vec[i]);
		p_new_buffer->bind_analyzer(analyzer_thread_vec[i]);
		p_new_buffer->bind_tx_device(p_configure_param->output_port, i);
		
		if (j_cfg_buffer.size() != 0) {
			p_new_buffer->configure_via_json(j_cfg_buffer);
		}
	}

#ifdef DISP_PARAM
	if (verbose) {
		buffer_thread_vec[0]->p_buffer_config->display_params();
	}
#endif

	return true;
}


void DeviceConfig::interrupt_callback(void* cookie) {
	ThreadStateManagement * args = (ThreadStateManagement *) cookie;

	printf("\n ----- Model stopped ----- \n");

	// stop worker threads

	for (auto & _p_thread: args->analyzer_worker_thread_vec) {
		_p_thread->stop();
	}
	for (auto & _p_thread: args->pool_worker_thread_vec) {
		_p_thread->stop();
	}
	for (auto & _p_thread: args->parser_worker_thread_vec) {
		_p_thread->stop();
	}
	for (auto & _p_thread: args->buffer_worker_thread_vec) {
		_p_thread->stop();
	}

	pthread_barrier_wait(args->p_stop_barrier);
	printf("All thread stopped.\n");
	pthread_barrier_destroy(args->p_stop_barrier);
	delete args->p_stop_barrier;

	// print final stats for every worker thread plus sum of all threads and free worker threads memory
	double_t overall_parser_num = 0, overall_parser_len = 0;
	bool __is_print_parser = false;
	for (auto & _p_thread: args->parser_worker_thread_vec) {
		const auto ref = _p_thread->get_overall_performance();
		overall_parser_num += ref.first;
		overall_parser_len += ref.second;
		if (_p_thread->p_parser_config->verbose_mode & _p_thread->p_parser_config->SUMMARY) {
			__is_print_parser = true;
		}
	}
	if (__is_print_parser) {
		LOGF("Parser Overall Performance: [%5.3lf Mpps / %5.3lf Gbps]", overall_parser_num, overall_parser_len);
	}

	args->stop = true;
}


auto DeviceConfig::configure_dpdk_nic(const CoreMask mask_all_used_core, const uint8_t master_core) const -> device_list_t {
	LOGF("Init DPDK device.");

	// initialize DPDK
	if (dpdk_init_once) {
		LOGF("DPDK has already init.");
	} else {
		char ** tmp = new char * [p_configure_param->eal_param_vec.size()];
		for (int i = 0; i < p_configure_param->eal_param_vec.size(); i++) {
			tmp[i] = new char [p_configure_param->eal_param_vec[i].size() + 1];
			strcpy(tmp[i], p_configure_param->eal_param_vec[i].c_str());
		}
		
		if (!DpdkDeviceList::initDpdk(mask_all_used_core, p_configure_param->mbuf_pool_size, master_core, 
				p_configure_param->eal_param_vec.size(), tmp)) {
			for (int i = 0; i < p_configure_param->eal_param_vec.size(); i++) {
				delete tmp[i];
			}
			delete tmp;
			FATAL_ERROR("Couldn't initialize DPDK.");
		} else {
			dpdk_init_once = true;
			LOGF("DPDK initialized.");
			for (int i = 0; i < p_configure_param->eal_param_vec.size(); i++) {
				delete tmp[i];
			}
			delete tmp;
		}
	}

	// collect the list of DPDK devices
	device_list_t device_to_use;
	for (const auto dpdk_port: p_configure_param->dpdk_port_vec) {
		const auto p_dev = DpdkDeviceList::getInstance().getDeviceByPort(dpdk_port);
		if (p_dev == nullptr) {
			FATAL_ERROR("Couldn't initialize DPDK device.");
		}
		device_to_use.push_back(p_dev);
	}

	// go over all devices and open them
	for (const auto & p_dev: device_to_use) {
		if (p_dev->getTotalNumOfRxQueues() < p_configure_param->number_rx_queue) {
			WARNF("Number of requeired receive queue exceeds (%d) NIC support (%d).", 
					p_configure_param->number_rx_queue, p_dev->getTotalNumOfRxQueues());
			FATAL_ERROR("Device opening fail.");
		}
		if (p_dev->getTotalNumOfTxQueues() < p_configure_param->number_tx_queue) {
			WARNF("Number of requeired transit queue exceeds (%d) NIC support (%d).", 
					p_configure_param->number_tx_queue, p_dev->getTotalNumOfTxQueues());
			FATAL_ERROR("Device opening fail.");
		}

		DpdkDevice::DpdkDeviceConfiguration dev_cfg;


		static uint8_t rss_hash_key[40] =
			{ 0x6D, 0x5A, 0x6D, 0x5A, 0x6D, 0x5A, 0x6D, 0x5A 
			, 0x6D, 0x5A, 0x6D, 0x5A, 0x6D, 0x5A, 0x6D, 0x5A 
			, 0x6D, 0x5A, 0x6D, 0x5A, 0x6D, 0x5A, 0x6D, 0x5A 
			, 0x6D, 0x5A, 0x6D, 0x5A, 0x6D, 0x5A, 0x6D, 0x5A
			, 0x6D, 0x5A, 0x6D, 0x5A, 0x6D, 0x5A, 0x6D, 0x5A 
			};
		
		dev_cfg.rssKey = rss_hash_key;
		dev_cfg.rssKeyLength = sizeof(rss_hash_key) / sizeof(rss_hash_key[0]);
		dev_cfg.rssHashFunction = DpdkDevice::RSS_IPV4 | DpdkDevice::RSS_NONFRAG_IPV4_TCP | DpdkDevice::RSS_NONFRAG_IPV4_UDP;

		if (p_dev->openMultiQueues(p_configure_param->number_rx_queue, p_configure_param->number_tx_queue, dev_cfg)) {
			LOGF("Device open %s success.", p_dev->getDeviceName().c_str());
		} else {
			FATAL_ERROR("Device opening fail.");
		}
	}
	
	return device_to_use;
}


void DeviceConfig::do_init() {
	LOGF("Configure Model runtime environment.");

	static const auto _f_check_device_configure_param = [] (decltype(p_configure_param) p_param)-> bool {
		if (!p_param) {
			WARNF("Configure struct not found.");
			return false;
		}
		if (p_param->dpdk_port_vec.empty()) {
			WARNF("DPDK port list is empty.");
			return false;
		}

		CoreMask core_mask_master = 0;
		CoreMask core_mask_analyzer = 0;
		CoreMask core_mask_buffer = 0;
		CoreMask core_mask_parser = 0;
		CoreMask core_mask_pool = 0;

		bool core_is_used[MAX_NUM_OF_CORES];
		memset(core_is_used, 0, sizeof(core_is_used));

		if (p_param->master_core < MAX_NUM_OF_CORES) {
			core_mask_master |= SystemCores::IdToSystemCore[p_param->master_core].Mask;
			core_is_used[p_param->master_core] = true;
		} else {
			WARNF("Core for Master exceeds the range supported by Libpcapplusplus.");
			return false;
		}

		for (auto core: p_param->core_list_for_analyzer) {
			if (core < MAX_NUM_OF_CORES) {
				if (!core_is_used[core]) {
					core_mask_analyzer |= SystemCores::IdToSystemCore[core].Mask;
					core_is_used[core] = true;
				} else {
					WARNF("Core for Analyzer has been used.");
					return false;
				}
			} else {
				WARNF("Core for Analyzer exceeds the range supported by Libpcapplusplus.");
				return false;
			}
		}

		for (auto core: p_param->core_list_for_buffer) {
			if (core < MAX_NUM_OF_CORES) {
				if (!core_is_used[core]) {
					core_mask_buffer |= SystemCores::IdToSystemCore[core].Mask;
					core_is_used[core] = true;
				} else {
					WARNF("Core for Buffer has been used.");
					return false;
				}
			} else {
				WARNF("Core for Buffer exceeds the range supported by Libpcapplusplus.");
				return false;
			}
		}

		for (auto core: p_param->core_list_for_parser) {
			if (core < MAX_NUM_OF_CORES) {
				if (!core_is_used[core]) {
					core_mask_parser |= SystemCores::IdToSystemCore[core].Mask;
					core_is_used[core] = true;
				} else {
					WARNF("Core for Parser has been used.");
					return false;
				}
			} else {
				WARNF("Core for Parser exceeds the range supported by Libpcapplusplus.");
				return false;
			}
		}

		for (auto core: p_param->core_list_for_pool) {
			if (core < MAX_NUM_OF_CORES) {
				if (!core_is_used[core]) {
					core_mask_pool |= SystemCores::IdToSystemCore[core].Mask;
					core_is_used[core] = true;
				} else {
					WARNF("Core for Pool has been used.");
					return false;
				}
			} else {
				WARNF("Core for Pool exceeds the range supported by Libpcapplusplus.");
				return false;
			}
		}

		if (p_param->core_num_for_analyzer != p_param->core_num_for_parser) {
			WARNF("Imbalanced number of cores for Analyzer and Parser.");
			return false;
		}
		if (p_param->core_num_for_buffer != p_param->core_num_for_parser) {
			WARNF("Imbalanced number of cores for Buffer and Parser.");
			return false;
		}
		if (p_param->core_num_for_pool != p_param->core_num_for_parser) {
			WARNF("Imbalanced number of cores for Pool and Parser.");
			return false;
		}

		if (p_param->core_num_for_buffer > p_param->number_tx_queue) {
			WARNF("Number of cores Buffer exceeds the number_tx_queue.");
			return false;
		}

		if (p_param->core_list_for_analyzer.size() != p_param->core_num_for_analyzer) {
			WARNF("Conflict number of cores for Analyzer.");
			return false;
		}
		if (p_param->core_list_for_buffer.size() != p_param->core_num_for_buffer) {
			WARNF("Conflict number of cores for Buffer.");
			return false;
		}
		if (p_param->core_list_for_parser.size() != p_param->core_num_for_parser) {
			WARNF("Conflict number of cores for Parser.");
			return false;
		}
		if (p_param->core_list_for_pool.size() != p_param->core_num_for_pool) {
			WARNF("Conflict number of cores for Pool.");
			return false;
		}
		if (p_param->core_num_for_analyzer + p_param->core_num_for_buffer + p_param->core_num_for_parser 
				+ p_param->core_num_for_pool + 1 != p_param->core_num) {
			WARNF("Conflict total number of cores.");
			return false;
		}
		return true;
	};

	if (!_f_check_device_configure_param(p_configure_param)) {
		FATAL_ERROR("Configure is invalid.");
	}

	// configure PcapPlusPlus Log Error Level
	// Logger::getInstance().enableLogs();	
	
	// CoreMask core_mask_to_use = (1 << p_configure_param->core_num) - 1;
	CoreMask core_mask_to_use = 0;
	CoreMask core_mask_analyzer = 0;
	CoreMask core_mask_buffer = 0;
	CoreMask core_mask_parser = 0;
	CoreMask core_mask_pool = 0;
	CoreMask core_mask_master = SystemCores::IdToSystemCore[p_configure_param->master_core].Mask;

	for (auto core: p_configure_param->core_list_for_analyzer) {
		core_mask_analyzer |= SystemCores::IdToSystemCore[core].Mask;
	}
	for (auto core: p_configure_param->core_list_for_buffer) {
		core_mask_buffer |= SystemCores::IdToSystemCore[core].Mask;
	}
	for (auto core: p_configure_param->core_list_for_parser) {
		core_mask_parser |= SystemCores::IdToSystemCore[core].Mask;
	}
	for (auto core: p_configure_param->core_list_for_pool) {
		core_mask_pool |= SystemCores::IdToSystemCore[core].Mask;
	}

	core_mask_to_use = core_mask_analyzer | core_mask_buffer | core_mask_parser | core_mask_pool | core_mask_master;

	// configure DPDK NIC
	const auto device_list = this->configure_dpdk_nic(core_mask_to_use, p_configure_param->master_core);

	// prepare configuration for every core
	CoreMask core_mask_without_master = core_mask_to_use & ~(DpdkDeviceList::getInstance().getDpdkMasterCore().Mask);

	vector<SystemCore> core_analyzer;
	createCoreVectorFromCoreMask(core_mask_analyzer, core_analyzer);
	vector<SystemCore> core_buffer;
	createCoreVectorFromCoreMask(core_mask_buffer, core_buffer);
	vector<SystemCore> core_parser;
	createCoreVectorFromCoreMask(core_mask_parser, core_parser);
	vector<SystemCore> core_pool;
	createCoreVectorFromCoreMask(core_mask_pool, core_pool);



	/*for (auto _core: core_parser) {
		printf("![%" PRIx64 ", %u]\n", _core.Mask, _core.Id);
	}*/

	assign_queue_t nic_queue_assign = assign_queue_to_parser(device_list, core_parser);

	// create worker parser thread for each core
	
	vector<shared_ptr<AnalyzerWorkerThread> > analyzer_thread_vec;
	vector<shared_ptr<BufferWorkerThread> > buffer_thread_vec;
	vector<shared_ptr<ParserWorkerThread> > parser_thread_vec;
	vector<shared_ptr<PoolWorkerThread> > pool_thread_vec;

	pthread_barrier_t *p_start_barrier = new pthread_barrier_t;
	pthread_barrier_t *p_stop_barrier = new pthread_barrier_t;

	pthread_barrier_init(p_start_barrier, nullptr,
			p_configure_param->core_num_for_buffer + p_configure_param->core_num_for_analyzer
			+ p_configure_param->core_num_for_parser + p_configure_param->core_num_for_pool + 1);
	
	pthread_barrier_init(p_stop_barrier, nullptr,
			p_configure_param->core_num_for_buffer + p_configure_param->core_num_for_analyzer
			+ p_configure_param->core_num_for_parser + p_configure_param->core_num_for_pool + 1);

	if (!create_worker_threads(nic_queue_assign, analyzer_thread_vec, buffer_thread_vec, parser_thread_vec, pool_thread_vec,
		p_start_barrier, p_stop_barrier)) {
		pthread_barrier_destroy(p_start_barrier);
		pthread_barrier_destroy(p_stop_barrier);
		delete p_start_barrier;
		delete p_stop_barrier;
		FATAL_ERROR("Thread allocation failed.");
	}

	// start all worker threads, memory safe
// #define SPLIT_START_SUPPORT_PCPP

#ifdef SPLIT_START_SUPPORT_PCPP
	vector<DpdkWorkerThread *> _thread_vec_all;
	transform(analyzer_thread_vec.cbegin(), analyzer_thread_vec.cend(), back_inserter(_thread_vec_all),
			[] (decltype(analyzer_thread_vec)::value_type _p) -> DpdkWorkerThread * {return _p.get(); });

	assert(core_analyzer.size() == _thread_vec_all.size());
	if (!DpdkDeviceList::getInstance().startDpdkWorkerThreads(core_mask_analyzer, _thread_vec_all)) {
		FATAL_ERROR("Couldn't start analyzer worker threads");
	}

	_thread_vec_all.clear();
	transform(buffer_thread_vec.cbegin(), buffer_thread_vec.cend(), back_inserter(_thread_vec_all),
			[] (decltype(buffer_thread_vec)::value_type _p) -> DpdkWorkerThread * {return _p.get(); });

	assert(core_buffer.size() == _thread_vec_all.size());
	if (!DpdkDeviceList::getInstance().startDpdkWorkerThreads(core_mask_buffer, _thread_vec_all)) {
		FATAL_ERROR("Couldn't start buffer worker threads");
	}

	_thread_vec_all.clear();
	transform(parser_thread_vec.cbegin(), parser_thread_vec.cend(), back_inserter(_thread_vec_all),
			[] (decltype(parser_thread_vec)::value_type _p) -> DpdkWorkerThread * {return _p.get(); });

	assert(core_parser.size() == _thread_vec_all.size());
	if (!DpdkDeviceList::getInstance().startDpdkWorkerThreads(core_mask_parser, _thread_vec_all)) {
		FATAL_ERROR("Couldn't start parser worker threads");
	}

	_thread_vec_all.clear();
	transform(pool_thread_vec.cbegin(), pool_thread_vec.cend(), back_inserter(_thread_vec_all),
			[] (decltype(pool_thread_vec)::value_type _p) -> DpdkWorkerThread * {return _p.get(); });

	assert(core_pool.size() == _thread_vec_all.size());
	if (!DpdkDeviceList::getInstance().startDpdkWorkerThreads(core_mask_pool, _thread_vec_all)) {
		FATAL_ERROR("Couldn't start pool worker threads");
	}
#else

	vector<DpdkWorkerThread *> _thread_vec_all;
	int analyzer_core_to_id[64];
	int buffer_core_to_id[64];
	int parser_core_to_id[64];
	int pool_core_to_id[64];

	for (int i = 0; i < p_configure_param->core_num_for_analyzer; i++) {
		analyzer_core_to_id[p_configure_param->core_list_for_analyzer[i]] = i;
	}
	for (int i = 0; i < p_configure_param->core_num_for_buffer; i++) {
		buffer_core_to_id[p_configure_param->core_list_for_buffer[i]] = i;
	}
	for (int i = 0; i < p_configure_param->core_num_for_parser; i++) {
		parser_core_to_id[p_configure_param->core_list_for_parser[i]] = i;
	}
	for (int i = 0; i < p_configure_param->core_num_for_pool; i++) {
		pool_core_to_id[p_configure_param->core_list_for_pool[i]] = i;
	}
	// The Pcap++ has to be modified to support 64 lcores
	for (uint8_t i = 0; i <= 63; i++) {
		if (core_mask_analyzer & SystemCores::IdToSystemCore[i].Mask) {
			_thread_vec_all.push_back(analyzer_thread_vec[analyzer_core_to_id[i]].get());
		} else if (core_mask_buffer & SystemCores::IdToSystemCore[i].Mask) {
			_thread_vec_all.push_back(buffer_thread_vec[buffer_core_to_id[i]].get());
		} else if (core_mask_parser & SystemCores::IdToSystemCore[i].Mask) {
			_thread_vec_all.push_back(parser_thread_vec[parser_core_to_id[i]].get());
		} else if (core_mask_pool & SystemCores::IdToSystemCore[i].Mask) {
			_thread_vec_all.push_back(pool_thread_vec[pool_core_to_id[i]].get());
		}
	}
	if (!DpdkDeviceList::getInstance().startDpdkWorkerThreads(core_mask_without_master, _thread_vec_all)) {
		FATAL_ERROR("Couldn't start worker threads");
	}

#endif

	// register the on app close event to print summary stats on app termination
	ThreadStateManagement args(analyzer_thread_vec, buffer_thread_vec, parser_thread_vec, pool_thread_vec, p_stop_barrier);
	ApplicationEventHandler::getInstance().onApplicationInterrupted(interrupt_callback, &args);
	pthread_barrier_wait(p_start_barrier);
	printf("All thread started.\n");
	pthread_barrier_destroy(p_start_barrier);
	delete p_start_barrier;
	double_t _t = __get_double_ts();
	uint64_t _ipackets = 0;
	uint64_t _opackets = 0;
	uint64_t _ibytes = 0;
	uint64_t _obytes = 0;
	uint64_t _imissed = 0;
	rte_eth_stats_reset(0);
	while (!args.stop) {
		multiPlatformSleep(3);
		rte_eth_stats stats;
		rte_eth_stats_get(0, &stats);
		double _tmp = __get_double_ts();
		double _delta = _tmp - _t;
		_t = _tmp;
		
		stringstream ss;
		ss << fixed << setprecision(4) << "RTE_STAT: ipackets:" << (double_t)(stats.ipackets - _ipackets) / _delta / 1e6 
				<< " Mpps; opackets: " << (double_t)(stats.opackets - _opackets) / _delta / 1e6 << " Mpps;" << endl
				<< "  ibytes :" << (double_t)(stats.ibytes - _ibytes) * 8 / _delta / 1e9  
				<< "  Gbps; obytes: " << (double_t)(stats.obytes - _obytes) * 8 / _delta / 1e9 << " Gbps;" << endl
				<< "  imissed: " << (double_t)(stats.imissed - _imissed) / _delta / 1e3 << " kpps; rx_nombuf: " 
				<< stats.rx_nombuf << " pkts" << endl;
		const string &str2 = ss.str();
		LOGF("%s", str2.c_str());
		_ipackets = stats.ipackets;
		_opackets = stats.opackets;
		_ibytes = stats.ibytes;
		_obytes = stats.obytes;
		_imissed = stats.imissed;
	}
}


auto DeviceConfig::configure_via_json(const json & jin) -> bool {
	
	if (p_configure_param) {
		LOGF("Device init param modification.");
		p_configure_param = nullptr;
	}

	try {
		// parameters for dpdk
		const auto _device_param = make_shared<DeviceConfigParam>();
		if (_device_param == nullptr) {
			WARNF("device paramerter bad allocation");
			throw bad_alloc();
		}
		if (jin.find("DPDK") == jin.end()) {
			WARNF("DPDK config enrty not found.");
			return false;
		}
		const auto & dpdk_config = jin["DPDK"];

		if (dpdk_config.count("mbuf_pool_size"))
			_device_param->mbuf_pool_size = static_cast<mem_pool_size_t>(dpdk_config["mbuf_pool_size"]);

		if (dpdk_config.count("number_rx_queue")) 
			_device_param->number_rx_queue = static_cast<nic_queue_id_t>(dpdk_config["number_rx_queue"]);
		
		if (dpdk_config.count("number_tx_queue")) 
			_device_param->number_tx_queue = static_cast<nic_queue_id_t>(dpdk_config["number_tx_queue"]);

		if (dpdk_config.count("core_num_for_analyzer")) 
			_device_param->core_num_for_analyzer = static_cast<cpu_core_id_t>(dpdk_config["core_num_for_analyzer"]);

		if (dpdk_config.count("core_num_for_buffer")) 
			_device_param->core_num_for_buffer = static_cast<cpu_core_id_t>(dpdk_config["core_num_for_buffer"]);
		
		if (dpdk_config.count("core_num_for_parser")) 
			_device_param->core_num_for_parser = static_cast<cpu_core_id_t>(dpdk_config["core_num_for_parser"]);
		
		if (dpdk_config.count("core_num_for_pool")) 
			_device_param->core_num_for_pool = static_cast<cpu_core_id_t>(dpdk_config["core_num_for_pool"]);

		if (dpdk_config.count("core_num")) 
			_device_param->core_num = static_cast<cpu_core_id_t>(dpdk_config["core_num"]);
		
		if (dpdk_config.count("core_list_for_analyzer")) {
			const auto & _port_array = dpdk_config["core_list_for_analyzer"];
			_device_param->core_list_for_analyzer.clear();
			for (auto idx : _port_array) _device_param->core_list_for_analyzer.push_back(static_cast<cpu_core_id_t>(idx));
		} else {
			WARNF("Missing core list for Analyzer.");
			return false;
		}

		if (dpdk_config.count("core_list_for_buffer")) {
			const auto & _port_array = dpdk_config["core_list_for_buffer"];
			_device_param->core_list_for_buffer.clear();
			for (auto idx : _port_array) _device_param->core_list_for_buffer.push_back(static_cast<cpu_core_id_t>(idx));
		} else {
			WARNF("Missing core list for Buffer.");
			return false;
		}

		if (dpdk_config.count("core_list_for_parser")) {
			const auto & _port_array = dpdk_config["core_list_for_parser"];
			_device_param->core_list_for_parser.clear();
			for (auto idx : _port_array) _device_param->core_list_for_parser.push_back(static_cast<cpu_core_id_t>(idx));
		} else {
			WARNF("Missing core list for Parser.");
			return false;
		}

		if (dpdk_config.count("core_list_for_pool")) {
			const auto & _port_array = dpdk_config["core_list_for_pool"];
			_device_param->core_list_for_pool.clear();
			for (auto idx : _port_array) _device_param->core_list_for_pool.push_back(static_cast<cpu_core_id_t>(idx));
		}
		 else {
			WARNF("Missing core list for Pool.");
			return false;
		}

		if (dpdk_config.count("master_core")) {
			_device_param->master_core = 
				static_cast<cpu_core_id_t>(dpdk_config["master_core"]);
		} else {
			WARNF("Missing Master core.");
			return false;
		}

		if (dpdk_config.count("verbose")) {
			verbose = dpdk_config["verbose"];
		}

		if (dpdk_config.count("dpdk_port_vec")) {
			const auto & _port_array = dpdk_config["dpdk_port_vec"];
			_device_param->dpdk_port_vec.clear();
			_device_param->dpdk_port_vec.assign(_port_array.cbegin(), _port_array.cend());
		}
		_device_param->dpdk_port_vec.shrink_to_fit();

		if (dpdk_config.count("output_port")) {
			_device_param->output_port = static_cast<cpu_core_id_t>(dpdk_config["output_port"]);
		}

		if (dpdk_config.count("eal_param_vec")) {
			const auto & _param_array = dpdk_config["eal_param_vec"];
			_device_param->eal_param_vec.clear();
			_device_param->eal_param_vec.assign(_param_array.cbegin(), _param_array.cend());
		}
		_device_param->dpdk_port_vec.shrink_to_fit();


		p_configure_param = _device_param;

		// paramters for Analyzer, Buffer, Parser, Pool
		
		if (jin.find("Analyzer") != jin.end()) j_cfg_analyzer = jin["Analyzer"];
		else WARNF("Analyzer configuration not found, use default.");
		
		if (jin.find("Buffer") != jin.end()) j_cfg_buffer = jin["Buffer"];
		else WARNF("Buffer configuration not found, use default.");

		if (jin.find("Parser") != jin.end()) j_cfg_parser = jin["Parser"];
		else WARNF("Parser configuration not found, use default.");

		if (jin.find("Pool") != jin.end()) j_cfg_pool = jin["Pool"];
		else WARNF("Pool configuration not found, use default.");

		// if (jin.find("background_flows") != jin.end()) j_cfg_pool["background_flows"] = (int)jin["background_flows"] / (int)dpdk_config["core_num_for_analyzer"];

	} catch(exception & e) {
		FATAL_ERROR(e.what());
	}

    LOGF("Modify device configure param via json.");
	return true;
}