import json
import binascii
import numpy as np
from tqdm import tqdm
from PIL import Image
import scapy.all as scapy
from torchvision import transforms


def load_data(data_file, read_packet_count):
    flows = []
    with open(data_file) as fp:
        data_json = json.load(fp)
    for flow in tqdm(data_json, desc='Loading flows'):
        pcap_file = data_file.split('/json/')[0] + '/pcap/' + flow['source_file'].split('/pcap/')[-1]
        flow['source_file'] = pcap_file

        pkts = scapy.rdpcap(pcap_file, count=read_packet_count)
        pkts = [pkt['IP'] for pkt in pkts]
        flow['packets'] = pkts
        
        flows.append(flow)
    
    return flows


def flow_pre_processing(flow, input_packets, header_bytes, payload_bytes, row_bytes):
    header_hexes = 2 * header_bytes
    payload_hexes = 2 * payload_bytes
    
    headers_payloads = []
    packets = flow['packets']
    prev_src_sport = ''
    direction = -1
    for packet in packets[:input_packets]:
        header = (binascii.hexlify(bytes(packet['IP']))).decode()
        try:
            payload = (binascii.hexlify(bytes(packet['Raw']))).decode()
            header = header.replace(payload, '')
        except:
            payload = ''
        
        # Mask IP addresses & ports
        ip = packet['IP'].copy()
        try:
            tcp_udp = packet['TCP'].copy()
        except:
            tcp_udp = packet['UDP'].copy()

        src_sport = ip.src + '-' + str(tcp_udp.sport)
        if src_sport != prev_src_sport:
            direction = - direction
            prev_src_sport = src_sport

        ip.payload = scapy.NoPayload()
        ip_hex_string = (binascii.hexlify(bytes(ip))).decode()
        tcp_udp.payload = scapy.NoPayload()
        tcp_udp_hex_string = (binascii.hexlify(bytes(tcp_udp))).decode()
        if direction == 1:
            header = header.replace(ip_hex_string[:40], ip_hex_string[:24] + 'ffffffff00000000')
            header = header.replace(tcp_udp_hex_string, 'ffff0000' + tcp_udp_hex_string[8:])
        else:
            header = header.replace(ip_hex_string[:40], ip_hex_string[:24] + '00000000ffffffff')
            header = header.replace(tcp_udp_hex_string, '0000ffff' + tcp_udp_hex_string[8:])

        if len(header) > header_hexes:
            header = header[:header_hexes]
        elif len(header) < header_hexes:
            header += '0' * (header_hexes - len(header))

        if len(payload) > payload_hexes:
            payload = payload[:payload_hexes]
        elif len(payload) < payload_hexes:
            payload += '0' * (payload_hexes - len(payload))
        
        headers_payloads.append((header, payload))

    if len(headers_payloads) < input_packets:
        for _ in range(input_packets - len(headers_payloads)):
            headers_payloads.append(('0' * header_hexes, '0' * payload_hexes))

    flow_hex_string = ''
    for header, payload in headers_payloads:
        flow_hex_string += header
        flow_hex_string += payload
    
    flow_byte_array = np.array([int(flow_hex_string[i:i + 2], 16) for i in range(0, len(flow_hex_string), 2)])
    flow_byte_array = np.reshape(flow_byte_array, (-1, row_bytes))
    flow_byte_array = np.uint8(flow_byte_array)
    flow_img = Image.fromarray(flow_byte_array)

    mean = [0.5]
    std = [0.5]
    transform = transforms.Compose([
        transforms.Grayscale(num_output_channels=1),
        transforms.ToTensor(),
        transforms.Normalize(mean, std),
    ])
    flow_img_transformed = transform(flow_img)

    return flow_img_transformed, flow['label']
