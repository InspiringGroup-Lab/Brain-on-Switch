# bfrt_python <path to project>/setup/install_table_entry.py

import bfrtcli
from bfrtcli import bfrt

def install_table_entry(path_to_project: str) -> None:

  install_dir = path_to_project + '/install/'

  print("========================================")
  print('Start importing entries')

  with open(install_dir + 'entry_embed_interval.json','r') as f:
    s = f.read()
    bfrt.BoS_testbed.pipe_a.SwitchIngress.tab_embed_interval.clear()
    bfrt.BoS_testbed.pipe_a.SwitchIngress.tab_embed_interval.add_from_json(s)

  with open(install_dir + 'default_embed_interval.txt','r') as f:
    s = f.read()
    bfrt.BoS_testbed.pipe_a.SwitchIngress.tab_embed_interval.set_default_with_act_embed_interval(int(s))

  with open(install_dir + 'entry_embed_length.json','r') as f:
    s = f.read()
    bfrt.BoS_testbed.pipe_a.SwitchIngress.tab_embed_length.clear()
    bfrt.BoS_testbed.pipe_a.SwitchIngress.tab_embed_length.add_from_json(s)

  with open(install_dir + 'default_embed_length.txt','r') as f:
    s = f.read()
    bfrt.BoS_testbed.pipe_a.SwitchIngress.tab_embed_length.set_default_with_act_embed_length(int(s))

  with open(install_dir + 'entry_embed_fc.json','r') as f:
    s = f.read()
    bfrt.BoS_testbed.pipe_a.SwitchIngress.tab_embed_fc.clear()
    bfrt.BoS_testbed.pipe_a.SwitchIngress.tab_embed_fc.add_from_json(s)

  print('Embedding Layer imported')

  with open(install_dir + 'entry_gru0_gru1.json','r') as f:
    s = f.read()
    bfrt.BoS_testbed.pipe_a.SwitchIngress.tab_gru0_gru1.clear()
    bfrt.BoS_testbed.pipe_a.SwitchIngress.tab_gru0_gru1.add_from_json(s)

  with open(install_dir + 'entry_gru2.json','r') as f:
    s = f.read()
    bfrt.BoS_testbed.pipe_a.SwitchIngress.tab_gru2.clear()
    bfrt.BoS_testbed.pipe_a.SwitchIngress.tab_gru2.add_from_json(s)

  with open(install_dir + 'entry_gru3.json','r') as f:
    s = f.read()
    bfrt.BoS_testbed.pipe_a.SwitchIngress.tab_gru3.clear()
    bfrt.BoS_testbed.pipe_a.SwitchIngress.tab_gru3.add_from_json(s)

  with open(install_dir + 'entry_gru4.json','r') as f:
    s = f.read()
    bfrt.BoS_testbed.pipe_b.SwitchEgress.tab_gru4.clear()
    bfrt.BoS_testbed.pipe_b.SwitchEgress.tab_gru4.add_from_json(s)

  with open(install_dir + 'entry_gru5.json','r') as f:
    s = f.read()
    bfrt.BoS_testbed.pipe_b.SwitchEgress.tab_gru5.clear()
    bfrt.BoS_testbed.pipe_b.SwitchEgress.tab_gru5.add_from_json(s)

  with open(install_dir + 'entry_gru6.json','r') as f:
    s = f.read()
    bfrt.BoS_testbed.pipe_b.SwitchEgress.tab_gru6.clear()
    bfrt.BoS_testbed.pipe_b.SwitchEgress.tab_gru6.add_from_json(s)

  with open(install_dir + 'entry_gru7_output.json','r') as f:
    s = f.read()
    bfrt.BoS_testbed.pipe_b.SwitchEgress.tab_gru7_output.clear()
    bfrt.BoS_testbed.pipe_b.SwitchEgress.tab_gru7_output.add_from_json(s)

  print('GRU & Output imported')

  print('Finish importing entries')

  print('You can use "info" command to check the table usage & capacity.')
  print("========================================")

if __name__ == '__bfrtcli__':
  pass
  # install_table_entry(<path to project>)
  