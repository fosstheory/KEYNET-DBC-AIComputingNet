#include "rest_api_service.h"
#include <boost/exception/all.hpp>
#include <boost/xpressive/xpressive_dynamic.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/format.hpp>
#include "rapidjson/rapidjson.h"
#include "rapidjson/error/en.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"
#include "rapidjson/document.h"
#include "rapidjson/error/error.h"
#include "log/log.h"
#include "service_module/service_message_id.h"
#include "timer/time_tick_notification.h"
#include "../message/protocol_coder/matrix_coder.h"
#include "../message/message_id.h"
#include "data/service_info/service_info_collection.h"
#include "../peer_request_service/p2p_net_service.h"
#include "data/resource/system_info.h"

#define HTTP_REQUEST_KEY             "hreq_context"

int32_t rest_api_service::init() {
    service_module::init();

    const dbc::network::http_path_handler uri_prefixes[] = {
            {"/tasks", false, std::bind(&rest_api_service::rest_task, this, std::placeholders::_1, std::placeholders::_2)},
            {"/mining_nodes", false, std::bind(&rest_api_service::rest_mining_nodes, this, std::placeholders::_1, std::placeholders::_2)},
            {"/peers",        false, std::bind(&rest_api_service::rest_peers, this, std::placeholders::_1, std::placeholders::_2)},
            {"/stat",         false, std::bind(&rest_api_service::rest_stat, this, std::placeholders::_1, std::placeholders::_2)},
            {"/config",       false, std::bind(&rest_api_service::rest_config, this, std::placeholders::_1, std::placeholders::_2)},
            {"",              true,  std::bind(&rest_api_service::rest_api_version, this, std::placeholders::_1, std::placeholders::_2)},
    };

    for (const auto &uri_prefixe : uri_prefixes) {
        m_path_handlers.emplace_back(REST_API_URI + uri_prefixe.m_prefix, uri_prefixe.m_exact_match, uri_prefixe.m_handler);
    }

    const dbc::network::response_msg_handler rsp_handlers[] = {
            {     NODE_CREATE_TASK_RSP, std::bind(&rest_api_service::on_node_create_task_rsp, this, std::placeholders::_1, std::placeholders::_2) },
            {      NODE_START_TASK_RSP, std::bind(&rest_api_service::on_node_start_task_rsp, this, std::placeholders::_1, std::placeholders::_2) },
            {       NODE_STOP_TASK_RSP, std::bind(&rest_api_service::on_node_stop_task_rsp, this, std::placeholders::_1, std::placeholders::_2) },
            {    NODE_RESTART_TASK_RSP, std::bind(&rest_api_service::on_node_restart_task_rsp, this, std::placeholders::_1, std::placeholders::_2) },
            {      NODE_RESET_TASK_RSP, std::bind(&rest_api_service::on_node_reset_task_rsp, this, std::placeholders::_1, std::placeholders::_2) },
            {     NODE_DELETE_TASK_RSP, std::bind(&rest_api_service::on_node_delete_task_rsp, this, std::placeholders::_1, std::placeholders::_2) },
            {       NODE_TASK_LOGS_RSP, std::bind(&rest_api_service::on_node_task_logs_rsp, this, std::placeholders::_1, std::placeholders::_2) },
            {       NODE_LIST_TASK_RSP, std::bind(&rest_api_service::on_node_list_task_rsp, this, std::placeholders::_1, std::placeholders::_2) },
            {     NODE_MODIFY_TASK_RSP, std::bind(&rest_api_service::on_node_modify_task_rsp, this, std::placeholders::_1, std::placeholders::_2) },
            { NODE_QUERY_NODE_INFO_RSP, std::bind(&rest_api_service::on_node_query_node_info_rsp, this, std::placeholders::_1, std::placeholders::_2) },
            {      NODE_SESSION_ID_RSP, std::bind(&rest_api_service::on_node_session_id_rsp, this, std::placeholders::_1, std::placeholders::_2) }
    };

    for (const auto &rsp_handler : rsp_handlers) {
        std::string name = rsp_handler.name;
        m_rsp_handlers[name] = rsp_handler.handler;
    }

    return E_SUCCESS;
}

void rest_api_service::init_timer() {
    m_timer_invokers[NODE_CREATE_TASK_TIMER] = std::bind(&rest_api_service::on_node_create_task_timer, this, std::placeholders::_1);
    m_timer_invokers[NODE_START_TASK_TIMER] = std::bind(&rest_api_service::on_node_start_task_timer, this, std::placeholders::_1);
    m_timer_invokers[NODE_STOP_TASK_TIMER] = std::bind(&rest_api_service::on_node_stop_task_timer, this, std::placeholders::_1);
    m_timer_invokers[NODE_RESTART_TASK_TIMER] = std::bind(&rest_api_service::on_node_restart_task_timer, this, std::placeholders::_1);
    m_timer_invokers[NODE_RESET_TASK_TIMER] = std::bind(&rest_api_service::on_node_reset_task_timer, this, std::placeholders::_1);
    m_timer_invokers[NODE_DELETE_TASK_TIMER] = std::bind(&rest_api_service::on_node_delete_task_timer, this, std::placeholders::_1);
    m_timer_invokers[NODE_TASK_LOGS_TIMER] = std::bind(&rest_api_service::on_node_task_logs_timer, this, std::placeholders::_1);
    m_timer_invokers[NODE_LIST_TASK_TIMER] = std::bind(&rest_api_service::on_node_list_task_timer, this, std::placeholders::_1);
    m_timer_invokers[NODE_MODIFY_TASK_TIMER] = std::bind(&rest_api_service::on_node_modify_task_timer, this, std::placeholders::_1);
    m_timer_invokers[NODE_QUERY_NODE_INFO_TIMER] = std::bind(&rest_api_service::on_node_query_node_info_timer, this, std::placeholders::_1);
    m_timer_invokers[NODE_SESSION_ID_TIMER] = std::bind(&rest_api_service::on_node_session_id_timer, this, std::placeholders::_1);
}

void rest_api_service::init_invoker() {
    m_invokers[NODE_CREATE_TASK_RSP] = std::bind(&rest_api_service::on_call_rsp_handler, this, std::placeholders::_1);
    m_invokers[NODE_START_TASK_RSP] = std::bind(&rest_api_service::on_call_rsp_handler, this, std::placeholders::_1);
    m_invokers[NODE_STOP_TASK_RSP] = std::bind(&rest_api_service::on_call_rsp_handler, this, std::placeholders::_1);
    m_invokers[NODE_RESTART_TASK_RSP] = std::bind(&rest_api_service::on_call_rsp_handler, this, std::placeholders::_1);
    m_invokers[NODE_RESET_TASK_RSP] = std::bind(&rest_api_service::on_call_rsp_handler, this, std::placeholders::_1);
    m_invokers[NODE_DELETE_TASK_RSP] = std::bind(&rest_api_service::on_call_rsp_handler, this, std::placeholders::_1);
    m_invokers[NODE_TASK_LOGS_RSP] = std::bind(&rest_api_service::on_call_rsp_handler, this, std::placeholders::_1);
    m_invokers[NODE_LIST_TASK_RSP] = std::bind(&rest_api_service::on_call_rsp_handler, this, std::placeholders::_1);
    m_invokers[NODE_MODIFY_TASK_RSP] = std::bind(&rest_api_service::on_call_rsp_handler, this, std::placeholders::_1);
    m_invokers[NODE_QUERY_NODE_INFO_RSP] = std::bind(&rest_api_service::on_call_rsp_handler, this, std::placeholders::_1);
    m_invokers[NODE_SESSION_ID_RSP] = std::bind(&rest_api_service::on_call_rsp_handler, this, std::placeholders::_1);
    //m_invokers[BINARY_FORWARD_MSG] = std::bind(&rest_api_service::on_binary_forward, this, std::placeholders::_1);
}

void rest_api_service::init_subscription() {
    SUBSCRIBE_BUS_MESSAGE(NODE_CREATE_TASK_RSP)
    SUBSCRIBE_BUS_MESSAGE(NODE_START_TASK_RSP)
    SUBSCRIBE_BUS_MESSAGE(NODE_STOP_TASK_RSP)
    SUBSCRIBE_BUS_MESSAGE(NODE_RESTART_TASK_RSP)
    SUBSCRIBE_BUS_MESSAGE(NODE_RESET_TASK_RSP)
    SUBSCRIBE_BUS_MESSAGE(NODE_DELETE_TASK_RSP)
    SUBSCRIBE_BUS_MESSAGE(NODE_TASK_LOGS_RSP)
    SUBSCRIBE_BUS_MESSAGE(NODE_LIST_TASK_RSP)
    SUBSCRIBE_BUS_MESSAGE(NODE_MODIFY_TASK_RSP)
    SUBSCRIBE_BUS_MESSAGE(NODE_QUERY_NODE_INFO_RSP)
    SUBSCRIBE_BUS_MESSAGE(NODE_SESSION_ID_RSP)
    //SUBSCRIBE_BUS_MESSAGE(BINARY_FORWARD_MSG)
}

void rest_api_service::on_http_request_event(std::shared_ptr<dbc::network::http_request> &hreq) {
    std::string str_uri = hreq->get_uri();
    if (str_uri.substr(0, REST_API_URI.size()) != REST_API_URI) {
        LOG_ERROR << "request uri is invalid, uri:" << str_uri;
        hreq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_REQUEST, "request uri is invalid, uri:" + str_uri);
        return;
    }

    std::string path;
    std::vector<dbc::network::http_path_handler>::const_iterator i = m_path_handlers.begin();
    std::vector<dbc::network::http_path_handler>::const_iterator iend = m_path_handlers.end();
    for (; i != iend; ++i) {
        bool match = false;
        if (i->m_exact_match) {
            match = (str_uri == i->m_prefix);
        } else {
            match = (str_uri.substr(0, i->m_prefix.size()) == i->m_prefix);
        }

        if (match) {
            path = str_uri.substr(i->m_prefix.size());
            break;
        }
    }

    if (i == iend) {
        LOG_ERROR << "not support uri:" << str_uri;
        hreq->reply_comm_rest_err(HTTP_NOTFOUND, RPC_METHOD_NOT_FOUND, "not support uri:" + str_uri);
    } else {
        i->m_handler(hreq, path);
    }
}

bool rest_api_service::check_rsp_header(const std::shared_ptr<dbc::network::message> &rsp_msg) {
    if (!rsp_msg) {
        LOG_ERROR << "msg is nullptr";
        return false;
    }

    std::shared_ptr<dbc::network::msg_base> base = rsp_msg->content;
    if (!base) {
        LOG_ERROR << "msg.containt is nullptr";
        return false;
    }

    if (base->header.nonce.empty()) {
        LOG_ERROR << "header.nonce is empty";
        return false;
    }

    if (!util::check_id(base->header.nonce)) {
        LOG_ERROR << "header.nonce check_id failed";
        return false;
    }

    if (m_nonceCache.contains(base->header.nonce)) {
        LOG_ERROR << "header.nonce is already used";
        return false;
    }
    else {
        m_nonceCache.insert(base->header.nonce, 1);
    }

    if (base->header.session_id.empty()) {
        LOG_ERROR << "base->header.session_id is empty";
        return false;
    }

    if (!util::check_id(base->header.session_id)) {
        LOG_ERROR << "header.session_id check failed";
        return false;
    }

    if (base->header.exten_info.size() < 2) {
        LOG_ERROR << "header.exten_info size < 2";
        return false;
    }

    if (base->header.exten_info.count("pub_key") <= 0) {
        LOG_ERROR << "header.extern_info has no pub_Key";
        return false;
    }

    return true;
}

void rest_api_service::on_call_rsp_handler(const std::shared_ptr<dbc::network::message> &rsp_msg) {
    if (rsp_msg == nullptr) {
        LOG_ERROR << "rsp_msg is nullptr";
        return;
    }

    auto node_rsp_msg = rsp_msg->content;
    if (!node_rsp_msg) {
        LOG_ERROR << "node_rsp_msg is nullptr";
        return;
    }

    const std::string &name = rsp_msg->get_name();
    const std::string &session_id = node_rsp_msg->header.session_id;

    std::shared_ptr<service_session> session = get_session(session_id);
    if (nullptr == session) {
        LOG_DEBUG << "rsp name: " << name << ",but cannot find  session_id:" << session_id;
        dbc::network::connection_manager::instance().send_resp_message(rsp_msg);
        return;
    }

    try {
        variables_map &vm = session->get_context().get_args();
        if (0 == vm.count(HTTP_REQUEST_KEY)) {
            LOG_ERROR << "rsp name: " << name << ",session_id:" << session_id << ",but get null hreq_key";
            return;
        }

        auto hreq_context = vm[HTTP_REQUEST_KEY].as<std::shared_ptr<dbc::network::http_request_context>>();
        if (nullptr == hreq_context) {
            LOG_ERROR << "rsp name: " << name << ",session_id:" << session_id << ",but get null hreq_context";
            return;
        }

        if (!check_rsp_header(rsp_msg)) {
            LOG_ERROR << "rsp header check failed";
            return;
        }

        std::string sign_msg = node_rsp_msg->header.nonce + node_rsp_msg->header.session_id;
        if (!util::verify_sign(node_rsp_msg->header.exten_info["sign"], sign_msg, hreq_context->peer_node_id)) {
            LOG_ERROR << "verify sign failed peer_node_id:" << hreq_context->peer_node_id;
            return;
        }

        auto it = m_rsp_handlers.find(name);
        if (it == m_rsp_handlers.end()) {
            LOG_ERROR << "rsp name: " << name << ",session_id:" << session_id << ",but get null rsp_handler";
            return;
        }

        LOG_INFO << "rsp name: " << name << ",session_id:" << session_id << ", and call rsp_handler";

        auto func = it->second;
        func(hreq_context, rsp_msg);

        remove_timer(session->get_timer_id());
        session->clear();
        remove_session(session_id);
    } catch (std::exception &e) {
        LOG_ERROR << e.what();
    }
}

int32_t rest_api_service::create_request_session(const std::string& timer_id,
                                              const std::shared_ptr<dbc::network::http_request>& hreq,
                                              const std::shared_ptr<dbc::network::message>& req_msg,
                                              const std::string& session_id, const std::string& peer_node_id) {
    auto hreq_context = std::make_shared<dbc::network::http_request_context>();
    hreq_context->m_hreq = hreq;
    hreq_context->m_req_msg = req_msg;
    hreq_context->peer_node_id = peer_node_id;

    do {
        std::string str_uri = hreq->get_uri();

        if (get_session_count() >= MAX_SESSION_COUNT) {
            LOG_ERROR << "session pool is full, uri:" << str_uri;
            return E_DEFAULT;
        }

        uint32_t id = add_timer(timer_id, MAX_WAIT_HTTP_RESPONSE_TIME, ONLY_ONE_TIME, session_id);
        if (INVALID_TIMER_ID == id) {
            LOG_ERROR << "add_timer failed, uri:" << str_uri;
            return E_DEFAULT;
        }

        std::shared_ptr<service_session> session = std::make_shared<service_session>(id, session_id);
        variable_value val;
        val.value() = hreq_context;
        session->get_context().add(HTTP_REQUEST_KEY, val);

        int32_t ret = this->add_session(session_id, session);
        if (E_SUCCESS != ret) {
            remove_timer(id);
            LOG_ERROR << "add_session failed, uri:" << str_uri;
            return E_DEFAULT;
        }
    } while (0);

    return E_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////

// /tasks
void rest_api_service::rest_task(const std::shared_ptr<dbc::network::http_request>& httpReq, const std::string &path) {
    std::vector<std::string> path_list;
    util::split_path(path, path_list);

    if (path_list.empty()) {
        rest_list_task(httpReq, path);
        return;
    }

    if (path_list.size() == 1) {
        const std::string &first_param = path_list[0];
        // create
        if (first_param == "start") {
            rest_create_task(httpReq, path);
        } else {
            rest_list_task(httpReq, path);
        }
        return;
    }

    if (path_list.size() == 2) {
        const std::string &second_param = path_list[1];
        // start <task_id>
        if (second_param == "start") {
            rest_start_task(httpReq, path);
        }
        // /tasks/<task_id>/restart
        else if (second_param == "restart") {
            rest_restart_task(httpReq, path);
        }
        // /tasks/<task_id>/stop 停止某个task
        else if (second_param == "stop") {
            rest_stop_task(httpReq, path);
        }
        // /tasks/<task_id>/reset
        else if (second_param == "reset") {
            rest_reset_task(httpReq, path);
        }
        // /tasks/<task_id>/destroy
        else if (second_param == "delete") {
            rest_delete_task(httpReq, path);
        }
        // /tasks/<task_id>/logs
        else if (second_param == "logs") {
            rest_task_logs(httpReq, path);
        }
        // /tasks/<task_id>/modify
        else if (second_param == "modify") {
            rest_modify_task(httpReq, path);
        }
        return;
    }

    httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_REQUEST, "invalid requests uri");
}

// create task
void rest_api_service::rest_create_task(const std::shared_ptr<dbc::network::http_request>& httpReq, const std::string &path) {
    if (httpReq->get_request_method() != dbc::network::http_request::POST) {
        LOG_ERROR << "http request is not post";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_REQUEST, "only support POST request");
        return;
    }

    std::vector<std::string> path_list;
    util::split_path(path, path_list);
    if (path_list.size() != 1) {
        LOG_ERROR << "path_list's size != 1";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "invalid uri, please use: /tasks/start");
        return;
    }

    req_body body;

    std::string s_body = httpReq->read_body();
    if (s_body.empty()) {
        LOG_ERROR << "http request body is empty";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "http request body is empty");
        return;
    }

    rapidjson::Document doc;
    rapidjson::ParseResult ok = doc.Parse(s_body.c_str());
    if (!ok) {
        std::stringstream ss;
        ss << "json parse error: " << rapidjson::GetParseError_En(ok.Code()) << "(" << ok.Offset() << ")";
        LOG_ERROR << ss.str();
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, ss.str());
        return;
    }

    if (!doc.IsObject()) {
        LOG_ERROR << "invalid json";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "invalid json");
        return;
    }

    // peer_nodes_list
    std::string peer_node_id;
    if (doc.HasMember("peer_nodes_list")) {
        if (doc["peer_nodes_list"].IsArray()) {
            uint32_t list_size = doc["peer_nodes_list"].Size();
            if (list_size < 1) {
                LOG_ERROR << "peer_nodes_list's size < 1";
                httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "peer_nodes_list's size < 1");
                return;
            }

            // 暂时只支持一次操作1个节点
            std::string node(doc["peer_nodes_list"][0].GetString());
            body.peer_nodes_list.push_back(node);
            peer_node_id = node;
        } else {
            LOG_ERROR << "peer_nodes_list is not array";
            httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "peer_nodes_list is not array");
            return;
        }
    } else {
        LOG_ERROR << "has no peer_nodes_list";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "has no peer_nodes_list");
        return;
    }
    // additional
    if (doc.HasMember("additional")) {
        if (doc["additional"].IsObject()) {
            const rapidjson::Value &obj = doc["additional"];
            rapidjson::StringBuffer buffer;
            rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
            obj.Accept(writer);
            body.additional = buffer.GetString();
        } else {
            LOG_ERROR << "additional is not object";
            httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "additional is not object");
            return;
        }
    } else {
        LOG_ERROR << "has no additional";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "has no additional");
        return;
    }
    // sign
    if (doc.HasMember("sign")) {
        if (doc["sign"].IsString()) {
            body.sign = doc["sign"].GetString();
        } else {
            LOG_ERROR << "sign is not string";
            httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "sign is not string");
            return;
        }
    } else {
        LOG_ERROR << "has no sign";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "has no sign");
        return;
    }
    // nonce
    if (doc.HasMember("nonce")) {
        if (doc["nonce"].IsString()) {
            body.nonce = doc["nonce"].GetString();
        } else {
            LOG_ERROR << "nonce is not string";
            httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "nonce is not string");
            return;
        }
    } else {
        LOG_ERROR << "has no nonce";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "has no nonce");
        return;
    }
    // wallet
    if (doc.HasMember("wallet")) {
        if (doc["wallet"].IsString()) {
            body.wallet = doc["wallet"].GetString();
        } else {
            LOG_ERROR << "wallet is not string";
            httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "wallet is not string");
            return;
        }
    } else {
        LOG_ERROR << "has no wallet";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "has no wallet");
        return;
    }
    // session_id
    if (doc.HasMember("session_id")) {
        if (doc["session_id"].IsString()) {
            body.session_id = doc["session_id"].GetString();
        } else {
            LOG_ERROR << "session_id is not string";
            httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "session_id is not string");
            return;
        }
    }
    // session_id_sign
    if (doc.HasMember("session_id_sign")) {
        if (doc["session_id_sign"].IsString()) {
            body.session_id_sign = doc["session_id_sign"].GetString();
        } else {
            LOG_ERROR << "session_id_sign is not string";
            httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "session_id_sign is not string");
            return;
        }
    }
    // pub_key
    body.pub_key = conf_manager::instance().get_pub_key();
    if (body.pub_key.empty()) {
        LOG_ERROR << "pub_key is empty";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "pub_key is empty");
        return;
    }

    std::string head_session_id = util::create_session_id();

    auto node_req_msg = create_node_create_task_req_msg(head_session_id, body);
    if (nullptr == node_req_msg) {
        LOG_ERROR << "creaate node request failed";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_RESPONSE_ERROR, "creaate node request failed");
        return;
    }

    if (E_SUCCESS != create_request_session(NODE_CREATE_TASK_TIMER, httpReq, node_req_msg, head_session_id, peer_node_id)) {
        LOG_ERROR << "create request session failed";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_RESPONSE_ERROR, "creaate request session failed");
        return;
    }

    if (dbc::network::connection_manager::instance().broadcast_message(node_req_msg) != E_SUCCESS) {
        LOG_ERROR << "broadcast request failed";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_RESPONSE_ERROR, "broadcast request failed");
        return;
    }
}

std::shared_ptr<dbc::network::message> rest_api_service::create_node_create_task_req_msg(const std::string &head_session_id, const req_body& body) {
    auto req_content = std::make_shared<dbc::node_create_task_req>();
    // header
    req_content->header.__set_magic(conf_manager::instance().get_net_flag());
    req_content->header.__set_msg_name(NODE_CREATE_TASK_REQ);
    req_content->header.__set_nonce(util::create_nonce());
    req_content->header.__set_session_id(head_session_id);
    std::map<std::string, std::string> exten_info;
    exten_info["pub_key"] = body.pub_key;
    exten_info["sign"] = body.sign;
    exten_info["nonce"] = body.nonce;
    req_content->header.__set_exten_info(exten_info);
    std::vector<std::string> path;
    path.push_back(conf_manager::instance().get_node_id());
    req_content->header.__set_path(path);

    // body
    dbc::node_create_task_req_data req_data;
    req_data.__set_peer_nodes_list(body.peer_nodes_list);
    req_data.__set_additional(body.additional);
    req_data.__set_wallet(body.wallet);
    req_data.__set_session_id(body.session_id);
    req_data.__set_session_id_sign(body.session_id_sign);

    // encrypt
    std::shared_ptr<byte_buf> out_buf = std::make_shared<byte_buf>();
    dbc::network::binary_protocol proto(out_buf.get());
    req_data.write(&proto);

    dbc::node_service_info service_info;
    bool bfound = service_info_collection::instance().find(body.peer_nodes_list[0], service_info);
    if (bfound) {
        std::string pub_key = service_info.kvs.count("pub_key") ? service_info.kvs["pub_key"] : "";
        std::string priv_key = conf_manager::instance().get_priv_key();

        if (!pub_key.empty() && !priv_key.empty()) {
            std::string s_data = encrypt_data((unsigned char*) out_buf->get_read_ptr(), out_buf->get_valid_read_len(), pub_key, priv_key);
            req_content->body.__set_data(s_data);
        } else {
            LOG_ERROR << "pub_key is empty, node_id:" << body.peer_nodes_list[0];
            return nullptr;
        }
    } else {
        LOG_ERROR << "service_info_collection not found node_id:" << body.peer_nodes_list[0];
        return nullptr;
    }

    std::shared_ptr<dbc::network::message> req_msg = std::make_shared<dbc::network::message>();
    req_msg->set_name(NODE_CREATE_TASK_REQ);
    req_msg->set_content(req_content);

    return req_msg;
}

void rest_api_service::on_node_create_task_rsp(const std::shared_ptr<dbc::network::http_request_context>& hreq_context,
                                                  const std::shared_ptr<dbc::network::message>& rsp_msg) {
    auto node_rsp_msg = std::dynamic_pointer_cast<dbc::node_create_task_rsp>(rsp_msg->content);
    if (!node_rsp_msg) {
        LOG_ERROR << "node_rsp_msg is nullptr";
        return;
    }

    const std::shared_ptr<dbc::network::http_request> &httpReq = hreq_context->m_hreq;

    // decrypt
    std::string pub_key = node_rsp_msg->header.exten_info["pub_key"];
    std::string priv_key = conf_manager::instance().get_priv_key();
    if (pub_key.empty() || priv_key.empty()) {
        httpReq->reply_comm_rest_err(HTTP_INTERNAL, -1, "pub_key or priv_key is empty");
        LOG_ERROR << "pub_key or priv_key is empty";
        return;
    }

    std::string ori_message;
    try {
        bool succ = decrypt_data(node_rsp_msg->body.data, pub_key, priv_key, ori_message);
        if (!succ || ori_message.empty()) {
            httpReq->reply_comm_rest_err(HTTP_INTERNAL, -1, "rsq decrypt error1");
            LOG_ERROR << "rsq decrypt error1";
            return;
        }
    } catch (std::exception &e) {
        httpReq->reply_comm_rest_err(HTTP_INTERNAL, -1, "req decrypt error2");
        LOG_ERROR << "req decrypt error2";
        return;
    }

    rapidjson::Document doc;
    rapidjson::ParseResult ok = doc.Parse(ori_message.c_str());
    if (!ok) {
        httpReq->reply_comm_rest_err(HTTP_INTERNAL, -1, "response parse error");
        LOG_ERROR << "response parse error: " << rapidjson::GetParseError_En(ok.Code()) << "(" << ok.Offset() << ")";
        return;
    }

    httpReq->reply_comm_rest_succ2(ori_message);
}

void rest_api_service::on_node_create_task_timer(const std::shared_ptr<core_timer>& timer) {
    if (nullptr == timer) {
        LOG_ERROR << "timer is nullptr";
        return;
    }

    const std::string &session_id = timer->get_session_id();
    std::shared_ptr<service_session> session = get_session(session_id);
    if (nullptr == session) {
        LOG_ERROR << "session is nullptr";
        return;
    }

    variables_map &vm = session->get_context().get_args();
    if (0 == vm.count(HTTP_REQUEST_KEY)) {
        LOG_ERROR << "session's context has no HTTP_REQUEST_KEY";
        session->clear();
        this->remove_session(session_id);
        return;
    }

    auto hreq_context = vm[HTTP_REQUEST_KEY].as<std::shared_ptr<dbc::network::http_request_context>>();
    if (nullptr != hreq_context) {
        hreq_context->m_hreq->reply_comm_rest_err(HTTP_INTERNAL, -1, "create task timeout");
    }

    session->clear();
    this->remove_session(session_id);
}

// start task
void rest_api_service::rest_start_task(const std::shared_ptr<dbc::network::http_request>& httpReq, const std::string &path) {
    if (httpReq->get_request_method() != dbc::network::http_request::POST) {
        LOG_ERROR << "http request is not post";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_REQUEST, "only support POST request");
        return;
    }

    std::vector<std::string> path_list;
    util::split_path(path, path_list);
    if (path_list.size() != 2) {
        LOG_ERROR << "path_list's size != 2";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "invalid uri, please use /tasks/<task_id>/start");
        return;
    }

    req_body body;

    body.task_id = path_list[0];
    if (body.task_id.empty()) {
        LOG_ERROR << "task_id is empty";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "task_id is empty");
        return;
    }

    std::string s_body = httpReq->read_body();
    if (s_body.empty()) {
        LOG_ERROR << "http request body is empty";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "http request body is empty");
        return;
    }

    rapidjson::Document doc;
    rapidjson::ParseResult ok = doc.Parse(s_body.c_str());
    if (!ok) {
        std::stringstream ss;
        ss << "json parse error: " << rapidjson::GetParseError_En(ok.Code()) << "(" << ok.Offset() << ")";
        LOG_ERROR << ss.str();
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, ss.str());
        return;
    }

    if (!doc.IsObject()) {
        LOG_ERROR << "invalid json";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "invalid json");
        return;
    }

    // peer_nodes_list
    std::string peer_node_id;
    if (doc.HasMember("peer_nodes_list")) {
        if (doc["peer_nodes_list"].IsArray()) {
            uint32_t list_size = doc["peer_nodes_list"].Size();
            if (list_size < 1) {
                LOG_ERROR << "peer_nodes_list's size < 1";
                httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "peer_nodes_list's size < 1");
                return;
            }

            // 暂时只支持一次操作1个节点
            std::string node(doc["peer_nodes_list"][0].GetString());
            body.peer_nodes_list.push_back(node);
            peer_node_id = node;
        } else {
            LOG_ERROR << "peer_nodes_list is not array";
            httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "peer_nodes_list is not array");
            return;
        }
    } else {
        LOG_ERROR << "has no peer_nodes_list";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "has no peer_nodes_list");
        return;
    }
    // additional
    if (doc.HasMember("additional")) {
        if (doc["additional"].IsObject()) {
            const rapidjson::Value &obj = doc["additional"];
            rapidjson::StringBuffer buffer;
            rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
            obj.Accept(writer);
            body.additional = buffer.GetString();
        } else {
            LOG_ERROR << "additional is not object";
            httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "additional is not object");
            return;
        }
    } else {
        LOG_ERROR << "has no additional";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "has no additional");
        return;
    }
    // sign
    if (doc.HasMember("sign")) {
        if (doc["sign"].IsString()) {
            body.sign = doc["sign"].GetString();
        } else {
            LOG_ERROR << "sign is not string";
            httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "sign is not string");
            return;
        }
    } else {
        LOG_ERROR << "has no sign";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "has no sign");
        return;
    }
    // nonce
    if (doc.HasMember("nonce")) {
        if (doc["nonce"].IsString()) {
            body.nonce = doc["nonce"].GetString();
        } else {
            LOG_ERROR << "nonce is not string";
            httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "nonce is not string");
            return;
        }
    } else {
        LOG_ERROR << "has no nonce";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "has no nonce");
        return;
    }
    // wallet
    if (doc.HasMember("wallet")) {
        if (doc["wallet"].IsString()) {
            body.wallet = doc["wallet"].GetString();
        } else {
            LOG_ERROR << "wallet is not string";
            httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "wallet is not string");
            return;
        }
    } else {
        LOG_ERROR << "has no wallet";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "has no wallet");
        return;
    }
    // session_id
    if (doc.HasMember("session_id")) {
        if (doc["session_id"].IsString()) {
            body.session_id = doc["session_id"].GetString();
        } else {
            LOG_ERROR << "session_id is not string";
            httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "session_id is not string");
            return;
        }
    }
    // session_id_sign
    if (doc.HasMember("session_id_sign")) {
        if (doc["session_id_sign"].IsString()) {
            body.session_id_sign = doc["session_id_sign"].GetString();
        } else {
            LOG_ERROR << "session_id_sign is not string";
            httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "session_id_sign is not string");
            return;
        }
    }
    // pub_key
    body.pub_key = conf_manager::instance().get_pub_key();
    if (body.pub_key.empty()) {
        LOG_ERROR << "pub_key is empty";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "pub_key is empty");
        return;
    }

    std::string head_session_id = util::create_session_id();

    auto node_req_msg = create_node_start_task_req_msg(head_session_id, body);
    if (nullptr == node_req_msg) {
        LOG_ERROR << "creaate node request failed";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_RESPONSE_ERROR, "creaate node request failed");
        return;
    }

    if (E_SUCCESS != create_request_session(NODE_START_TASK_TIMER, httpReq, node_req_msg, head_session_id, peer_node_id)) {
        LOG_ERROR << "create request session failed";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_RESPONSE_ERROR, "creaate request session failed");
        return;
    }

    if (dbc::network::connection_manager::instance().broadcast_message(node_req_msg) != E_SUCCESS) {
        LOG_ERROR << "broadcast request failed";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_RESPONSE_ERROR, "broadcast request failed");
        return;
    }
}

std::shared_ptr<dbc::network::message> rest_api_service::create_node_start_task_req_msg(const std::string &head_session_id, const req_body& body) {
    // 创建 node_ 请求
    auto req_content = std::make_shared<dbc::node_start_task_req>();
    // header
    req_content->header.__set_magic(conf_manager::instance().get_net_flag());
    req_content->header.__set_msg_name(NODE_START_TASK_REQ);
    req_content->header.__set_nonce(util::create_nonce());
    req_content->header.__set_session_id(head_session_id);
    std::map<std::string, std::string> exten_info;
    exten_info["pub_key"] = body.pub_key;
    exten_info["sign"] = body.sign;
    exten_info["nonce"] = body.nonce;
    req_content->header.__set_exten_info(exten_info);
    std::vector<std::string> path;
    path.push_back(conf_manager::instance().get_node_id());
    req_content->header.__set_path(path);

    // body
    dbc::node_start_task_req_data req_data;
    req_data.__set_task_id(body.task_id);
    req_data.__set_peer_nodes_list(body.peer_nodes_list);
    req_data.__set_additional(body.additional);
    req_data.__set_wallet(body.wallet);
    req_data.__set_session_id(body.session_id);
    req_data.__set_session_id_sign(body.session_id_sign);

    // encrypt
    std::shared_ptr<byte_buf> out_buf = std::make_shared<byte_buf>();
    dbc::network::binary_protocol proto(out_buf.get());
    req_data.write(&proto);

    dbc::node_service_info service_info;
    bool bfound = service_info_collection::instance().find(body.peer_nodes_list[0], service_info);
    if (bfound) {
        std::string pub_key = service_info.kvs.count("pub_key") ? service_info.kvs["pub_key"] : "";
        std::string priv_key = conf_manager::instance().get_priv_key();

        if (!pub_key.empty() && !priv_key.empty()) {
            std::string s_data = encrypt_data((unsigned char*) out_buf->get_read_ptr(), out_buf->get_valid_read_len(), pub_key, priv_key);
            req_content->body.__set_data(s_data);
        } else {
            LOG_ERROR << "pub_key is empty, node_id:" << body.peer_nodes_list[0];
            return nullptr;
        }
    } else {
        LOG_ERROR << "service_info_collection not found node_id:" << body.peer_nodes_list[0];
        return nullptr;
    }

    std::shared_ptr<dbc::network::message> req_msg = std::make_shared<dbc::network::message>();
    req_msg->set_name(NODE_START_TASK_REQ);
    req_msg->set_content(req_content);

    return req_msg;
}

void rest_api_service::on_node_start_task_rsp(const std::shared_ptr<dbc::network::http_request_context>& hreq_context,
                                              const std::shared_ptr<dbc::network::message>& rsp_msg) {
    auto node_rsp_msg = std::dynamic_pointer_cast<dbc::node_start_task_rsp>(rsp_msg->content);
    if (!node_rsp_msg) {
        LOG_ERROR << "node_rsp_msg is nullptr";
        return;
    }

    const std::shared_ptr<dbc::network::http_request> &httpReq = hreq_context->m_hreq;

    // decrypt
    std::string pub_key = node_rsp_msg->header.exten_info["pub_key"];
    std::string priv_key = conf_manager::instance().get_priv_key();
    if (pub_key.empty() || priv_key.empty()) {
        httpReq->reply_comm_rest_err(HTTP_INTERNAL, -1, "pub_key or priv_key is empty");
        LOG_ERROR << "pub_key or priv_key is empty";
        return;
    }

    std::string ori_message;
    try {
        bool succ = decrypt_data(node_rsp_msg->body.data, pub_key, priv_key, ori_message);
        if (!succ || ori_message.empty()) {
            httpReq->reply_comm_rest_err(HTTP_INTERNAL, -1, "rsq decrypt error1");
            LOG_ERROR << "rsq decrypt error1";
            return;
        }
    } catch (std::exception &e) {
        httpReq->reply_comm_rest_err(HTTP_INTERNAL, -1, "req decrypt error2");
        LOG_ERROR << "req decrypt error2";
        return;
    }

    rapidjson::Document doc;
    rapidjson::ParseResult ok = doc.Parse(ori_message.c_str());
    if (!ok) {
        httpReq->reply_comm_rest_err(HTTP_INTERNAL, -1, "response parse error");
        LOG_ERROR << "response parse error: " << rapidjson::GetParseError_En(ok.Code()) << "(" << ok.Offset() << ")";
        return;
    }

    httpReq->reply_comm_rest_succ2(ori_message);
}

void rest_api_service::on_node_start_task_timer(const std::shared_ptr<core_timer> &timer) {
    if (nullptr == timer) {
        LOG_ERROR << "timer is nullptr";
        return;
    }

    const std::string &session_id = timer->get_session_id();
    std::shared_ptr<service_session> session = get_session(session_id);
    if (nullptr == session) {
        LOG_ERROR << "session is nullptr";
        return;
    }

    variables_map &vm = session->get_context().get_args();
    if (0 == vm.count(HTTP_REQUEST_KEY)) {
        LOG_ERROR << "session's context has no HTTP_REQUEST_KEY";
        session->clear();
        this->remove_session(session_id);
        return;
    }

    auto hreq_context = vm[HTTP_REQUEST_KEY].as<std::shared_ptr<dbc::network::http_request_context>>();
    if (nullptr != hreq_context) {
        hreq_context->m_hreq->reply_comm_rest_err(HTTP_INTERNAL, -1, "start task timeout");
    }

    session->clear();
    this->remove_session(session_id);
}

// stop task
void rest_api_service::rest_stop_task(const std::shared_ptr<dbc::network::http_request>& httpReq, const std::string &path) {
    if (httpReq->get_request_method() != dbc::network::http_request::POST) {
        LOG_ERROR << "http request is not post";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_REQUEST, "only support POST request");
        return;
    }

    std::vector<std::string> path_list;
    util::split_path(path, path_list);
    if (path_list.size() != 2) {
        LOG_ERROR << "path_list's size != 2";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "invalid uri, please use /tasks/<task_id>/stop");
        return;
    }

    req_body body;

    body.task_id = path_list[0];
    if (body.task_id.empty()) {
        LOG_ERROR << "task_id is empty";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "task_id is empty");
        return;
    }

    std::string s_body = httpReq->read_body();
    if (s_body.empty()) {
        LOG_ERROR << "http request body is empty";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "http request body is empty");
        return;
    }

    rapidjson::Document doc;
    rapidjson::ParseResult ok = doc.Parse(s_body.c_str());
    if (!ok) {
        std::stringstream ss;
        ss << "json parse error: " << rapidjson::GetParseError_En(ok.Code()) << "(" << ok.Offset() << ")";
        LOG_ERROR << ss.str();
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, ss.str());
        return;
    }

    if (!doc.IsObject()) {
        LOG_ERROR << "invalid json";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "invalid json");
        return;
    }

    // peer_nodes_list
    std::string peer_node_id;
    if (doc.HasMember("peer_nodes_list")) {
        if (doc["peer_nodes_list"].IsArray()) {
            uint32_t list_size = doc["peer_nodes_list"].Size();
            if (list_size < 1) {
                LOG_ERROR << "peer_nodes_list's size < 1";
                httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "peer_nodes_list's size < 1");
                return;
            }

            // 暂时只支持一次操作1个节点
            std::string node(doc["peer_nodes_list"][0].GetString());
            body.peer_nodes_list.push_back(node);
            peer_node_id = node;
        } else {
            LOG_ERROR << "peer_nodes_list is not array";
            httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "peer_nodes_list is not array");
            return;
        }
    } else {
        LOG_ERROR << "has no peer_nodes_list";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "has no peer_nodes_list");
        return;
    }
    // additional
    if (doc.HasMember("additional")) {
        if (doc["additional"].IsObject()) {
            const rapidjson::Value &obj = doc["additional"];
            rapidjson::StringBuffer buffer;
            rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
            obj.Accept(writer);
            body.additional = buffer.GetString();
        } else {
            LOG_ERROR << "additional is not object";
            httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "additional is not object");
            return;
        }
    } else {
        LOG_ERROR << "has no additional";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "has no additional");
        return;
    }
    // sign
    if (doc.HasMember("sign")) {
        if (doc["sign"].IsString()) {
            body.sign = doc["sign"].GetString();
        } else {
            LOG_ERROR << "sign is not string";
            httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "sign is not string");
            return;
        }
    } else {
        LOG_ERROR << "has no sign";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "has no sign");
        return;
    }
    // nonce
    if (doc.HasMember("nonce")) {
        if (doc["nonce"].IsString()) {
            body.nonce = doc["nonce"].GetString();
        } else {
            LOG_ERROR << "nonce is not string";
            httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "nonce is not string");
            return;
        }
    } else {
        LOG_ERROR << "has no nonce";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "has no nonce");
        return;
    }
    // wallet
    if (doc.HasMember("wallet")) {
        if (doc["wallet"].IsString()) {
            body.wallet = doc["wallet"].GetString();
        } else {
            LOG_ERROR << "wallet is not string";
            httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "wallet is not string");
            return;
        }
    } else {
        LOG_ERROR << "has no wallet";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "has no wallet");
        return;
    }
    // session_id
    if (doc.HasMember("session_id")) {
        if (doc["session_id"].IsString()) {
            body.session_id = doc["session_id"].GetString();
        } else {
            LOG_ERROR << "session_id is not string";
            httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "session_id is not string");
            return;
        }
    }
    // session_id_sign
    if (doc.HasMember("session_id_sign")) {
        if (doc["session_id_sign"].IsString()) {
            body.session_id_sign = doc["session_id_sign"].GetString();
        } else {
            LOG_ERROR << "session_id_sign is not string";
            httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "session_id_sign is not string");
            return;
        }
    }
    // pub_key
    body.pub_key = conf_manager::instance().get_pub_key();
    if (body.pub_key.empty()) {
        LOG_ERROR << "pub_key is empty";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "pub_key is empty");
        return;
    }

    std::string head_session_id = util::create_session_id();

    auto node_req_msg = create_node_stop_task_req_msg(head_session_id, body);
    if (nullptr == node_req_msg) {
        LOG_ERROR << "creaate node request failed";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_RESPONSE_ERROR, "creaate node request failed");
        return;
    }

    if (E_SUCCESS != create_request_session(NODE_STOP_TASK_TIMER, httpReq, node_req_msg, head_session_id, peer_node_id)) {
        LOG_ERROR << "create request session failed";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_RESPONSE_ERROR, "creaate request session failed");
        return;
    }

    if (dbc::network::connection_manager::instance().broadcast_message(node_req_msg) != E_SUCCESS) {
        LOG_ERROR << "broadcast request failed";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_RESPONSE_ERROR, "broadcast request failed");
        return;
    }
}

std::shared_ptr<dbc::network::message> rest_api_service::create_node_stop_task_req_msg(const std::string &head_session_id,
                                                                                       const req_body& body) {
    // 创建 node_ 请求
    std::shared_ptr<dbc::node_stop_task_req> req_content = std::make_shared<dbc::node_stop_task_req>();
    // header
    req_content->header.__set_magic(conf_manager::instance().get_net_flag());
    req_content->header.__set_msg_name(NODE_STOP_TASK_REQ);
    req_content->header.__set_nonce(util::create_nonce());
    req_content->header.__set_session_id(head_session_id);
    std::map<std::string, std::string> exten_info;
    exten_info["pub_key"] = body.pub_key;
    exten_info["sign"] = body.sign;
    exten_info["nonce"] = body.nonce;
    req_content->header.__set_exten_info(exten_info);
    std::vector<std::string> path;
    path.push_back(conf_manager::instance().get_node_id());
    req_content->header.__set_path(path);

    // body
    dbc::node_stop_task_req_data req_data;
    req_data.__set_task_id(body.task_id);
    req_data.__set_peer_nodes_list(body.peer_nodes_list);
    req_data.__set_additional(body.additional);
    req_data.__set_wallet(body.wallet);
    req_data.__set_session_id(body.session_id);
    req_data.__set_session_id_sign(body.session_id_sign);

    // encrypt
    std::shared_ptr<byte_buf> out_buf = std::make_shared<byte_buf>();
    dbc::network::binary_protocol proto(out_buf.get());
    req_data.write(&proto);

    dbc::node_service_info service_info;
    bool bfound = service_info_collection::instance().find(body.peer_nodes_list[0], service_info);
    if (bfound) {
        std::string pub_key = service_info.kvs.count("pub_key") ? service_info.kvs["pub_key"] : "";
        std::string priv_key = conf_manager::instance().get_priv_key();

        if (!pub_key.empty() && !priv_key.empty()) {
            std::string s_data = encrypt_data((unsigned char*) out_buf->get_read_ptr(), out_buf->get_valid_read_len(), pub_key, priv_key);
            req_content->body.__set_data(s_data);
        } else {
            LOG_ERROR << "pub_key is empty, node_id:" << body.peer_nodes_list[0];
            return nullptr;
        }
    } else {
        LOG_ERROR << "service_info_collection not found node_id:" << body.peer_nodes_list[0];
        return nullptr;
    }

    std::shared_ptr<dbc::network::message> req_msg = std::make_shared<dbc::network::message>();
    req_msg->set_name(NODE_STOP_TASK_REQ);
    req_msg->set_content(req_content);

    return req_msg;
}

void rest_api_service::on_node_stop_task_rsp(const std::shared_ptr<dbc::network::http_request_context>& hreq_context,
                                             const std::shared_ptr<dbc::network::message>& rsp_msg) {
    auto node_rsp_msg = std::dynamic_pointer_cast<dbc::node_stop_task_rsp>(rsp_msg->content);
    if (!node_rsp_msg) {
        LOG_ERROR << "node_rsp_msg is nullptr";
        return;
    }

    const std::shared_ptr<dbc::network::http_request> &httpReq = hreq_context->m_hreq;

    // decrypt
    std::string pub_key = node_rsp_msg->header.exten_info["pub_key"];
    std::string priv_key = conf_manager::instance().get_priv_key();
    if (pub_key.empty() || priv_key.empty()) {
        httpReq->reply_comm_rest_err(HTTP_INTERNAL, -1, "pub_key or priv_key is empty");
        LOG_ERROR << "pub_key or priv_key is empty";
        return;
    }

    std::string ori_message;
    try {
        bool succ = decrypt_data(node_rsp_msg->body.data, pub_key, priv_key, ori_message);
        if (!succ || ori_message.empty()) {
            httpReq->reply_comm_rest_err(HTTP_INTERNAL, -1, "rsq decrypt error1");
            LOG_ERROR << "rsq decrypt error1";
            return;
        }
    } catch (std::exception &e) {
        httpReq->reply_comm_rest_err(HTTP_INTERNAL, -1, "req decrypt error2");
        LOG_ERROR << "req decrypt error2";
        return;
    }

    rapidjson::Document doc;
    rapidjson::ParseResult ok = doc.Parse(ori_message.c_str());
    if (!ok) {
        httpReq->reply_comm_rest_err(HTTP_INTERNAL, -1, "response parse error");
        LOG_ERROR << "response parse error: " << rapidjson::GetParseError_En(ok.Code()) << "(" << ok.Offset() << ")";
        return;
    }

    httpReq->reply_comm_rest_succ2(ori_message);
}

void rest_api_service::on_node_stop_task_timer(const std::shared_ptr<core_timer> &timer) {
    if (nullptr == timer) {
        LOG_ERROR << "timer is nullptr";
        return;
    }

    const std::string &session_id = timer->get_session_id();
    std::shared_ptr<service_session> session = get_session(session_id);
    if (nullptr == session) {
        LOG_ERROR << "session is nullptr";
        return;
    }

    variables_map &vm = session->get_context().get_args();
    if (0 == vm.count(HTTP_REQUEST_KEY)) {
        LOG_ERROR << "session's context has no HTTP_REQUEST_KEY";
        session->clear();
        this->remove_session(session_id);
        return;
    }

    auto hreq_context = vm[HTTP_REQUEST_KEY].as<std::shared_ptr<dbc::network::http_request_context>>();
    if (nullptr != hreq_context) {
        hreq_context->m_hreq->reply_comm_rest_err(HTTP_INTERNAL, -1, "stop task timeout");
    }

    session->clear();
    this->remove_session(session_id);
}

// restart task
void rest_api_service::rest_restart_task(const std::shared_ptr<dbc::network::http_request>& httpReq, const std::string &path) {
    if (httpReq->get_request_method() != dbc::network::http_request::POST) {
        LOG_ERROR << "http request is not post";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_REQUEST, "only support POST request");
        return;
    }

    std::vector<std::string> path_list;
    util::split_path(path, path_list);
    if (path_list.size() != 2) {
        LOG_ERROR << "path_list's size != 2";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "invalid uri, please use /tasks/<task_id>/restart");
        return;
    }

    req_body body;

    body.task_id = path_list[0];
    if (body.task_id.empty()) {
        LOG_ERROR << "task_id is empty";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "task_id is empty");
        return;
    }

    std::string s_body = httpReq->read_body();
    if (s_body.empty()) {
        LOG_ERROR << "http request body is empty";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "http request body is empty");
        return;
    }

    rapidjson::Document doc;
    rapidjson::ParseResult ok = doc.Parse(s_body.c_str());
    if (!ok) {
        std::stringstream ss;
        ss << "json parse error: " << rapidjson::GetParseError_En(ok.Code()) << "(" << ok.Offset() << ")";
        LOG_ERROR << ss.str();
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, ss.str());
        return;
    }

    if (!doc.IsObject()) {
        LOG_ERROR << "invalid json";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "invalid json");
        return;
    }

    // peer_nodes_list
    std::string peer_node_id;
    if (doc.HasMember("peer_nodes_list")) {
        if (doc["peer_nodes_list"].IsArray()) {
            uint32_t list_size = doc["peer_nodes_list"].Size();
            if (list_size < 1) {
                LOG_ERROR << "peer_nodes_list's size < 1";
                httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "peer_nodes_list's size < 1");
                return;
            }

            // 暂时只支持一次操作1个节点
            std::string node(doc["peer_nodes_list"][0].GetString());
            body.peer_nodes_list.push_back(node);
            peer_node_id = node;
        } else {
            LOG_ERROR << "peer_nodes_list is not array";
            httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "peer_nodes_list is not array");
            return;
        }
    } else {
        LOG_ERROR << "has no peer_nodes_list";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "has no peer_nodes_list");
        return;
    }
    // additional
    if (doc.HasMember("additional")) {
        if (doc["additional"].IsObject()) {
            const rapidjson::Value &obj = doc["additional"];
            rapidjson::StringBuffer buffer;
            rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
            obj.Accept(writer);
            body.additional = buffer.GetString();
        } else {
            LOG_ERROR << "additional is not object";
            httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "additional is not object");
            return;
        }
    } else {
        LOG_ERROR << "has no additional";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "has no additional");
        return;
    }
    // sign
    if (doc.HasMember("sign")) {
        if (doc["sign"].IsString()) {
            body.sign = doc["sign"].GetString();
        } else {
            LOG_ERROR << "sign is not string";
            httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "sign is not string");
            return;
        }
    } else {
        LOG_ERROR << "has no sign";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "has no sign");
        return;
    }
    // nonce
    if (doc.HasMember("nonce")) {
        if (doc["nonce"].IsString()) {
            body.nonce = doc["nonce"].GetString();
        } else {
            LOG_ERROR << "nonce is not string";
            httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "nonce is not string");
            return;
        }
    } else {
        LOG_ERROR << "has no nonce";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "has no nonce");
        return;
    }
    // wallet
    if (doc.HasMember("wallet")) {
        if (doc["wallet"].IsString()) {
            body.wallet = doc["wallet"].GetString();
        } else {
            LOG_ERROR << "wallet is not string";
            httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "wallet is not string");
            return;
        }
    } else {
        LOG_ERROR << "has no wallet";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "has no wallet");
        return;
    }
    // session_id
    if (doc.HasMember("session_id")) {
        if (doc["session_id"].IsString()) {
            body.session_id = doc["session_id"].GetString();
        } else {
            LOG_ERROR << "session_id is not string";
            httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "session_id is not string");
            return;
        }
    }
    // session_id_sign
    if (doc.HasMember("session_id_sign")) {
        if (doc["session_id_sign"].IsString()) {
            body.session_id_sign = doc["session_id_sign"].GetString();
        } else {
            LOG_ERROR << "session_id_sign is not string";
            httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "session_id_sign is not string");
            return;
        }
    }
    // pub_key
    body.pub_key = conf_manager::instance().get_pub_key();
    if (body.pub_key.empty()) {
        LOG_ERROR << "pub_key is empty";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "pub_key is empty");
        return;
    }

    std::string head_session_id = util::create_session_id();

    auto node_req_msg = create_node_restart_task_req_msg(head_session_id, body);
    if (nullptr == node_req_msg) {
        LOG_ERROR << "creaate node request failed";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_RESPONSE_ERROR, "creaate node request failed");
        return;
    }

    if (E_SUCCESS != create_request_session(NODE_RESTART_TASK_TIMER, httpReq, node_req_msg, head_session_id, peer_node_id)) {
        LOG_ERROR << "create request session failed";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_RESPONSE_ERROR, "creaate request session failed");
        return;
    }

    if (dbc::network::connection_manager::instance().broadcast_message(node_req_msg) != E_SUCCESS) {
        LOG_ERROR << "broadcast request failed";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_RESPONSE_ERROR, "broadcast request failed");
        return;
    }
}

std::shared_ptr<dbc::network::message> rest_api_service::create_node_restart_task_req_msg(const std::string &head_session_id,
                    const req_body& body) {
    // 创建 node_ 请求
    auto req_content = std::make_shared<dbc::node_restart_task_req>();
    // header
    req_content->header.__set_magic(conf_manager::instance().get_net_flag());
    req_content->header.__set_msg_name(NODE_RESTART_TASK_REQ);
    req_content->header.__set_nonce(util::create_nonce());
    req_content->header.__set_session_id(head_session_id);
    std::map<std::string, std::string> exten_info;
    exten_info["pub_key"] = body.pub_key;
    exten_info["sign"] = body.sign;
    exten_info["nonce"] = body.nonce;
    req_content->header.__set_exten_info(exten_info);
    std::vector<std::string> path;
    path.push_back(conf_manager::instance().get_node_id());
    req_content->header.__set_path(path);

    // body
    dbc::node_restart_task_req_data req_data;
    req_data.__set_task_id(body.task_id);
    req_data.__set_peer_nodes_list(body.peer_nodes_list);
    req_data.__set_additional(body.additional);
    req_data.__set_wallet(body.wallet);
    req_data.__set_session_id(body.session_id);
    req_data.__set_session_id_sign(body.session_id_sign);

    // encrypt
    std::shared_ptr<byte_buf> out_buf = std::make_shared<byte_buf>();
    dbc::network::binary_protocol proto(out_buf.get());
    req_data.write(&proto);

    dbc::node_service_info service_info;
    bool bfound = service_info_collection::instance().find(body.peer_nodes_list[0], service_info);
    if (bfound) {
        std::string pub_key = service_info.kvs.count("pub_key") ? service_info.kvs["pub_key"] : "";
        std::string priv_key = conf_manager::instance().get_priv_key();

        if (!pub_key.empty() && !priv_key.empty()) {
            std::string s_data = encrypt_data((unsigned char*) out_buf->get_read_ptr(), out_buf->get_valid_read_len(), pub_key, priv_key);
            req_content->body.__set_data(s_data);
        } else {
            LOG_ERROR << "pub_key is empty, node_id:" << body.peer_nodes_list[0];
            return nullptr;
        }
    } else {
        LOG_ERROR << "service_info_collection not found node_id:" << body.peer_nodes_list[0];
        return nullptr;
    }

    std::shared_ptr<dbc::network::message> req_msg = std::make_shared<dbc::network::message>();
    req_msg->set_name(NODE_RESTART_TASK_REQ);
    req_msg->set_content(req_content);

    return req_msg;
}

void rest_api_service::on_node_restart_task_rsp(const std::shared_ptr<dbc::network::http_request_context>& hreq_context,
                                                   const std::shared_ptr<dbc::network::message>& rsp_msg) {
    auto node_rsp_msg = std::dynamic_pointer_cast<dbc::node_restart_task_rsp>(rsp_msg->content);
    if (!node_rsp_msg) {
        LOG_ERROR << "node_rsp_msg is nullptr";
        return;
    }

    const std::shared_ptr<dbc::network::http_request> &httpReq = hreq_context->m_hreq;

    // decrypt
    std::string pub_key = node_rsp_msg->header.exten_info["pub_key"];
    std::string priv_key = conf_manager::instance().get_priv_key();
    if (pub_key.empty() || priv_key.empty()) {
        httpReq->reply_comm_rest_err(HTTP_INTERNAL, -1, "pub_key or priv_key is empty");
        LOG_ERROR << "pub_key or priv_key is empty";
        return;
    }

    std::string ori_message;
    try {
        bool succ = decrypt_data(node_rsp_msg->body.data, pub_key, priv_key, ori_message);
        if (!succ || ori_message.empty()) {
            httpReq->reply_comm_rest_err(HTTP_INTERNAL, -1, "rsq decrypt error1");
            LOG_ERROR << "rsq decrypt error1";
            return;
        }
    } catch (std::exception &e) {
        httpReq->reply_comm_rest_err(HTTP_INTERNAL, -1, "req decrypt error2");
        LOG_ERROR << "req decrypt error2";
        return;
    }

    rapidjson::Document doc;
    rapidjson::ParseResult ok = doc.Parse(ori_message.c_str());
    if (!ok) {
        httpReq->reply_comm_rest_err(HTTP_INTERNAL, -1, "response parse error");
        LOG_ERROR << "response parse error: " << rapidjson::GetParseError_En(ok.Code()) << "(" << ok.Offset() << ")";
        return;
    }

    httpReq->reply_comm_rest_succ2(ori_message);
}

void rest_api_service::on_node_restart_task_timer(const std::shared_ptr<core_timer> &timer) {
    if (nullptr == timer) {
        LOG_ERROR << "timer is nullptr";
        return;
    }

    const std::string &session_id = timer->get_session_id();
    std::shared_ptr<service_session> session = get_session(session_id);
    if (nullptr == session) {
        LOG_ERROR << "session is nullptr";
        return;
    }

    variables_map &vm = session->get_context().get_args();
    if (0 == vm.count(HTTP_REQUEST_KEY)) {
        LOG_ERROR << "session's context has no HTTP_REQUEST_KEY";
        session->clear();
        this->remove_session(session_id);
        return;
    }

    auto hreq_context = vm[HTTP_REQUEST_KEY].as<std::shared_ptr<dbc::network::http_request_context>>();
    if (nullptr != hreq_context) {
        hreq_context->m_hreq->reply_comm_rest_err(HTTP_INTERNAL, -1, "restart task timeout");
    }

    session->clear();
    this->remove_session(session_id);
}

// reset task
void rest_api_service::rest_reset_task(const std::shared_ptr<dbc::network::http_request>& httpReq, const std::string &path) {
    if (httpReq->get_request_method() != dbc::network::http_request::POST) {
        LOG_ERROR << "http request is not post";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_REQUEST, "only support POST request");
        return;
    }

    std::vector<std::string> path_list;
    util::split_path(path, path_list);
    if (path_list.size() != 2) {
        LOG_ERROR << "path_list's size != 2";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "invalid uri, please use /tasks/<task_id>/reset");
        return;
    }

    req_body body;

    body.task_id = path_list[0];
    if (body.task_id.empty()) {
        LOG_ERROR << "task_id is empty";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "task_id is empty");
        return;
    }

    std::string s_body = httpReq->read_body();
    if (s_body.empty()) {
        LOG_ERROR << "http request body is empty";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "http request body is empty");
        return;
    }

    rapidjson::Document doc;
    rapidjson::ParseResult ok = doc.Parse(s_body.c_str());
    if (!ok) {
        std::stringstream ss;
        ss << "json parse error: " << rapidjson::GetParseError_En(ok.Code()) << "(" << ok.Offset() << ")";
        LOG_ERROR << ss.str();
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, ss.str());
        return;
    }

    if (!doc.IsObject()) {
        LOG_ERROR << "invalid json";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "invalid json");
        return;
    }

    // peer_nodes_list
    std::string peer_node_id;
    if (doc.HasMember("peer_nodes_list")) {
        if (doc["peer_nodes_list"].IsArray()) {
            uint32_t list_size = doc["peer_nodes_list"].Size();
            if (list_size < 1) {
                LOG_ERROR << "peer_nodes_list's size < 1";
                httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "peer_nodes_list's size < 1");
                return;
            }

            // 暂时只支持一次操作1个节点
            std::string node(doc["peer_nodes_list"][0].GetString());
            body.peer_nodes_list.push_back(node);
            peer_node_id = node;
        } else {
            LOG_ERROR << "peer_nodes_list is not array";
            httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "peer_nodes_list is not array");
            return;
        }
    } else {
        LOG_ERROR << "has no peer_nodes_list";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "has no peer_nodes_list");
        return;
    }
    // additional
    if (doc.HasMember("additional")) {
        if (doc["additional"].IsObject()) {
            const rapidjson::Value &obj = doc["additional"];
            rapidjson::StringBuffer buffer;
            rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
            obj.Accept(writer);
            body.additional = buffer.GetString();
        } else {
            LOG_ERROR << "additional is not object";
            httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "additional is not object");
            return;
        }
    } else {
        LOG_ERROR << "has no additional";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "has no additional");
        return;
    }
    // sign
    if (doc.HasMember("sign")) {
        if (doc["sign"].IsString()) {
            body.sign = doc["sign"].GetString();
        } else {
            LOG_ERROR << "sign is not string";
            httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "sign is not string");
            return;
        }
    } else {
        LOG_ERROR << "has no sign";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "has no sign");
        return;
    }
    // nonce
    if (doc.HasMember("nonce")) {
        if (doc["nonce"].IsString()) {
            body.nonce = doc["nonce"].GetString();
        } else {
            LOG_ERROR << "nonce is not string";
            httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "nonce is not string");
            return;
        }
    } else {
        LOG_ERROR << "has no nonce";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "has no nonce");
        return;
    }
    // wallet
    if (doc.HasMember("wallet")) {
        if (doc["wallet"].IsString()) {
            body.wallet = doc["wallet"].GetString();
        } else {
            LOG_ERROR << "wallet is not string";
            httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "wallet is not string");
            return;
        }
    } else {
        LOG_ERROR << "has no wallet";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "has no wallet");
        return;
    }
    // session_id
    if (doc.HasMember("session_id")) {
        if (doc["session_id"].IsString()) {
            body.session_id = doc["session_id"].GetString();
        } else {
            LOG_ERROR << "session_id is not string";
            httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "session_id is not string");
            return;
        }
    }
    // session_id_sign
    if (doc.HasMember("session_id_sign")) {
        if (doc["session_id_sign"].IsString()) {
            body.session_id_sign = doc["session_id_sign"].GetString();
        } else {
            LOG_ERROR << "session_id_sign is not string";
            httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "session_id_sign is not string");
            return;
        }
    }
    // pub_key
    body.pub_key = conf_manager::instance().get_pub_key();
    if (body.pub_key.empty()) {
        LOG_ERROR << "pub_key is empty";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "pub_key is empty");
        return;
    }

    std::string head_session_id = util::create_session_id();

    auto node_req_msg = create_node_reset_task_req_msg(head_session_id, body);
    if (nullptr == node_req_msg) {
        LOG_ERROR << "create node request failed";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_RESPONSE_ERROR, "create node request failed");
        return;
    }

    if (E_SUCCESS != create_request_session(NODE_RESET_TASK_TIMER, httpReq, node_req_msg, head_session_id, peer_node_id)) {
        LOG_ERROR << "create request session failed";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_RESPONSE_ERROR, "creaate request session failed");
        return;
    }

    if (dbc::network::connection_manager::instance().broadcast_message(node_req_msg) != E_SUCCESS) {
        LOG_ERROR << "broadcast request failed";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_RESPONSE_ERROR, "broadcast request failed");
        return;
    }
}

std::shared_ptr<dbc::network::message> rest_api_service::create_node_reset_task_req_msg(const std::string &head_session_id,
                                                                                        const req_body& body) {
    // 创建 node_ 请求
    std::shared_ptr<dbc::node_reset_task_req> req_content = std::make_shared<dbc::node_reset_task_req>();
    // header
    req_content->header.__set_magic(conf_manager::instance().get_net_flag());
    req_content->header.__set_msg_name(NODE_RESET_TASK_REQ);
    req_content->header.__set_nonce(util::create_nonce());
    req_content->header.__set_session_id(head_session_id);
    std::map<std::string, std::string> exten_info;
    exten_info["pub_key"] = body.pub_key;
    exten_info["sign"] = body.sign;
    exten_info["nonce"] = body.nonce;
    req_content->header.__set_exten_info(exten_info);
    std::vector<std::string> path;
    path.push_back(conf_manager::instance().get_node_id());
    req_content->header.__set_path(path);

    // body
    dbc::node_reset_task_req_data req_data;
    req_data.__set_task_id(body.task_id);
    req_data.__set_peer_nodes_list(body.peer_nodes_list);
    req_data.__set_additional(body.additional);
    req_data.__set_wallet(body.wallet);
    req_data.__set_session_id(body.session_id);
    req_data.__set_session_id_sign(body.session_id_sign);

    // encrypt
    std::shared_ptr<byte_buf> out_buf = std::make_shared<byte_buf>();
    dbc::network::binary_protocol proto(out_buf.get());
    req_data.write(&proto);

    dbc::node_service_info service_info;
    bool bfound = service_info_collection::instance().find(body.peer_nodes_list[0], service_info);
    if (bfound) {
        std::string pub_key = service_info.kvs.count("pub_key") ? service_info.kvs["pub_key"] : "";
        std::string priv_key = conf_manager::instance().get_priv_key();

        if (!pub_key.empty() && !priv_key.empty()) {
            std::string s_data = encrypt_data((unsigned char*) out_buf->get_read_ptr(), out_buf->get_valid_read_len(), pub_key, priv_key);
            req_content->body.__set_data(s_data);
        } else {
            LOG_ERROR << "pub_key is empty, node_id:" << body.peer_nodes_list[0];
            return nullptr;
        }
    } else {
        LOG_ERROR << "service_info_collection not found node_id:" << body.peer_nodes_list[0];
        return nullptr;
    }

    std::shared_ptr<dbc::network::message> req_msg = std::make_shared<dbc::network::message>();
    req_msg->set_name(NODE_RESET_TASK_REQ);
    req_msg->set_content(req_content);

    return req_msg;
}

void rest_api_service::on_node_reset_task_rsp(const std::shared_ptr<dbc::network::http_request_context>& hreq_context,
                                                 const std::shared_ptr<dbc::network::message>& rsp_msg) {
    auto node_rsp_msg = std::dynamic_pointer_cast<dbc::node_reset_task_rsp>(rsp_msg->content);
    if (!node_rsp_msg) {
        LOG_ERROR << "node_rsp_msg is nullptr";
        return;
    }

    const std::shared_ptr<dbc::network::http_request> &httpReq = hreq_context->m_hreq;

    // decrypt
    std::string pub_key = node_rsp_msg->header.exten_info["pub_key"];
    std::string priv_key = conf_manager::instance().get_priv_key();
    if (pub_key.empty() || priv_key.empty()) {
        httpReq->reply_comm_rest_err(HTTP_INTERNAL, -1, "pub_key or priv_key is empty");
        LOG_ERROR << "pub_key or priv_key is empty";
        return;
    }

    std::string ori_message;
    try {
        bool succ = decrypt_data(node_rsp_msg->body.data, pub_key, priv_key, ori_message);
        if (!succ || ori_message.empty()) {
            httpReq->reply_comm_rest_err(HTTP_INTERNAL, -1, "rsq decrypt error1");
            LOG_ERROR << "rsq decrypt error1";
            return;
        }
    } catch (std::exception &e) {
        httpReq->reply_comm_rest_err(HTTP_INTERNAL, -1, "req decrypt error2");
        LOG_ERROR << "req decrypt error2";
        return;
    }

    rapidjson::Document doc;
    rapidjson::ParseResult ok = doc.Parse(ori_message.c_str());
    if (!ok) {
        httpReq->reply_comm_rest_err(HTTP_INTERNAL, -1, "response parse error");
        LOG_ERROR << "response parse error: " << rapidjson::GetParseError_En(ok.Code()) << "(" << ok.Offset() << ")";
        return;
    }

    httpReq->reply_comm_rest_succ2(ori_message);
}

void rest_api_service::on_node_reset_task_timer(const std::shared_ptr<core_timer> &timer) {
    if (nullptr == timer) {
        LOG_ERROR << "timer is nullptr";
        return;
    }

    const std::string &session_id = timer->get_session_id();
    std::shared_ptr<service_session> session = get_session(session_id);
    if (nullptr == session) {
        LOG_ERROR << "session is nullptr";
        return;
    }

    variables_map &vm = session->get_context().get_args();
    if (0 == vm.count(HTTP_REQUEST_KEY)) {
        LOG_ERROR << "session's context has no HTTP_REQUEST_KEY";
        session->clear();
        this->remove_session(session_id);
        return;
    }

    auto hreq_context = vm[HTTP_REQUEST_KEY].as<std::shared_ptr<dbc::network::http_request_context>>();
    if (nullptr != hreq_context) {
        hreq_context->m_hreq->reply_comm_rest_err(HTTP_INTERNAL, -1, "reset task timeout");
    }

    session->clear();
    this->remove_session(session_id);
}

// delete task
void rest_api_service::rest_delete_task(const std::shared_ptr<dbc::network::http_request>& httpReq, const std::string &path) {
    if (httpReq->get_request_method() != dbc::network::http_request::POST) {
        LOG_ERROR << "http request is not post";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_REQUEST, "only support POST request");
        return;
    }

    std::vector<std::string> path_list;
    util::split_path(path, path_list);
    if (path_list.size() != 2) {
        LOG_ERROR << "path_list's size != 2";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "invalid uri, please use /tasks/<task_id>/delete");
        return;
    }

    req_body body;

    body.task_id = path_list[0];
    if (body.task_id.empty()) {
        LOG_ERROR << "task_id is empty";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "task_id is empty");
        return;
    }

    std::string s_body = httpReq->read_body();
    if (s_body.empty()) {
        LOG_ERROR << "http request body is empty";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "http request body is empty");
        return;
    }

    rapidjson::Document doc;
    rapidjson::ParseResult ok = doc.Parse(s_body.c_str());
    if (!ok) {
        std::stringstream ss;
        ss << "json parse error: " << rapidjson::GetParseError_En(ok.Code()) << "(" << ok.Offset() << ")";
        LOG_ERROR << ss.str();
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, ss.str());
        return;
    }

    if (!doc.IsObject()) {
        LOG_ERROR << "invalid json";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "invalid json");
        return;
    }

    // peer_nodes_list
    std::string peer_node_id;
    if (doc.HasMember("peer_nodes_list")) {
        if (doc["peer_nodes_list"].IsArray()) {
            uint32_t list_size = doc["peer_nodes_list"].Size();
            if (list_size < 1) {
                LOG_ERROR << "peer_nodes_list's size < 1";
                httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "peer_nodes_list's size < 1");
                return;
            }

            // 暂时只支持一次操作1个节点
            std::string node(doc["peer_nodes_list"][0].GetString());
            body.peer_nodes_list.push_back(node);
            peer_node_id = node;
        } else {
            LOG_ERROR << "peer_nodes_list is not array";
            httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "peer_nodes_list is not array");
            return;
        }
    } else {
        LOG_ERROR << "has no peer_nodes_list";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "has no peer_nodes_list");
        return;
    }
    // additional
    if (doc.HasMember("additional")) {
        if (doc["additional"].IsObject()) {
            const rapidjson::Value &obj = doc["additional"];
            rapidjson::StringBuffer buffer;
            rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
            obj.Accept(writer);
            body.additional = buffer.GetString();
        } else {
            LOG_ERROR << "additional is not object";
            httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "additional is not object");
            return;
        }
    } else {
        LOG_ERROR << "has no additional";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "has no additional");
        return;
    }
    // sign
    if (doc.HasMember("sign")) {
        if (doc["sign"].IsString()) {
            body.sign = doc["sign"].GetString();
        } else {
            LOG_ERROR << "sign is not string";
            httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "sign is not string");
            return;
        }
    } else {
        LOG_ERROR << "has no sign";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "has no sign");
        return;
    }
    // nonce
    if (doc.HasMember("nonce")) {
        if (doc["nonce"].IsString()) {
            body.nonce = doc["nonce"].GetString();
        } else {
            LOG_ERROR << "nonce is not string";
            httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "nonce is not string");
            return;
        }
    } else {
        LOG_ERROR << "has no nonce";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "has no nonce");
        return;
    }
    // wallet
    if (doc.HasMember("wallet")) {
        if (doc["wallet"].IsString()) {
            body.wallet = doc["wallet"].GetString();
        } else {
            LOG_ERROR << "wallet is not string";
            httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "wallet is not string");
            return;
        }
    } else {
        LOG_ERROR << "has no wallet";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "has no wallet");
        return;
    }
    // session_id
    if (doc.HasMember("session_id")) {
        if (doc["session_id"].IsString()) {
            body.session_id = doc["session_id"].GetString();
        } else {
            LOG_ERROR << "session_id is not string";
            httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "session_id is not string");
            return;
        }
    }
    // session_id_sign
    if (doc.HasMember("session_id_sign")) {
        if (doc["session_id_sign"].IsString()) {
            body.session_id_sign = doc["session_id_sign"].GetString();
        } else {
            LOG_ERROR << "session_id_sign is not string";
            httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "session_id_sign is not string");
            return;
        }
    }
    // pub_key
    body.pub_key = conf_manager::instance().get_pub_key();
    if (body.pub_key.empty()) {
        LOG_ERROR << "pub_key is empty";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "pub_key is empty");
        return;
    }

    std::string head_session_id = util::create_session_id();

    auto node_req_msg = create_node_delete_task_req_msg(head_session_id, body);
    if (nullptr == node_req_msg) {
        LOG_ERROR << "create node request failed";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_RESPONSE_ERROR, "create node request failed");
        return;
    }

    if (E_SUCCESS != create_request_session(NODE_DELETE_TASK_TIMER, httpReq, node_req_msg, head_session_id, peer_node_id)) {
        LOG_ERROR << "create request session failed";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_RESPONSE_ERROR, "creaate request session failed");
        return;
    }

    if (dbc::network::connection_manager::instance().broadcast_message(node_req_msg) != E_SUCCESS) {
        LOG_ERROR << "broadcast request failed";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_RESPONSE_ERROR, "broadcast request failed");
        return;
    }
}

std::shared_ptr<dbc::network::message> rest_api_service::create_node_delete_task_req_msg(const std::string &head_session_id,
                                                                                         const req_body& body) {
    // 创建 node_ 请求
    std::shared_ptr<dbc::node_delete_task_req> req_content = std::make_shared<dbc::node_delete_task_req>();
    // header
    req_content->header.__set_magic(conf_manager::instance().get_net_flag());
    req_content->header.__set_msg_name(NODE_DELETE_TASK_REQ);
    req_content->header.__set_nonce(util::create_nonce());
    req_content->header.__set_session_id(head_session_id);
    std::map<std::string, std::string> exten_info;
    exten_info["pub_key"] = body.pub_key;
    exten_info["sign"] = body.sign;
    exten_info["nonce"] = body.nonce;
    req_content->header.__set_exten_info(exten_info);
    std::vector<std::string> path;
    path.push_back(conf_manager::instance().get_node_id());
    req_content->header.__set_path(path);

    // body
    dbc::node_delete_task_req_data req_data;
    req_data.__set_task_id(body.task_id);
    req_data.__set_peer_nodes_list(body.peer_nodes_list);
    req_data.__set_additional(body.additional);
    req_data.__set_wallet(body.wallet);
    req_data.__set_session_id(body.session_id);
    req_data.__set_session_id_sign(body.session_id_sign);

    // encrypt
    std::shared_ptr<byte_buf> out_buf = std::make_shared<byte_buf>();
    dbc::network::binary_protocol proto(out_buf.get());
    req_data.write(&proto);

    dbc::node_service_info service_info;
    bool bfound = service_info_collection::instance().find(body.peer_nodes_list[0], service_info);
    if (bfound) {
        std::string pub_key = service_info.kvs.count("pub_key") ? service_info.kvs["pub_key"] : "";
        std::string priv_key = conf_manager::instance().get_priv_key();

        if (!pub_key.empty() && !priv_key.empty()) {
            std::string s_data = encrypt_data((unsigned char*) out_buf->get_read_ptr(), out_buf->get_valid_read_len(), pub_key, priv_key);
            req_content->body.__set_data(s_data);
        } else {
            LOG_ERROR << "pub_key is empty, node_id:" << body.peer_nodes_list[0];
            return nullptr;
        }
    } else {
        LOG_ERROR << "service_info_collection not found node_id:" << body.peer_nodes_list[0];
        return nullptr;
    }

    std::shared_ptr<dbc::network::message> req_msg = std::make_shared<dbc::network::message>();
    req_msg->set_name(NODE_DELETE_TASK_REQ);
    req_msg->set_content(req_content);

    return req_msg;
}

void rest_api_service::on_node_delete_task_rsp(const std::shared_ptr<dbc::network::http_request_context>& hreq_context,
                                                  const std::shared_ptr<dbc::network::message>& rsp_msg) {
    auto node_rsp_msg = std::dynamic_pointer_cast<dbc::node_delete_task_rsp>(rsp_msg->content);
    if (!node_rsp_msg) {
        LOG_ERROR << "rsp is nullptr";
        return;
    }

    const std::shared_ptr<dbc::network::http_request> &httpReq = hreq_context->m_hreq;

    // decrypt
    std::string pub_key = node_rsp_msg->header.exten_info["pub_key"];
    std::string priv_key = conf_manager::instance().get_priv_key();
    if (pub_key.empty() || priv_key.empty()) {
        httpReq->reply_comm_rest_err(HTTP_INTERNAL, -1, "pub_key or priv_key is empty");
        LOG_ERROR << "pub_key or priv_key is empty";
        return;
    }

    std::string ori_message;
    try {
        bool succ = decrypt_data(node_rsp_msg->body.data, pub_key, priv_key, ori_message);
        if (!succ || ori_message.empty()) {
            httpReq->reply_comm_rest_err(HTTP_INTERNAL, -1, "rsq decrypt error1");
            LOG_ERROR << "rsq decrypt error1";
            return;
        }
    } catch (std::exception &e) {
        httpReq->reply_comm_rest_err(HTTP_INTERNAL, -1, "req decrypt error2");
        LOG_ERROR << "req decrypt error2";
        return;
    }

    rapidjson::Document doc;
    rapidjson::ParseResult ok = doc.Parse(ori_message.c_str());
    if (!ok) {
        httpReq->reply_comm_rest_err(HTTP_INTERNAL, -1, "response parse error");
        LOG_ERROR << "response parse error: " << rapidjson::GetParseError_En(ok.Code()) << "(" << ok.Offset() << ")";
        return;
    }

    httpReq->reply_comm_rest_succ2(ori_message);
}

void rest_api_service::on_node_delete_task_timer(const std::shared_ptr<core_timer> &timer) {
    if (nullptr == timer) {
        LOG_ERROR << "timer is nullptr";
        return;
    }

    const std::string &session_id = timer->get_session_id();
    std::shared_ptr<service_session> session = get_session(session_id);
    if (nullptr == session) {
        LOG_ERROR << "session is nullptr";
        return;
    }

    variables_map &vm = session->get_context().get_args();
    if (0 == vm.count(HTTP_REQUEST_KEY)) {
        LOG_ERROR << "session's context has no HTTP_REQUEST_KEY";
        session->clear();
        this->remove_session(session_id);
        return;
    }

    auto hreq_context = vm[HTTP_REQUEST_KEY].as<std::shared_ptr<dbc::network::http_request_context>>();
    if (nullptr != hreq_context) {
        hreq_context->m_hreq->reply_comm_rest_err(HTTP_INTERNAL, -1, "delete task timeout");
    }

    session->clear();
    this->remove_session(session_id);
}

// list task
void rest_api_service::rest_list_task(const std::shared_ptr<dbc::network::http_request>& httpReq, const std::string &path) {
    if (httpReq->get_request_method() != dbc::network::http_request::POST) {
        LOG_ERROR << "http request is not post";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_REQUEST, "only support POST request");
        return;
    }

    req_body body;

    std::vector<std::string> path_list;
    util::split_path(path, path_list);
    if (path_list.empty()) {
        body.task_id = "";
    } else if (path_list.size() == 1) {
        body.task_id = path_list[0];
    } else {
        LOG_ERROR << "path_list's size > 1";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "invalid uri, please use /tasks");
        return;
    }

    std::string s_body = httpReq->read_body();
    if (s_body.empty()) {
        LOG_ERROR << "http request body is empty";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "http request body is empty");
        return;
    }

    rapidjson::Document doc;
    rapidjson::ParseResult ok = doc.Parse(s_body.c_str());
    if (!ok) {
        std::stringstream ss;
        ss << "json parse error: " << rapidjson::GetParseError_En(ok.Code()) << "(" << ok.Offset() << ")";
        LOG_ERROR << ss.str();
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, ss.str());
        return;
    }

    if (!doc.IsObject()) {
        LOG_ERROR << "invalid json";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "invalid json");
        return;
    }

    // peer_nodes_list
    std::string peer_node_id;
    if (doc.HasMember("peer_nodes_list")) {
        if (doc["peer_nodes_list"].IsArray()) {
            uint32_t list_size = doc["peer_nodes_list"].Size();
            if (list_size < 1) {
                LOG_ERROR << "peer_nodes_list's size < 1";
                httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "peer_nodes_list's size < 1");
                return;
            }

            // 暂时只支持一次操作1个节点
            std::string node(doc["peer_nodes_list"][0].GetString());
            body.peer_nodes_list.push_back(node);
            peer_node_id = node;
        } else {
            LOG_ERROR << "peer_nodes_list is not array";
            httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "peer_nodes_list is not array");
            return;
        }
    } else {
        LOG_ERROR << "has no peer_nodes_list";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "has no peer_nodes_list");
        return;
    }
    // additional
    if (doc.HasMember("additional")) {
        if (doc["additional"].IsObject()) {
            const rapidjson::Value &obj = doc["additional"];
            rapidjson::StringBuffer buffer;
            rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
            obj.Accept(writer);
            body.additional = buffer.GetString();
        } else {
            LOG_ERROR << "additional is not object";
            httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "additional is not object");
            return;
        }
    } else {
        LOG_ERROR << "has no additional";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "has no additional");
        return;
    }
    // sign
    if (doc.HasMember("sign")) {
        if (doc["sign"].IsString()) {
            body.sign = doc["sign"].GetString();
        } else {
            LOG_ERROR << "sign is not string";
            httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "sign is not string");
            return;
        }
    } else {
        LOG_ERROR << "has no sign";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "has no sign");
        return;
    }
    // nonce
    if (doc.HasMember("nonce")) {
        if (doc["nonce"].IsString()) {
            body.nonce = doc["nonce"].GetString();
        } else {
            LOG_ERROR << "nonce is not string";
            httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "nonce is not string");
            return;
        }
    } else {
        LOG_ERROR << "has no nonce";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "has no nonce");
        return;
    }
    // wallet
    if (doc.HasMember("wallet")) {
        if (doc["wallet"].IsString()) {
            body.wallet = doc["wallet"].GetString();
        } else {
            LOG_ERROR << "wallet is not string";
            httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "wallet is not string");
            return;
        }
    } else {
        LOG_ERROR << "has no wallet";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "has no wallet");
        return;
    }
    // session_id
    if (doc.HasMember("session_id")) {
        if (doc["session_id"].IsString()) {
            body.session_id = doc["session_id"].GetString();
        } else {
            LOG_ERROR << "session_id is not string";
            httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "session_id is not string");
            return;
        }
    }
    // session_id_sign
    if (doc.HasMember("session_id_sign")) {
        if (doc["session_id_sign"].IsString()) {
            body.session_id_sign = doc["session_id_sign"].GetString();
        } else {
            LOG_ERROR << "session_id_sign is not string";
            httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "session_id_sign is not string");
            return;
        }
    }
    // pub_key
    body.pub_key = conf_manager::instance().get_pub_key();
    if (body.pub_key.empty()) {
        LOG_ERROR << "pub_key is empty";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "pub_key is empty");
        return;
    }

    std::string head_session_id = util::create_session_id();

    auto node_req_msg = create_node_list_task_req_msg(head_session_id, body);
    if (nullptr == node_req_msg) {
        LOG_ERROR << "create node request failed";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_RESPONSE_ERROR, "create node request failed");
        return;
    }

    if (E_SUCCESS != create_request_session(NODE_LIST_TASK_TIMER, httpReq, node_req_msg, head_session_id, peer_node_id)) {
        LOG_ERROR << "create request session failed";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_RESPONSE_ERROR, "creaate request session failed");
        return;
    }

    if (dbc::network::connection_manager::instance().broadcast_message(node_req_msg) != E_SUCCESS) {
        LOG_ERROR << "broadcast request failed";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_RESPONSE_ERROR, "broadcast request failed");
        return;
    }
}

std::shared_ptr<dbc::network::message> rest_api_service::create_node_list_task_req_msg(const std::string &head_session_id,
                                                                                       const req_body& body) {
    auto req_content = std::make_shared<dbc::node_list_task_req>();
    // header
    req_content->header.__set_magic(conf_manager::instance().get_net_flag());
    req_content->header.__set_msg_name(NODE_LIST_TASK_REQ);
    req_content->header.__set_nonce(util::create_nonce());
    req_content->header.__set_session_id(head_session_id);
    std::map<std::string, std::string> exten_info;
    exten_info["pub_key"] = body.pub_key;
    exten_info["sign"] = body.sign;
    exten_info["nonce"] = body.nonce;
    req_content->header.__set_exten_info(exten_info);
    std::vector<std::string> path;
    path.push_back(conf_manager::instance().get_node_id());
    req_content->header.__set_path(path);

    // body
    dbc::node_list_task_req_data req_data;
    req_data.__set_task_id(body.task_id);
    req_data.__set_peer_nodes_list(body.peer_nodes_list);
    req_data.__set_additional(body.additional);
    req_data.__set_wallet(body.wallet);
    req_data.__set_session_id(body.session_id);
    req_data.__set_session_id_sign(body.session_id_sign);

    // encrypt
    std::shared_ptr<byte_buf> out_buf = std::make_shared<byte_buf>();
    dbc::network::binary_protocol proto(out_buf.get());
    req_data.write(&proto);

    dbc::node_service_info service_info;
    bool bfound = service_info_collection::instance().find(body.peer_nodes_list[0], service_info);
    if (bfound) {
        std::string pub_key = service_info.kvs.count("pub_key") ? service_info.kvs["pub_key"] : "";
        std::string priv_key = conf_manager::instance().get_priv_key();

        if (!pub_key.empty() && !priv_key.empty()) {
            std::string s_data = encrypt_data((unsigned char*) out_buf->get_read_ptr(), out_buf->get_valid_read_len(), pub_key, priv_key);
            req_content->body.__set_data(s_data);
        } else {
            LOG_ERROR << "pub_key is empty, node_id:" << body.peer_nodes_list[0];
            return nullptr;
        }
    } else {
        LOG_ERROR << "service_info_collection not found node_id:" << body.peer_nodes_list[0];
        return nullptr;
    }

    std::shared_ptr<dbc::network::message> req_msg = std::make_shared<dbc::network::message>();
    req_msg->set_name(NODE_LIST_TASK_REQ);
    req_msg->set_content(req_content);

    return req_msg;
}

void rest_api_service::on_node_list_task_rsp(const std::shared_ptr<dbc::network::http_request_context>& hreq_context,
                                             const std::shared_ptr<dbc::network::message>& rsp_msg) {
    auto node_rsp_msg = std::dynamic_pointer_cast<dbc::node_list_task_rsp>(rsp_msg->content);
    if (!node_rsp_msg) {
        LOG_ERROR << "node_rsp_msg is nullptr";
        return;
    }

    const std::shared_ptr<dbc::network::http_request> &httpReq = hreq_context->m_hreq;

    // decrypt
    std::string pub_key = node_rsp_msg->header.exten_info["pub_key"];
    std::string priv_key = conf_manager::instance().get_priv_key();
    if (pub_key.empty() || priv_key.empty()) {
        httpReq->reply_comm_rest_err(HTTP_INTERNAL, -1, "pub_key or priv_key is empty");
        LOG_ERROR << "pub_key or priv_key is empty";
        return;
    }

    std::string ori_message;
    try {
        bool succ = decrypt_data(node_rsp_msg->body.data, pub_key, priv_key, ori_message);
        if (!succ || ori_message.empty()) {
            httpReq->reply_comm_rest_err(HTTP_INTERNAL, -1, "rsq decrypt error1");
            LOG_ERROR << "rsq decrypt error1";
            return;
        }
    } catch (std::exception &e) {
        httpReq->reply_comm_rest_err(HTTP_INTERNAL, -1, "req decrypt error2");
        LOG_ERROR << "req decrypt error2";
        return;
    }

    rapidjson::Document doc;
    rapidjson::ParseResult ok = doc.Parse(ori_message.c_str());
    if (!ok) {
        httpReq->reply_comm_rest_err(HTTP_INTERNAL, -1, "response parse error");
        LOG_ERROR << "response parse error: " << rapidjson::GetParseError_En(ok.Code()) << "(" << ok.Offset() << ")";
        return;
    }

    httpReq->reply_comm_rest_succ2(ori_message);
}

void rest_api_service::on_node_list_task_timer(const std::shared_ptr<core_timer> &timer) {
    if (nullptr == timer) {
        LOG_ERROR << "timer is nullptr";
        return;
    }

    const std::string &session_id = timer->get_session_id();
    std::shared_ptr<service_session> session = get_session(session_id);
    if (nullptr == session) {
        LOG_ERROR << "session is nullptr";
        return;
    }

    variables_map &vm = session->get_context().get_args();
    if (0 == vm.count(HTTP_REQUEST_KEY)) {
        LOG_ERROR << "session's context has no HTTP_REQUEST_KEY";
        session->clear();
        this->remove_session(session_id);
        return;
    }

    auto hreq_context = vm[HTTP_REQUEST_KEY].as<std::shared_ptr<dbc::network::http_request_context>>();
    if (nullptr != hreq_context) {
        hreq_context->m_hreq->reply_comm_rest_err(HTTP_INTERNAL, -1, "list task timeout");
    }

    session->clear();
    this->remove_session(session_id);
}

// modify task
void rest_api_service::rest_modify_task(const std::shared_ptr<dbc::network::http_request>& httpReq, const std::string &path) {
    if (httpReq->get_request_method() != dbc::network::http_request::POST) {
        LOG_ERROR << "http request is not post";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_REQUEST, "only support POST request");
        return;
    }

    std::vector<std::string> path_list;
    util::split_path(path, path_list);
    if (path_list.size() != 2) {
        LOG_ERROR << "path_list's size != 2";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "invalid uri, please use /tasks/<task_id>/modify");
        return;
    }

    req_body body;

    body.task_id = path_list[0];
    if (body.task_id.empty()) {
        LOG_ERROR << "task_id is empty";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "task_id is empty");
        return;
    }

    std::string s_body = httpReq->read_body();
    if (s_body.empty()) {
        LOG_ERROR << "http request body is empty";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "http request body is empty");
        return;
    }

    rapidjson::Document doc;
    rapidjson::ParseResult ok = doc.Parse(s_body.c_str());
    if (!ok) {
        std::stringstream ss;
        ss << "json parse error: " << rapidjson::GetParseError_En(ok.Code()) << "(" << ok.Offset() << ")";
        LOG_ERROR << ss.str();
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, ss.str());
        return;
    }

    if (!doc.IsObject()) {
        LOG_ERROR << "invalid json";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "invalid json");
        return;
    }

    // peer_nodes_list
    std::string peer_node_id;
    if (doc.HasMember("peer_nodes_list")) {
        if (doc["peer_nodes_list"].IsArray()) {
            uint32_t list_size = doc["peer_nodes_list"].Size();
            if (list_size < 1) {
                LOG_ERROR << "peer_nodes_list's size < 1";
                httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "peer_nodes_list's size < 1");
                return;
            }

            // 暂时只支持一次操作1个节点
            std::string node(doc["peer_nodes_list"][0].GetString());
            body.peer_nodes_list.push_back(node);
            peer_node_id = node;
        } else {
            LOG_ERROR << "peer_nodes_list is not array";
            httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "peer_nodes_list is not array");
            return;
        }
    } else {
        LOG_ERROR << "has no peer_nodes_list";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "has no peer_nodes_list");
        return;
    }
    // additional
    if (doc.HasMember("additional")) {
        if (doc["additional"].IsObject()) {
            const rapidjson::Value &obj = doc["additional"];
            rapidjson::StringBuffer buffer;
            rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
            obj.Accept(writer);
            body.additional = buffer.GetString();
        } else {
            LOG_ERROR << "additional is not object";
            httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "additional is not object");
            return;
        }
    } else {
        LOG_ERROR << "has no additional";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "has no additional");
        return;
    }
    // sign
    if (doc.HasMember("sign")) {
        if (doc["sign"].IsString()) {
            body.sign = doc["sign"].GetString();
        } else {
            LOG_ERROR << "sign is not string";
            httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "sign is not string");
            return;
        }
    } else {
        LOG_ERROR << "has no sign";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "has no sign");
        return;
    }
    // nonce
    if (doc.HasMember("nonce")) {
        if (doc["nonce"].IsString()) {
            body.nonce = doc["nonce"].GetString();
        } else {
            LOG_ERROR << "nonce is not string";
            httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "nonce is not string");
            return;
        }
    } else {
        LOG_ERROR << "has no nonce";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "has no nonce");
        return;
    }
    // wallet
    if (doc.HasMember("wallet")) {
        if (doc["wallet"].IsString()) {
            body.wallet = doc["wallet"].GetString();
        } else {
            LOG_ERROR << "wallet is not string";
            httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "wallet is not string");
            return;
        }
    } else {
        LOG_ERROR << "has no wallet";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "has no wallet");
        return;
    }
    // session_id
    if (doc.HasMember("session_id")) {
        if (doc["session_id"].IsString()) {
            body.session_id = doc["session_id"].GetString();
        } else {
            LOG_ERROR << "session_id is not string";
            httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "session_id is not string");
            return;
        }
    }
    // session_id_sign
    if (doc.HasMember("session_id_sign")) {
        if (doc["session_id_sign"].IsString()) {
            body.session_id_sign = doc["session_id_sign"].GetString();
        } else {
            LOG_ERROR << "session_id_sign is not string";
            httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "session_id_sign is not string");
            return;
        }
    }
    // pub_key
    body.pub_key = conf_manager::instance().get_pub_key();
    if (body.pub_key.empty()) {
        LOG_ERROR << "pub_key is empty";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "pub_key is empty");
        return;
    }

    std::string head_session_id = util::create_session_id();

    auto node_req_msg = create_node_modify_task_req_msg(head_session_id, body);
    if (nullptr == node_req_msg) {
        LOG_ERROR << "create node request failed";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_RESPONSE_ERROR, "create node request failed");
        return;
    }

    if (E_SUCCESS != create_request_session(NODE_MODIFY_TASK_TIMER, httpReq, node_req_msg, head_session_id, peer_node_id)) {
        LOG_ERROR << "create request session failed";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_RESPONSE_ERROR, "creaate request session failed");
        return;
    }

    if (dbc::network::connection_manager::instance().broadcast_message(node_req_msg) != E_SUCCESS) {
        LOG_ERROR << "broadcast request failed";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_RESPONSE_ERROR, "broadcast request failed");
        return;
    }
}

std::shared_ptr<dbc::network::message> rest_api_service::create_node_modify_task_req_msg(const std::string &head_session_id,
                                                                                         const req_body& body) {
    // 创建 node_ 请求
    std::shared_ptr<dbc::node_modify_task_req> req_content = std::make_shared<dbc::node_modify_task_req>();
    // header
    req_content->header.__set_magic(conf_manager::instance().get_net_flag());
    req_content->header.__set_msg_name(NODE_MODIFY_TASK_REQ);
    req_content->header.__set_nonce(util::create_nonce());
    req_content->header.__set_session_id(head_session_id);
    std::map<std::string, std::string> exten_info;
    exten_info["pub_key"] = body.pub_key;
    exten_info["sign"] = body.sign;
    exten_info["nonce"] = body.nonce;
    req_content->header.__set_exten_info(exten_info);
    std::vector<std::string> path;
    path.push_back(conf_manager::instance().get_node_id());
    req_content->header.__set_path(path);

    // body
    dbc::node_modify_task_req_data req_data;
    req_data.__set_task_id(body.task_id);
    req_data.__set_peer_nodes_list(body.peer_nodes_list);
    req_data.__set_additional(body.additional);
    req_data.__set_wallet(body.wallet);
    req_data.__set_session_id(body.session_id);
    req_data.__set_session_id_sign(body.session_id_sign);

    // encrypt
    std::shared_ptr<byte_buf> out_buf = std::make_shared<byte_buf>();
    dbc::network::binary_protocol proto(out_buf.get());
    req_data.write(&proto);

    dbc::node_service_info service_info;
    bool bfound = service_info_collection::instance().find(body.peer_nodes_list[0], service_info);
    if (bfound) {
        std::string pub_key = service_info.kvs.count("pub_key") ? service_info.kvs["pub_key"] : "";
        std::string priv_key = conf_manager::instance().get_priv_key();

        if (!pub_key.empty() && !priv_key.empty()) {
            std::string s_data = encrypt_data((unsigned char*) out_buf->get_read_ptr(), out_buf->get_valid_read_len(), pub_key, priv_key);
            req_content->body.__set_data(s_data);
        } else {
            LOG_ERROR << "pub_key is empty, node_id:" << body.peer_nodes_list[0];
            return nullptr;
        }
    } else {
        LOG_ERROR << "service_info_collection not found node_id:" << body.peer_nodes_list[0];
        return nullptr;
    }

    std::shared_ptr<dbc::network::message> req_msg = std::make_shared<dbc::network::message>();
    req_msg->set_name(NODE_MODIFY_TASK_REQ);
    req_msg->set_content(req_content);

    return req_msg;
}

void rest_api_service::on_node_modify_task_rsp(const std::shared_ptr<dbc::network::http_request_context>& hreq_context,
                                                  const std::shared_ptr<dbc::network::message>& rsp_msg) {
    auto node_rsp_msg = std::dynamic_pointer_cast<dbc::node_modify_task_rsp>(rsp_msg->content);
    if (!node_rsp_msg) {
        LOG_ERROR << "node_rsp_msg is nullptr";
        return;
    }

    const std::shared_ptr<dbc::network::http_request> &httpReq = hreq_context->m_hreq;

    // decrypt
    std::string pub_key = node_rsp_msg->header.exten_info["pub_key"];
    std::string priv_key = conf_manager::instance().get_priv_key();
    if (pub_key.empty() || priv_key.empty()) {
        httpReq->reply_comm_rest_err(HTTP_INTERNAL, -1, "pub_key or priv_key is empty");
        LOG_ERROR << "pub_key or priv_key is empty";
        return;
    }

    std::string ori_message;
    try {
        bool succ = decrypt_data(node_rsp_msg->body.data, pub_key, priv_key, ori_message);
        if (!succ || ori_message.empty()) {
            httpReq->reply_comm_rest_err(HTTP_INTERNAL, -1, "rsq decrypt error1");
            LOG_ERROR << "rsq decrypt error1";
            return;
        }
    } catch (std::exception &e) {
        httpReq->reply_comm_rest_err(HTTP_INTERNAL, -1, "req decrypt error2");
        LOG_ERROR << "req decrypt error2";
        return;
    }

    rapidjson::Document doc;
    rapidjson::ParseResult ok = doc.Parse(ori_message.c_str());
    if (!ok) {
        httpReq->reply_comm_rest_err(HTTP_INTERNAL, -1, "response parse error");
        LOG_ERROR << "response parse error: " << rapidjson::GetParseError_En(ok.Code()) << "(" << ok.Offset() << ")";
        return;
    }

    httpReq->reply_comm_rest_succ2(ori_message);
}

void rest_api_service::on_node_modify_task_timer(const std::shared_ptr<core_timer> &timer) {
    if (nullptr == timer) {
        LOG_ERROR << "timer is nullptr";
        return;
    }

    const std::string &session_id = timer->get_session_id();
    std::shared_ptr<service_session> session = get_session(session_id);
    if (nullptr == session) {
        LOG_ERROR << "session is nullptr";
        return;
    }

    variables_map &vm = session->get_context().get_args();
    if (0 == vm.count(HTTP_REQUEST_KEY)) {
        LOG_ERROR << "session's context has no HTTP_REQUEST_KEY";
        session->clear();
        this->remove_session(session_id);
        return;
    }

    auto hreq_context = vm[HTTP_REQUEST_KEY].as<std::shared_ptr<dbc::network::http_request_context>>();
    if (nullptr != hreq_context) {
        hreq_context->m_hreq->reply_comm_rest_err(HTTP_INTERNAL, -1, "modify task timeout");
    }

    session->clear();
    this->remove_session(session_id);
}

// task logs
void rest_api_service::rest_task_logs(const std::shared_ptr<dbc::network::http_request>& httpReq, const std::string &path) {
    if (httpReq->get_request_method() != dbc::network::http_request::POST) {
        LOG_ERROR << "http request is not post";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_REQUEST, "only support POST request");
        return;
    }

    std::vector<std::string> path_list;
    util::split_path(path, path_list);
    if (path_list.size() != 2) {
        LOG_ERROR << "path_list's size != 2";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "invalid uri, please use /tasks/{task_id}/trace?flag=tail&line_num=100");
        return;
    }

    req_body body;

    std::map<std::string, std::string> query_table;
    util::split_path_into_kvs(path, query_table);

    std::string head_or_tail = query_table["flag"];
    std::string number_of_lines = query_table["line_num"];

    if (head_or_tail.empty()) {
        head_or_tail = "tail";
        number_of_lines = "100";
    } else if (head_or_tail == "head" || head_or_tail == "tail") {
        if (number_of_lines.empty()) {
            number_of_lines = "100";
        }
    } else {
        LOG_ERROR << "head_or_tail is invalid";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "head_or_tail is invalid\"");
        return;
    }

    // number of lines
    int32_t lines = atoi(number_of_lines);
    if (lines > MAX_NUMBER_OF_LINES) {
        lines = MAX_NUMBER_OF_LINES - 1;
    }
    body.number_of_lines = lines;

    // tail or head
    if (head_or_tail == "tail") {
        body.head_or_tail = GET_LOG_TAIL;
    } else if (head_or_tail == "head") {
        body.head_or_tail = GET_LOG_HEAD;
    } else {
        body.head_or_tail = GET_LOG_TAIL;
    }

    body.task_id = path_list[0];
    if (body.task_id.empty()) {
        LOG_ERROR << "task_id is empty";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "task_id is empty");
        return;
    }

    std::string s_body = httpReq->read_body();
    if (s_body.empty()) {
        LOG_ERROR << "http request body is empty";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "http request body is empty");
        return;
    }

    rapidjson::Document doc;
    rapidjson::ParseResult ok = doc.Parse(s_body.c_str());
    if (!ok) {
        std::stringstream ss;
        ss << "json parse error: " << rapidjson::GetParseError_En(ok.Code()) << "(" << ok.Offset() << ")";
        LOG_ERROR << ss.str();
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, ss.str());
        return;
    }

    if (!doc.IsObject()) {
        LOG_ERROR << "invalid json";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "invalid json");
        return;
    }

    // peer_nodes_list
    std::string peer_node_id;
    if (doc.HasMember("peer_nodes_list")) {
        if (doc["peer_nodes_list"].IsArray()) {
            uint32_t list_size = doc["peer_nodes_list"].Size();
            if (list_size < 1) {
                LOG_ERROR << "peer_nodes_list's size < 1";
                httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "peer_nodes_list's size < 1");
                return;
            }

            // 暂时只支持一次操作1个节点
            std::string node(doc["peer_nodes_list"][0].GetString());
            body.peer_nodes_list.push_back(node);
            peer_node_id = node;
        } else {
            LOG_ERROR << "peer_nodes_list is not array";
            httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "peer_nodes_list is not array");
            return;
        }
    } else {
        LOG_ERROR << "has no peer_nodes_list";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "has no peer_nodes_list");
        return;
    }
    // additional
    if (doc.HasMember("additional")) {
        if (doc["additional"].IsObject()) {
            const rapidjson::Value &obj = doc["additional"];
            rapidjson::StringBuffer buffer;
            rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
            obj.Accept(writer);
            body.additional = buffer.GetString();
        } else {
            LOG_ERROR << "additional is not object";
            httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "additional is not object");
            return;
        }
    } else {
        LOG_ERROR << "has no additional";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "has no additional");
        return;
    }
    // sign
    if (doc.HasMember("sign")) {
        if (doc["sign"].IsString()) {
            body.sign = doc["sign"].GetString();
        } else {
            LOG_ERROR << "sign is not string";
            httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "sign is not string");
            return;
        }
    } else {
        LOG_ERROR << "has no sign";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "has no sign");
        return;
    }
    // nonce
    if (doc.HasMember("nonce")) {
        if (doc["nonce"].IsString()) {
            body.nonce = doc["nonce"].GetString();
        } else {
            LOG_ERROR << "nonce is not string";
            httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "nonce is not string");
            return;
        }
    } else {
        LOG_ERROR << "has no nonce";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "has no <nonce");
        return;
    }
    // wallet
    if (doc.HasMember("wallet")) {
        if (doc["wallet"].IsString()) {
            body.wallet = doc["wallet"].GetString();
        } else {
            LOG_ERROR << "wallet is not string";
            httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "wallet is not string");
            return;
        }
    } else {
        LOG_ERROR << "has no wallet";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "has no wallet");
        return;
    }
    // session_id
    if (doc.HasMember("session_id")) {
        if (doc["session_id"].IsString()) {
            body.session_id = doc["session_id"].GetString();
        } else {
            LOG_ERROR << "session_id is not string";
            httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "session_id is not string");
            return;
        }
    }
    // session_id_sign
    if (doc.HasMember("session_id_sign")) {
        if (doc["session_id_sign"].IsString()) {
            body.session_id_sign = doc["session_id_sign"].GetString();
        } else {
            LOG_ERROR << "session_id_sign is not string";
            httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "session_id_sign is not string");
            return;
        }
    }
    // pub_key
    body.pub_key = conf_manager::instance().get_pub_key();
    if (body.pub_key.empty()) {
        LOG_ERROR << "pub_key is empty";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "pub_key is empty");
        return;
    }

    std::string head_session_id = util::create_session_id();

    auto node_req_msg = create_node_task_logs_req_msg(head_session_id, body);
    if (nullptr == node_req_msg) {
        LOG_ERROR << "create node request failed";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_RESPONSE_ERROR, "create node request failed");
        return;
    }

    if (E_SUCCESS != create_request_session(NODE_TASK_LOGS_TIMER, httpReq, node_req_msg, head_session_id, peer_node_id)) {
        LOG_ERROR << "create request session failed";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_RESPONSE_ERROR, "creaate request session failed");
        return;
    }

    if (dbc::network::connection_manager::instance().broadcast_message(node_req_msg) != E_SUCCESS) {
        LOG_ERROR << "broadcast request failed";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_RESPONSE_ERROR, "broadcast request failed");
        return;
    }
}

std::shared_ptr<dbc::network::message> rest_api_service::create_node_task_logs_req_msg(const std::string &head_session_id,
                                                   const req_body& body) {
    auto req_content = std::make_shared<dbc::node_task_logs_req>();
    //header
    req_content->header.__set_magic(conf_manager::instance().get_net_flag());
    req_content->header.__set_msg_name(NODE_TASK_LOGS_REQ);
    req_content->header.__set_nonce(util::create_nonce());
    req_content->header.__set_session_id(head_session_id);
    std::map<std::string, std::string> exten_info;
    exten_info["pub_key"] = body.pub_key;
    exten_info["sign"] = body.sign;
    exten_info["nonce"] = body.nonce;
    req_content->header.__set_exten_info(exten_info);
    std::vector<std::string> path;
    path.push_back(conf_manager::instance().get_node_id());
    req_content->header.__set_path(path);

    //body
    dbc::node_task_logs_req_data req_data;
    req_data.__set_task_id(body.task_id);
    req_data.__set_head_or_tail(body.head_or_tail);
    req_data.__set_number_of_lines(body.number_of_lines);
    req_data.__set_peer_nodes_list(body.peer_nodes_list);
    req_data.__set_additional(body.additional);
    req_data.__set_wallet(body.wallet);
    req_data.__set_session_id(body.session_id);
    req_data.__set_session_id_sign(body.session_id_sign);

    // encrypt
    std::shared_ptr<byte_buf> out_buf = std::make_shared<byte_buf>();
    dbc::network::binary_protocol proto(out_buf.get());
    req_data.write(&proto);

    dbc::node_service_info service_info;
    bool bfound = service_info_collection::instance().find(body.peer_nodes_list[0], service_info);
    if (bfound) {
        std::string pub_key = service_info.kvs.count("pub_key") ? service_info.kvs["pub_key"] : "";
        std::string priv_key = conf_manager::instance().get_priv_key();

        if (!pub_key.empty() && !priv_key.empty()) {
            std::string s_data = encrypt_data((unsigned char*) out_buf->get_read_ptr(), out_buf->get_valid_read_len(), pub_key, priv_key);
            req_content->body.__set_data(s_data);
        } else {
            LOG_ERROR << "pub_key is empty, node_id:" << body.peer_nodes_list[0];
            return nullptr;
        }
    } else {
        LOG_ERROR << "service_info_collection not found node_id:" << body.peer_nodes_list[0];
        return nullptr;
    }

    std::shared_ptr<dbc::network::message> req_msg = std::make_shared<dbc::network::message>();
    req_msg->set_name(NODE_TASK_LOGS_REQ);
    req_msg->set_content(req_content);

    return req_msg;
}

void rest_api_service::on_node_task_logs_rsp(const std::shared_ptr<dbc::network::http_request_context>& hreq_context,
                                                const std::shared_ptr<dbc::network::message>& rsp_msg) {
    auto node_rsp_msg = std::dynamic_pointer_cast<dbc::node_task_logs_rsp>(rsp_msg->content);
    if (!node_rsp_msg) {
        LOG_ERROR << "node_rsp_msg is nullptr";
        return;
    }

    const std::shared_ptr<dbc::network::http_request> &httpReq = hreq_context->m_hreq;

    // decrypt
    std::string pub_key = node_rsp_msg->header.exten_info["pub_key"];
    std::string priv_key = conf_manager::instance().get_priv_key();
    if (pub_key.empty() || priv_key.empty()) {
        httpReq->reply_comm_rest_err(HTTP_INTERNAL, -1, "pub_key or priv_key is empty");
        LOG_ERROR << "pub_key or priv_key is empty";
        return;
    }

    std::string ori_message;
    try {
        bool succ = decrypt_data(node_rsp_msg->body.data, pub_key, priv_key, ori_message);
        if (!succ || ori_message.empty()) {
            httpReq->reply_comm_rest_err(HTTP_INTERNAL, -1, "rsq decrypt error1");
            LOG_ERROR << "rsq decrypt error1";
            return;
        }
    } catch (std::exception &e) {
        httpReq->reply_comm_rest_err(HTTP_INTERNAL, -1, "req decrypt error2");
        LOG_ERROR << "req decrypt error2";
        return;
    }

    rapidjson::Document doc;
    rapidjson::ParseResult ok = doc.Parse(ori_message.c_str());
    if (!ok) {
        httpReq->reply_comm_rest_err(HTTP_INTERNAL, -1, "response parse error");
        LOG_ERROR << "response parse error: " << rapidjson::GetParseError_En(ok.Code()) << "(" << ok.Offset() << ")";
        return;
    }

    httpReq->reply_comm_rest_succ2(ori_message);
}

void rest_api_service::on_node_task_logs_timer(const std::shared_ptr<core_timer> &timer) {
    if (nullptr == timer) {
        LOG_ERROR << "timer is nullptr";
        return;
    }

    const std::string &session_id = timer->get_session_id();
    std::shared_ptr<service_session> session = get_session(session_id);
    if (nullptr == session) {
        LOG_ERROR << "session is nullptr";
        return;
    }

    variables_map &vm = session->get_context().get_args();
    if (0 == vm.count(HTTP_REQUEST_KEY)) {
        LOG_ERROR << "session's context has no HTTP_REQUEST_KEY";
        session->clear();
        this->remove_session(session_id);
        return;
    }

    auto hreq_context = vm[HTTP_REQUEST_KEY].as<std::shared_ptr<dbc::network::http_request_context>>();
    if (nullptr != hreq_context) {
        hreq_context->m_hreq->reply_comm_rest_err(HTTP_INTERNAL, -1, "task logs timeout");
    }

    session->clear();
    this->remove_session(session_id);
}

// /mining_nodes
void rest_api_service::rest_mining_nodes(const std::shared_ptr<dbc::network::http_request>& httpReq, const std::string &path) {
    std::vector<std::string> path_list;
    util::split_path(path, path_list);

    if (path_list.empty()) {
        rest_list_mining_nodes(httpReq, path);
        return;
    }

    if (path_list.size() == 1) {
        const std::string &first_param = path_list[0];
        if (first_param == "session_id") {
            rest_node_session_id(httpReq, path);
        }
        return;
    }

    httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_REQUEST, "invalid requests uri");
}

// list mining node
void reply_node_list(const std::shared_ptr<std::map<std::string, dbc::node_service_info>> &service_list,
                     std::string &data_json) {
    std::stringstream ss;
    ss << "{";
    ss << "\"error_code\":0";
    ss << ", \"data\":{";
    ss << "\"mining_nodes\":[";
    int service_count = 0;
    for (auto &it : *service_list) {
        if (service_count > 0) ss << ",";
        ss << "{";
        std::string service_list;
        for (int i = 0; i < it.second.service_list.size(); i++) {
            if (i > 0) service_list += ",";
            service_list += it.second.service_list[i];
        }
        ss << "\"service_list\":" << "\"" << service_list << "\"";
        std::string node_id = it.first;
        node_id = util::rtrim(node_id, '\n');
        ss << ",\"node_id\":" << "\"" << node_id << "\"";
        ss << ",\"name\":" << "\"" << it.second.name << "\"";
        std::string ver = it.second.kvs.count("version") ? it.second.kvs["version"] : "N/A";
        ss << ",\"version\":" << "\"" << ver << "\"";
        //ss << ",\"state\":" << "\"" << it.second.kvs["state"] << "\"";
        ss << "}";

        service_count++;
    }
    ss << "]}";
    ss << "}";

    data_json = ss.str();
}

void rest_api_service::rest_list_mining_nodes(const std::shared_ptr<dbc::network::http_request>& httpReq, const std::string &path) {
    if (httpReq->get_request_method() != dbc::network::http_request::POST) {
        LOG_ERROR << "http request is not post";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_REQUEST, "only support POST request");
        return;
    }

    std::vector<std::string> path_list;
    util::split_path(path, path_list);
    if (!path_list.empty()) {
        LOG_ERROR << "path_list's size != 0";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "invalid uri, please use /mining_nodes");
        return;
    }

    req_body body;

    std::string s_body = httpReq->read_body();
    if (s_body.empty()) {
        LOG_ERROR << "http request body is empty";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "http request body is empty");
        return;
    }

    rapidjson::Document doc;
    rapidjson::ParseResult ok = doc.Parse(s_body.c_str());
    if (!ok) {
        std::stringstream ss;
        ss << "json parse error: " << rapidjson::GetParseError_En(ok.Code()) << "(" << ok.Offset() << ")";
        LOG_ERROR << ss.str();
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, ss.str());
        return;
    }

    if (!doc.IsObject()) {
        LOG_ERROR << "invalid json";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "invalid json");
        return;
    }

    // peer_nodes_list
    std::string peer_node_id;
    if (doc.HasMember("peer_nodes_list")) {
        if (doc["peer_nodes_list"].IsArray()) {
            uint32_t list_size = doc["peer_nodes_list"].Size();
            if (list_size > 0) {
                // 暂时只支持一次操作1个节点
                std::string node(doc["peer_nodes_list"][0].GetString());
                body.peer_nodes_list.push_back(node);
                peer_node_id = node;
            }
        } else {
            LOG_ERROR << "peer_nodes_list is not array";
            httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "peer_nodes_list is not array");
            return;
        }
    } else {
        LOG_ERROR << "has no peer_nodes_list";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "has no peer_nodes_list");
        return;
    }
    // additional
    if (doc.HasMember("additional")) {
        if (doc["additional"].IsObject()) {
            const rapidjson::Value &obj = doc["additional"];
            rapidjson::StringBuffer buffer;
            rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
            obj.Accept(writer);
            body.additional = buffer.GetString();
        } else {
            LOG_ERROR << "additional is not object";
            httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "additional is not object");
            return;
        }
    } else {
        LOG_ERROR << "has no additional";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "has no additional");
        return;
    }
    // sign
    if (doc.HasMember("sign")) {
        if (doc["sign"].IsString()) {
            body.sign = doc["sign"].GetString();
        } else {
            LOG_ERROR << "sign is not string";
            httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "sign is not string");
            return;
        }
    } else {
        LOG_ERROR << "has no sign";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "has no sign");
        return;
    }
    // nonce
    if (doc.HasMember("nonce")) {
        if (doc["nonce"].IsString()) {
            body.nonce = doc["nonce"].GetString();
        } else {
            LOG_ERROR << "nonce is not string";
            httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "nonce is not string");
            return;
        }
    } else {
        LOG_ERROR << "has no nonce";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "has no nonce");
        return;
    }
    // wallet
    if (doc.HasMember("wallet")) {
        if (doc["wallet"].IsString()) {
            body.wallet = doc["wallet"].GetString();
        } else {
            LOG_ERROR << "wallet is not string";
            httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "wallet is not string");
            return;
        }
    } else {
        LOG_ERROR << "has no wallet";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "has no wallet");
        return;
    }
    // session_id
    if (doc.HasMember("session_id")) {
        if (doc["session_id"].IsString()) {
            body.session_id = doc["session_id"].GetString();
        } else {
            LOG_ERROR << "session_id is not string";
            httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "session_id is not string");
            return;
        }
    }
    // session_id_sign
    if (doc.HasMember("session_id_sign")) {
        if (doc["session_id_sign"].IsString()) {
            body.session_id_sign = doc["session_id_sign"].GetString();
        } else {
            LOG_ERROR << "session_id_sign is not string";
            httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "session_id_sign is not string");
            return;
        }
    }
    // pub_key
    body.pub_key = conf_manager::instance().get_pub_key();
    if (body.pub_key.empty()) {
        LOG_ERROR << "pub_key is empty";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "pub_key is empty");
        return;
    }

    // peer_nodes_list
    if (body.peer_nodes_list.empty()) {
        std::shared_ptr<std::map<std::string, dbc::node_service_info>> id_2_services =
                service_info_collection::instance().get_all();
        std::string data_json;
        reply_node_list(id_2_services, data_json);
        httpReq->reply_comm_rest_succ2(data_json);
    } else {
        std::string head_session_id = util::create_session_id();

        auto node_req_msg = create_node_query_node_info_req_msg(head_session_id, body);
        if (nullptr == node_req_msg) {
            LOG_ERROR << "create node request failed";
            httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_RESPONSE_ERROR, "create node request failed");
            return;
        }

        if (E_SUCCESS != create_request_session(NODE_QUERY_NODE_INFO_TIMER, httpReq, node_req_msg, head_session_id, peer_node_id)) {
            LOG_ERROR << "create request session failed";
            httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_RESPONSE_ERROR, "creaate request session failed");
            return;
        }

        if (dbc::network::connection_manager::instance().broadcast_message(node_req_msg) != E_SUCCESS) {
            LOG_ERROR << "broadcast request failed";
            httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_RESPONSE_ERROR, "broadcast request failed");
            return;
        }
    }
}

std::shared_ptr<dbc::network::message> rest_api_service::create_node_query_node_info_req_msg(const std::string &head_session_id,
                                                                                             const req_body& body) {
    auto req_content = std::make_shared<dbc::node_query_node_info_req>();
    // header
    req_content->header.__set_magic(conf_manager::instance().get_net_flag());
    req_content->header.__set_msg_name(NODE_QUERY_NODE_INFO_REQ);
    req_content->header.__set_nonce(util::create_nonce());
    req_content->header.__set_session_id(head_session_id);
    std::map<std::string, std::string> exten_info;
    exten_info["pub_key"] = body.pub_key;
    exten_info["sign"] = body.sign;
    exten_info["nonce"] = body.nonce;
    req_content->header.__set_exten_info(exten_info);
    std::vector<std::string> path;
    path.push_back(conf_manager::instance().get_node_id());
    req_content->header.__set_path(path);

    // body
    dbc::node_query_node_info_req_data req_data;
    req_data.__set_peer_nodes_list(body.peer_nodes_list);
    req_data.__set_additional(body.additional);
    req_data.__set_wallet(body.wallet);
    req_data.__set_session_id(body.session_id);
    req_data.__set_session_id_sign(body.session_id_sign);

    // encrypt
    std::shared_ptr<byte_buf> out_buf = std::make_shared<byte_buf>();
    dbc::network::binary_protocol proto(out_buf.get());
    req_data.write(&proto);

    dbc::node_service_info service_info;
    bool bfound = service_info_collection::instance().find(body.peer_nodes_list[0], service_info);
    if (bfound) {
        std::string pub_key = service_info.kvs.count("pub_key") ? service_info.kvs["pub_key"] : "";
        std::string priv_key = conf_manager::instance().get_priv_key();

        if (!pub_key.empty() && !priv_key.empty()) {
            std::string s_data = encrypt_data((unsigned char*) out_buf->get_read_ptr(), out_buf->get_valid_read_len(), pub_key, priv_key);
            req_content->body.__set_data(s_data);
        } else {
            LOG_ERROR << "pub_key is empty, node_id:" << body.peer_nodes_list[0];
            return nullptr;
        }
    } else {
        LOG_ERROR << "service_info_collection not found node_id:" << body.peer_nodes_list[0];
        return nullptr;
    }

    auto req_msg = std::make_shared<dbc::network::message>();
    req_msg->set_name(NODE_QUERY_NODE_INFO_REQ);
    req_msg->set_content(req_content);

    return req_msg;
}

void rest_api_service::on_node_query_node_info_rsp(const std::shared_ptr<dbc::network::http_request_context>& hreq_context,
                                                      const std::shared_ptr<dbc::network::message>& rsp_msg) {
    auto node_rsp_msg = std::dynamic_pointer_cast<dbc::node_query_node_info_rsp>(rsp_msg->content);
    if (!node_rsp_msg) {
        LOG_ERROR << "node_rsp_msg is nullptr";
        return;
    }

    const std::shared_ptr<dbc::network::http_request> &httpReq = hreq_context->m_hreq;

    // decrypt
    std::string pub_key = node_rsp_msg->header.exten_info["pub_key"];
    std::string priv_key = conf_manager::instance().get_priv_key();
    if (pub_key.empty() || priv_key.empty()) {
        httpReq->reply_comm_rest_err(HTTP_INTERNAL, -1, "pub_key or priv_key is empty");
        LOG_ERROR << "pub_key or priv_key is empty";
        return;
    }

    std::string ori_message;
    try {
        bool succ = decrypt_data(node_rsp_msg->body.data, pub_key, priv_key, ori_message);
        if (!succ || ori_message.empty()) {
            httpReq->reply_comm_rest_err(HTTP_INTERNAL, -1, "rsq decrypt error1");
            LOG_ERROR << "rsq decrypt error1";
            return;
        }
    } catch (std::exception &e) {
        httpReq->reply_comm_rest_err(HTTP_INTERNAL, -1, "rsq decrypt error2");
        LOG_ERROR << "rsq decrypt error2";
        return;
    }

    rapidjson::Document doc;
    rapidjson::ParseResult ok = doc.Parse(ori_message.c_str());
    if (!ok) {
        httpReq->reply_comm_rest_err(HTTP_INTERNAL, -1, "response parse error");
        LOG_ERROR << "response parse error: " << rapidjson::GetParseError_En(ok.Code()) << "(" << ok.Offset() << ")";
        return;
    }

    httpReq->reply_comm_rest_succ2(ori_message);
}

void rest_api_service::on_node_query_node_info_timer(const std::shared_ptr<core_timer>& timer)
{
    if (nullptr == timer) {
        LOG_ERROR << "timer is nullptr";
        return;
    }

    const std::string &session_id = timer->get_session_id();
    std::shared_ptr<service_session> session = get_session(session_id);
    if (nullptr == session) {
        LOG_ERROR << "session is nullptr";
        return;
    }

    variables_map &vm = session->get_context().get_args();
    if (0 == vm.count(HTTP_REQUEST_KEY)) {
        LOG_ERROR << "session's context has no HTTP_REQUEST_KEY";
        session->clear();
        this->remove_session(session_id);
        return;
    }

    auto hreq_context = vm[HTTP_REQUEST_KEY].as<std::shared_ptr<dbc::network::http_request_context>>();
    if (nullptr != hreq_context) {
        hreq_context->m_hreq->reply_comm_rest_err(HTTP_INTERNAL, -1, "query node info timeout");
    }

    session->clear();
    this->remove_session(session_id);
}

// node session_id
void rest_api_service::rest_node_session_id(const std::shared_ptr<dbc::network::http_request> &httpReq,
                                            const std::string &path) {
    if (httpReq->get_request_method() != dbc::network::http_request::POST) {
        LOG_ERROR << "http request is not post";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_REQUEST, "only support POST request");
        return;
    }

    std::vector<std::string> path_list;
    util::split_path(path, path_list);
    if (path_list.size() != 1) {
        LOG_ERROR << "path_list's size != 1";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "invalid uri, please use /mining_nodes/session_id");
        return;
    }

    req_body body;

    std::string s_body = httpReq->read_body();
    if (s_body.empty()) {
        LOG_ERROR << "http request body is empty";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "http request body is empty");
        return;
    }

    rapidjson::Document doc;
    rapidjson::ParseResult ok = doc.Parse(s_body.c_str());
    if (!ok) {
        std::stringstream ss;
        ss << "json parse error: " << rapidjson::GetParseError_En(ok.Code()) << "(" << ok.Offset() << ")";
        LOG_ERROR << ss.str();
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, ss.str());
        return;
    }

    if (!doc.IsObject()) {
        LOG_ERROR << "invalid json";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "invalid json");
        return;
    }

    // peer_nodes_list
    std::string peer_node_id;
    if (doc.HasMember("peer_nodes_list")) {
        if (doc["peer_nodes_list"].IsArray()) {
            uint32_t list_size = doc["peer_nodes_list"].Size();
            if (list_size < 1) {
                LOG_ERROR << "peer_nodes_list's size < 1";
                httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "peer_nodes_list's size < 1");
                return;
            }

            // 暂时只支持一次操作1个节点
            std::string node(doc["peer_nodes_list"][0].GetString());
            body.peer_nodes_list.push_back(node);
            peer_node_id = node;
        } else {
            LOG_ERROR << "peer_nodes_list is not array";
            httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "peer_nodes_list is not array");
            return;
        }
    } else {
        LOG_ERROR << "has no peer_nodes_list";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "has no peer_nodes_list");
        return;
    }
    // additional
    if (doc.HasMember("additional")) {
        if (doc["additional"].IsObject()) {
            const rapidjson::Value &obj = doc["additional"];
            rapidjson::StringBuffer buffer;
            rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
            obj.Accept(writer);
            body.additional = buffer.GetString();
        } else {
            LOG_ERROR << "additional is not object";
            httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "additional is not object");
            return;
        }
    } else {
        LOG_ERROR << "has no additional";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "has no additional");
        return;
    }
    // sign
    if (doc.HasMember("sign")) {
        if (doc["sign"].IsString()) {
            body.sign = doc["sign"].GetString();
        } else {
            LOG_ERROR << "sign is not string";
            httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "sign is not string");
            return;
        }
    } else {
        LOG_ERROR << "has no sign";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "has no sign");
        return;
    }
    // nonce
    if (doc.HasMember("nonce")) {
        if (doc["nonce"].IsString()) {
            body.nonce = doc["nonce"].GetString();
        } else {
            LOG_ERROR << "nonce is not string";
            httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "nonce is not string");
            return;
        }
    } else {
        LOG_ERROR << "has no nonce";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "has no nonce");
        return;
    }
    // wallet
    if (doc.HasMember("wallet")) {
        if (doc["wallet"].IsString()) {
            body.wallet = doc["wallet"].GetString();
        } else {
            LOG_ERROR << "wallet is not string";
            httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "wallet is not string");
            return;
        }
    } else {
        LOG_ERROR << "has no wallet";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "has no wallet");
        return;
    }
    // session_id
    if (doc.HasMember("session_id")) {
        if (doc["session_id"].IsString()) {
            body.session_id = doc["session_id"].GetString();
        } else {
            LOG_ERROR << "session_id is not string";
            httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "session_id is not string");
            return;
        }
    }
    // session_id_sign
    if (doc.HasMember("session_id_sign")) {
        if (doc["session_id_sign"].IsString()) {
            body.session_id_sign = doc["session_id_sign"].GetString();
        } else {
            LOG_ERROR << "session_id_sign is not string";
            httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "session_id_sign is not string");
            return;
        }
    }
    // pub_key
    body.pub_key = conf_manager::instance().get_pub_key();
    if (body.pub_key.empty()) {
        LOG_ERROR << "pub_key is empty";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "pub_key is empty");
        return;
    }

    std::string head_session_id = util::create_session_id();

    auto node_req_msg = create_node_session_id_req_msg(head_session_id, body);
    if (nullptr == node_req_msg) {
        LOG_ERROR << "create node request failed";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_RESPONSE_ERROR, "create node request failed");
        return;
    }

    if (E_SUCCESS != create_request_session(NODE_SESSION_ID_TIMER, httpReq, node_req_msg, head_session_id, peer_node_id)) {
        LOG_ERROR << "create request session failed";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_RESPONSE_ERROR, "creaate request session failed");
        return;
    }

    if (dbc::network::connection_manager::instance().broadcast_message(node_req_msg) != E_SUCCESS) {
        LOG_ERROR << "broadcast request failed";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_RESPONSE_ERROR, "broadcast request failed");
        return;
    }
}

std::shared_ptr<dbc::network::message> rest_api_service::create_node_session_id_req_msg(const std::string &head_session_id,
                                                                                        const req_body &body) {
    // 创建 node_ 请求
    std::shared_ptr<dbc::node_session_id_req> req_content = std::make_shared<dbc::node_session_id_req>();
    // header
    req_content->header.__set_magic(conf_manager::instance().get_net_flag());
    req_content->header.__set_msg_name(NODE_SESSION_ID_REQ);
    req_content->header.__set_nonce(util::create_nonce());
    req_content->header.__set_session_id(head_session_id);
    std::map<std::string, std::string> exten_info;
    exten_info["pub_key"] = body.pub_key;
    exten_info["sign"] = body.sign;
    exten_info["nonce"] = body.nonce;
    req_content->header.__set_exten_info(exten_info);
    std::vector<std::string> path;
    path.push_back(conf_manager::instance().get_node_id());
    req_content->header.__set_path(path);

    // body
    dbc::node_session_id_req_data req_data;
    req_data.__set_peer_nodes_list(body.peer_nodes_list);
    req_data.__set_additional(body.additional);
    req_data.__set_wallet(body.wallet);
    req_data.__set_session_id(body.session_id);
    req_data.__set_session_id_sign(body.session_id_sign);

    // encrypt
    std::shared_ptr<byte_buf> out_buf = std::make_shared<byte_buf>();
    dbc::network::binary_protocol proto(out_buf.get());
    req_data.write(&proto);

    dbc::node_service_info service_info;
    bool bfound = service_info_collection::instance().find(body.peer_nodes_list[0], service_info);
    if (bfound) {
        std::string pub_key = service_info.kvs.count("pub_key") ? service_info.kvs["pub_key"] : "";
        std::string priv_key = conf_manager::instance().get_priv_key();

        if (!pub_key.empty() && !priv_key.empty()) {
            std::string s_data = encrypt_data((unsigned char*) out_buf->get_read_ptr(), out_buf->get_valid_read_len(), pub_key, priv_key);
            req_content->body.__set_data(s_data);
        } else {
            LOG_ERROR << "pub_key is empty, node_id:" << body.peer_nodes_list[0];
            return nullptr;
        }
    } else {
        LOG_ERROR << "service_info_collection not found node_id:" << body.peer_nodes_list[0];
        return nullptr;
    }

    std::shared_ptr<dbc::network::message> req_msg = std::make_shared<dbc::network::message>();
    req_msg->set_name(NODE_SESSION_ID_REQ);
    req_msg->set_content(req_content);

    return req_msg;
}

void rest_api_service::on_node_session_id_rsp(const std::shared_ptr<dbc::network::http_request_context> &hreq_context,
                                              const std::shared_ptr<dbc::network::message> &rsp_msg) {
    auto node_rsp_msg = std::dynamic_pointer_cast<dbc::node_session_id_rsp>(rsp_msg->content);
    if (!node_rsp_msg) {
        LOG_ERROR << "rsp is nullptr";
        return;
    }

    const std::shared_ptr<dbc::network::http_request> &httpReq = hreq_context->m_hreq;

    // decrypt
    std::string pub_key = node_rsp_msg->header.exten_info["pub_key"];
    std::string priv_key = conf_manager::instance().get_priv_key();
    if (pub_key.empty() || priv_key.empty()) {
        httpReq->reply_comm_rest_err(HTTP_INTERNAL, -1, "pub_key or priv_key is empty");
        LOG_ERROR << "pub_key or priv_key is empty";
        return;
    }

    std::string ori_message;
    try {
        bool succ = decrypt_data(node_rsp_msg->body.data, pub_key, priv_key, ori_message);
        if (!succ || ori_message.empty()) {
            httpReq->reply_comm_rest_err(HTTP_INTERNAL, -1, "rsq decrypt error1");
            LOG_ERROR << "rsq decrypt error1";
            return;
        }
    } catch (std::exception &e) {
        httpReq->reply_comm_rest_err(HTTP_INTERNAL, -1, "rsq decrypt error2");
        LOG_ERROR << "rsq decrypt error2";
        return;
    }

    rapidjson::Document doc;
    rapidjson::ParseResult ok = doc.Parse(ori_message.c_str());
    if (!ok) {
        httpReq->reply_comm_rest_err(HTTP_INTERNAL, -1, "response parse error");
        LOG_ERROR << "response parse error: " << rapidjson::GetParseError_En(ok.Code()) << "(" << ok.Offset() << ")";
        return;
    }

    httpReq->reply_comm_rest_succ2(ori_message);
}

void rest_api_service::on_node_session_id_timer(const std::shared_ptr<core_timer> &timer) {
    if (nullptr == timer) {
        LOG_ERROR << "timer is nullptr";
        return;
    }

    const std::string &session_id = timer->get_session_id();
    std::shared_ptr<service_session> session = get_session(session_id);
    if (nullptr == session) {
        LOG_ERROR << "session is nullptr";
        return;
    }

    variables_map &vm = session->get_context().get_args();
    if (0 == vm.count(HTTP_REQUEST_KEY)) {
        LOG_ERROR << "session's context has no HTTP_REQUEST_KEY";
        session->clear();
        this->remove_session(session_id);
        return;
    }

    auto hreq_context = vm[HTTP_REQUEST_KEY].as<std::shared_ptr<dbc::network::http_request_context>>();
    if (nullptr != hreq_context) {
        hreq_context->m_hreq->reply_comm_rest_err(HTTP_INTERNAL, -1, "get session_id timeout");
    }

    session->clear();
    this->remove_session(session_id);
}

// /peers/
void rest_api_service::rest_peers(const std::shared_ptr<dbc::network::http_request>& httpReq, const std::string &path) {
    std::vector<std::string> path_list;
    util::split_path(path, path_list);

    if (path_list.size() == 1) {
        const std::string &sOption = path_list[0];
        rest_get_peer_nodes(httpReq, path);
        return;
    }

    httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_REQUEST, "invalid requests uri");
}

void reply_peer_nodes_list(const std::list<cmd_peer_node_info> &peer_nodes_list, std::string &data_json) {
    std::stringstream ss;
    ss << "{";
    ss << "\"error_code\":0";
    ss << ", \"data\":{";
    ss << "\"peer_nodes\":[";
    int peers_count = 0;
    for (auto& iter : peer_nodes_list) {
        if (peers_count > 0) ss << ",";
        ss << "{";
        ss << "\"node_id\":" << "\"" << iter.peer_node_id << "\"";
        ss << ", \"addr\":" << "\"" << iter.addr.ip << ":" << iter.addr.port << "\"";
        ss << "}";
    }
    ss << "]}";
    ss << "}";
    data_json = ss.str();
}

void rest_api_service::rest_get_peer_nodes(const std::shared_ptr<dbc::network::http_request>& httpReq, const std::string &path) {
    if (httpReq->get_request_method() != dbc::network::http_request::POST) {
        LOG_ERROR << "http request is not post";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_REQUEST, "only support POST request");
        return;
    }

    std::vector<std::string> path_list;
    util::split_path(path, path_list);
    if (path_list.size() != 1) {
        LOG_ERROR << "path_list's size != 1";
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "invalid uri, please use /peers/active or /peers/global");
        return;
    }

    req_body body;
    body.option = path_list[0];
    if (body.option == "active") {
        body.flag = flag_active;
        std::unordered_map<std::string, std::shared_ptr<peer_node>> peer_nodes_map =
                p2p_net_service::instance().get_peer_nodes_map();
        std::list<cmd_peer_node_info> peer_nodes_list;
        for (auto itn = peer_nodes_map.begin(); itn != peer_nodes_map.end(); ++itn) {
            if (conf_manager::instance().get_node_id() == itn->second->m_id ||
                SystemInfo::instance().get_publicip() == itn->second->m_peer_addr.get_ip())
                continue;

            cmd_peer_node_info node_info;
            node_info.peer_node_id = itn->second->m_id;
            node_info.live_time_stamp = itn->second->m_live_time;
            node_info.addr.ip = itn->second->m_peer_addr.get_ip();
            node_info.addr.port = itn->second->m_peer_addr.get_port();
            node_info.node_type = (int8_t)itn->second->m_node_type;
            node_info.service_list.clear();
            node_info.service_list.push_back(std::string("ai_training"));
            peer_nodes_list.push_back(std::move(node_info));
        }

        std::string data_json;
        reply_peer_nodes_list(peer_nodes_list, data_json);
        httpReq->reply_comm_rest_succ2(data_json);
    } else if (body.option == "global") {
        body.flag = flag_global;
        std::list<std::shared_ptr<peer_candidate>> peer_candidates =
                p2p_net_service::instance().get_peer_candidates();
        std::list<cmd_peer_node_info> peer_nodes_list;
        for (auto it = peer_candidates.begin(); it != peer_candidates.end(); ++it) {
            if (conf_manager::instance().get_node_id() == (*it)->node_id ||
                SystemInfo::instance().get_publicip() == (*it)->tcp_ep.address().to_string())
                continue;

            cmd_peer_node_info node_info;
            node_info.peer_node_id = (*it)->node_id;
            node_info.live_time_stamp = 0;
            node_info.net_st = (int8_t)(*it)->net_st;
            node_info.addr.ip = (*it)->tcp_ep.address().to_string();
            node_info.addr.port = (*it)->tcp_ep.port();
            node_info.node_type = (int8_t)(*it)->node_type;
            node_info.service_list.clear();
            node_info.service_list.push_back(std::string("ai_training"));
            peer_nodes_list.push_back(std::move(node_info));
        }

        std::string data_json;
        reply_peer_nodes_list(peer_nodes_list, data_json);
        httpReq->reply_comm_rest_succ2(data_json);
    } else {
        httpReq->reply_comm_rest_err(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "invalid option. the option is active or global");
    }
}

// /stat/
void rest_api_service::rest_stat(const std::shared_ptr<dbc::network::http_request>& httpReq, const std::string &path) {
    /*
    rapidjson::Document document;
    rapidjson::Document::AllocatorType &allocator = document.GetAllocator();

    rapidjson::Value data(rapidjson::kObjectType);
    std::string node_id = conf_manager::instance().get_node_id();

    data.AddMember("node_id", STRING_REF(node_id), allocator);
    data.AddMember("session_count", rest_api_service::instance().get_session_count(), allocator);
    data.AddMember("startup_time", rest_api_service::instance().get_startup_time(), allocator);
    SUCC_REPLY(data)
    return nullptr;
    */
}

// /config/
void rest_api_service::rest_config(const std::shared_ptr<dbc::network::http_request>& httpReq, const std::string &path) {
    /*
    std::vector<std::string> path_list;
    util::split_path(path, path_list);

    if (path_list.empty()) {

        std::string body = httpReq->read_body();
        if (body.empty()) {
            ERROR_REPLY(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "Invalid body. Use /api/v1/config")
            return nullptr;
        }

        rapidjson::Document document;
        try {
            document.Parse(body.c_str());
            if (!document.IsObject()) {
                ERROR_REPLY(HTTP_BADREQUEST, RPC_INVALID_PARAMS, "Invalid JSON. Use /api/v1/config")
                return nullptr;
            }
        } catch (...) {
            ERROR_REPLY(HTTP_BADREQUEST, RPC_MISC_ERROR, "Parse JSON Error. Use /api/v1/config")
            return nullptr;
        }

        std::string log_level;


        JSON_PARSE_STRING(document, "log_level", log_level)

        std::map<std::string, uint32_t> log_level_to_int = {

                {"trace",   0},
                {"debug",   1},
                {"info",    2},
                {"warning", 3},
                {"error",   4},
                {"fatal",   5}
        };

        if (log_level_to_int.count(log_level)) {

            LOG_ERROR << "set log level " << log_level;
            uint32_t log_level_int = log_level_to_int[log_level];
            log::set_filter_level((boost::log::trivial::severity_level)
            log_level_int);
        } else {
            ERROR_REPLY(HTTP_BADREQUEST, RPC_MISC_ERROR, "illegal log level value")
            return nullptr;
        }
    }


    rapidjson::Document document;
    rapidjson::Document::AllocatorType &allocator = document.GetAllocator();

    rapidjson::Value data(rapidjson::kObjectType);

    data.AddMember("result", "ok", allocator);
    SUCC_REPLY(data)

    return nullptr;
    */
}

// /
void rest_api_service::rest_api_version(const std::shared_ptr<dbc::network::http_request>& httpReq, const std::string &path) {
    /*
    rapidjson::Document document;
    rapidjson::Document::AllocatorType &allocator = document.GetAllocator();

    rapidjson::Value data(rapidjson::kObjectType);

    data.AddMember("version", STRING_REF(REST_API_VERSION), allocator);

    std::string node_id = conf_manager::instance().get_node_id();

    data.AddMember("node_id", STRING_REF(node_id), allocator);
    SUCC_REPLY(data)
    return nullptr;
    */
}

void rest_api_service::on_binary_forward(const std::shared_ptr<dbc::network::message> &msg) {
    /*
    if (!msg) {
        LOG_ERROR << "recv logs_resp but msg is nullptr";
        return E_DEFAULT;
    }


    std::shared_ptr<dbc::network::msg_forward> content = std::dynamic_pointer_cast<dbc::network::msg_forward>(
            msg->content);

    if (!content) {
        LOG_ERROR << "not a valid binary forward msg";
        return E_DEFAULT;
    }

    // support request message name end with "_req" postfix
    auto &msg_name = msg->content->header.msg_name;

    if (msg_name.substr(msg_name.size() - 4) == std::string("_req")) {
        // add path
        msg->content->header.path.push_back(conf_manager::instance().get_node_id());

        LOG_INFO << "broadcast_message binary forward msg";
        dbc::network::connection_manager::instance().broadcast_message(msg);
    } else {
        dbc::network::connection_manager::instance().send_resp_message(msg);
    }

    return E_SUCCESS;
    */
}
