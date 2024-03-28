# Train Transformer-based Model YaTC

1. Parameters Configuration

* By modifying [opt.py](https://github.com/InspiringGroup-Lab/Brain-on-Switch/blob/main/IMIS/YaTC/opts.py), you can configure the parameters for building and training the Transformer-based model [YaTC](https://github.com/NSSL-SJTU/YaTC). The explanation and default settting for each parameter can be found in the file.

2. Model Training

* The pre-trained model of YaTC can be downloaded at [Link](https://drive.google.com/file/d/1wWmZN87NgwujSd2-o5nm3HaQUIzWlv16/view?usp=drive_link). Put the pre-trained model according to the path dependency in [finetune.py](https://github.com/InspiringGroup-Lab/Brain-on-Switch/blob/main/IMIS/YaTC/finetune.py).
* Run `python finetune.py --dataset DATASET_NAME`. The model checkpoints and training logs will be saved in `./save/DATASET_NAME/`.
