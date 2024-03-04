import os
import time
import json
import argparse
from opts import *
from model import BinaryRNN
from utils.model_rwi import *
from utils.seed import set_seed
from trainer import build_optimizer, build_data_loader, BRNNTrainer


def main():
    parser = argparse.ArgumentParser(formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    # Dataset
    parser.add_argument("--dataset", default="ISCXVPN2016",
                        choices=["ISCXVPN2016", "BOTIOT", "CICIOT2022", "PeerRush"])
    
    # Model options
    model_opts(parser)
    # Training options
    training_opts(parser)
    
    args = parser.parse_args()
    # Set dataset & model path according to options
    args.train_path = '../dataset/{}/json/train.json'.format(args.dataset)
    args.test_path = '../dataset/{}/json/test.json'.format(args.dataset)
    args.output_dir = './save/{}/brnn_len{}_ipd{}_ev{}_hidden{}_{}/'.format(
        args.dataset,
        args.len_embedding_bits, args.ipd_embedding_bits, args.embedding_vector_bits, args.rnn_hidden_bits,
        str(args.loss_factor) + '_' + str(args.focal_loss_gamma) + '_' + args.loss_type + '_' + str(args.learning_rate))
    if not os.path.exists(args.output_dir):
        os.makedirs(args.output_dir)
    print(args.output_dir)
    with open('../dataset/{}/json/statistics.json'.format(args.dataset)) as fp:
        statistics = json.load(fp)
        args.labels_num = statistics['label_num']
    
    set_seed(args.seed)

    # Build the binary RNN model & initialize parameters
    model = BinaryRNN(args)
    initialize_parameters(args, model)

    # Build optimizer and scheduler
    optimizer, scheduler = build_optimizer(args, model)

    # Assign gpu
    gpu_id = args.gpu_id
    if gpu_id is not None:
        torch.cuda.set_device(gpu_id)
        model.cuda(gpu_id)
        print("Using GPU %d for training." % args.gpu_id)
    else:
        print("Using CPU mode for training.")

    # Build data loader
    train_loader = build_data_loader(args, args.train_path, args.batch_size, is_train=True, shuffle=True)
    test_loader = build_data_loader(args, args.test_path, args.batch_size, is_train=False, shuffle=True)

    trainer = BRNNTrainer(args)
    trainer.train(args, gpu_id, train_loader, test_loader, model, optimizer, scheduler)
    

if __name__ == "__main__":
    main()
