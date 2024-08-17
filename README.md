# Brain-on-Switch
The repository of paper [Brain-on-Switch: Towards Advanced Intelligent Network Data Plane via NN-Driven Traffic Analysis at Line-Speed](https://www.usenix.org/conference/nsdi24/presentation/yan), published in USENIX NSDI 2024. (The PDF is available at [Link](https://arxiv.org/abs/2403.11090))

# Repository Overview
Our artifact includes the following directories: 

* [dataset/](https://github.com/InspiringGroup-Lab/Brain-on-Switch/tree/main/dataset) contains our datasets and the source code for data pre-processing.
* [model/](https://github.com/InspiringGroup-Lab/Brain-on-Switch/tree/main/model) contains the source code for training our binary RNN model, and converting the trained model into feed forward tables for switch-side implementation.
* [PerPacket/](https://github.com/InspiringGroup-Lab/Brain-on-Switch/tree/main/PerPacket) contains the source code for training the tree-based model for handling the flows without dedicated per-flow storage on switch.
* [p4/](https://github.com/InspiringGroup-Lab/Brain-on-Switch/tree/main/p4) contains the source code for configuring a Barefoot Tofino 1 (Wedge 100BF-32X) switch and running our RNN-based traffic analysis on it.
* [IMIS/](https://github.com/InspiringGroup-Lab/Brain-on-Switch/tree/main/IMIS) contains the source code for training the Transformer-based model for escalated traffic analysis, and the implementation of our server-side Integrated Model Inference System.

Follow the detailed instructions in each directory to implement the switch-side traffic analysis logic and server-side IMIS.

To forward the escalated traffic between the switch and the server, you need build a network topology to connect them. 

# Contact
Please post a Github issue or send an email to [yanjz22@mails.tsinghua.edu.cn](yanjz22@mails.tsinghua.edu.cn) if you have any questions.

# Credit
Cite this paper as follows  if you find this code repo is useful to you. 

@inproceedings{yan2024brain, 
  title={{Brain-on-Switch: Towards Advanced Intelligent Network Data Plane via NN-Driven Traffic Analysis at Line-Speed}},
  
  author={Yan, Jinzhu and Xu, Haotian and Liu, Zhuotao and Li, Qi and Xu, Ke and Xu, Mingwei and Wu, Jianping},
  
  booktitle={21st USENIX Symposium on Networked Systems Design and Implementation (NSDI 24)},
  
  pages={419--440},
  
  year={2024}
  
}
