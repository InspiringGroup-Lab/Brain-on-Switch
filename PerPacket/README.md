# Train PerPacket Model

1. Parameters Configuration

* By modifying [opt.py](https://github.com/InspiringGroup-Lab/Brain-on-Switch/blob/main/PerPacket/opts.py), you can configure the parameters for the tree-based model. The explanation for each parameter and the default setttings for each dataset can be found in the file.

2. Model Training

* Run `python train.py --dataset DATASET_NAME`. The trained model, accuracy on the testing set and the feature importances will be saved in `./save/DATASET_NAME/`.
