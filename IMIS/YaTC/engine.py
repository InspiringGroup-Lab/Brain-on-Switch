import sys
import math
import time
from typing import Iterable, Optional

import torch

from timm.data import Mixup
from timm.utils import accuracy

import util.lr_sched as lr_sched

from sklearn.metrics import precision_score, recall_score, f1_score, confusion_matrix

import numpy as np

import random
from dataset import flow_pre_processing


def train_one_epoch(model: torch.nn.Module, criterion: torch.nn.Module,
                    flows: list, optimizer: torch.optim.Optimizer,
                    device: torch.device, epoch: int, max_norm: float = 0,
                    mixup_fn: Optional[Mixup] = None,
                    args=None):
    model.train(True)
    random_index = random.sample(range(len(flows)), len(flows))

    accum_iter = args.accum_iter

    optimizer.zero_grad()

    y_true = []
    y_pred = []

    time_start = time.time()
    epoch_loss = 0
    batch_x = []
    batch_y = []
    data_iter_step = 0
    for i in range(len(flows)):
        flow = flows[random_index[i]]
            
        flow_x_tensor, flow_y = flow_pre_processing(flow, args.input_packets, args.header_bytes, args.payload_bytes, args.row_bytes)
        batch_x.append(flow_x_tensor)
        batch_y.append(flow_y)

        if i % args.batch_size == args.batch_size - 1 or i == len(flows) - 1:
            # we use a per iteration (instead of per epoch) lr scheduler
            if data_iter_step % accum_iter == 0:
                lr_sched.adjust_learning_rate(optimizer, i / len(flows) + epoch, args)

            batch_x_tensor = torch.stack(batch_x).to(device)
            batch_y_tensor = torch.tensor(batch_y, device=device)

            if mixup_fn is not None:
                batch_x_tensor, batch_y_tensor = mixup_fn(batch_x_tensor, batch_y_tensor)

            with torch.cuda.amp.autocast():
                logits = model(batch_x_tensor)
                loss = criterion(logits, batch_y_tensor)
            
            _, pred = logits.topk(1, 1, True, True)
            pred = pred.t()
            y_pred.extend(pred[0].cpu())
            y_true.extend(batch_y)
            
            loss_value = loss.item()
            epoch_loss += loss_value * len(batch_y)
            if not math.isfinite(loss_value):
                print("Loss is {}, stopping training".format(loss_value))
                sys.exit(1)
            
            loss /= accum_iter
            loss.backward()
            if (data_iter_step + 1) % accum_iter == 0:
                optimizer.step()
                optimizer.zero_grad()

            data_iter_step += 1
            batch_x = []
            batch_y = []
    
    precision = precision_score(y_true, y_pred, average='macro')
    recall = recall_score(y_true, y_pred, average='macro')
    f1score = f1_score(y_true, y_pred, average='macro')

    print('epoch_loss_avg', epoch_loss / len(flows), 'precision', precision, 'recall', recall, 'f1', f1score, 'cost_time', time.time() - time_start)
    print(confusion_matrix(y_true, y_pred))

    return f1score


@torch.no_grad()
def evaluate(flows, model, device, args):
    model.eval()

    criterion = torch.nn.CrossEntropyLoss()

    y_true = []
    y_pred = []

    test_loss = 0
    batch_x = []
    batch_y = []
    for i in range(len(flows)):
        flow = flows[i]
            
        flow_x_tensor, flow_y = flow_pre_processing(flow, args.input_packets, args.header_bytes, args.payload_bytes, args.row_bytes)
        batch_x.append(flow_x_tensor)
        batch_y.append(flow_y)

        if i % args.batch_size == args.batch_size - 1 or i == len(flows) - 1:
            batch_x_tensor = torch.stack(batch_x).to(device)
            batch_y_tensor = torch.tensor(batch_y, device=device)

            with torch.cuda.amp.autocast():
                logits = model(batch_x_tensor)
                loss = criterion(logits, batch_y_tensor)
            
            _, pred = logits.topk(1, 1, True, True)
            pred = pred.t()
            y_pred.extend(pred[0].cpu())
            y_true.extend(batch_y)
            
            loss_value = loss.item()
            test_loss += loss_value * len(batch_y)
            
            batch_x = []
            batch_y = []
    
    precision = precision_score(y_true, y_pred, average='macro')
    recall = recall_score(y_true, y_pred, average='macro')
    f1score = f1_score(y_true, y_pred, average='macro')

    print('test_loss_avg', test_loss / len(flows), 'precision', precision, 'recall', recall, 'f1', f1score)
    print(confusion_matrix(y_true, y_pred))

    return f1score