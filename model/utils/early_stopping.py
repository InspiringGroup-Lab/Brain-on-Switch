import numpy as np
import torch
from utils.model_rwi import save_model


EARLY_STOP = 1
BEST_SCORE_UPDATED = 2


class EarlyStopping:
    """Early stops the training if validation loss doesn't improve after a given patience."""
    def __init__(self, patience=7, delta=0, verbose=False):
        """
        Args:
            patience (int): How long to wait after last time validation loss improved.
                            Default: 7
            verbose (bool): If True, prints a message for each validation loss improvement. 
                            Default: False
            delta (float): Minimum change in the monitored quantity to qualify as an improvement.
                            Default: 0
            
        """
        self.patience = patience
        self.delta = delta
        self.verbose = verbose
        self.counter = 0
        self.best_score = None
        self.early_stop = False
        self.val_loss_min = np.Inf


    def __call__(self, val_loss):

        score = - val_loss

        if self.best_score is None:
            self.best_score = score
            return BEST_SCORE_UPDATED
        elif score > self.best_score + self.delta:
            if self.verbose:
                print(f'Validation loss decreased ({self.val_loss_min:.6f} --> {val_loss:.6f}).')
            self.best_score = score
            self.val_loss_min = val_loss

            self.counter = 0
            
            return BEST_SCORE_UPDATED
        else:
            self.counter += 1
            if self.verbose:
                print(f'EarlyStopping counter: {self.counter} out of {self.patience}')
            if self.counter >= self.patience:
                self.early_stop = True
                print(f'Early Stop!')
                return EARLY_STOP
