# Binary RNN Model Training

1. Parameters Configuration

* By modifying [opt.py](https://github.com/InspiringGroup-Lab/Brain-on-Switch/blob/main/model/opts.py), you can configure the parameters for building and training our binary RNN model architecture. The explanation for each parameter and the default setttings for each dataset can be found in the file.

2. Model Training

* Run `python train.py --dataset DATASET_NAME`. The model checkpoints and training logs will be saved in `./save/DATASET_NAME/`.
* The binary RNN model is trained with the flow segments sliced by the sliding window. To test the accuracy with the intermediate results aggregation, run `python aggregator.py --dataset= DATASET_NAME`. The results will also be saved in `./save/DATASET_NAME/`.

# Convert Binary RNN Model into Feed Forward Tables

* Run `python model_convertion.py --dataset DATASET_NAME`. The feed forward tables will be saved in `./save/DATASET_NAME/`.
