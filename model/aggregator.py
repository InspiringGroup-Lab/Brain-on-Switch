import json
import math
import argparse
import numpy as np
from opts import *
from tqdm import tqdm
from model import BinaryRNN
from utils.model_rwi import *
from trainer import build_data_loader, batch2segs
from utils.metric import metric_from_confuse_matrix
import matplotlib.pyplot as plt
import seaborn as sns
import statsmodels.api as sm 


def quantization(value, floor, ceil, quantization_num):
    unit = (ceil - floor) / quantization_num
    if value <= floor:
        return 0
    elif value >= ceil:
        return quantization_num - 1
    else:
        return int((value - floor) // unit)


def plot_pkts_confidence(args, correct_pkts_confidence, wrong_pkts_confidence, model_path):
    confidence_distribution = {
    }
    sns.set(style='white')
    fig = plt.figure(figsize=(6,4 * args.labels_num))
    for label in range(args.labels_num):        
        ax = fig.add_subplot(args.labels_num * 100 + 11 + label)
        ecdf_correct = sm.distributions.ECDF(correct_pkts_confidence[label])
        ecdf_wrong = sm.distributions.ECDF(wrong_pkts_confidence[label])
        x = [i for i in range(args.quantization_num)]
        # y_correct = ecdf_correct(x) * len(correct_pkts[label])
        # y_wrong = ecdf_wrong(x) * len(wrong_pkts[label])
        y_correct = ecdf_correct(x)
        y_wrong = ecdf_wrong(x)
        curve_correct = ax.plot(x, y_correct, color='g')
        curve_wrong = ax.plot(x, y_wrong, color='r')
        ax.grid()
        ax.legend(labels=['correct: {} pkts'.format(len(correct_pkts_confidence[label])), 
                          'wrong: {} pkts'.format(len(wrong_pkts_confidence[label]))], frameon=False)
        ax.set_ylabel('CDF of Packets\nclassified as Label {}'.format(label))
        ax.yaxis.set_major_locator(plt.MultipleLocator(0.05))
        ax.set_ylim(-0.02, 1.02)
        plt.xticks([i for i in range(args.quantization_num)], 
                   [str(i) for i in range(args.quantization_num)])
        confidence_distribution['label {}'.format(label)] = {
            'x': x,
            'y_correct': y_correct.tolist(),
            'y_wrong': y_wrong.tolist()
        }
    ax.set_xlabel('Average Accumulative Probability')
    plt.xticks([i for i in range(args.quantization_num)], 
               [str(i) for i in range(args.quantization_num)])
    plt.subplots_adjust(top=0.98, bottom=0.05, left=0.15, right=0.98, hspace=0.05)
    plt.show()
    plt.savefig(model_path + '-aggregator-{}-pkt-confidence.png'.format(args.reset_cycle), format='png')
    with open(model_path + '-confidence-distribution.json', 'w') as fp:
        json.dump(confidence_distribution, fp, indent=1)


def main():
    parser = argparse.ArgumentParser(formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    # Dataset
    parser.add_argument("--dataset", default="ISCXVPN2016",
                        choices=["ISCXVPN2016", "BOTIOT", "CICIOT2022", "PeerRush"])
    
    # Model options
    model_opts(parser)
    # Aggregator options
    aggregator_opts(parser)
    # Training options
    training_opts(parser)

    args = parser.parse_args()
    # Set dataset & model path according to options
    args.test_path = '../dataset/{}/json/test.json'.format(args.dataset)
    args.model_dir = './save/{}/brnn_len{}_ipd{}_ev{}_hidden{}_{}/'.format(
        args.dataset, args.len_embedding_bits, args.ipd_embedding_bits, args.embedding_vector_bits, args.rnn_hidden_bits,
        str(args.loss_factor) + '_' + str(args.focal_loss_gamma) + '_' + args.loss_type + '_' + str(args.learning_rate))
    with open('../dataset/{}/json/statistics.json'.format(args.dataset)) as fp:
        args.labels_num = json.load(fp)['label_num']

    # Build the BRNN model & load parameters
    model_path = args.model_dir + 'brnn-best'
    model = BinaryRNN(args)
    load_model(model, model_path)

    # Assign gpu
    gpu_id = args.gpu_id
    if gpu_id is not None:
        torch.cuda.set_device(gpu_id)
        model.cuda(gpu_id)
        print("Using GPU %d for testing with aggregator." % args.gpu_id)
    else:
        print("Using CPU mode for testing with aggregator.")

    # Build data loader for testing set
    test_loader = build_data_loader(args, args.test_path, batch_size=1, is_train=False, shuffle=True)

    # Testing
    print('testdata_path: {}'.format(args.test_path))
    print('model_path: {}'.format(model_path))
    flow_num = [0 for i in range(args.labels_num)]
    conf_mat_test = np.zeros([args.labels_num, args.labels_num])
    correct_pkts_confidence = [[] for i in range(args.labels_num)]
    wrong_pkts_confidence = [[] for i in range(args.labels_num)]
    wrong_segs = {}
    for label in range(args.labels_num):
        wrong_segs[str(label)] = []

    model.eval()    
    with torch.no_grad():
        for batch in tqdm(test_loader, desc='Testing flows'):
            len_x_batch, ipd_x_batch, label_batch = batch2segs(args, batch, max_cluster_segs=None)
            logits = model(len_x_batch, ipd_x_batch)
            probability_vectors = torch.nn.functional.softmax(logits)
            probability_vectors = probability_vectors.reshape([-1, args.labels_num])
            accumulative_probabilities = [0] * args.labels_num

            label = int(label_batch[0].cpu())
            flow_num[label] += 1
            for seg_idx in range(len(probability_vectors)):                
                # Periodically reset Accumulative Probability Counters
                if args.reset_cycle is not None and seg_idx % args.reset_cycle == 0:
                    accumulative_probabilities = [0] * args.labels_num
                
                # Update Accumulative Probability Counters & argmax to predict
                probability_vector = probability_vectors[seg_idx].tolist()
                for cls in range(args.labels_num):
                    accumulative_probabilities[cls] += quantization(probability_vector[cls], 0, 1, args.quantization_num)
                max_class = np.argmax(accumulative_probabilities)
                
                # Record the result
                conf_mat_test[label, max_class] += 1
                segs_num = seg_idx % args.reset_cycle + 1 if args.reset_cycle is not None else seg_idx + 1
                pkt_confidence = accumulative_probabilities[max_class] / segs_num
                if max_class == label:
                    correct_pkts_confidence[max_class].append(pkt_confidence)
                else:
                    wrong_pkts_confidence[max_class].append(pkt_confidence)

                pred_single_seg = int(np.argmax(probability_vector))
                if pred_single_seg != label:
                    wrong_segs[str(label)].append(
                        (torch.cat((len_x_batch[seg_idx], ipd_x_batch[seg_idx]), dim=-1).tolist(), 'pred:{}'.format(pred_single_seg)))

    # Saving result & anlysis data
    with open(model_path + '-wrong-segs.json', 'w') as fp:
        json.dump(wrong_segs, fp, indent=1)

    with open(model_path + '-result-aggregator-{}.txt'.format(args.reset_cycle)
              , 'w') as fp:
        
        precisions, recalls, f1s, _ = metric_from_confuse_matrix(conf_mat_test)

        segs = 0
        flows = 0
        for i in range(len(precisions)):
            segs += sum(conf_mat_test[i])
            flows += flow_num[i]
        fp.writelines("| segs {} (flows {})\n".format(int(segs), flows))

        for i in range(len(precisions)):
            fp.writelines("| label {:8d}"
                "| segs {:8d} (flows {:8d})"    
                "| precision {:8.3f}"
                "| recall {:8.3f}"
                "| f1 {:8.3f}\n".format(
                i, int(sum(conf_mat_test[i])), flow_num[i], precisions[i], recalls[i], f1s[i]
                ))
            
        fp.writelines("| Macro"    
                "| precision {:8.3f}"
                "| recall {:8.3f}"
                "| f1 {:8.3f}\n\n".format(np.mean(precisions), np.mean(recalls), np.mean(f1s)))
    
    plot_pkts_confidence(args, correct_pkts_confidence, wrong_pkts_confidence, model_path)

    
if __name__ == "__main__":
    main()
