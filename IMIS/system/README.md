# IMIS Implementation

## Dependency

We only show our settings, other settings may also work.

* PyTorch 2.0.1 + cu117

* DPDK 20.11.9 LTS

* PcapPlusPlus 22.11
    - PcapPlusPlus 22.11 only supports lcore 0~31, but in our server, due to the NUMA structure, we have to use lcore greater than 32, so we modify the `MAX_NUM_OF_CORES` and `SystemCore` to support lcore 0~63.

## Build

```
mkdir build && cd build
cmake -G Ninja ..
ninja
sudo ./IMIS
```

## Others

* Set your customized configuration in the `configTemplate.json`.
