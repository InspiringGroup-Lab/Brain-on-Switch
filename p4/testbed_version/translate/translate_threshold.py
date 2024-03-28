import argparse
import json
import numpy as np
import os

def translate_threshold(args):
    # read threshold
    with open(args.data_path + 'threshold.json', 'r') as f:
        js = json.load(f)
    
    assert(js['number_of_class'] == args.n_cls)
    assert(len(js['t_conf']) == args.n_cls)
    js['number_of_class'] = args.n_p4_cls
    js['t_conf'].extend([0] * (args.n_p4_cls - args.n_cls))
    with open(args.output_dir + 'threshold.json', 'w') as f:
        json.dump(js, f)

def main():
    parser = argparse.ArgumentParser(formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    # Dataset
    parser.add_argument('--dataset', default='ISCXVPN2016', 
        choices=['PeerRush', 'BOTIOT', 'CICIOT2022', 'ISCXVPN2016'])
    parser.add_argument('--n_cls', type=int, default=6)
    parser.add_argument('--n_p4_cls', type=int, default=6)

    args = parser.parse_args()
    assert(args.n_cls <= args.n_p4_cls)
    args.data_path = '../../parameter/{}/'.format(args.dataset)
    args.output_dir = '../install/'.format(args.dataset)
    if not os.path.exists(args.output_dir):
            os.makedirs(args.output_dir)
    
    translate_threshold(args)

if __name__ == '__main__':
    main()