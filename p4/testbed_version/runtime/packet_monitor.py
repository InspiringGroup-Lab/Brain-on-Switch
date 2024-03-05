# bfrt_python <path to p4>/testbed_version/runtime/packet_monitor.py

import time
import json
import os
import bfrtcli
from bfrtcli import bfrt

term_size = os.get_terminal_size()

n_repeat_time = 1

n_cls = 6
assert(n_cls in range(8))
sleep_time = 1

name = ['Class {}'.format(i) for i in range(n_cls)]

dividing_line = "*" * term_size.columns

for _ in range(n_repeat_time):
  
  rnn_ground_truth = [0] * n_cls
  rnn_conf_mat = []
  for _ in range(n_cls):
    rnn_conf_mat.append([0] * n_cls)
  
  rnn_pred = [0] * n_cls
  rnn_tp = [0] * n_cls
  rnn_f1 = [0] * n_cls

  perpacket_ground_truth = [0] * n_cls
  perpacket_conf_mat = []
  for _ in range(n_cls):
    perpacket_conf_mat.append([0] * n_cls)

  perpacket_pred = [0] * n_cls
  perpacket_tp = [0] * n_cls
  perpacket_f1 = [0] * n_cls

  pre_analysis_ground_truth = [0] * n_cls
  IMIS_ground_truth = [0] * n_cls
  collision_IMIS_ground_truth = [0] * n_cls

  print(dividing_line)
  tmp = bfrt.BoS_testbed.pipe_b.AuxIngress.r_global_rnn_ground_truth_cnt.dump(from_hw=True,json=True)
  js = json.loads(tmp)
  for i in range(n_cls):
    rnn_ground_truth[i] = js[i]['data']['AuxIngress.r_global_rnn_ground_truth_cnt.f1'][0]
  
  tmp = bfrt.BoS_testbed.pipe_b.AuxIngress.r_global_rnn_conf_mat_cnt.dump(from_hw=True,json=True)
  js = json.loads(tmp)
  for i in range(n_cls):
    for j in range(n_cls):
      rnn_conf_mat[i][j] = js[i*8+j]['data']['AuxIngress.r_global_rnn_conf_mat_cnt.f1'][0]
  
  for i in range(n_cls):
    tmp = 0
    for j in range(n_cls):
      tmp += rnn_conf_mat[i][j]
    assert(tmp == rnn_ground_truth[i])
  
  for j in range(n_cls):
    rnn_pred[j] = 0
    for i in range(n_cls):
      rnn_pred[j] += rnn_conf_mat[i][j]
  for i in range(n_cls):
    rnn_tp[i] = rnn_conf_mat[i][i]

  tmp = bfrt.BoS_testbed.pipe_b.AuxIngress.r_global_perpacket_ground_truth_cnt.dump(from_hw=True,json=True)
  js = json.loads(tmp)
  for i in range(n_cls):
    perpacket_ground_truth[i] = js[i]['data']['AuxIngress.r_global_perpacket_ground_truth_cnt.f1'][0]
  
  tmp = bfrt.BoS_testbed.pipe_b.AuxIngress.r_global_perpacket_conf_mat_cnt.dump(from_hw=True,json=True)
  js = json.loads(tmp)
  for i in range(n_cls):
    for j in range(n_cls):
      perpacket_conf_mat[i][j] = js[i*8+j]['data']['AuxIngress.r_global_perpacket_conf_mat_cnt.f1'][0]
  for i in range(n_cls):
    tmp = 0
    for j in range(n_cls):
      tmp += perpacket_conf_mat[i][j]
    assert(tmp == perpacket_ground_truth[i])
  
  for j in range(n_cls):
    perpacket_pred[j] = 0
    for i in range(n_cls):
      perpacket_pred[j] += perpacket_conf_mat[i][j]
  for i in range(n_cls):
    perpacket_tp[i] = perpacket_conf_mat[i][i]

  tmp = bfrt.BoS_testbed.pipe_b.AuxIngress.r_global_IMIS_ground_truth_cnt.dump(from_hw=True,json=True)
  js = json.loads(tmp)
  for i in range(n_cls):
    IMIS_ground_truth[i] = js[i]['data']['AuxIngress.r_global_IMIS_ground_truth_cnt.f1'][0]
  
  tmp = bfrt.BoS_testbed.pipe_b.AuxIngress.r_global_pre_analysis_ground_truth_cnt.dump(from_hw=True,json=True)
  js = json.loads(tmp)
  for i in range(n_cls):
    pre_analysis_ground_truth[i] = js[i]['data']['AuxIngress.r_global_pre_analysis_ground_truth_cnt.f1'][0]

  tmp = bfrt.BoS_testbed.pipe_b.AuxIngress.r_global_collision_IMIS_ground_truth_cnt.dump(from_hw=True,json=True)
  js = json.loads(tmp)
  for i in range(n_cls):
    collision_IMIS_ground_truth[i] = js[i]['data']['AuxIngress.r_global_collision_IMIS_ground_truth_cnt.f1'][0]

  tmp = bfrt.BoS_testbed.pipe_b.AuxIngress.r_aux_ig_global_pkt_cnt.dump(from_hw=True,json=True)
  js = json.loads(tmp)
  aux_ig_pkt_sum = js[0]['data']['AuxIngress.r_aux_ig_global_pkt_cnt.f1'][0]
  
  tmp = bfrt.BoS_testbed.pipe_a.SwitchIngress.r_ig_global_pkt_cnt.dump(from_hw=True,json=True)
  js = json.loads(tmp)
  ig_pkt_sum = js[0]['data']['SwitchIngress.r_ig_global_pkt_cnt.f1'][0]

  tmp = bfrt.BoS_testbed.pipe_a.SwitchIngress.r_ig_global_recir_pkt_cnt.dump(from_hw=True,json=True)
  js = json.loads(tmp)
  ig_recir_sum = js[0]['data']['SwitchIngress.r_ig_global_recir_pkt_cnt.f1'][0]

  tmp = bfrt.BoS_testbed.pipe_b.SwitchEgress.r_eg_global_pkt_cnt.dump(from_hw=True,json=True)
  js = json.loads(tmp)
  eg_pkt_sum = js[0]['data']['SwitchEgress.r_eg_global_pkt_cnt.f1'][0]

  tmp = bfrt.BoS_testbed.pipe_b.SwitchEgress.r_eg_global_to_mirror_pkt_cnt.dump(from_hw=True,json=True)
  js = json.loads(tmp)
  eg_to_mirror_sum = js[0]['data']['SwitchEgress.r_eg_global_to_mirror_pkt_cnt.f1'][0]

  tmp = bfrt.BoS_testbed.pipe_a.AuxEgress.r_aux_eg_global_to_recir_pkt_cnt.dump(from_hw=True,json=True)
  js = json.loads(tmp)
  aux_eg_to_recir_sum = js[0]['data']['AuxEgress.r_aux_eg_global_to_recir_pkt_cnt.f1'][0]

  tmp = bfrt.BoS_testbed.pipe_a.AuxEgress.r_aux_eg_global_normal_pkt_cnt.dump(from_hw=True,json=True)
  js = json.loads(tmp)
  aux_eg_normal_sum = js[0]['data']['AuxEgress.r_aux_eg_global_normal_pkt_cnt.f1'][0]

  print(dividing_line)
  print('RNN part')
  print('{:7} |'.format(''), end='')
  for i in range(n_cls):
    print(' {:>10} |'.format(name[i]), end='')
  print('')
  for i in range(n_cls):
    print('{:7} |'.format(name[i]), end='')
    for j in range(n_cls):
      print(' {:10d} |'.format(rnn_conf_mat[i][j]), end='')
    print('')
  print(dividing_line)
  print('RNN part')
  for i in range(n_cls):
    precision = float(rnn_tp[i]) / rnn_pred[i] if rnn_pred[i] > 0 else 1.0
    recall = float(rnn_tp[i]) / rnn_ground_truth[i] if rnn_ground_truth[i] > 0 else 1.0
    rnn_f1[i] =  2 * precision * recall / (precision + recall) if precision > 1e-6 and recall > 1e-6 else 0.0
    print('{} | ground_truth={:10d} | pred={:10d} | TP={:10d} | precision={:4.3f} | recall={:4.3f} | F1={:4.3f}'
        .format(name[i], rnn_ground_truth[i], rnn_pred[i], rnn_tp[i], precision, recall, rnn_f1[i]))
  print('Macro-F1={:4.3f}'.format(sum(rnn_f1) / n_cls))
  print(dividing_line)
  print('Per packet part')
  print('{:7} |'.format(''), end='')
  for i in range(n_cls):
    print(' {:>10} |'.format(name[i]), end='')
  print('')
  for i in range(n_cls):
    print('{:7} |'.format(name[i]), end='')
    for j in range(n_cls):
      print(' {:10d} |'.format(perpacket_conf_mat[i][j]), end='')
    print('')
  print(dividing_line)
  print('Per packet part')
  for i in range(n_cls):
    precision = float(perpacket_tp[i]) / perpacket_pred[i] if perpacket_pred[i] > 0 else 1.0
    recall = float(perpacket_tp[i]) / perpacket_ground_truth[i] if perpacket_ground_truth[i] > 0 else 1.0
    perpacket_f1[i] =  2 * precision * recall / (precision + recall) if precision > 1e-6 and recall > 1e-6 else 0.0
    print('{} | ground_truth={:10d} | pred={:10d} | TP={:10d} | precision={:4.3f} | recall={:4.3f} | F1={:4.3f}'
        .format(name[i], perpacket_ground_truth[i], perpacket_pred[i], perpacket_tp[i], precision, recall, perpacket_f1[i]))
  print('Macro-F1={:4.3f}'.format(sum(perpacket_f1) / n_cls))
  print(dividing_line)
  for i in range(n_cls):
    print('{} | rnn={:10d} | pre-analysis={:10d} | IMIS={:10d} | per-packet={:10d} | collision-IMIS={:10d} |'
        .format(name[i], rnn_ground_truth[i], pre_analysis_ground_truth[i], IMIS_ground_truth[i], 
        perpacket_ground_truth[i], collision_IMIS_ground_truth[i]))
  print('RNN: {}, pre-analysis: {}, IMIS: {}, per-packet: {}, collision-IMIS: {}, Total: {}'.format(\
    sum(rnn_ground_truth), sum(pre_analysis_ground_truth), sum(IMIS_ground_truth), sum(perpacket_ground_truth), sum(collision_IMIS_ground_truth), \
    sum(rnn_ground_truth) + sum(pre_analysis_ground_truth) + sum(IMIS_ground_truth) + sum(perpacket_ground_truth) + sum(collision_IMIS_ground_truth)))
  print(dividing_line)
  print('Ig Total: {} pkts, Ig Recir: {} pkts, Eg Total: {} pkts, Eg to-Mirror: {} pkts, Aux Eg to-Recir: {} pkts, Aux Ig: {} pkts, Aux Eg normal: {} pkts'.format(\
    ig_pkt_sum, ig_recir_sum, eg_pkt_sum, eg_to_mirror_sum, aux_eg_to_recir_sum, aux_ig_pkt_sum, aux_eg_normal_sum))
  print(dividing_line)
  time.sleep(sleep_time)
