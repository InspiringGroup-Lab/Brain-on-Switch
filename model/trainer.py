import time
import json
import torch
import numpy as np
from tqdm import tqdm
from utils.model_rwi import *
from utils.early_stopping import *
from torch.utils.data import DataLoader
from utils.data_loader import FlowDataset
from utils.metric import metric_from_confuse_matrix
import torch.optim as optim


def build_optimizer(args, model):
    if args.optimizer == 'adamw':
        optimizer = optim.AdamW(model.parameters(), lr=args.learning_rate)
    else:
        optimizer = optim.Adam(model.parameters(), lr=args.learning_rate)
    
    return optimizer, None


def build_data_loader(args, filename, batch_size, is_train=False, shuffle=True):
    dataset = FlowDataset(args, filename)
    data_loader = DataLoader(dataset, batch_size=batch_size, 
                             shuffle=shuffle)
    print('The size of {}_set is {}.'.format(
            'train' if is_train else 'test', len(dataset)))
    return data_loader


def batch2segs(args, batch):
    len_x_batch = []
    ipd_x_batch = []
    label_batch = []
    for i in range(len(batch[0])):
        label = batch[1][i]
        seqs = batch[0][i].split(';')
        len_seq = eval(seqs[0])
        ipd_seq = eval(seqs[1])
        flow_packets = len(len_seq)
        if flow_packets < args.window_size:
            raise Exception('Flow packets < window size!!!')
            # continue
        else:
            segs_idx = [idx for idx in range(0, flow_packets - args.window_size + 1)]
            batch_segs_idx = segs_idx

            for idx in batch_segs_idx:
                len_x_batch.append(len_seq[idx: idx + args.window_size])
                ipd_x_batch.append(ipd_seq[idx: idx + args.window_size])
                label_batch.append(label)

    len_x_batch = torch.LongTensor(len_x_batch)
    ipd_x_batch = torch.LongTensor(ipd_x_batch)
    label_batch = torch.tensor(label_batch)

    if args.gpu_id is not None:
        len_x_batch = len_x_batch.cuda(args.gpu_id)
        ipd_x_batch = ipd_x_batch.cuda(args.gpu_id)
        label_batch = label_batch.cuda(args.gpu_id)

    return len_x_batch, ipd_x_batch, label_batch


def save_checkpoint(output_dir, model_name, model, result_log):
    print('Saving model: {}'.format(output_dir + model_name))
    save_model(model, output_dir + model_name)
    with open(output_dir + model_name + '-result.txt', 'w') as fp:
        for line in result_log:
            # print(line)
            fp.writelines(line + '\n')


class BRNNTrainer(object):
    def __init__(self, args):
        self.current_epoch = 0
        self.total_epochs = args.total_epochs
        self.save_checkpoint_epochs = args.save_checkpoint_epochs
        
        self.labels_num = args.labels_num
        self.output_dir = args.output_dir

        self.loss_factor = args.loss_factor
        self.focal_loss_gamma = args.focal_loss_gamma
        self.loss_type = args.loss_type


    def forward_propagation(self, len_x_batch, ipd_x_batch, label_batch, model):
        logits = model(len_x_batch, ipd_x_batch)

        softmax = torch.nn.functional.softmax(logits)
        one_hot = torch.nn.functional.one_hot(label_batch, num_classes=self.labels_num)
        alpha = torch.tensor([self.class_weights[label] for label in label_batch], device=one_hot.device)

        p_y = softmax[one_hot == 1]
        loss_y = - (1 - p_y) ** self.focal_loss_gamma * torch.log(p_y)
        
        if self.loss_type == 'single':
            remove_p = one_hot.float()
            remove_p[remove_p == 1] = -torch.inf
            max_without_p, _ = (softmax + remove_p).max(dim=1, keepdim=True)
            max_without_p = torch.squeeze(max_without_p)
            loss_others = - max_without_p ** self.focal_loss_gamma * torch.log(1 - max_without_p)
        else:
            p_others = softmax[one_hot == 0].reshape(shape=(len(softmax), self.labels_num - 1))
            loss_others = - p_others ** self.focal_loss_gamma * torch.log(1 - p_others)
            loss_others = torch.sum(loss_others, dim=1)
        
        if self.weighted != 'unweighted':
            loss_y = alpha * loss_y
            loss_others = alpha * loss_others

        loss_1 = torch.sum(loss_y) / len(softmax)
        loss_2 = torch.sum(loss_others) / len(softmax)
        loss = loss_1 + self.loss_factor * loss_2
        
        return loss, logits, loss_1, loss_2


    def validate(self, args, test_loader, model):
        model.eval()

        test_samples = 0
        test_total_loss = 0
        test_total_loss_1 = 0
        test_total_loss_2 = 0
        conf_mat_test = np.zeros([args.labels_num, args.labels_num])
        with torch.no_grad():
            for batch in test_loader:
                len_x_batch, ipd_x_batch, label_batch = batch2segs(args, batch, args.max_cluster_segs)
                loss, logits, loss_1, loss_2 = self.forward_propagation(len_x_batch, ipd_x_batch, label_batch, model)
                
                test_samples += len_x_batch.shape[0]
                test_total_loss += loss.item() * len_x_batch.shape[0]
                test_total_loss_1 += loss_1.item() * len_x_batch.shape[0]
                test_total_loss_2 += loss_2.item() * len_x_batch.shape[0]
                pred = logits.max(dim=1, keepdim=True)[1]
                for i in range(len(pred)):
                    conf_mat_test[label_batch[i].cpu(), pred[i].cpu()] += 1
        
        return conf_mat_test, test_total_loss, test_total_loss_1, test_total_loss_2, test_samples
            

    def train(self, args, train_loader, test_loader, model, optimizer):
        learning_curves = {
            'train_loss': [],
            'train_loss_1': [],
            'train_loss_2': [],
            'train_precision': [],
            'train_recall': [],
            'train_f1': [],
            'test_loss': [],
            'test_loss_1': [],
            'test_loss_2': [],
            'test_precision': [],
            'test_recall': [],
            'test_f1': [],
        }

        early_stopping = EarlyStopping(patience=50, delta=0, verbose=False)
        while True:
            self.current_epoch += 1
            if self.current_epoch == self.total_epochs + 1:
                return
            start_time = time.time()
            
            # Train for an epoch
            model.train()
            train_samples = 0
            train_total_loss = 0
            train_total_loss_1 = 0
            train_total_loss_2 = 0
            conf_mat_train = np.zeros([args.labels_num, args.labels_num])
            for batch in train_loader:
                len_x_batch, ipd_x_batch, label_batch = batch2segs(args, batch)
                loss, logits, loss_1, loss_2 = self.forward_propagation(len_x_batch, ipd_x_batch, label_batch, model)

                loss.backward()
                optimizer.step()
                model.zero_grad()

                train_samples += len_x_batch.shape[0]
                train_total_loss += loss.item() * len_x_batch.shape[0]
                train_total_loss_1 += loss_1.item() * len_x_batch.shape[0]
                train_total_loss_2 += loss_2.item() * len_x_batch.shape[0]
                pred = logits.max(dim=1, keepdim=True)[1]
                for i in range(len(pred)):
                    conf_mat_train[label_batch[i].cpu(), pred[i].cpu()] += 1

            # Validation
            conf_mat_test, test_total_loss, test_total_loss_1, test_total_loss_2, test_samples  = self.validate(args, test_loader, model)
            
            # Report losses
            train_avg_loss = train_total_loss / train_samples
            train_avg_loss_1 = train_total_loss_1 / train_samples
            train_avg_loss_2 = train_total_loss_2 / train_samples
            test_avg_loss = test_total_loss / test_samples
            test_avg_loss_1 = test_total_loss_1 / test_samples
            test_avg_loss_2 = test_total_loss_2 / test_samples
            print("| {:5d}/{:5d} epochs ({:5.2f} s, lr {:8.5f})"
                    "| Train segs {:7d}, Test segs {:7d} "
                    "| Train loss {:7.2f} loss_1 {:7.2f} loss_2 {:7.2f}"
                    "| Test loss {:7.2f} loss_1 {:7.2f} loss_2 {:7.2f}".format(
                    self.current_epoch, self.total_epochs, time.time() - start_time, optimizer.param_groups[0]['lr'],
                    train_samples, test_samples,
                    train_avg_loss, train_avg_loss_1, train_avg_loss_2,
                    test_avg_loss, test_avg_loss_1, test_avg_loss_2
                ))
            
            # Early Stopping
            status = early_stopping(test_total_loss / test_samples)
            if status == EARLY_STOP:
                return
            
            # Save model
            if status == BEST_SCORE_UPDATED or self.current_epoch % self.save_checkpoint_epochs == 0:
                pres_train, recs_train, f1s_train, logs_train = metric_from_confuse_matrix(conf_mat_train)
                pres_test, recs_test, f1s_test, logs_test = metric_from_confuse_matrix(conf_mat_test)
                logs = ['Training set: {} segs, average loss {}'.format(train_samples, train_avg_loss)]
                logs.extend(logs_train)
                logs.append('Testing set: {} segs, average loss {}'.format(test_samples, test_avg_loss))
                logs.extend(logs_test)
                if status == BEST_SCORE_UPDATED:
                    save_checkpoint(output_dir = self.output_dir, 
                                    model_name = 'brnn-best',
                                    model=model,
                                    result_log=logs)
                if self.current_epoch % self.save_checkpoint_epochs == 0:
                    save_checkpoint(output_dir = self.output_dir, 
                                    model_name = 'brnn-' + str(self.current_epoch),
                                    model=model,
                                    result_log=logs)
            
            # Save learning curves
            learning_curves['train_loss'].append(train_avg_loss)
            learning_curves['train_loss_1'].append(train_avg_loss_1)
            learning_curves['train_loss_2'].append(train_avg_loss_2)
            learning_curves['test_loss'].append(test_avg_loss)
            learning_curves['test_loss_1'].append(test_avg_loss_1)
            learning_curves['test_loss_2'].append(test_avg_loss_2)
            with open(self.output_dir + 'learning_curves.json', 'w') as fp:
                json.dump(learning_curves, fp, indent=1)
