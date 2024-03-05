# bfrt_python <path to p4>/testbed_version/runtime/cleanup.py

import bfrtcli
from bfrtcli import bfrt

# bfrt.BoS_testbed.pipe_a.SwitchIngress.r_flow_info.dump(print_zero=False,from_hw=True)

bfrt.BoS_testbed.pipe_a.SwitchIngress.r_flow_info.clear()
bfrt.BoS_testbed.pipe_a.SwitchIngress.r_pkt_cnt_initial_8.clear()
bfrt.BoS_testbed.pipe_a.SwitchIngress.r_pkt_cnt_mod_7.clear()

# bfrt.BoS_testbed.pipe_b.AuxIngress.r_global_rnn_cnt.clear()
bfrt.BoS_testbed.pipe_b.AuxIngress.r_global_rnn_ground_truth_cnt.clear()
# bfrt.BoS_testbed.pipe_b.AuxIngress.r_global_rnn_pred_cnt.clear()
# bfrt.BoS_testbed.pipe_b.AuxIngress.r_global_rnn_tp_cnt.clear()
bfrt.BoS_testbed.pipe_b.AuxIngress.r_global_rnn_conf_mat_cnt.clear()

bfrt.BoS_testbed.pipe_b.AuxIngress.r_global_perpacket_ground_truth_cnt.clear()
bfrt.BoS_testbed.pipe_b.AuxIngress.r_global_perpacket_conf_mat_cnt.clear()

bfrt.BoS_testbed.pipe_b.AuxIngress.r_global_IMIS_ground_truth_cnt.clear()
bfrt.BoS_testbed.pipe_b.AuxIngress.r_global_collision_IMIS_ground_truth_cnt.clear()
bfrt.BoS_testbed.pipe_b.AuxIngress.r_global_pre_analysis_ground_truth_cnt.clear()

bfrt.BoS_testbed.pipe_b.AuxIngress.r_aux_ig_global_pkt_cnt.clear()

bfrt.BoS_testbed.pipe_a.SwitchIngress.r_ig_global_pkt_cnt.clear()
bfrt.BoS_testbed.pipe_a.SwitchIngress.r_ig_global_recir_pkt_cnt.clear()

bfrt.BoS_testbed.pipe_b.SwitchEgress.r_eg_global_pkt_cnt.clear()
bfrt.BoS_testbed.pipe_b.SwitchEgress.r_eg_global_to_mirror_pkt_cnt.clear()

bfrt.BoS_testbed.pipe_a.AuxEgress.r_aux_eg_global_to_recir_pkt_cnt.clear()
bfrt.BoS_testbed.pipe_a.AuxEgress.r_aux_eg_global_normal_pkt_cnt.clear()
