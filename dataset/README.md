# Datasets and Pre-processing
You can download the complete dataset directories at [Link]().
There are three directories for each dataset (ISCXVPN2016, BOTIOT, CICIOT2022, PeerRush):
1. `source/`
  -  Each class in the dataset corresponds to a subdirectory in `source/`, which contains the original pcap files for this class.
  -  By running `prepare_dataset.py`, we extract 5-tuple-identified flows from the original pcap files and save each flow as a single pcap file in `pcap/`.
2. `pcap/`
  -  Each class in the dataset corresponds to a subdirectory in `pcap/`, which contains the pcap files for 5-tuple-identified flows extracted from `source/`.
  -  By running `pcap2json.py`, we parse the metadata of each flow from its pcap file and generate the training/testing sets. The training/testing sets are saved in json format in `json/`.
3. `json/`
  -  `statistics.json` contains the total number of flows in the training/testing set, the number of classes and the number of flows for each individual class.
  -  `train.json` and `test.json` contain the flow records of the training set and the testing set respectively. Each flow record is a dictionary instance containing metadata such as flow label, pcap file path (in `pcap/`), packet length sequence, inter-packet delay sequence, etc.

## Explanation of Data Pre-processing 
For every dataset used in our evaluations, we collect flow records from the original pcap files using the following procedure. 
1. We collect the original pcap files for each class in the dataset separately, and all the flow records extracted from a pcap file are labelled as the class of this file.
2. For each pcap file, we collect the TCP and UDP packets of IPv4, and remove other irrelevant packets, e.g., packets of Domain Name System (DNS), Address Resolution Protocol (ARP), Dynamic Host Configuration Protocol (DHCP) and so on.
3. We split a clean pcap file by five tuple (using [SplitCap](https://github.com/InspiringGroup-Lab/Brain-on-Switch/blob/main/dataset/SplitCap.exe)), and further split packets of the same five tuple into flow records by inter-packet delays. Specifically, if the inter-packet delay between two packets is greater than 256 ms, we consider the latter packet as the first packet of a new flow record. This is consistent with our online inference where we consider a flow is completed if we do not receive new packets for the flow for 256 ms.
4. 80% of flow records in a dataset are used as the training set and the remaining records are used as the testing set.
