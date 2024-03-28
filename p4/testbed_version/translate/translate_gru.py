import argparse
import json
import numpy as np
import os


def translate_gru(args):

    # dim_hidden = 9
    # dim_ev = 6
    # n_cls = 6
    # dim_inter_pr = 4

    range_hidden = int(2 ** args.dim_hidden)
    range_p4_hidden = int(2 ** args.dim_p4_hidden)
    range_ev = int(2 ** args.dim_ev)
    range_inter_pr = int(2 ** args.dim_inter_pr)

    # read gru layer

    # init_gru = np.random.random_integers(0, range_hidden-1, size=(range_ev, 1))

    init_gru = np.zeros((range_ev, 1), dtype=int)

    with open(args.data_path + 'init_gru.json', 'r') as f:
        tbl = json.load(f)
    assert(len(tbl['table']) == range_ev)
    for ent in tbl['table']:
        init_gru[int(ent[0], 2)][0] = int(ent[1], 2)

    # gru = np.random.random_integers(0, range_hidden-1, size=(range_hidden, range_ev))

    gru = np.zeros((range_hidden, range_ev), dtype=int)

    with open(args.data_path + 'gru.json', 'r') as f:
        tbl = json.load(f)
    assert(len(tbl['table']) == range_ev * range_hidden)
    for ent in tbl['table']:
        gru[int(ent[1], 2)][int(ent[0], 2)] = int(ent[2], 2)
    

    # read output layer

    # output = np.zeros((range_hidden, n_cls), dtype=int)
    # output[:,:] = np.random.random_integers(0, range_inter_pr-1, size=(range_hidden, n_cls))

    output = np.zeros((range_hidden, args.n_cls), dtype=int)

    with open(args.data_path + 'output.json', 'r') as f:
        tbl = json.load(f)
    assert(len(tbl['table']) == range_hidden)
    for ent in tbl['table']:
        output[int(ent[0], 2)][:] = ent[3]
        assert(len(ent[3]) == args.n_cls)

    # write entries

    tmp = '{{"table_name":"pipe_a.SwitchIngress.tab_gru0_gru1","action":"SwitchIngress.act_gru0_gru1","key":'\
        + '{{"ev0":{ev0},"ev1":{ev1}}},"data":{{"h":{h}}}}}'

    with open(args.output_dir + 'entry_gru0_gru1.json', 'w') as f:
        f.write('[')
        first = True
        for ev0 in range(0, range_ev):
            for ev1 in range(0, range_ev):
                if first:
                    first = False
                else:
                    f.write(',')
                f.write(tmp.format(ev0=ev0, ev1=ev1, h=gru[init_gru[ev0][0]][ev1]))
        f.write(']')
    
    for i in range(2, args.window_size-1):
        pipe = 'pipe_a' if i < args.n_ingress_gru else 'pipe_b'
        gress = 'SwitchIngress' if i < args.n_ingress_gru else 'SwitchEgress'
        tmp = '{{{{"table_name":"{pipe}.{gress}.tab_gru{i}","action":"{gress}.act_gru{i}","key":' \
            + '{{{{"h{last_i}":{{last_h}},"ev{i}":{{new_ev}}}}}},"data":{{{{"h":{{new_h}}}}}}}}}}'
        tmp = tmp.format(pipe=pipe, gress=gress, i=i, last_i=i-1)

        with open(args.output_dir + 'entry_gru{}.json'.format(i), 'w') as f:
            f.write('[')
            first = True
            for hi in range(0, int(2**(args.dim_p4_hidden-args.dim_hidden))):
                for last_h in range(0, range_hidden):
                    for new_ev in range(0, range_ev):
                        if first:
                            first = False
                        else:
                            f.write(',')
                        f.write(tmp.format(last_h=hi*int(2**args.dim_hidden)+last_h, new_ev=new_ev, 
                                new_h=gru[last_h][new_ev]))
            f.write(']')

    pipe = 'pipe_a' if args.window_size-1 < args.n_ingress_gru else 'pipe_b'
    gress = 'SwitchIngress' if args.window_size-1 < args.n_ingress_gru else 'SwitchEgress'
    tmp = '{{{{"table_name":"{pipe}.{gress}.tab_gru{i}_output","action":"{gress}.act_gru{i}_output","key":' \
        + '{{{{"h{last_i}":{{last_h}},"ev{i}":{{new_ev}}}}}},"data":{{{{{{data}}}}}}}}}}'
    tmp = tmp.format(pipe=pipe, gress=gress, i=args.window_size-1, last_i=args.window_size-2)

    with open(args.output_dir + 'entry_gru{i}_output.json'.format(i=args.window_size-1), 'w') as f:
        f.write('[')
        first = True
        for hi in range(0, int(2**(args.dim_p4_hidden-args.dim_hidden))):
            for last_h in range(0, range_hidden):
                for new_ev in range(0, range_ev):
                    if first:
                        first = False
                    else:
                        f.write(',')
                    prs = output[gru[last_h][new_ev]]
                    f.write(tmp.format(last_h=hi*int(2**args.dim_hidden)+last_h, new_ev=new_ev, 
                            data=",".join(['"pr{}":{}'.format(i, prs[i] if i < args.n_cls else 0) for i in range(args.n_p4_cls)])))
        f.write(']')

def main():
    parser = argparse.ArgumentParser(formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    # Dataset
    parser.add_argument('--dataset', default='ISCXVPN2016', 
        choices=['PeerRush', 'BOTIOT', 'CICIOT2022', 'ISCXVPN2016'])
    parser.add_argument('--dim_hidden', type=int, default=9)
    parser.add_argument('--dim_p4_hidden', type=int, default=9)
    parser.add_argument('--dim_ev', type=int, default=6)
    parser.add_argument('--n_cls', type=int, default=6)
    parser.add_argument('--n_p4_cls', type=int, default=6)
    parser.add_argument('--dim_inter_pr', type=int, default=4)
    parser.add_argument('--window_size', type=int, default=8)
    parser.add_argument('--n_ingress_gru', type=int, default=4)

    args = parser.parse_args()
    assert(args.dim_hidden <= args.dim_p4_hidden)
    assert(args.n_cls <= args.n_p4_cls)
    args.data_path = '../../parameter/{}/'.format(args.dataset)
    args.output_dir = '../install/'.format(args.dataset)
    if not os.path.exists(args.output_dir):
            os.makedirs(args.output_dir)
    
    translate_gru(args)

if __name__ == '__main__':
    main()