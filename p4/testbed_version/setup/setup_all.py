# bfrt_python <path to p4>/testbed_version/setup/setup_all.py

import os
import sys

# Replace this with the path to the project
path_to_project = '/root/BoS/p4/testbed_version/'

sys.path.append(path_to_project + 'setup/')

# import check_device

from setup_forward import setup_forward

from setup_mirror import setup_mirror

from install_per_packet_entry import install_per_packet_entry

from install_table_entry import install_table_entry

from setup_threshold import setup_threshold

setup_forward()

setup_mirror()

install_per_packet_entry(path_to_project)

setup_threshold(path_to_project, collision_escalation_ratio=0.05)

install_table_entry(path_to_project)

