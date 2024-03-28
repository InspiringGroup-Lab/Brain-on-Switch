'''
Preprocess the .pcap file of flows: .pcap -> .json
*   flow = {
        "label": ...,
        "source": ...,

        "ip_hl": ...,
        "ip_ttl": ...,
        "ip_tos": ...,
        "tcp_off": ...,
        "tcp_win": ...,

        "packet_num": ..., 
        "len_seq": ...,
        "ts_seq": ...
    }
'''
import argparse
import json
import os
import shutil
import scapy.all as scapy
import sys
sys.path.append('..')

def preprocess_flow(flow_file, label, args):
    pkts = scapy.rdpcap(flow_file)
    if len(pkts) < args.window_size:
        return None
    
    flow = {
        "label": label,
        "source": '',
        # pkt level features
        "ip_hl_seq": [],
        "ip_ttl_seq": [],
        "ip_tos_seq": [],
        "tcp_off_seq": [],
        "tcp_win_seq": [],
        # flow level features
        "packet_num": len(pkts),
        "len_seq": [],
        "ts_seq": []
    }

    for pkt in pkts:
        layers_string = ''
        flow['ts_seq'].append(pkt.time)
        layers = pkt.copy()
        while layers.name != 'NoPayload':
            layers_string += layers.name + '_'
            if layers.name == 'IP':
                flow['ip_hl_seq'].append(layers.ihl)
                flow['ip_ttl_seq'].append(layers.ttl)
                flow['ip_tos_seq'].append(layers.tos)
                flow['len_seq'].append(layers.len)
            elif layers.name == 'TCP':
                flow['tcp_off_seq'].append(layers.dataofs)
                flow['tcp_win_seq'].append(layers.window)
            elif layers.name == 'UDP':
                flow['tcp_off_seq'].append(0)
                flow['tcp_win_seq'].append(0)
            layers = layers.payload
        
        layers_string = layers_string[:-1]
        if 'TCP' not in layers_string and 'UDP' not in layers_string:
            print('Not TCP or UDP flow: ' + layers_string)
            return None

    ts_start = flow['ts_seq'][0]
    for i in range(len(flow['ts_seq'])):
        flow['ts_seq'][i] = float(flow['ts_seq'][i] - ts_start)
        if i > 0 and flow['ts_seq'][i] < flow['ts_seq'][i - 1]:
            print('Timestamp error!')
            return None
    
    return flow


def main():
    parser = argparse.ArgumentParser(formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument("--dataset", type=str, default="ISCXVPN2016",
                        choices=['CICIOT2022', 'ISCXVPN2016', 'PeerRush', 'BOTIOT'])
    parser.add_argument("--window_size", type=int, default=8)
    args = parser.parse_args()
    
    sample_num = {}
    len_distribution = {}
    with open ('./{}/labels.json'.format(args.dataset)) as fp:
        labels = json.load(fp)

    if not os.path.exists('./{}/json'.format(args.dataset)):
        os.makedirs('./{}/json'.format(args.dataset))

    for label in labels.keys():
        label_dir = './{}/json/{}'.format(args.dataset, label)
        if os.path.exists(label_dir):
            shutil.rmtree(label_dir)
            # continue
        os.mkdir(label_dir)

        sample_num[label] = 0
        len_distribution[label] = {}
        src_pcap_dir = './{}/pcap'.format(args.dataset)
        for _, dirs, _ in os.walk(src_pcap_dir):
            for dir in dirs:
                if dir.find(label) == -1:
                    continue
                file_num = 0

                for parent, _, files in os.walk(src_pcap_dir + '/' + dir):
                    for flow_file in files:
                        file_num += 1
                        if flow_file.find('.pcap') == -1:
                            continue
                        print('{} ... {} / {}'.format(os.path.join(parent, flow_file), file_num, len(files)))
                        
                        flow = preprocess_flow(os.path.join(parent, flow_file), label, args)
                        if flow is None:
                            continue
                        else:
                            flow['source'] = os.path.join(parent, flow_file)
                        
                        sample_num[label] += 1
                        if flow['packet_num'] not in len_distribution[label].keys():
                            len_distribution[label][flow['packet_num']] = 0
                        len_distribution[label][flow['packet_num']] += 1

                        tgt_dir = label_dir + parent.split('pcap')[-1]
                        if not os.path.exists(tgt_dir):
                            os.makedirs(tgt_dir)
                        with open(os.path.join(tgt_dir, flow_file.replace('.pcap', '.json')), 'w') as fp:
                            json.dump(flow, fp, indent=1)
            break
        for l in len_distribution[label].keys():
            len_distribution[label][l] = len_distribution[label][l] / sample_num[label]
    
    with open('./{}/json/statistics_org.json'.format(args.dataset), 'w') as fp:
        json.dump({
            'sample_num': sample_num,
            'len_distribution': len_distribution
        }, fp, indent=1)

    
if __name__ == "__main__":
    main()
