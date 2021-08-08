#ifndef DBC_NODE_REQUEST_SERVICE_H
#define DBC_NODE_REQUEST_SERVICE_H

#include "util/utils.h"
#include <leveldb/db.h>
#include "service_module/service_module.h"
#include "document.h"
#include <boost/process.hpp>
#include "data/task/vm/image_manager.h"
#include "data/task/TaskManager.h"
#include "util/LruCache.hpp"
#include "util/websocket_client.h"
#include "../message/matrix_types.h"
#include "data/session_id/sessionid_db.h"

#define AI_TRAINING_TASK_TIMER                                   "training_task"
//#define AI_TRAINING_TASK_TIMER_INTERVAL                        (30 * 1000)                                                 //30s timer
#define AI_PRUNE_TASK_TIMER                                      "prune_task"
#define AI_PRUNE_TASK_TIMER_INTERVAL                             (10 * 60 * 1000)                                                 //10min timer

namespace bp = boost::process;

class node_request_service : public service_module, public Singleton<node_request_service> {
public:
    node_request_service();

    ~node_request_service() override = default;

protected:
    void init_timer() override;

    void init_invoker() override;

    void init_subscription() override;

    int32_t service_init(bpo::variables_map& options) override;

    int32_t service_exit() override;

    int32_t on_node_create_task_req(std::shared_ptr<dbc::network::message>& msg);

    int32_t on_node_start_task_req(std::shared_ptr<dbc::network::message>& msg);

    int32_t on_node_restart_task_req(std::shared_ptr<dbc::network::message>& msg);

    int32_t on_node_stop_task_req(std::shared_ptr<dbc::network::message>& msg);

    int32_t on_node_reset_task_req(std::shared_ptr<dbc::network::message>& msg);

    int32_t on_node_delete_task_req(std::shared_ptr<dbc::network::message>& msg);

    int32_t on_node_task_logs_req(std::shared_ptr<dbc::network::message>& msg);

    int32_t on_node_list_task_req(std::shared_ptr<dbc::network::message>& msg);

    int32_t task_create(const std::shared_ptr<dbc::node_create_task_req>& req);

    int32_t task_start(const std::shared_ptr<dbc::node_start_task_req>& req);

    int32_t task_stop(const std::shared_ptr<dbc::node_stop_task_req>& req);

    int32_t task_restart(const std::shared_ptr<dbc::node_restart_task_req>& req);

    int32_t task_reset(const std::shared_ptr<dbc::node_reset_task_req>& req);

    int32_t task_delete(const std::shared_ptr<dbc::node_delete_task_req>& req);

    int32_t task_logs(const std::shared_ptr<dbc::node_task_logs_req>& req);

    int32_t task_list(const std::shared_ptr<dbc::node_list_task_req>& req);

    int32_t on_get_task_queue_size_req(std::shared_ptr<dbc::network::message>& msg);

    int32_t on_training_task_timer(const std::shared_ptr<core_timer>& timer);

    int32_t on_prune_task_timer(const std::shared_ptr<core_timer>& timer);

    std::string format_logs(const std::string& raw_logs, uint16_t max_lines);

    int32_t check_sign(const std::string& message, const std::string& sign, const std::string& origin_id,
        const std::string& sign_algo);

    std::string get_task_id(const std::string& server_specification);

private:
    bool check_req_header(std::shared_ptr<dbc::network::message> &msg);

    bool check_rsp_header(std::shared_ptr<dbc::network::message> &msg);

    bool check_nonce(const std::string& nonce);

    bool hit_node(const std::vector<std::string>& peer_node_list, const std::string& node_id);

    void on_ws_msg(int32_t err_code, const std::string& msg);

protected:
    TaskMgr m_task_scheduler;

    uint32_t m_training_task_timer_id = INVALID_TIMER_ID;
    uint32_t m_prune_task_timer_id = INVALID_TIMER_ID;

    lru::Cache<std::string, int32_t, std::mutex> m_nonceCache{ 1000000, 0 };

    SessionIdDB m_sessionid_db;
    std::map<std::string, std::string> m_wallet_sessionid;

    std::shared_ptr<dbc::node_create_task_req> m_create_req;
    websocket_client m_websocket_client;
};

#endif
