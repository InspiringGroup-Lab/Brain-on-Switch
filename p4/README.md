# P4 Implementation

## Two versions (variants)

There are two versions of p4 code, the normal version and the testbed version.

The normal version uses a single pipe to implement the Recurrent Neural Network (RNN) model, as introduced in paper.

The testbed version utilizes a unique configuration with two pipes in a twisted arrangement. In this version, the packet traverses from pipeA.ingress to pipeB.egress, then recirculates to pipeB.ingress and finally reaches pipeA.egress. RNN is implemented in pipeA.ingress and pipeB.egress, except that the logic of using mirror/recirculate to update the escalation flag is implemented in pipeA.egress. PipeB.ingress is used for on-switch statistics collection.

## Build

The code is compatible with SDE version 9.7.0. As our switch has only two pipes, the output .conf file by the compiler needs to be manually modified to ensure proper program execution.

## Additional Considerations

* Replace the parameter of per-packet model with your customized model. The provided per_packet.p4 serves as an illustrative example, showcasing where and how to integrate the per-packet model into the control flow.

* Modify the embedding of inter-packet delay (called "time interval" in the code) to enable diverse embedding precision. For instance, change `ig_md.interval_1ns[27:14]: exact @name("ig_md.interval_16384ns");` into `ig_md.interval_1ns[27:12]: exact @name("ig_md.interval_4096ns");` along with the corresponding table size and table entries.

* The control plane code is dependent on the environment, i.e., we use `bfrt_python` while you may use `grpc`, so it is not easy to give the control plane code that can be run by everyone. Basically, during setting up, you may need to read table entries from files (e.g.., `json`) first, and then add them to the data plane with the API in your environment. As for the confidence thresholds (t_conf), read the thresholds  and subsequently calculate and install the entry `i -> (i+1)*t_conf` for every `i`.

Currently, we provide the control plane code for testbed version. More instructions will be released later.
