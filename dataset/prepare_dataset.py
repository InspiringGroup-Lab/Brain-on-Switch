'''
Handle the raw dataset: raw_pcap -> pcap
* only tcp packets
* split flow by session & interval time
'''
import argparse
import os
import dpkt
from dpkt.pcap import DLT_RAW, DLT_EN10MB
import shutil
import scapy.all as scapy


# clean_protocols = '"not arp and not (dns or mdns) and not stun and not dhcpv6 and not icmpv6 and not icmp and not dhcp and not llmnr and not nbns and not ntp and not igmp"'
clean_protocols = '"' + \
                    'not ipv6 and (tcp or udp)' + \
                    ' and not (tcp.analysis.keep_alive or tcp.analysis.keep_alive_ack or tcp.analysis.retransmission or tcp.flags.reset eq 1)' + \
                    ' and not (dns or mdns or dhcp or ntp or nbns or stun or ssdp or arp or llmnr or icmp or igmp or isakmp or udpencap.nat_keepalive)' + \
                  '"'

interval_seconds = {
    'ISCXVPN2016': 0.256,
    "BOTIOT": 0.256,
    'CICIOT2022': 0.256,
    'PeerRush': 0.256
}

PeerRush_IP_filter = {
    'eMule': ' and (ip.src==192.168.1.2 or ip.src==192.168.3.2 or ip.dst==192.168.1.2 or ip.dst==192.168.3.2) and not udp',
    'uTorrent': ' and (ip.src==192.168.1.2 or ip.src==192.168.3.2 or ip.dst==192.168.1.2 or ip.dst==192.168.3.2) and not udp',
    'Vuze': ' and (ip.src==192.168.2.2 or ip.src==192.168.4.2 or ip.dst==192.168.2.2 or ip.dst==192.168.4.2) and not udp'
}


def split_pcap_by_time(input_file, output_dir, time_interval, args):
    # time_interval = time_interval * 1e9 // 16384 * 16384  # taking into account the precision of tofino operations
    os.mkdir(output_dir)    
    flow_end_ts = -1
    flows = []  # [flow, ...]
    flow = []  # [(ts, pkt), ...]

    reader = dpkt.pcap.Reader(open(input_file, 'rb'))
    for ts, pkt in reader:
        if flow_end_ts != -1 and ts - flow_end_ts > time_interval:
            # start of a new flow
            flows.append(flow)
            flow = [(ts, pkt)]        
        else:
            flow.append((ts, pkt))

        flow_end_ts = ts

    if len(flow) != 0:
        flows.append(flow)
    
    for flow in flows:
        if args.dataset in ['ISCXVPN2016']:
            linktype = DLT_RAW
        else:    
            linktype = DLT_EN10MB
        writer = dpkt.pcap.Writer(
                open(output_dir + '/{}.pcap'.format(round(flow[0][0])), 'wb'), linktype=linktype
            )
        writer.writepkts(flow)


def main():
    parser = argparse.ArgumentParser(formatter_class=argparse.ArgumentDefaultsHelpFormatter)

    # Dataset
    parser.add_argument("--dataset", default="ISCXVPN2016",
                        choices=["ISCXVPN2016", "BOTIOT", "CICIOT2022", "PeerRush"])
    args = parser.parse_args()

    for p, dirs_label, _ in os.walk('./{}/raw_pcap'.format(args.dataset)):
        for dir_label in dirs_label:
            tgt_dir = os.path.join(p.replace('raw_pcap', 'pcap'), dir_label)
            if os.path.exists(tgt_dir):
                shutil.rmtree(tgt_dir)
            os.mkdir(tgt_dir)

            print("---------- {}/{}".format(p, dir_label))
            for pp, _, files in os.walk(os.path.join(p, dir_label)):
                for file in files:
                    
                    # filter for Idle traffic of CICIOT2022 dataset 
                    if args.dataset == 'CICIOT2022' and dir_label == 'Idle' and file != '2021_12_01_Idle.pcap':
                        continue
                    
                    if args.dataset == 'PeerRush':
                        if dir_label == 'eMule' and '2011033000' not in file:
                            continue
                        if dir_label in ['uTorrent', 'Vuze'] and '2011040700' not in file:
                            continue

                    # filter for targeted IP of PeerRush dataset
                    if args.dataset == 'PeerRush':
                        tshark_filter = clean_protocols[:-1] + PeerRush_IP_filter[dir_label] + '"'
                    else:
                        tshark_filter = clean_protocols

                    # rename pcapng as pcap
                    if file.find('.pcapng') != -1:
                        shutil.move(os.path.join(pp, file), os.path.join(pp, file.replace('.pcapng', '.pcap')))
                        file = file.replace('.pcapng', '.pcap')
                    if file.find('.pcap') == -1:
                        continue
                    
                    org_file = os.path.join(pp, file)
                    clean_file = os.path.join(tgt_dir, file)
                    print(org_file + ' ...')
                    
                    # remove DHCP etc.
                    os.system(f"tshark -F pcap -r \"{org_file}\" -Y {tshark_filter} -w {clean_file}")

                    # split by session & interval time
                    session_dir = clean_file.replace('.pcap', '')
                    os.system(f"./SplitCap.exe -r {clean_file} -s session -o {session_dir} > /dev/null")
                    os.remove(clean_file)
                    for _, _, session_pcap_files in os.walk(session_dir):
                        for session_pcap_file in session_pcap_files:
                            session_pcap_file = os.path.join(session_dir, session_pcap_file)
                            segment_dir = session_pcap_file.replace(file + '.', '').replace('.pcap', '')
                            split_pcap_by_time(session_pcap_file, segment_dir, interval_seconds[args.dataset], args, dir_label)
                            os.remove(session_pcap_file)
                        break
        break


if __name__ == "__main__":
    main()
