import json
import copy
from torch.utils.data import Dataset


class FlowDataset(Dataset):
    def __init__(self, args, filename):
        super().__init__()
        self.flows = []

        with open(filename) as fp:
            instances = json.load(fp)
        for ins in instances:
            len_seq = ins['len_seq']
            real_len_seq = copy.deepcopy(len_seq)
            # Truncate the pakcet length
            for i in range(len(len_seq)):
                len_seq[i] = min(len_seq[i], args.len_vocab - 1)
            
            ts_seq = ins['ts_seq']
            ipd_seq = [0]
            ipd_seq.extend([ts_seq[i] - ts_seq[i - 1] for i in range(1, len(ts_seq))])
            real_ipd_seq_us = [i * 1e6 for i in ipd_seq]
            # Truncate the ipd, unit: 16384 ns
            for i in range(len(ipd_seq)):
                ipd_seq[i] = min(ipd_seq[i] * 1e9 // 16384, args.ipd_vocab - 1)
                assert ipd_seq[i] >= 0
            
            # Truncate the flow
            if len(len_seq) > 4096:
                len_seq = len_seq[:4096]
                ipd_seq = ipd_seq[:4096]
                real_len_seq = real_len_seq[:4096]
                real_ipd_seq_us = real_ipd_seq_us[:4096]
            
            self.flows.append(
                {
                    'label': ins['label'],
                    'len_seq': len_seq,
                    'ipd_seq': ipd_seq,
                    'ip_hl': ins['ip_hl'],
                    'ip_ttl': ins['ip_ttl'],
                    'ip_tos': ins['ip_tos'],
                    'tcp_off': ins['tcp_off'],
                    'real_len_seq': real_len_seq,
                    'real_ipd_seq_us': real_ipd_seq_us
                }
            )
                

    def __len__(self):
        return len(self.flows)
    
    
    def __getitem__(self, index):
        flow = self.flows[index]
        return str(flow['len_seq']) + ';' + str(flow['ipd_seq']), flow['label']
    