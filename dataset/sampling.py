'''
Sample the training and testing set from the dataset:
'''
import argparse
import json
import os
import random

window_size = 8

def main():
    parser = argparse.ArgumentParser(formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument("--dataset", type=str, default="ISCXVPN2016",
                        choices=['CICIOT2022', 'PeerRush', 'ISCXVPN2016', 'BOTIOT'])
    parser.add_argument("--train_ratio", type=float, default=0.8)
    args = parser.parse_args()

    train = []
    test = []
    train_num_flow = {
        "sum": 0
    }
    test_num_flow = {
        "sum": 0
    }
    train_num_seg = {
        "sum": 0
    }
    test_num_seg = {
        "sum": 0
    }


    with open ('./{}/labels.json'.format(args.dataset)) as fp:
        labels = json.load(fp)
    for label in labels.keys():
        label_dir = './{}/json/{}'.format(args.dataset, label)
        print('--------------- {}'.format(label_dir))

        samples_filename = []
        for parent, _, files in os.walk(label_dir):
            # sampling only an hour of capture in PeerRush dataset
            if args.dataset == 'PeerRush' and parent.find('00.clean') == -1:
                continue
            
            for file in files:
                if file.find('.json') != -1:
                    samples_filename.append(os.path.join(parent, file))

        samples = []
        for filename in samples_filename:
            with open(filename) as fp:
                sample = json.load(fp)
                sample['label'] = labels[label]
                samples.append(sample)
        random.shuffle(samples)

        split_idx = int(len(samples) * args.train_ratio)
        train.extend(samples[:split_idx])
        train_num_flow[label] = split_idx
        train_num_flow['sum'] += split_idx
        train_num_seg[label] = 0
        for sample in samples[:split_idx]:
            train_num_seg[label] += min(sample['packet_num'], 4096) - window_size + 1
        train_num_seg['sum'] += train_num_seg[label]

        test.extend(samples[split_idx:])
        test_num_flow[label] = len(samples) - split_idx
        test_num_flow['sum'] += len(samples) - split_idx
        test_num_seg[label] = 0
        for sample in samples[split_idx:]:
            test_num_seg[label] += min(sample['packet_num'], 4096) - window_size + 1
        test_num_seg['sum'] += test_num_seg[label]
    
    random.shuffle(train)
    random.shuffle(test)
    with open('./{}/json/train.json'.format(args.dataset), 'w') as fp:
        json.dump(train, fp, indent=1)
    with open('./{}/json/test.json'.format(args.dataset), 'w') as fp:
        json.dump(test, fp, indent=1)

    sample_num_flow = {}
    sample_num_seg = {}
    for key in train_num_flow.keys():
        sample_num_flow[key] = '{} / {}'.format(train_num_flow[key], test_num_flow[key])
        sample_num_seg[key] = '{} / {}'.format(train_num_seg[key], test_num_seg[key])

    with open('./{}/json/statistics.json'.format(args.dataset), 'w') as fp:
        json.dump({
            "train num / test num (flow)": sample_num_flow,
            "train num / test num (seg)": sample_num_seg,
            "label_num": len(labels),
        }, fp, indent=1)


if __name__ == "__main__":
    main()
