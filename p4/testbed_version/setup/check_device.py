# bfrt_python <path to project>/setup/check_device.py

import bfrtcli
from bfrtcli import bfrt

print("========================================")
print("Checking - Device Configuration")
bfrt.tf1.device_configuration.dump(from_hw=True)
print("Finished")
print("========================================")
print("Checking - Port Configuration")
bfrt.port.port.dump(from_hw=True)
print("Finished")
print("========================================")
