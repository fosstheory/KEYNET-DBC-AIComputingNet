#ifndef DBC_DATA_QUERY_SERVICE_H
#define DBC_DATA_QUERY_SERVICE_H

#include <boost/asio.hpp>
#include <string>
#include "service_module/service_module.h"
#include "network/handler_create_functor.h"
#include "data/node_info/service_info_collection.h"
#include "data/node_info/node_info_collection.h"
#include "../message/cmd_message.h"
#include "util/LruCache.hpp"
#include "network/http_client.h"

using namespace boost::asio::ip;

class data_query_service : public service_module, public Singleton<data_query_service>
{
public:
    data_query_service() = default;

    ~data_query_service() override = default;

    int32_t init(bpo::variables_map &options) override;

protected:
    void init_subscription() override;

    void init_invoker() override;

    void init_timer() override;

    void add_self_to_servicelist(bpo::variables_map &options);

    int32_t on_timer_node_info_collection(const std::shared_ptr<core_timer>& timer);


    bool check_req_header(std::shared_ptr<dbc::network::message> &msg);

    bool check_rsp_header(std::shared_ptr<dbc::network::message> &msg);

    bool hit_node(const std::vector<std::string>& peer_node_list, const std::string& node_id);

    bool check_nonce(const std::string& nonce);


    std::shared_ptr<dbc::network::message> create_service_broadcast_req_msg(const service_info_map& mp);

    int32_t on_timer_service_broadcast(const std::shared_ptr<core_timer>& timer);

    int32_t on_get_task_queue_size_resp(std::shared_ptr<dbc::network::message> &msg);

    // list node
    std::shared_ptr<dbc::network::message> create_node_query_node_info_req_msg(const std::shared_ptr<::cmd_list_node_req> &cmd_req);

    int32_t on_cmd_query_node_info_req(std::shared_ptr<dbc::network::message> &msg);

    int32_t on_node_query_node_info_rsp(std::shared_ptr<dbc::network::message> &msg);

    int32_t on_node_query_node_info_timer(const std::shared_ptr<core_timer>& timer);


    int32_t on_node_query_node_info_req(std::shared_ptr<dbc::network::message> &msg);

    int32_t query_node_info(const std::shared_ptr<dbc::node_query_node_info_req>& req);


    int32_t on_net_service_broadcast_req(std::shared_ptr<dbc::network::message> &msg);

private:
    bool m_is_computing_node = false;

    service_info_collection m_service_info_collection;
    node_info_collection m_node_info_collection;
    lru::Cache<std::string, int32_t, std::mutex> m_nonceCache{ 1000000, 0 };
};

#endif
