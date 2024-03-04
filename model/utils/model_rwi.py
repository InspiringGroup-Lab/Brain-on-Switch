import torch


def load_model(model, model_path):
    if hasattr(model, "module"):
        model.module.load_state_dict(torch.load(model_path, map_location="cpu"), strict=False)
    else:
        model.load_state_dict(torch.load(model_path, map_location="cpu"), strict=False)
    return model


def save_model(model, model_path):
    if hasattr(model, "module"):
        torch.save(model.module.state_dict(), model_path)
    else:
        torch.save(model.state_dict(), model_path)


def initialize_parameters(args, model):
    # Initialize with normal distribution.
    for n, p in list(model.named_parameters()):
        if "gamma" not in n and "beta" not in n:
            p.data.normal_(0, 0.02)