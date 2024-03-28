# Datasets and Pre-processing
There are three directories in the folder of each dataset:
* `source/`
  * Each class in the dataset corresponds to a subdirectory in `source/`, which contains the original pcap files for this class.
  *  By running `prepare_dataset.py`, we extract 5-tuple-identified flows from the original pcap files and save each flow as a single pcap file in `pcap/`.
* `pcap/`
  *  Each class in the dataset corresponds to a subdirectory in `pcap/`, which contains the pcap files for 5-tuple-identified flows extracted from `source/`.
  *  By running `pcap2json.py`, we parse the metadata of each flow from its pcap file and sample the training/testing sets. The training/testing sets are saved in json format in `json/`.
*  `json/`
  *  
