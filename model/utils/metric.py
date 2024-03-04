import numpy as np


def clac_f1(precisions, recalls):
    f1s = []
    epsilon = 1e-6
    for (p,r) in zip(precisions, recalls):
        if (p>epsilon) and (r>epsilon):
            f1s.append( (2*p*r) / (p+r) )
        else:
            f1s.append(0.0)
    return f1s


def seperated_p_r(conf_mat):
    preds = conf_mat.sum(axis=0) * 1.0
    trues = conf_mat.sum(axis=1) * 1.0
    p, r = [], []
    for i in range(conf_mat.shape[0]):
        if preds[i]:
            p.append(conf_mat[i,i] / preds[i])
        else:
            p.append(0.0)
        r.append(conf_mat[i,i] / trues[i])
    return p, r


def metric_from_confuse_matrix(conf_mat):
    r"""Calc f1 score from true_label list and pred_label list
    Args:
        conf_mat (np.array): 2D confuse_matrix of int, conf_mat[i,j]
            stands for samples with true label_i predicted as label_j.
    Returns:
        micro_p_r_f (float): micro precision and recall
        macro_f1 (float): macro_f1 value
        macro_p (float): macro_precision value
        macro_r (float): macro_recall value
    """
    precisions, recalls = seperated_p_r(conf_mat)
    f1s = clac_f1(precisions, recalls)

    logs = []
    for i in range(len(precisions)):
        logs.append("| label {:8d}"
            "| segs {:8d}"    
            "| precision {:8.2f}"
            "| recall {:8.2f}"
            "| f1 {:8.2f}".format(
            i, int(sum(conf_mat[i])), precisions[i], recalls[i], f1s[i])
        )
    logs.append("| Macro"    
            "| precision {:8.2f}"
            "| recall {:8.2f}"
            "| f1 {:8.2f}\n".format(np.mean(precisions), np.mean(recalls), np.mean(f1s))
    )

    return precisions, recalls, f1s, logs


def get_conf_mat(true_label, pred_label):
    r"""Calc f1 score from true_label list and pred_label list
    Args:
        conf_mat (np.array): 2D confuse_matrix of int, conf_mat[i,j]
            stands for samples with true label_i predicted as label_j.
    Returns:
        micro_p_r_f (float): micro precision and recall
        macro_f1 (float): macro_f1 value
        macro_p (float): macro_precision value
        macro_r (float): macro_recall value
    """
    conf_mat = np.zeros([2, 2])
    for i in range(len(pred_label)):
        conf_mat[true_label[i], pred_label[i]] += 1

    return conf_mat
