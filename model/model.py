import torch
import torch.nn as nn
from utils.bnn_modules import Binary_a


class BinaryGRUCell(nn.Module):
    def __init__(self, rnn_in_bits, rnn_hidden_bits, activation_quantizer):
        super(BinaryGRUCell, self).__init__()
        self.cell = nn.GRUCell(rnn_in_bits, rnn_hidden_bits)
        self.rnn_in_bits = rnn_in_bits
        self.rnn_hidden_bits = rnn_hidden_bits
        self.activation_quantizer = activation_quantizer

    def forward(self, input, time_steps):
        init_hidden = torch.zeros(input.size(0), self.rnn_hidden_bits, device=input.device)
        outputs = [init_hidden]

        for time_step in range(time_steps):
            batch = input[:, time_step]
            hidden = outputs[-1]

            next_hidden = self.cell(batch, hidden)
            next_hidden = self.activation_quantizer(next_hidden)

            outputs.append(next_hidden)

        final_hidden = outputs[-1]
        return final_hidden


class BinaryRNN(nn.Module):
    def __init__(self, args):
        super(BinaryRNN, self).__init__()
        # Feature embedding
        self.len_vocab = args.len_vocab
        self.len_embedding_bits = args.len_embedding_bits
        self.ipd_vocab = args.ipd_vocab
        self.ipd_embedding_bits = args.ipd_embedding_bits
        self.embedding_vector_bits = args.embedding_vector_bits
        # RNN cell
        self.window_size = args.window_size
        self.rnn_in_pkts = args.rnn_in_pkts
        self.rnn_in_bits = args.rnn_in_pkts * args.embedding_vector_bits
        self.rnn_hidden_bits = args.rnn_hidden_bits
        # Output layer
        self.labels_num = args.labels_num

        self.activation_quantizer = Binary_a.apply

        self.len_embedding = nn.Embedding(self.len_vocab, self.len_embedding_bits)
        self.ipd_embedding = nn.Embedding(self.ipd_vocab, self.ipd_embedding_bits)
        self.fc = nn.Linear(self.len_embedding_bits + self.ipd_embedding_bits, self.embedding_vector_bits)
        self.binary_gru = BinaryGRUCell(self.rnn_in_bits, self.rnn_hidden_bits, self.activation_quantizer)
        self.out_layer = nn.Linear(self.rnn_hidden_bits, self.labels_num)

    def forward(self, len_x, ipd_x):
        len_ebd = self.len_embedding(len_x)
        len_ebd_bin = self.activation_quantizer(len_ebd)
        ipd_ebd = self.ipd_embedding(ipd_x)
        ipd_ebd_bin = self.activation_quantizer(ipd_ebd)
        ebd_bin_cat = torch.cat((len_ebd_bin, ipd_ebd_bin), dim=-1)
        batch_size = ebd_bin_cat.shape[0]

        evs = self.fc(ebd_bin_cat)
        evs_bin = self.activation_quantizer(evs)
        rnn_in_bin = evs_bin.view(batch_size, -1, self.rnn_in_bits)
        rnn_out_bin = self.binary_gru(rnn_in_bin, self.window_size)
        logits = self.out_layer(rnn_out_bin)
        return logits

    def get_evs(self, len_x, ipd_x):
        len_ebd = self.len_embedding(len_x)
        len_ebd_bin = self.activation_quantizer(len_ebd)
        ipd_ebd = self.ipd_embedding(ipd_x)
        ipd_ebd_bin = self.activation_quantizer(ipd_ebd)
        ebd_bin_cat = torch.cat((len_ebd_bin, ipd_ebd_bin), dim=-1)
        batch_size = ebd_bin_cat.shape[0]

        evs = self.fc(ebd_bin_cat)
        evs_bin = self.activation_quantizer(evs)
        return evs_bin
