#ifndef DBC_PEER_NODE_DEF_H
#define DBC_PEER_NODE_DEF_H

enum peer_node_type
{
    NORMAL_NODE = 0,
    SEED_NODE = 1
};

enum get_peers_flag
{
    flag_active,
    flag_global
};

#endif // DBC_PEER_NODE_DEF_H
