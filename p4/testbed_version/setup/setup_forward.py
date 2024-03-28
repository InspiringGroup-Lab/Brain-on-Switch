# bfrt_python <path to project>/setup/setup_forward.py

import bfrtcli
from bfrtcli import bfrt

def setup_forward():

    print("========================================")
    print("Configuring Forward Table")
    bfrt.BoS_testbed.pipe_a.SwitchIngress.tab_execute_forward_action.clear()

    escalation_port = 28
    normal_port = 24

    # ADD your forwarding policy here

    # collision-escalation
    bfrt.BoS_testbed.pipe_a.SwitchIngress.tab_execute_forward_action.add_with_forward(\
        ig_use_rnn=0, ig_use_rnn_mask=0x0, ig_use_per_packet=0, ig_use_per_packet_mask=0x1, ig_collision=1, ig_collision_mask=0x1,
        ig_port=0, ig_port_mask=0x0, ig_stored_escalation_flag=0, ig_stored_escalation_flag_mask=0x0, port=escalation_port)

    # rnn-escalation
    bfrt.BoS_testbed.pipe_a.SwitchIngress.tab_execute_forward_action.add_with_forward(\
        ig_use_rnn=1, ig_use_rnn_mask=0x1, ig_use_per_packet=0, ig_use_per_packet_mask=0x0, ig_collision=0, ig_collision_mask=0x0,
        ig_port=0, ig_port_mask=0x0, ig_stored_escalation_flag=0xff, ig_stored_escalation_flag_mask=0xff, port=escalation_port)

    # rnn-escalation
    bfrt.BoS_testbed.pipe_a.SwitchIngress.tab_execute_forward_action.add_with_forward(\
        ig_use_rnn=0, ig_use_rnn_mask=0x0, ig_use_per_packet=0, ig_use_per_packet_mask=0x0, ig_collision=0, ig_collision_mask=0x0,
        ig_port=0, ig_port_mask=0x0, ig_stored_escalation_flag=0, ig_stored_escalation_flag_mask=0x0, port=normal_port)

    print("Finished")
    print("========================================")

if __name__ == '__bfrtcli__':
    setup_forward()
