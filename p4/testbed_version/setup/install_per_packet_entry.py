# bfrt_python <path to project>/setup/install_per_packet_entry.py

import bfrtcli
from bfrtcli import bfrt

def install_per_packet_entry(path_to_project: str):

    bfrt.BoS_testbed.pipe_b.SwitchEgress.per_packet.Feat1.clear()
    bfrt.BoS_testbed.pipe_b.SwitchEgress.per_packet.Feat4.clear()
    bfrt.BoS_testbed.pipe_b.SwitchEgress.per_packet.Feat5.clear()
    bfrt.BoS_testbed.pipe_b.SwitchEgress.per_packet.Feat7.clear()
    bfrt.BoS_testbed.pipe_b.SwitchEgress.per_packet.Feat9.clear()
    bfrt.BoS_testbed.pipe_b.SwitchEgress.per_packet.Pkt_Tree.clear()

    # ADD your per packet model here


if __name__ == '__bfrtcli__':
  install_per_packet_entry()
