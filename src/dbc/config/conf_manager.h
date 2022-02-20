﻿#ifndef DBC_CONF_MANAGER_H
#define DBC_CONF_MANAGER_H

#include "util/utils.h"
#include "network/compress/matrix_capacity.h"

namespace bpo = boost::program_options;

struct peer_seeds {
    const char * seed;
    uint16_t port;
};

struct ImageServer {
    std::string ip;
    std::string port;
    std::string username;
    std::string passwd;
    std::string image_dir;
    std::string id;

    std::string to_string();
    void from_string(const std::string& str);
};

class ConfManager : public Singleton<ConfManager>
{
public:
    ConfManager();

    virtual ~ConfManager() = default;

    ERRCODE Init();

    std::string GetVersion() const { return m_version; }

    int32_t GetLogLevel() const { return m_log_level; }

    std::string GetNetType() const { return m_net_type; }

    int32_t GetNetFlag() const { return m_net_flag; }

    std::string GetNetListenIp() const { return m_net_listen_ip; }

    int32_t GetNetListenPort() const { return m_net_listen_port; }

    int32_t GetMaxConnectCount() const { return m_max_connect_count; }

    std::string GetHttpListenIp() const { return m_http_listen_ip; }

    int32_t GetHttpListenPort() const { return m_http_listen_port; }

    std::vector<std::string> GetDbcChainDomain() const { return m_dbc_chain_domain; }

    const std::vector<ImageServer>& GetImageServers() const { return m_image_server; }

    const std::vector<std::string> & GetPeers() { return m_peers; }

    const std::vector<std::string> &GetInternalIpSeeds() { return m_internal_ip_seeds; }

    const std::vector<std::string> &GetInternalDnsSeeds() { return m_internal_dns_seeds; }

    std::string GetNodeId() const { return m_node_id; }

    std::string GetNodePrivateKey() const { return m_node_private_key; }

    std::string GetPubKey() const { return m_pub_key; }

    std::string GetPrivKey() const { return m_priv_key; }

    const dbc::network::matrix_capacity& get_proto_capacity() {
        return m_proto_capacity;
    }

    // const
    static const int32_t TIME_SERVICE_INFO_LIST_EXPIRED = 300; // second

    static const int32_t MAX_RECV_SPEED = 0; 

protected:
    ERRCODE ParseConf();

    ERRCODE ParseDat();

    ERRCODE CreateNewNodeId();

    ERRCODE SerializeNodeInfo(const util::machine_node_info &info);

    bool CheckNodeId();

    void CreateCryptoKeypair();

private:
    std::string m_version = "0.3.7.3";
    int32_t m_log_level = 2;
    std::string m_net_type = "mainnet";
    int32_t m_net_flag = 0xF1E1B0F9;
    std::string m_net_listen_ip = "0.0.0.0";
    int32_t m_net_listen_port = 5001;
    int32_t m_max_connect_count = 1024;
    std::string m_http_listen_ip = "127.0.0.1";
    int32_t m_http_listen_port = 5050;
    std::vector<std::string> m_dbc_chain_domain;
    std::vector<ImageServer> m_image_server;

    std::vector<std::string> m_peers;
    std::vector<std::string> m_internal_ip_seeds;
    std::vector<std::string> m_internal_dns_seeds;

    std::string m_node_id;
    std::string m_node_private_key;
    std::string m_pub_key;
    std::string m_priv_key;

    dbc::network::matrix_capacity m_proto_capacity;
};

#endif
