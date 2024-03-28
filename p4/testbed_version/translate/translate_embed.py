import argparse
import json
import numpy as np
import os


def translate_embed(args):
    ipd_relative_precision = float(args.ipd_p4_precision) / float(args.ipd_original_precision)
    range_ebd_len = int(2 ** args.dim_ebd_len)
    range_ebd_ipd = int(2 ** args.dim_ebd_ipd)

    # read ebd_len layer

    ebd_len = np.zeros((args.range_len, 1), dtype=int)

    with open(args.data_path + 'embedding_len.json', 'r') as f:
        tbl = json.load(f)
    assert(len(tbl['table']) == args.range_len)
    for ent in tbl['table']:
        ebd_len[int(ent[0])][0] = int(ent[1], 2)

    # read ebd_ipd layer

    ebd_ipd = np.zeros((args.range_p4_ipd, 1), dtype=int)

    with open(args.data_path + 'embedding_ipd.json', 'r') as f:
        tbl = json.load(f)
    assert(len(tbl['table']) == args.range_original_ipd)
    mapping = dict()
    for ent in tbl['table']:
        mapping[int(ent[0])] = int(ent[1], 2)

    for i in range(args.range_p4_ipd):
        ebd_ipd[i][0] = mapping[min(round(i * ipd_relative_precision), args.range_original_ipd - 1)]

    # read fc layer

    fc = np.zeros((range_ebd_len, range_ebd_ipd), dtype=int)

    with open(args.data_path + 'fc.json', 'r') as f:
        tbl = json.load(f)
    assert(len(tbl['table']) == range_ebd_len * range_ebd_ipd)
    for ent in tbl['table']:
        fc[int(ent[1], 2)][int(ent[0], 2)] = int(ent[2], 2)

    # write entries

    tmp = '{{"table_name":"pipe_a.SwitchIngress.tab_embed_length","action":"SwitchIngress.act_embed_length","key":' \
        + '{{"ip_len":{ip_len}}},"data":{{"len":{len}}}}}'

    with open(args.output_dir + 'entry_embed_length.json', 'w') as f:
        f.write('[')
        first = True
        for ip_len in range(0, args.range_len):
            if first:
                first = False
            else:
                f.write(',')
            f.write(tmp.format(ip_len=ip_len, len=ebd_len[ip_len][0]))
        f.write(']')
    
    with open(args.output_dir + 'default_embed_length.txt', 'w') as f:
        f.write('{}'.format(ebd_len[args.range_len-1][0]))

    tmp = '{{"table_name":"pipe_a.SwitchIngress.tab_embed_interval","action":"SwitchIngress.act_embed_interval","key":' \
        + '{{"ig_md.interval_{ipd_p4_precision}ns":{ig_interval}}},"data":{{"interval":{interval}}}}}'

    with open(args.output_dir + 'entry_embed_interval.json', 'w') as f:
        f.write('[')
        first = True
        for ig_interval in range(0, args.range_p4_ipd):
            if first:
                first = False
            else:
                f.write(',')
            f.write(tmp.format(ipd_p4_precision=args.ipd_p4_precision, ig_interval=ig_interval, interval=ebd_ipd[ig_interval][0]))
        f.write(']')
    
    with open(args.output_dir + 'default_embed_interval.txt', 'w') as f:
        f.write('{}'.format(ebd_ipd[args.range_p4_ipd-1][0]))
    
    tmp = '{{"table_name":"pipe_a.SwitchIngress.tab_embed_fc","action":"SwitchIngress.act_embed_fc","key":' \
        + '{{"ig_md.embedded_length":{embedded_length},"ig_md.embedded_interval":{embedded_interval}}},"data":{{"ev":{ev}}}}}'

    with open(args.output_dir + 'entry_embed_fc.json', 'w') as f:
        f.write('[')
        first = True
        for embedded_length in range(0, range_ebd_len):
            for embedded_interval in range(0, range_ebd_ipd):
                if first:
                    first = False
                else:
                    f.write(',')
                f.write(tmp.format(embedded_length=embedded_length, embedded_interval=embedded_interval, ev=fc[embedded_length][embedded_interval]))
        f.write(']')

def main():
    parser = argparse.ArgumentParser(formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    # Dataset
    parser.add_argument('--dataset', default='ISCXVPN2016', 
        choices=['PeerRush', 'BOTIOT', 'CICIOT2022', 'ISCXVPN2016'])
    parser.add_argument('--range_len', type=int, default=1501)
    parser.add_argument('--range_original_ipd', type=int, default=15626)
    parser.add_argument('--range_p4_ipd', type=int, default=16384)
    parser.add_argument('--ipd_original_precision', type=int, default=16384)
    parser.add_argument('--ipd_p4_precision', type=int, default=16384)

    parser.add_argument('--dim_ebd_len', type=int, default=10)
    parser.add_argument('--dim_ebd_ipd', type=int, default=8)

    args = parser.parse_args()
    args.data_path = '../../parameter/{}/'.format(args.dataset)
    args.output_dir = '../install/'.format(args.dataset)
    if not os.path.exists(args.output_dir):
            os.makedirs(args.output_dir)
    
    translate_embed(args)

if __name__ == '__main__':
    main()
