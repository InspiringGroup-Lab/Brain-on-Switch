# bfrt_python <path to project>/setup/setup_threshold.py

import bfrtcli
from bfrtcli import bfrt

import json

def setup_threshold(path_to_project: str, cpr_reset_period=128, collision_escalation_reset_period=256, 
    collision_escalation_ratio=0.05) -> None:

  install_dir = path_to_project + '/install/'

  with open(install_dir + 'threshold.json','r') as f:
    js = json.load(f)
  
  print("========================================")
  print('Start installing confidence threshold as follows:')

  n_cls = js['number_of_class']
  t_conf = js['t_conf']
  t_esc = js['t_esc']

  assert(n_cls == 6)
  assert(len(t_conf) == n_cls)

  for i in range(n_cls):
    print("Class {}: {}".format(i, t_conf[i]))

  bfrt.BoS_testbed.pipe_b.SwitchEgress.tab_confidence_threshold.clear()
  for i in range(cpr_reset_period):
    bfrt.BoS_testbed.pipe_b.SwitchEgress.tab_confidence_threshold.add_with_act_confidence_threshold(
        intermediate_result_cnt=i, t_conf0=max(0, int((i+1)*t_conf[0])-1), t_conf1=max(0, int((i+1)*t_conf[1])-1), 
        t_conf2=max(0, int((i+1)*t_conf[2])-1), t_conf3=max(0, int((i+1)*t_conf[3])-1), 
        t_conf4=max(0, int((i+1)*t_conf[4])-1), t_conf5=max(0, int((i+1)*t_conf[5])-1))

  print('Finish installing confidence threshold')

  print('Escalation happens after {} low confidence packets'.format(t_esc))

  bfrt.BoS_testbed.pipe_b.SwitchEgress.tab_t_esc.clear()
  bfrt.BoS_testbed.pipe_b.SwitchEgress.tab_t_esc.set_default_with_act_t_esc(t_esc=int(t_esc))

  print('Setting collision-escalation ratio to {}'.format(collision_escalation_ratio))
  bfrt.BoS_testbed.pipe_a.SwitchIngress.tab_global_collision_escalation_threshold.clear()
  for i in range(collision_escalation_reset_period):
    bfrt.BoS_testbed.pipe_a.SwitchIngress.tab_global_collision_escalation_threshold.add_with_act_global_collision_escalation_threshold(
        global_pkt_cnt=i, threshold=int(i*float(collision_escalation_ratio)))
  print("========================================")

if __name__ == '__bfrtcli__':
  pass
  # setup_threshold(<path to project>)
  