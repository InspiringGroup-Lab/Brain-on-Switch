# bfrt_python <path to project>/setup/setup_mirror.py

import bfrtcli
from bfrtcli import bfrt

def setup_mirror(num_pipe=2, sid_offset=4):

    print("========================================")
    print("Configuring Mirror Sessions")

    bfrt.mirror.cfg
    for i in range(num_pipe):
        if (bfrt.mirror.cfg.get(sid=sid_offset+i, from_hw=True) != -1):
            bfrt.mirror.cfg.delete(sid=sid_offset+i)
        else:
            # bfrt will print 'Error: table_entry_get failed on table mirror.cfg. [Object not found]'
            pass
        bfrt.mirror.cfg.add_with_normal(sid=sid_offset+i, session_enable=True, direction="EGRESS", 
            ucast_egress_port=68+(i*128), ucast_egress_port_valid=True)
    print("Finished")
    print("========================================")
    print("Configuring Mirror Table")
    bfrt.BoS_testbed.pipe_a.SwitchIngress.tab_set_egr_mir_ses.clear()

    for i in range(num_pipe):
        bfrt.BoS_testbed.pipe_a.SwitchIngress.tab_set_egr_mir_ses.add_with_set_egr_mir_ses(\
            ig_pipe=i, egr_mir_ses=sid_offset+i)

    print("Finished")
    print("========================================")

if __name__ == '__bfrtcli__':
    setup_mirror()