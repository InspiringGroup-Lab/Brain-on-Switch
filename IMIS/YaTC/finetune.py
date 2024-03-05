import os
os.environ['CUDA_VISIBLE_DEVICES']='1'

import argparse
from opts import *

import json
import torch
import numpy as np
import torch.backends.cudnn as cudnn

import timm
assert timm.__version__ == "0.6.13"  # version check
from timm.models.layers import trunc_normal_
from timm.data.mixup import Mixup
from timm.loss import LabelSmoothingCrossEntropy, SoftTargetCrossEntropy

import models_YaTC
import util.lr_decay as lrd
from dataset import load_data
from util.pos_embed import interpolate_pos_embed

from engine import train_one_epoch, evaluate


def logging(filename, line):
    with open(filename, 'a') as fp:
        fp.writelines(line)


def main():
    # args --------------------------------------
    parser = argparse.ArgumentParser(formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    
    # Dataset
    parser.add_argument("--dataset", default="ISCXVPN2016",
                        choices=["ISCXVPN2016", "BOTIOT", "CICIOT2022", "PeerRush"])

    # YaTC model opts
    model_opts(parser)

    args = parser.parse_args()

    args.dataset_dir = '../../dataset/{}/json/'.format(args.dataset)
    args.save_dir = './save/{}/'.format(args.dataset)
    if not os.path.exists(args.save_dir):
        os.makedirs(args.save_dir)

    args.device = 'cuda'
    print('Finetuning the base YaTC model')
    print('save dir: {}'.format(args.save_dir))
    print('--------------------------------------')
    # args --------------------------------------
    
    # set up
    device = torch.device(args.device)
    with open(args.dataset_dir + 'statistics.json') as fp:
        statistics_json = json.load(fp)
    args.nb_classes = statistics_json['label_num']
    
    # fix the seed for reproducibility
    seed = args.seed
    torch.manual_seed(seed)
    np.random.seed(seed)

    cudnn.benchmark = True

    print ("Loading the data for training, and evaluating the model")
    read_packet_count = args.input_packets
    train_flows = load_data(args.dataset_dir + 'train.json', read_packet_count)
    test_flows = load_data(args.dataset_dir + 'test.json', read_packet_count)

    mixup_fn = None
    mixup_active = args.mixup > 0 or args.cutmix > 0. or args.cutmix_minmax is not None
    if mixup_active:
        print("Mixup is activated!")
        mixup_fn = Mixup(
            mixup_alpha=args.mixup, cutmix_alpha=args.cutmix, cutmix_minmax=args.cutmix_minmax,
            prob=args.mixup_prob, switch_prob=args.mixup_switch_prob, mode=args.mixup_mode,
            label_smoothing=args.smoothing, num_classes=args.nb_classes)

    print ("Building the YaTC model")
    model = models_YaTC.__dict__[args.model](
        num_classes=args.nb_classes,
        drop_path_rate=args.drop_path,
    )

    checkpoint = torch.load(args.finetune, map_location='cpu')
    print("Load pre-trained checkpoint from: %s" % args.finetune)
    checkpoint_model = checkpoint['model']
    state_dict = model.state_dict()
    for k in ['head.weight', 'head.bias']:
        if k in checkpoint_model and checkpoint_model[k].shape != state_dict[k].shape:
            print(f"Removing key {k} from pretrained checkpoint")
            del checkpoint_model[k]
    
    # interpolate position embedding
    interpolate_pos_embed(model, checkpoint_model)

    # load pre-trained model
    msg = model.load_state_dict(checkpoint_model, strict=False)
    print(msg)

    # manually initialize fc layer
    trunc_normal_(model.head.weight, std=2e-5)

    model.to(device)
    print(args.device)
    print(torch.cuda.get_device_name(args.device))

    model_without_ddp = model
    n_parameters = sum(p.numel() for p in model.parameters() if p.requires_grad)

    print("Model = %s" % str(model_without_ddp))
    print('number of params (M): %.2f' % (n_parameters / 1.e6))

    eff_batch_size = args.batch_size * args.accum_iter

    if args.lr is None:  # only base_lr is specified
        args.lr = args.blr * eff_batch_size / 256
    
    print("base lr: %.2e" % (args.lr * 256 / eff_batch_size))
    print("actual lr: %.2e" % args.lr)

    print("accumulate grad iterations: %d" % args.accum_iter)
    print("effective batch size: %d" % eff_batch_size)

    # build optimizer with layer-wise lr decay (lrd)
    param_groups = lrd.param_groups_lrd(model_without_ddp, args.weight_decay,
                                        no_weight_decay_list=model_without_ddp.no_weight_decay(),
                                        layer_decay=args.layer_decay
                                        )
    optimizer = torch.optim.AdamW(param_groups, lr=args.lr)

    if mixup_fn is not None:
        # smoothing is handled with mixup label transform
        criterion = SoftTargetCrossEntropy()
    elif args.smoothing > 0.:
        criterion = LabelSmoothingCrossEntropy(smoothing=args.smoothing)
    else:
        criterion = torch.nn.CrossEntropyLoss()

    print("criterion = %s" % str(criterion))

    print(f"Start training for {args.epochs} epochs")
    
    early_stop = 0
    best_f1_score = 0
    logging(args.save_dir + 'training.txt', 
            'epoch'+','+'best_test_f1score'+','+'train_f1score'+','+'test_f1score'+'\n')
    for epoch in range(args.start_epoch, args.epochs):
        print('epoch', epoch)

        train_f1score = train_one_epoch(
            model, criterion, train_flows,
            optimizer, device, epoch,
            args.clip_grad, mixup_fn,
            args=args
        )

        test_f1score = evaluate(test_flows, model, device, args)
        
        if test_f1score>best_f1_score:
            best_f1_score = test_f1score
            torch.save(model.state_dict(), args.save_dir +'yatc_best.pt')
            early_stop = 0

        logging(args.save_dir + 'training.txt', 
                str(epoch)+','+str(best_f1_score)+','+str(train_f1score)+','+str(test_f1score)+'\n')

        torch.save(model.state_dict(), args.save_dir +'yatc.pt')
        early_stop += 1
        if early_stop >= 50:
            break
    
    return


if __name__ == '__main__':
    main()