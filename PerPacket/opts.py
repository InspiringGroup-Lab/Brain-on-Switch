def model_opts(parser):
    parser.add_argument("--tree_num", type=int, default=2)
    parser.add_argument("--tree_depth", type=int, default=9)
    parser.add_argument("--window_size", type=int, default=8)