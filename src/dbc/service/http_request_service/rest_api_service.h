#ifndef REST_API_SERVICE_H
#define REST_API_SERVICE_H

#include "util/utils.h"
#include "service_module/service_module.h"
#include "network/http_server.h"
#include "timer/timer.h"
#include "../message/cmd_message.h"

#define REST_API_SERVICE_MODULE                             "rest_api_service"
static const std::string REST_API_VERSION = "v1.1";
static const std::string REST_API_URI = "/api/v1";

constexpr int MIN_INIT_HTTP_SERVER_TIME = 5000; //ms
constexpr int MAX_WAIT_HTTP_RESPONSE_TIME = 30000; //ms
constexpr int MAX_SESSION_COUNT = 1024;

struct req_body {
    std::string task_id;
    std::vector<std::string> peer_nodes_list;
    std::string additional;
    std::string sign;
    std::string nonce;
    std::string wallet;
    std::string session_id;
    std::string session_id_sign;
    std::string pub_key;
};

class rest_api_service : public service_module, public dbc::network::http_request_event, public Singleton<rest_api_service> {
public:
    rest_api_service() = default;

    ~rest_api_service() override = default;

    int32_t init(bpo::variables_map &options) override;

    std::string module_name() const override { return REST_API_SERVICE_MODULE; }

    int32_t get_startup_time();

    //////////////////////////////////////////////////////////////////////////
    // /tasks
    void rest_task(const std::shared_ptr<dbc::network::http_request>& httpReq, const std::string &path);

    // create task
    void rest_create_task(const std::shared_ptr<dbc::network::http_request>& httpReq, const std::string &path);

    std::shared_ptr<dbc::network::message> create_node_create_task_req_msg(const std::string &head_session_id,
                                                                           const req_body& body);

    void on_node_create_task_rsp(const std::shared_ptr<dbc::network::http_request_context>& hreq_context,
                                    const std::shared_ptr<dbc::network::message>& rsp_msg);

    void on_node_create_task_timer(const std::shared_ptr<core_timer>& timer);

    // start task
    void rest_start_task(const std::shared_ptr<dbc::network::http_request>& httpReq, const std::string &path);

    std::shared_ptr<dbc::network::message> create_node_start_task_req_msg(const std::string &head_session_id,
                                                                          const req_body& body);

    void on_node_start_task_rsp(const std::shared_ptr<dbc::network::http_request_context>& hreq_context,
                                const std::shared_ptr<dbc::network::message>& rsp_msg);

    void on_node_start_task_timer(const std::shared_ptr<core_timer> &timer);

    // stop task
    void rest_stop_task(const std::shared_ptr<dbc::network::http_request>& httpReq, const std::string &path);

    std::shared_ptr<dbc::network::message> create_node_stop_task_req_msg(const std::string &head_session_id,
                                                                         const req_body& body);

    void on_node_stop_task_rsp(const std::shared_ptr<dbc::network::http_request_context>& hreq_context,
                               const std::shared_ptr<dbc::network::message>& rsp_msg);

    void on_node_stop_task_timer(const std::shared_ptr<core_timer> &timer);

    // restart task
    void rest_restart_task(const std::shared_ptr<dbc::network::http_request>& httpReq, const std::string &path);

    std::shared_ptr<dbc::network::message> create_node_restart_task_req_msg(const std::string &head_session_id,
                                                                            const req_body& body);

    void on_node_restart_task_rsp(const std::shared_ptr<dbc::network::http_request_context>& hreq_context,
                                  const std::shared_ptr<dbc::network::message>& rsp_msg);

    void on_node_restart_task_timer(const std::shared_ptr<core_timer> &timer);

    // reset task
    void rest_reset_task(const std::shared_ptr<dbc::network::http_request>& httpReq, const std::string &path);

    std::shared_ptr<dbc::network::message> create_node_reset_task_req_msg(const std::string &head_session_id,
                                                                          const req_body& body);

    void on_node_reset_task_rsp(const std::shared_ptr<dbc::network::http_request_context>& hreq_context,
                                const std::shared_ptr<dbc::network::message>& rsp_msg);

    void on_node_reset_task_timer(const std::shared_ptr<core_timer> &timer);

    // delete task
    void rest_delete_task(const std::shared_ptr<dbc::network::http_request>& httpReq, const std::string &path);

    std::shared_ptr<dbc::network::message> create_node_delete_task_req_msg(const std::string &head_session_id,
                                                                           const req_body& body);

    void on_node_delete_task_rsp(const std::shared_ptr<dbc::network::http_request_context>& hreq_context,
                                    const std::shared_ptr<dbc::network::message>& rsp_msg);

    void on_node_delete_task_timer(const std::shared_ptr<core_timer> &timer);

    // list task
    void rest_list_task(const std::shared_ptr<dbc::network::http_request>& httpReq, const std::string &path);

    std::shared_ptr<dbc::network::message> create_node_list_task_req_msg(const std::string &head_session_id,
                                                                         const req_body& body);

    void on_node_list_task_rsp(const std::shared_ptr<dbc::network::http_request_context>& hreq_context,
                                  const std::shared_ptr<dbc::network::message>& rsp_msg);

    void on_node_list_task_timer(const std::shared_ptr<core_timer> &timer);

    // modify task
    void rest_modify_task(const std::shared_ptr<dbc::network::http_request>& httpReq, const std::string &path);

    std::shared_ptr<dbc::network::message> create_node_modify_task_req_msg(const std::string &head_session_id,
                                                                           const req_body& body);

    void on_node_modify_task_rsp(const std::shared_ptr<dbc::network::http_request_context>& hreq_context,
                                 const std::shared_ptr<dbc::network::message>& rsp_msg);

    void on_node_modify_task_timer(const std::shared_ptr<core_timer> &timer);

    // task logs
    void rest_task_logs(const std::shared_ptr<dbc::network::http_request>& httpReq, const std::string &path);

    std::shared_ptr<dbc::network::message> create_node_task_logs_req_msg(const std::string &head_session_id,
                                                                         const req_body& body);

    void on_node_task_logs_rsp(const std::shared_ptr<dbc::network::http_request_context>& hreq_context,
                                  const std::shared_ptr<dbc::network::message>& rsp_msg);

    void on_node_task_logs_timer(const std::shared_ptr<core_timer> &timer);

    // list mining node
    void rest_mining_nodes(const std::shared_ptr<dbc::network::http_request>& httpReq, const std::string &path);

    std::shared_ptr<dbc::network::message> create_node_query_node_info_req_msg(const std::string &head_session_id,
                                                                               const req_body& body);

    void on_node_query_node_info_rsp(const std::shared_ptr<dbc::network::http_request_context>& hreq_context,
                                        const std::shared_ptr<dbc::network::message>& rsp_msg);

    void on_node_query_node_info_timer(const std::shared_ptr<core_timer>& timer);

    // peers
    void rest_peers(const std::shared_ptr<dbc::network::http_request>& httpReq, const std::string &path);

    void on_cmd_get_peer_nodes_rsp(const std::shared_ptr<dbc::network::http_request_context>& hreq_context,
                                   const std::shared_ptr<dbc::network::message>& rsp_msg);

    // /stat/
    void rest_stat(const std::shared_ptr<dbc::network::http_request>& httpReq, const std::string &path);

    // /config/
    void rest_config(const std::shared_ptr<dbc::network::http_request>& httpReq, const std::string &path);

    // /
    void rest_api_version(const std::shared_ptr<dbc::network::http_request>& httpReq, const std::string &path);

    void on_binary_forward(const std::shared_ptr<dbc::network::message> &msg);

protected:
    void init_timer() override;

    void init_invoker() override;

    void init_subscription() override;

    void on_http_request_event(std::shared_ptr<dbc::network::http_request> &hreq) override;

    bool check_invalid_http_request(std::shared_ptr<dbc::network::http_request> &hreq);

    int32_t create_request_session(const std::string& timer_id, const std::shared_ptr<dbc::network::http_request>& hreq,
                                   const std::shared_ptr<dbc::network::message>& req_msg, const std::string& session_id);

    void on_call_rsp_handler(const std::shared_ptr<dbc::network::message> &msg);

    bool check_rsp_header(const std::shared_ptr<dbc::network::message> &rsp_msg);

    bool check_nonce(const std::string& nonce);

private:
    std::vector<dbc::network::http_path_handler> m_path_handlers;
    std::map<std::string, dbc::network::response_call_handler> m_rsp_handlers;
    lru::Cache<std::string, int32_t, std::mutex> m_nonceCache{ 1000000, 0 };
};

#endif
