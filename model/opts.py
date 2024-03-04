def model_opts(parser):
    # Feature embedding
    parser.add_argument("--len_vocab", type=int, default=1501)
    parser.add_argument("--len_embedding_bits", type=int, default=10)
    parser.add_argument("--ipd_vocab", type=int, default=2561)
    parser.add_argument("--ipd_embedding_bits", type=int, default=8)
    parser.add_argument("--embedding_vector_bits", type=int, default=6)
    # RNN cell
    parser.add_argument("--window_size", type=int, default=8)
    parser.add_argument("--rnn_in_pkts", type=int, default=1)
    parser.add_argument("--rnn_hidden_bits", type=int, default=9,
                        help="ISCXVPN2016: 9, BOTIOT: 8, CICIOT2022: 6, PeerRush: 5")
    # Output layer
    parser.add_argument("--labels_num", type=int, default=None)

    # Loss
    parser.add_argument("--loss_factor", type=float, default=0.8,
                        help="ISCXVPN2016: 0.8, BOTIOT: 0.5, CICIOT2022: 3, PeerRush: 1")
    parser.add_argument("--focal_loss_gamma", type=float, default=0,
                        help="ISCXVPN2016: 0, BOTIOT: 0.5, CICIOT2022: 1, PeerRush: 0")
    parser.add_argument("--loss_type", default="all", choices=["single", "all"],
                        help="ISCXVPN2016: all, BOTIOT: all, CICIOT2022: single, PeerRush: all")


def training_opts(parser):
    # Steps
    parser.add_argument("--total_epochs", type=int, default=10000)
    parser.add_argument("--save_checkpoint_epochs", type=int, default=5)
    
    # Gpu
    parser.add_argument("--gpu_id", type=int, default=0)
    
    # Optimizer and scheduler
    parser.add_argument("--optimizer", default="adamw", choices=["adam", "adamw", "adafactor"])
    parser.add_argument("--learning_rate", type=float, default=5e-3,
                        help='ISCXVPN2016: 1e-2, BOTIOT: 5e-3, CICIOT2022: 5e-3, PeerRush: 5e-3')
    
    # Others
    parser.add_argument("--batch_size", type=int, default=32)
    parser.add_argument("--seed", type=int, default=7, 
                        help="Random seed.")


def aggregator_opts(parser):
    parser.add_argument("--quantization_num", type=int, default=16)
    parser.add_argument("--reset_cycle", type=int, default=128)


def simulation_opts(parser):
    parser.add_argument("--simulation_time_predefined_s", type=float, default=1)
    parser.add_argument("--flow_speed_up_times", type=int, default=8,
                        help="speed up the flow duration")
    parser.add_argument("--flows_per_second", type=int, default=160000,
                        help="network load")
    
    parser.add_argument('--reset_time_ms', type=float, default=256,
                        help="refresh stated memory for a flow on switch")
    parser.add_argument('--fall_back_to_imis', type=float, default=0.0,
                        help="the capacity of imis when two flows collided")
    
    parser.add_argument("--per_packet_tree_num", type=int, default=2)
    parser.add_argument("--per_packet_tree_depth", type=int, default=9)