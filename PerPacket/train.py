import os
import json
import random
import argparse
import numpy as np
from joblib import dump
from opts import model_opts
from sklearn.ensemble import RandomForestClassifier
from sklearn.metrics import precision_recall_fscore_support


def load_dataset(args, filename, is_train=False, shuffle=True):
    x = []
    y = []
    flow_num = [0 for i in range(args.labels_num)]

    with open(filename) as fp:
        instances = json.load(fp)
    if shuffle:
        random.shuffle(instances)

    for ins in instances:
        if ins['flow_packets'] < args.window_size:
            continue
        flow_num[ins['label']] += 1

        len_seq = ins['len_seq'][args.window_size - 1:]
        for i in range(len(len_seq)):
            len_seq[i] = min(len_seq[i], 1500)
        if len(len_seq) > 4096:
            len_seq = len_seq[:4096]
        
        ip_hl_seq = ins['ip_hl'][args.window_size - 1:]
        ip_ttl_seq = ins['ip_ttl'][args.window_size - 1:]
        ip_tos_seq = ins['ip_tos'][args.window_size - 1:]
        tcp_off_seq = ins['tcp_off'][args.window_size - 1:]

        for i in range(len(len_seq)):
            x.append([len_seq[i], 
                      ip_hl_seq[i], ip_ttl_seq[i], ip_tos_seq[i],
                      tcp_off_seq[i]])
            y.append(ins['label'])
    
    print('The size of {}_set is {}.'.format(
            'train' if is_train else 'test', len(x)))
    return x, y, flow_num


def main():
    parser = argparse.ArgumentParser(formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    # Dataset
    parser.add_argument("--dataset", default="ISCXVPN2016",
                        choices=["ISCXVPN2016", "BOTIOT", "CICIOT2022", "PeerRush"])
    
    # Model options
    model_opts(parser)

    args = parser.parse_args()

    # Set dataset & model path according to options
    args.train_path = '../dataset/{}/json/train.json'.format(args.dataset)
    args.test_path = '../dataset/{}/json/test.json'.format(args.dataset)
    args.output_dir = './save/{}/rf_tree{}_depth{}/'.format(
        args.dataset, args.tree_num, args.tree_depth)
    if not os.path.exists(args.output_dir):
        os.makedirs(args.output_dir)
    with open('../dataset/{}/json/statistics.json'.format(args.dataset)) as fp:
        args.labels_num = json.load(fp)['label_num']

    x_train, y_train, flow_num_train = load_dataset(args, args.train_path, is_train=True, shuffle=True)
    x_test, y_test, flow_num_test = load_dataset(args, args.test_path, is_train=False, shuffle=True)

    rf = RandomForestClassifier(n_estimators=args.tree_num, max_depth=args.tree_depth)
    rf.fit(x_train, y_train)

    pred_test = rf.predict(x_test)
    precisions, recalls, f1s, ss = precision_recall_fscore_support(y_test, pred_test)
    with open(args.output_dir + 'test_result.txt', 'w') as fp:
        fp.writelines(
                "| Tree num {}"
                "| Tree depth {}\n".format(
                args.tree_num, args.tree_depth
            ))

        for i in range(len(precisions)):
            fp.writelines("| label {:8d}"
                "| pkts {:8d} (flows {:8d})"    
                "| precision {:8.3f}"
                "| recall {:8.3f}"
                "| f1 {:8.3f}\n".format(
                i, sum([1 if y == i else 0 for y in y_test]), flow_num_test[i], precisions[i], recalls[i], f1s[i]
                ))
            
        fp.writelines("| Macro"    
                "| precision {:8.3f}"
                "| recall {:8.3f}"
                "| f1 {:8.3f}\n\n".format(np.mean(precisions), np.mean(recalls), np.mean(f1s)))

    
    dump(rf, args.output_dir + 'rf-per-packet.joblib')

    importances = rf.feature_importances_
    features = []
    i = 0
    for feature in ['pkt_length', 'ip_hl', 'ip_ttl', 'ip_tos', 'tcp_off']:
        features.append((importances[i], feature))
        i += 1
    features = sorted(features, reverse=True)
    for i in features:
        print(i)


if __name__ == "__main__":
    main()