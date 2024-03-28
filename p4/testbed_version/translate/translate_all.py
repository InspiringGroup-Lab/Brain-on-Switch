import argparse
import json
import numpy as np
import os
from translate_embed import translate_embed
from translate_gru import translate_gru
from translate_threshold import translate_threshold

def main():
    parser = argparse.ArgumentParser(formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    # Dataset
    parser.add_argument('--dataset', default='ISCXVPN2016',
    choices=['PeerRush', 'BOTIOT', 'CICIOT2022', 'ISCXVPN2016'])

    parser.add_argument('--dim_hidden', type=int, default=9)
    parser.add_argument('--n_cls', type=int, default=6)

    parser.add_argument('--range_len', type=int, default=1501)
    parser.add_argument('--range_original_ipd', type=int, default=15626)
    parser.add_argument('--range_p4_ipd', type=int, default=16384)
    parser.add_argument('--ipd_original_precision', type=int, default=16384)
    parser.add_argument('--ipd_p4_precision', type=int, default=16384)

    parser.add_argument('--dim_ebd_len', type=int, default=10)
    parser.add_argument('--dim_ebd_ipd', type=int, default=8)

    parser.add_argument('--dim_p4_hidden', type=int, default=9)
    parser.add_argument('--dim_ev', type=int, default=6)
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
    translate_embed(args)
    translate_gru(args)
    translate_threshold(args)

if __name__ == '__main__':
    main()