'''
Preprocess the .pcap file of flows: .pcap -> .json
*   flow instance:
        {
            'len_seq': list,
            'ts_seq': list,
            'ip_hl_seq': list,
            'ip_ttl_seq': list,
            'ip_tos_seq': list,
            'tcp_off_seq': list,
            'packet_num': int,
            'label': int,
            'source_file': str
        }
'''
import os
import json
import argparse
import numpy as np
import scapy.all as scapy


def pcap2json(flow_file, label, args):
    pkts = scapy.rdpcap(flow_file)
    if len(pkts) < args.min_flow_length:
        return None
    if len(pkts) > args.truncate_flow_length:
        pkts = pkts[:args.truncate_flow_length]
    
    flow = {
        'len_seq': [],
        'ts_seq': [],
        'ip_hl_seq': [],
        'ip_ttl_seq': [],
        'ip_tos_seq': [],
        'tcp_off_seq': [],
        'packet_num': len(pkts),
        'label': label,
        'source_file': flow_file
    }
    
    for pkt in pkts:
        try:
            ip = pkt['IP']
            if ip.len is None:
                return None
        except:
            return None
        
        TCP = False
        UDP = False
        try:
            tcp = pkt['TCP']
            TCP = True
        except:
            pass
        try:
            udp = pkt['UDP']
            UDP = True
        except:
            pass

        if (TCP is False) and (UDP is False):
            return None

        flow['len_seq'].append(ip.len)
        flow['ts_seq'].append(pkt.time)
        flow['ip_hl_seq'].append(ip.ihl)
        flow['ip_ttl_seq'].append(ip.ttl)
        flow['ip_tos_seq'].append(ip.tos)
        if TCP:
            flow['tcp_off_seq'].append(tcp.dataofs)
        else:
            flow['tcp_off_seq'].append(0)

    return flow


def main():
    parser = argparse.ArgumentParser(formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    # Dataset
    parser.add_argument("--dataset", default="ISCXVPN2016",
                        choices=["ISCXVPN2016", "BOTIOT", "CICIOT2022", "PeerRush"])
    parser.add_argument("--min_flow_length", type=int, default=8)
    parser.add_argument("--truncate_flow_length", type=int, default=4096)
    
    args = parser.parse_args()
    
    with open ('./{}/labels.json'.format(args.dataset)) as fp:
        labels = json.load(fp)
    
    all_json = []
    flow_length_distribution = [0 for i in range(args.truncate_flow_length + 1)]
    max_flow_length = 0
    for label in labels.keys():
        label_dir = './{}/pcap/{}'.format(args.dataset, label)
        print('--------------- {}'.format(label_dir))
        instance_num = 0
        for parent, _, files in os.walk(label_dir):
            for flow_file in files:
                if flow_file.find('.pcap') == -1:
                    continue
                print('{} ...'.format(os.path.join(parent, flow_file)))
                
                flow = pcap2json(os.path.join(parent, flow_file), labels[label], args)
                if flow is None:
                    continue
                
                all_json.append(flow)
                flow_length_distribution[flow['packet_num']] += 1
                max_flow_length = max(max_flow_length, flow['packet_num'])
    
    train_ratio = 0.8
    test_ratio = 0.2

    np.random.shuffle(all_json)   
    instance_num = len(all_json)
    train_json = all_json[:int(train_ratio * instance_num)]
    test_json = all_json[int(train_ratio * instance_num):]

    statistics_json = {'train:test': '{}:{}'.format(train_ratio, test_ratio),
        'label_num': len(labels.keys())
    }

    for filename, instances in [('all.json', all_json), 
                        ('train.json', train_json), 
                        ('test.json', test_json)]:
        statistics_json[filename] = {'total': 0}
        for label in labels.keys():
            statistics_json[filename][str(labels[label])] = 0

        for instance in instances:
            statistics_json[filename]['total'] += 1
            statistics_json[filename][str(instance['label'])] += 1
        
        with open('./{}/json/{}'.format(args.dataset, filename), 'w') as fp:
            json.dump(instances, fp, indent=1)

    for i in range(max_flow_length + 1):
        flow_length_distribution[i] /= len(all_json)
        if flow_length_distribution[i] > 0:
            statistics_json['flow_length_' + str(i)] = flow_length_distribution[i] * 100

    with open('./{}/json/statistics.json'.format(args.dataset), 'w') as fp:
        json.dump(statistics_json, fp, indent=1)

        
if __name__ == "__main__":
    main()
