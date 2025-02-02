#include "node_request_service.h"
#include <boost/property_tree/json_parser.hpp>
#include <boost/exception/all.hpp>
#include "server/server.h"
#include "config/conf_manager.h"
#include "message/matrix_types.h"
#include "message/protocol_coder/matrix_coder.h"
#include <boost/thread/thread.hpp>
#include <boost/format.hpp>
#include <boost/algorithm/string/join.hpp>
#include "message/message_id.h"
#include "util/base64.h"
#include "tweetnacl/tools.h"
#include "tweetnacl/randombytes.h"
#include "tweetnacl/tweetnacl.h"
#include "cpp_substrate.h"
#include "service/node_monitor_service/node_monitor_service.h"
#include "task/detail/VxlanManager.h"
#include "tinyxml2.h"

#define AI_TRAINING_TASK_TIMER      "training_task"
#define AI_PRUNE_TASK_TIMER         "prune_task"
#define SERVICE_BROADCAST_TIMER     "service_broadcast_timer"

#define TIME_SERVICE_INFO_LIST_EXPIRED  300 //second

std::string get_gpu_spec(const std::string& s) {
    if (s.empty()) {
        return "";
    }

    std::string rt;
    std::stringstream ss;
    ss << s;
    boost::property_tree::ptree pt;
    try {
        boost::property_tree::read_json(ss, pt);
        rt = pt.get<std::string>("env.NVIDIA_VISIBLE_DEVICES");
        if (!rt.empty()) {
            util::trim(rt);
        }
    }
    catch (...) {

    }

    return rt;
}

std::string get_is_update(const std::string& s) {
    if (s.empty()) return "";

    std::string operation;
    std::stringstream ss;
    ss << s;
    boost::property_tree::ptree pt;

    try {
        boost::property_tree::read_json(ss, pt);
        if (pt.count("operation") != 0) {
            operation = pt.get<std::string>("operation");
        }
    }
    catch (...) {

    }

    return operation;
}

ERRCODE node_request_service::init() {
	service_module::init();

	if (Server::NodeType == NODE_TYPE::ComputeNode) {
		add_self_to_servicelist();

		FResult fret = TaskMgr::instance().init();
		if (fret.errcode != ERR_SUCCESS) {
			LOG_ERROR << fret.errmsg;
			return ERR_ERROR;
		}
	}

	return ERR_SUCCESS;
}

void node_request_service::exit() {
	service_module::exit();

	if (Server::NodeType == NODE_TYPE::ComputeNode) {
		TaskMgr::instance().exit();
	}
}

void node_request_service::add_self_to_servicelist() {
    std::shared_ptr<dbc::node_service_info> info = std::make_shared<dbc::node_service_info>();
    info->service_list.emplace_back(SERVICE_NAME_AI_TRAINING);

    if (!Server::NodeName.empty()) {
        info->__set_name(Server::NodeName);
    } else {
        info->__set_name("null");
    }

    auto tnow = std::time(nullptr);
    info->__set_time_stamp(tnow);

    std::map<std::string, std::string> kvs;
    kvs["version"] = dbcversion();

    /*
    int32_t count = TaskMgr::instance().GetRunningTaskSize();
    std::string state;
    if (count <= 0) {
        state = "idle";
    }
    else {
        state = "busy(" + std::to_string(count) + ")";
    }
    kvs["state"] = state;
    */

    kvs["pub_key"] = ConfManager::instance().GetPubKey();
    info->__set_kvs(kvs);

    ServiceInfoManager::instance().add(ConfManager::instance().GetNodeId(), info);
}

void node_request_service::init_timer() {
    if (Server::NodeType == NODE_TYPE::ComputeNode) {
        // 10s
        add_timer(AI_TRAINING_TASK_TIMER, 10 * 1000, 10 * 1000, ULLONG_MAX, "",
            std::bind(&node_request_service::on_training_task_timer, this, std::placeholders::_1));

        // 1min
        add_timer(AI_PRUNE_TASK_TIMER, 60 * 1000, 60 * 1000, ULLONG_MAX, "",
            std::bind(&node_request_service::on_prune_task_timer, this, std::placeholders::_1));
    }

    // 10s
    add_timer(SERVICE_BROADCAST_TIMER, 10 * 1000, 10 * 1000, ULLONG_MAX, "",
        std::bind(&node_request_service::on_timer_service_broadcast, this, std::placeholders::_1));
}

void node_request_service::init_invoker() {
    reg_msg_handle(NODE_CREATE_TASK_REQ, CALLBACK_1(node_request_service::on_node_create_task_req, this));
    reg_msg_handle(NODE_START_TASK_REQ, CALLBACK_1(node_request_service::on_node_start_task_req, this));
    reg_msg_handle(NODE_RESTART_TASK_REQ, CALLBACK_1(node_request_service::on_node_restart_task_req, this));
    reg_msg_handle(NODE_SHUTDOWN_TASK_REQ, CALLBACK_1(node_request_service::on_node_shutdown_task_req, this));
	reg_msg_handle(NODE_POWEROFF_TASK_REQ, CALLBACK_1(node_request_service::on_node_poweroff_task_req, this));
	reg_msg_handle(NODE_STOP_TASK_REQ, CALLBACK_1(node_request_service::on_node_stop_task_req, this));
    reg_msg_handle(NODE_RESET_TASK_REQ, CALLBACK_1(node_request_service::on_node_reset_task_req, this));
    reg_msg_handle(NODE_DELETE_TASK_REQ, CALLBACK_1(node_request_service::on_node_delete_task_req, this));
    reg_msg_handle(NODE_TASK_LOGS_REQ, CALLBACK_1(node_request_service::on_node_task_logs_req, this));
    reg_msg_handle(NODE_MODIFY_TASK_REQ, CALLBACK_1(node_request_service::on_node_modify_task_req, this));
    reg_msg_handle(NODE_PASSWD_TASK_REQ, CALLBACK_1(node_request_service::on_node_passwd_task_req, this));
    reg_msg_handle(NODE_LIST_TASK_REQ, CALLBACK_1(node_request_service::on_node_list_task_req, this));

	reg_msg_handle(NODE_LIST_IMAGES_REQ, CALLBACK_1(node_request_service::on_node_list_images_req, this));
	reg_msg_handle(NODE_DOWNLOAD_IMAGE_REQ, CALLBACK_1(node_request_service::on_node_download_image_req, this));
	reg_msg_handle(NODE_DOWNLOAD_IMAGE_PROGRESS_REQ, CALLBACK_1(node_request_service::on_node_download_image_progress_req, this));
	reg_msg_handle(NODE_STOP_DOWNLOAD_IMAGE_REQ, CALLBACK_1(node_request_service::on_node_stop_download_image_req, this));
	reg_msg_handle(NODE_UPLOAD_IMAGE_REQ, CALLBACK_1(node_request_service::on_node_upload_image_req, this));
	reg_msg_handle(NODE_UPLOAD_IMAGE_PROGRESS_REQ, CALLBACK_1(node_request_service::on_node_upload_image_progress_req, this));
	reg_msg_handle(NODE_STOP_UPLOAD_IMAGE_REQ, CALLBACK_1(node_request_service::on_node_stop_upload_image_req, this));
	reg_msg_handle(NODE_DELETE_IMAGE_REQ, CALLBACK_1(node_request_service::on_node_delete_image_req, this));
	
    reg_msg_handle(NODE_LIST_SNAPSHOT_REQ, CALLBACK_1(node_request_service::on_node_list_snapshot_req, this));
	reg_msg_handle(NODE_CREATE_SNAPSHOT_REQ, CALLBACK_1(node_request_service::on_node_create_snapshot_req, this));
	reg_msg_handle(NODE_DELETE_SNAPSHOT_REQ, CALLBACK_1(node_request_service::on_node_delete_snapshot_req, this));
    
	reg_msg_handle(NODE_LIST_DISK_REQ, CALLBACK_1(node_request_service::on_node_list_disk_req, this));
	reg_msg_handle(NODE_RESIZE_DISK_REQ, CALLBACK_1(node_request_service::on_node_resize_disk_req, this));
	reg_msg_handle(NODE_ADD_DISK_REQ, CALLBACK_1(node_request_service::on_node_add_disk_req, this));
	reg_msg_handle(NODE_DELETE_DISK_REQ, CALLBACK_1(node_request_service::on_node_delete_disk_req, this));

    reg_msg_handle(NODE_QUERY_NODE_INFO_REQ, CALLBACK_1(node_request_service::on_node_query_node_info_req, this));
    reg_msg_handle(SERVICE_BROADCAST_REQ, CALLBACK_1(node_request_service::on_net_service_broadcast_req, this));
    reg_msg_handle(NODE_SESSION_ID_REQ, CALLBACK_1(node_request_service::on_node_session_id_req, this));
	reg_msg_handle(NODE_FREE_MEMORY_REQ, CALLBACK_1(node_request_service::on_node_free_memory_req, this));

    reg_msg_handle(NODE_LIST_MONITOR_SERVER_REQ, CALLBACK_1(node_request_service::on_node_list_monitor_server_req, this));
    reg_msg_handle(NODE_SET_MONITOR_SERVER_REQ, CALLBACK_1(node_request_service::on_node_set_monitor_server_req, this));
    
    reg_msg_handle(NODE_LIST_LAN_REQ, CALLBACK_1(node_request_service::on_node_list_lan_req, this));
    reg_msg_handle(NODE_CREATE_LAN_REQ, CALLBACK_1(node_request_service::on_node_create_lan_req, this));
    reg_msg_handle(NODE_DELETE_LAN_REQ, CALLBACK_1(node_request_service::on_node_delete_lan_req, this));
}

bool node_request_service::hit_node(const std::vector<std::string>& peer_node_list, const std::string& node_id) {
    bool hit = false;
    auto it = peer_node_list.begin();
    for (; it != peer_node_list.end(); it++) {
        if ((*it) == node_id) {
            hit = true;
            break;
        }
    }

    return hit;
}

bool node_request_service::check_req_header_nonce(const std::string& nonce) {
    if (nonce.empty()) {
        LOG_ERROR << "header.nonce is empty";
        return false;
    }

    if (!util::check_id(nonce)) {
        LOG_ERROR << "header.nonce check_id failed";
        return false;
    }

    if (m_nonce_filter.contains(nonce)) {
        LOG_ERROR << "header.nonce is already used";
        return false;
    }

    return true;
}

bool node_request_service::check_req_header(const std::shared_ptr<network::message> &msg) {
    if (!msg) {
        LOG_ERROR << "msg is nullptr";
        return false;
    }

    std::shared_ptr<network::msg_base> base = msg->content;
    if (!base) {
        LOG_ERROR << "msg.content is nullptr";
        return false;
    }

    if (base->header.session_id.empty()) {
        LOG_ERROR << "base->header.session_id is empty";
        return false;
    }

    if (!util::check_id(base->header.session_id)) {
        LOG_ERROR << "header.session_id check failed";
        return false;
    }

    if (base->header.exten_info.count("pub_key") <= 0) {
        LOG_ERROR << "header.exten_info has no pub_key";
        return false;
    }

    return true;
}

template <typename T>
void send_response_json(const std::string& msg_name, const network::base_header& header,
                   const std::string& rsp_data) {
    std::shared_ptr<T> rsp_msg_content = std::make_shared<T>();
    if (rsp_msg_content == nullptr) return;

    // header
    rsp_msg_content->header.__set_magic(ConfManager::instance().GetNetFlag());
    rsp_msg_content->header.__set_msg_name(msg_name);
    rsp_msg_content->header.__set_nonce(util::create_nonce());
    rsp_msg_content->header.__set_session_id(header.session_id);
    rsp_msg_content->header.__set_path(header.path);
    std::map<std::string, std::string> exten_info;
    std::string sign_message = rsp_msg_content->header.nonce + rsp_msg_content->header.session_id;
    std::string sign = util::sign(sign_message, ConfManager::instance().GetNodePrivateKey());
    exten_info["sign"] = sign;
    exten_info["pub_key"] = ConfManager::instance().GetPubKey();
    rsp_msg_content->header.__set_exten_info(exten_info);
    // body
    rsp_msg_content->body.__set_data(rsp_data);

    std::shared_ptr<network::message> rsp_msg = std::make_shared<network::message>();
    rsp_msg->set_name(msg_name);
    rsp_msg->set_content(rsp_msg_content);
    network::connection_manager::instance().send_resp_message(rsp_msg);
}

template <typename T>
void send_response_ok(const std::string& msg_name, const network::base_header& header) {
    std::stringstream ss;
    ss << "{";
    ss << "\"errcode\":" << ERR_SUCCESS;
    ss << ", \"message\":" << "\"ok\"";
    ss << "}";

    const std::map<std::string, std::string>& mp = header.exten_info;
    auto it = mp.find("pub_key");
    if (it != mp.end()) {
        std::string pub_key = it->second;
        std::string priv_key = ConfManager::instance().GetPrivKey();

        if (!pub_key.empty() && !priv_key.empty()) {
            std::string s_data = encrypt_data((unsigned char*) ss.str().c_str(), ss.str().size(), pub_key, priv_key);
            send_response_json<T>(msg_name, header, s_data);
        } else {
            LOG_ERROR << "pub_key or priv_key is empty";
        }
    } else {
        LOG_ERROR << "no pub_key";
    }
}

template <typename T>
void send_response_error(const std::string& msg_name, const network::base_header& header, int32_t result,
                         const std::string& result_msg) {
    std::stringstream ss;
    ss << "{";
    ss << "\"errcode\":" << result;
    ss << ", \"message\":" << "\"" << result_msg << "\"";
    ss << "}";

    const std::map<std::string, std::string>& mp = header.exten_info;
    auto it = mp.find("pub_key");
    if (it != mp.end()) {
        std::string pub_key = it->second;
        std::string priv_key = ConfManager::instance().GetPrivKey();

        if (!pub_key.empty() && !priv_key.empty()) {
            std::string s_data = encrypt_data((unsigned char*) ss.str().c_str(), ss.str().size(), pub_key, priv_key);
            send_response_json<T>(msg_name, header, s_data);
        } else {
            LOG_ERROR << "pub_key or priv_key is empty";
        }
    } else {
        LOG_ERROR << "no pub_key";
    }
}

FResult node_request_service::check_nonce(const std::string &wallet, const std::string &nonce, const std::string &sign) {
    if (wallet.empty()) {
        LOG_ERROR << "wallet is empty";
        return FResult(ERR_ERROR, "wallet is empty");
    }

    if (nonce.empty()) {
        LOG_ERROR << "nonce is empty";
        return FResult(ERR_ERROR, "nonce is empty");
    }

    if (sign.empty()) {
        LOG_ERROR << "sign is empty";
        return FResult(ERR_ERROR, "sign is empty");
    }

    if (!util::verify_sign(sign, nonce, wallet)) {
        LOG_ERROR << "verify sign failed";
        return FResult(ERR_ERROR, "verify sign failed");
    }

    if (m_nonce_filter.contains(nonce)) {
        LOG_ERROR << "nonce is already exist";
        return FResult(ERR_ERROR, "nonce is already exist");
    }

    return FResultOk;
}

FResult node_request_service::check_nonce(const std::vector<std::string>& multisig_wallets, int32_t multisig_threshold,
                                          const std::vector<dbc::multisig_sign_item>& multisig_signs) {
    if (multisig_wallets.empty() || multisig_signs.empty() || multisig_threshold <= 0) {
        return FResult(ERR_ERROR, "multisig wallet is emtpty");
    }

    if (multisig_threshold > multisig_wallets.size() || multisig_threshold > multisig_signs.size()) {
        LOG_ERROR << "threshold is invalid";
        return FResult(ERR_ERROR, "threshold is invalid");
    }

    for (auto &it: multisig_signs) {
        if (multisig_wallets.end() == std::find(multisig_wallets.begin(), multisig_wallets.end(), it.wallet)) {
            LOG_ERROR << "multisig sign_wallet " + it.wallet + " not found in wallets";
            return FResult(ERR_ERROR, "multisig sign_wallet " + it.wallet + " not found in wallets");
        }

        if (it.nonce.empty()) {
            LOG_ERROR << "multisig nonce is empty";
            return FResult(ERR_ERROR, "multisig nonce is empty");
        }

        if (it.sign.empty()) {
            LOG_ERROR << "multisig sign is empty";
            return FResult(ERR_ERROR, "multisig sign is empty");
        }

        if (!util::verify_sign(it.sign, it.nonce, it.wallet)) {
            LOG_ERROR << "verify multisig sign failed";
            return FResult(ERR_ERROR, "verify multisig sign failed");
        }

        if (m_nonce_filter.contains(it.nonce)) {
            LOG_ERROR << "multisig nonce is already exist";
            return FResult(ERR_ERROR, "multisig nonce is already exist");
        }
    }

    return FResultOk;
}

std::tuple<std::string, std::string> node_request_service::parse_wallet(const AuthorityParams& params) {
    FResult ret1 = check_nonce(params.wallet, params.nonce, params.sign);
    FResult ret2 = check_nonce(params.multisig_wallets, params.multisig_threshold, params.multisig_signs);

    std::string strWallet, strMultisigWallet;

    if (ret1.errcode == ERR_SUCCESS) {
        strWallet = params.wallet;
    }

    if (ret2.errcode == ERR_SUCCESS) {
        std::string accounts;
        for (int i = 0; i < params.multisig_wallets.size(); i++) {
            if (i > 0) accounts += ",";
            accounts += params.multisig_wallets[i];
        }
        substrate_MultisigAccountID* p = create_multisig_account(accounts.c_str(), params.multisig_threshold);
        if (p != nullptr) {
            strMultisigWallet = p->account_id;
            free_multisig_account(p);
        }
    }

    return {strWallet, strMultisigWallet};
}


void node_request_service::check_authority(const AuthorityParams& params, AuthoriseResult& result) {
    MACHINE_STATUS str_status = HttpDBCChainClient::instance().request_machine_status(ConfManager::instance().GetNodeId());
    if (str_status == MACHINE_STATUS::Unknown) {
        result.success = false;
        result.errmsg = "query machine_status failed";
        return;
    }
    
    // 验证中
    if (str_status == MACHINE_STATUS::Verify) {
        result.machine_status = MACHINE_STATUS::Verify;
        auto wallets = parse_wallet(params);
        std::string strWallet = std::get<0>(wallets);
        std::string strMultisigWallet = std::get<1>(wallets);
        if (strWallet.empty() && strMultisigWallet.empty()) {
            result.success = false;
            result.errmsg = "nonce check failed";
            return;
        }

        if (!strMultisigWallet.empty()) {
            bool ret = HttpDBCChainClient::instance().in_verify_time(ConfManager::instance().GetNodeId(), strMultisigWallet);
            if (!ret) {
                result.success = false;
                result.errmsg = "not in verify time";
            } else {
                TaskMgr::instance().deleteOtherCheckTasks(strMultisigWallet);

                result.success = true;
                result.user_role = USER_ROLE::Verifier;
                result.rent_wallet = strMultisigWallet;
            }
        }

        if (!result.success && !strWallet.empty()) {
            bool ret = HttpDBCChainClient::instance().in_verify_time(ConfManager::instance().GetNodeId(), strWallet);
            if (!ret) {
                result.success = false;
                result.errmsg = "not in verify time";
            } else {
                TaskMgr::instance().deleteOtherCheckTasks(strWallet);

                result.success = true;
                result.user_role = USER_ROLE::Verifier;
                result.rent_wallet = strWallet;
            }
        }

        if (!result.success) {
            result.errmsg = "not in verify time";
        }
    }
    // 验证完，已上线，但未租用
    else if (str_status == MACHINE_STATUS::Online) {
        result.machine_status = MACHINE_STATUS::Online;
        TaskMgr::instance().deleteAllCheckTasks();
        result.success = false;
        result.errmsg = "machine is not be rented";
    }
    // 租用中
    else if (str_status == MACHINE_STATUS::Rented) {
        result.machine_status = MACHINE_STATUS::Rented;
        TaskMgr::instance().deleteAllCheckTasks();

        std::string rent_wallet = TaskMgr::instance().checkSessionId(params.session_id, params.session_id_sign);
        if (rent_wallet.empty()) {
            auto wallets = parse_wallet(params);
            std::string strWallet = std::get<0>(wallets);
            std::string strMultisigWallet = std::get<1>(wallets);
            if (strWallet.empty() && strMultisigWallet.empty()) {
                result.success = false;
                if (!params.session_id.empty() && !params.session_id_sign.empty())
                    result.errmsg = "session_id is invalid";
                else
                    result.errmsg = "nonce check failed";
                return;
            }

            if (!strMultisigWallet.empty()) {
                int64_t rent_end = HttpDBCChainClient::instance().request_rent_end(ConfManager::instance().GetNodeId(), strMultisigWallet);
                if (rent_end > 0) {
                    result.success = true;
                    result.user_role = USER_ROLE::WalletRenter;
                    result.rent_wallet = strMultisigWallet;
                    result.rent_end = rent_end;

                    std::vector<std::string> vec;
                    for (auto& it : params.multisig_wallets) {
                        vec.push_back(it);
                    }
                    TaskMgr::instance().createSessionId(strMultisigWallet, vec);
                } else {
                    result.success = false;
                    result.errmsg = "wallet not rented";
                }
            }

            if (!result.success && !strWallet.empty()) {
                int64_t rent_end = HttpDBCChainClient::instance().request_rent_end(ConfManager::instance().GetNodeId(), strWallet);
                if (rent_end > 0) {
                    result.success = true;
                    result.user_role = USER_ROLE::WalletRenter;
                    result.rent_wallet = strWallet;
                    result.rent_end = rent_end;

                    TaskMgr::instance().createSessionId(strWallet);
                } else {
                    result.success = false;
                    result.errmsg = "wallet not rented";
                }
            }
        } else {
            int64_t rent_end = HttpDBCChainClient::instance().request_rent_end(ConfManager::instance().GetNodeId(), rent_wallet);
            if (rent_end > 0) {
                result.success = true;
                result.user_role = USER_ROLE::SessionIdRenter;
                result.rent_wallet = rent_wallet;
                result.rent_end = rent_end;
            } else {
                result.success = false;
                result.errmsg = "machine has already expired";
            }
        }
    }
    // 未知状态
    else {
        result.success = false;
        result.errmsg = "unknown machine status";
    }
}

//广播租用状态
bool node_request_service::found_other_running_domains() {
    TaskMgr::instance().broadcast_message("renting");

    bool found = false;
    std::vector<std::string> domains;

    for (int i = 0; i < 10; i++) {
        found = false;
        domains.clear();

		VmClient::instance().ListAllRunningDomains(domains);
		for (size_t i = 0; i < domains.size(); i++) {
			if (!TaskInfoMgr::instance().isExist(domains[i])) {
				found = true;
				break;
			}
		}

		if (found) {
            TaskMgr::instance().broadcast_message("renting");
			sleep(2);
        }
        else {
            break;
        }
    }

    if (found) {
        for (auto& iter : domains) {
            VmClient::instance().DestroyDomain(iter);
        }
    }

    found = false;
    domains.clear();
    VmClient::instance().ListAllRunningDomains(domains);
    for (size_t i = 0; i < domains.size(); i++) {
        if (!TaskInfoMgr::instance().isExist(domains[i])) {
            found = true;
            break;
        }
    }

    return found;
}

// task列表
void node_request_service::on_node_list_task_req(const std::shared_ptr<network::message>& msg) {
    auto node_req_msg = std::dynamic_pointer_cast<dbc::node_list_task_req>(msg->get_content());
    if (node_req_msg == nullptr) {
        return;
    }

    if (!check_req_header_nonce(node_req_msg->header.nonce)) {
        return;
    }

    if (Server::NodeType != NODE_TYPE::ComputeNode) {
        node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
        network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
        return;
    }

    if (!check_req_header(msg)) {
        LOG_ERROR << "request header check failed";
        node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
        network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
        return;
    }

    // decrypt
    std::string pub_key = node_req_msg->header.exten_info["pub_key"];
    std::string priv_key = ConfManager::instance().GetPrivKey();
    if (pub_key.empty() || priv_key.empty()) {
        LOG_ERROR << "pub_key or priv_key is empty";
        node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
        network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
        return;
    }

    std::shared_ptr<dbc::node_list_task_req_data> data = std::make_shared<dbc::node_list_task_req_data>();
    try {
        std::string ori_message;
        bool succ = decrypt_data(node_req_msg->body.data, pub_key, priv_key, ori_message);
        if (!succ || ori_message.empty()) {
            node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
            network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
            return;
        }

        std::shared_ptr<byte_buf> task_buf = std::make_shared<byte_buf>();
        task_buf->write_to_byte_buf(ori_message.c_str(), ori_message.size());
        network::binary_protocol proto(task_buf.get());
        data->read(&proto);
    } catch (std::exception &e) {
        node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
        network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
        return;
    }

    std::vector<std::string> req_peer_nodes = data->peer_nodes_list;
    bool hit_self = hit_node(req_peer_nodes, ConfManager::instance().GetNodeId());
    if (hit_self) {
        AuthorityParams params;
        params.wallet = data->wallet;
        params.nonce = data->nonce;
        params.sign = data->sign;
        params.multisig_wallets = data->multisig_wallets;
        params.multisig_threshold = data->multisig_threshold;
        params.multisig_signs = data->multisig_signs;
        params.session_id = data->session_id;
        params.session_id_sign = data->session_id_sign;
        AuthoriseResult result;
        check_authority(params, result);
        if (!result.success) {
            LOG_ERROR << "check authority failed: " << result.errmsg;
            send_response_error<dbc::node_list_task_rsp>(NODE_LIST_TASK_RSP, node_req_msg->header, E_DEFAULT, "check authority failed: " + result.errmsg);
            return;
        }

        task_list(node_req_msg->header, data, result);
    } else {
        node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
        network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
    }
}

void node_request_service::task_list(const network::base_header& header,
                                     const std::shared_ptr<dbc::node_list_task_req_data>& data, const AuthoriseResult& result) {
    int ret_code = ERR_SUCCESS;
    std::string ret_msg = "ok";

    std::stringstream ss_tasks;
    if (data->task_id.empty()) {
        ss_tasks << "[";
        std::vector<std::shared_ptr<TaskInfo>> taskinfos;
        TaskMgr::instance().listAllTask(result.rent_wallet, taskinfos);
        int idx = 0;
        for (auto &taskinfo : taskinfos) {
            if (idx > 0)
                ss_tasks << ",";

            ss_tasks << "{";
            ss_tasks << "\"task_id\":" << "\"" << taskinfo->getTaskId() << "\"";
            struct tm _tm{};
            time_t tt = taskinfo->getCreateTime();
            localtime_r(&tt, &_tm);
            char buf[256] = {0};
            memset(buf, 0, sizeof(char) * 256);
            strftime(buf, sizeof(char) * 256, "%Y-%m-%d %H:%M:%S", &_tm);
            ss_tasks << ", \"create_time\":" << "\"" << buf << "\"";
            ss_tasks << ", \"desc\":" << "\"" << taskinfo->getDesc() << "\"";
            ss_tasks << ", \"status\":" << "\"" 
                << task_status_string(TaskMgr::instance().queryTaskStatus(taskinfo->getTaskId())) << "\"";
            ss_tasks << "}";

            idx++;
        }
        ss_tasks << "]";
    } else {
        auto taskinfo = TaskMgr::instance().findTask(result.rent_wallet, data->task_id);
        if (nullptr != taskinfo) {
            ss_tasks << "{";
            ss_tasks << "\"task_id\":" << "\"" << taskinfo->getTaskId() << "\"";
            ss_tasks << ", \"os\":" << "\"" << taskinfo->getOperationSystem() << "\"";
            ss_tasks << ", \"ssh_ip\":" << "\"" <<
                     (taskinfo->getPublicIP().empty() ? SystemInfo::instance().GetPublicip() : taskinfo->getPublicIP()) << "\"";
            bool is_windows = isWindowsOS(taskinfo->getOperationSystem());
            std::string login_username = taskinfo->getLoginUsername();
            if (!is_windows) {
                ss_tasks << ", \"ssh_port\":" << "\"" << taskinfo->getSSHPort() << "\"";
                if (login_username.empty()) login_username = g_vm_ubuntu_login_username;
            } else {
                ss_tasks << ", \"rdp_port\":" << "\"" << taskinfo->getRDPPort() << "\"";
                if (login_username.empty()) login_username = g_vm_windows_login_username;
            }
            ss_tasks << ", \"user_name\":" << "\"" << login_username << "\"";
            ss_tasks << ", \"login_password\":" << "\"" << taskinfo->getLoginPassword() << "\"";
            
            int32_t cpu_cores = taskinfo->getTotalCores();
            auto gpus = TaskGpuMgr::instance().getTaskGpus(taskinfo->getTaskId());
            int32_t gpu_count = gpus.size();
            int64_t mem_size = taskinfo->getMemSize();
            uint16_t vnc_port = taskinfo->getVncPort();
            std::string vnc_password = taskinfo->getVncPassword();
            std::string network_name = taskinfo->getNetworkName();

            ss_tasks << ", \"cpu_cores\":" << cpu_cores;
            ss_tasks << ", \"gpu_count\":" << gpu_count;
            ss_tasks << ", \"mem_size\":" << "\"" << size2GB(mem_size) << "\"";
            ss_tasks << ", \"disk_system\":" << "\"" << g_disk_system_size << "G\"";
            ss_tasks << ", \"vnc_port\":" << "\"" << vnc_port << "\"";
            ss_tasks << ", \"vnc_password\":" << "\"" << vnc_password << "\"";
            if (!network_name.empty()) {
                ss_tasks << ", \"network_name\":" << "\"" << network_name << "\"";
            }

            struct tm _tm{};
            time_t tt = taskinfo->getCreateTime();
            localtime_r(&tt, &_tm);
            char buf[256] = {0};
            memset(buf, 0, sizeof(char) * 256);
            strftime(buf, sizeof(char) * 256, "%Y-%m-%d %H:%M:%S", &_tm);
            ss_tasks << ", \"create_time\":" << "\"" << buf << "\"";
            ss_tasks << ", \"desc\":" << "\"" << taskinfo->getDesc() << "\"";
            ss_tasks << ", \"status\":" << "\"" 
                << task_status_string(TaskMgr::instance().queryTaskStatus(taskinfo->getTaskId())) << "\"";

            if (!taskinfo->getMulticast().empty() || !network_name.empty()) {
                std::vector<std::tuple<std::string, std::string>> address;
                int addr_ret = TaskMgr::instance().getTaskAgentInterfaceAddress(taskinfo->getTaskId(), address);
                if (addr_ret >= 0 && !address.empty()) {
                    ss_tasks << ", \"local_address\":[";
                    int idx = 0;
                    for (const auto &addr : address) {
                        if (idx > 0)
                            ss_tasks << ",";
                        ss_tasks << "\"" << std::get<1>(addr) << "\"";
                        idx++;
                    }
                    ss_tasks << "]";
                }
            }

            std::map<std::string, std::shared_ptr<DiskInfo>> mpdisks;
            TaskDiskMgr::instance().listDisks(taskinfo->getTaskId(), mpdisks);
            if (!mpdisks.empty()) {
                ss_tasks << ", \"disks\":[";
                int idx = 0;
                for (const auto &disk : mpdisks) {
                    if (idx > 0)
                        ss_tasks << ",";
                    ss_tasks << "{";
                    ss_tasks << "\"name\":" << "\"" << disk.second->getName() << "\"";
				    ss_tasks << ", \"size\":" << "\"" << size2GB(disk.second->getVirtualSize()/1024L) << "\"";
                    ss_tasks << ", \"source_file\":" << "\"" << disk.second->getSourceFile() << "\"";
                    ss_tasks << "}";
                    idx++;
                }
                ss_tasks << "]";
            }

            auto nwfilter = taskinfo->getNwfilter();
            if (!nwfilter.empty()) {
                ss_tasks << ", \"network_filters\":[";
                int idx = 0;
                for (const auto& filter_item : nwfilter) {
                    if (idx > 0)
                        ss_tasks << ",";
                    ss_tasks << "\"" << filter_item << "\"";
                    idx++;
                }
                ss_tasks << "]";
            }

            // mac address
            std::vector<domainInterface> vecInterface;
            VmClient::instance().ListDomainInterface(taskinfo->getTaskId(), vecInterface);
            
            ss_tasks << ", \"interface\":[";
            for (int i = 0; i < vecInterface.size(); i++) {
                if (i > 0) ss_tasks << ",";

                ss_tasks << "{";
                ss_tasks << "\"name\":" << "\"" << vecInterface[i].name << "\"";
                ss_tasks << ",\"type\":" << "\"" << vecInterface[i].type << "\"";
                ss_tasks << ",\"mac_address\":" << "\"" << vecInterface[i].mac << "\"";
                ss_tasks << "}";
            }
            ss_tasks << "]";
            ss_tasks << "}";
        } else {
            ret_code = E_DEFAULT;
            ret_msg = "task_id not exist";
        }
    }

    std::stringstream ss;
    ss << "{";
    if (ret_code == ERR_SUCCESS) {
        ss << "\"errcode\":" << ERR_SUCCESS;
        ss << ", \"message\":" << ss_tasks.str();
    } else {
        ss << "\"errcode\":" << ret_code;
        ss << ", \"message\":" << "\"" << ret_msg << "\"";
    }
    ss << "}";

    const std::map<std::string, std::string>& mp = header.exten_info;
    auto it = mp.find("pub_key");
    if (it != mp.end()) {
        std::string pub_key = it->second;
        std::string priv_key = ConfManager::instance().GetPrivKey();

        if (!pub_key.empty() && !priv_key.empty()) {
            std::string s_data = encrypt_data((unsigned char*) ss.str().c_str(), ss.str().size(), pub_key, priv_key);
            send_response_json<dbc::node_list_task_rsp>(NODE_LIST_TASK_RSP, header, s_data);
        } else {
            LOG_ERROR << "pub_key or priv_key is empty";
            send_response_error<dbc::node_list_task_rsp>(NODE_LIST_TASK_RSP, header, E_DEFAULT, "pub_key or priv_key is empty");
        }
    } else {
        LOG_ERROR << "request no pub_key";
        send_response_error<dbc::node_list_task_rsp>(NODE_LIST_TASK_RSP, header, E_DEFAULT, "request no pub_key");
    }
}

// 创建task
void node_request_service::on_node_create_task_req(const std::shared_ptr<network::message>& msg) {
    auto node_req_msg = std::dynamic_pointer_cast<dbc::node_create_task_req>(msg->get_content());
    if (node_req_msg == nullptr) {
        return;
    }

    if (!check_req_header_nonce(node_req_msg->header.nonce)) {
        return;
    }

    if (Server::NodeType != NODE_TYPE::ComputeNode) {
        node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
        network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
        return;
    }

    if (!check_req_header(msg)) {
        LOG_ERROR << "request header check failed";
        node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
        network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
        return;
    }

    // decrypt
    std::string pub_key = node_req_msg->header.exten_info["pub_key"];
    std::string priv_key = ConfManager::instance().GetPrivKey();
    if (pub_key.empty() || priv_key.empty()) {
        LOG_ERROR << "pub_key or priv_key is empty";
        node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
        network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
        return;
    }

    std::shared_ptr<dbc::node_create_task_req_data> data = std::make_shared<dbc::node_create_task_req_data>();
    try {
        std::string ori_message;
        bool succ = decrypt_data(node_req_msg->body.data, pub_key, priv_key, ori_message);
        if (!succ || ori_message.empty()) {
            node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
            network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
            return;
        }

        std::shared_ptr<byte_buf> task_buf = std::make_shared<byte_buf>();
        task_buf->write_to_byte_buf(ori_message.c_str(), ori_message.size());
        network::binary_protocol proto(task_buf.get());
        data->read(&proto);
    } catch (std::exception &e) {
        node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
        network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
        return;
    }

    std::vector<std::string> req_peer_nodes = data->peer_nodes_list;
    bool hit_self = hit_node(req_peer_nodes, ConfManager::instance().GetNodeId());
    if (hit_self) {
        AuthorityParams params;
        params.wallet = data->wallet;
        params.nonce = data->nonce;
        params.sign = data->sign;
        params.multisig_wallets = data->multisig_wallets;
        params.multisig_threshold = data->multisig_threshold;
        params.multisig_signs = data->multisig_signs;
        params.session_id = data->session_id;
        params.session_id_sign = data->session_id_sign;
        AuthoriseResult result;
        check_authority(params, result);
        if (!result.success) {
            LOG_ERROR << "check authority failed: " << result.errmsg;
            send_response_error<dbc::node_create_task_rsp>(NODE_CREATE_TASK_RSP, node_req_msg->header, E_DEFAULT, "check authority failed: " + result.errmsg);
            return;
        }

        if (found_other_running_domains()) {
            send_response_error<dbc::node_create_task_rsp>(NODE_CREATE_TASK_RSP, node_req_msg->header, E_DEFAULT, "create task failed, please try again in a minute");
            return;
        }
        
        task_create(node_req_msg->header, data, result);
    } else {
        node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
        network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
    }
}

void node_request_service::task_create(const network::base_header& header,
                                       const std::shared_ptr<dbc::node_create_task_req_data>& data,
                                       const AuthoriseResult& result) {
    int ret_code = ERR_SUCCESS;
    std::string ret_msg = "ok";

    std::string task_id;
    auto fresult = TaskMgr::instance().createTask(result.rent_wallet, data, result.rent_end,
                                               result.user_role, task_id);
    ret_code = fresult.errcode;
    ret_msg = fresult.errmsg;

    std::stringstream ss;
    if (ret_code == ERR_SUCCESS) {
        ss << "{";
        ss << "\"errcode\":" << ret_code;
        ss << ", \"message\":" << "{";
        ss << "\"task_id\":" << "\"" << task_id << "\"";
        auto taskinfo = TaskMgr::instance().findTask(result.rent_wallet, task_id);
        struct tm _tm{};
        time_t tt = taskinfo == nullptr ? 0 : taskinfo->getCreateTime();
        localtime_r(&tt, &_tm);
        char buf[256] = {0};
        memset(buf, 0, sizeof(char) * 256);
        strftime(buf, sizeof(char) * 256, "%Y-%m-%d %H:%M:%S", &_tm);
        ss << ", \"create_time\":" << "\"" << buf << "\"";
        ss << ", \"desc\":" << "\"" << taskinfo->getDesc() << "\"";
        ss << ", \"status\":" << "\"" << "creating" << "\"";
        ss << "}";
        ss << "}";
    } else {
        ss << "{";
        ss << "\"errcode\":" << ret_code;
        ss << ", \"message\":" << "\"" << ret_msg << "\"";
        ss << "}";
    }

    const std::map<std::string, std::string>& mp = header.exten_info;
    auto it = mp.find("pub_key");
    if (it != mp.end()) {
        std::string pub_key = it->second;
        std::string priv_key = ConfManager::instance().GetPrivKey();

        if (!pub_key.empty() && !priv_key.empty()) {
            std::string s_data = encrypt_data((unsigned char*) ss.str().c_str(), ss.str().size(), pub_key, priv_key);
            send_response_json<dbc::node_create_task_rsp>(NODE_CREATE_TASK_RSP, header, s_data);
        } else {
            LOG_ERROR << "pub_key or priv_key is empty";
            send_response_error<dbc::node_create_task_rsp>(NODE_CREATE_TASK_RSP, header, E_DEFAULT, "pub_key or priv_key is empty");
        }
    } else {
        LOG_ERROR << "request no pub_key";
        send_response_error<dbc::node_create_task_rsp>(NODE_CREATE_TASK_RSP, header, E_DEFAULT, "request no pub_key");
    }
}

// 启动task
void node_request_service::on_node_start_task_req(const std::shared_ptr<network::message>& msg) {
    auto node_req_msg = std::dynamic_pointer_cast<dbc::node_start_task_req>(msg->get_content());
    if (node_req_msg == nullptr) {
        return;
    }

    if (!check_req_header_nonce(node_req_msg->header.nonce)) {
        return;
    }

    if (Server::NodeType != NODE_TYPE::ComputeNode) {
        node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
        network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
        return;
    }

    if (!check_req_header(msg)) {
        LOG_ERROR << "request header check failed";
        node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
        network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
        return;
    }

    // decrypt
    std::string pub_key = node_req_msg->header.exten_info["pub_key"];
    std::string priv_key = ConfManager::instance().GetPrivKey();
    if (pub_key.empty() || priv_key.empty()) {
        LOG_ERROR << "pub_key or priv_key is empty";
        node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
        network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
        return;
    }

    std::shared_ptr<dbc::node_start_task_req_data> data = std::make_shared<dbc::node_start_task_req_data>();
    try {
        std::string ori_message;
        bool succ = decrypt_data(node_req_msg->body.data, pub_key, priv_key, ori_message);
        if (!succ || ori_message.empty()) {
            node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
            network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
            return;
        }

        std::shared_ptr<byte_buf> task_buf = std::make_shared<byte_buf>();
        task_buf->write_to_byte_buf(ori_message.c_str(), ori_message.size());
        network::binary_protocol proto(task_buf.get());
        data->read(&proto);
    } catch (std::exception &e) {
        node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
        network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
        return;
    }

    std::vector<std::string> req_peer_nodes = data->peer_nodes_list;
    bool hit_self = hit_node(req_peer_nodes, ConfManager::instance().GetNodeId());
    if (hit_self) {
        AuthorityParams params;
        params.wallet = data->wallet;
        params.nonce = data->nonce;
        params.sign = data->sign;
        params.multisig_wallets = data->multisig_wallets;
        params.multisig_threshold = data->multisig_threshold;
        params.multisig_signs = data->multisig_signs;
        params.session_id = data->session_id;
        params.session_id_sign = data->session_id_sign;
        AuthoriseResult result;
        check_authority(params, result);
        if (!result.success) {
            LOG_ERROR << "check authority failed: " << result.errmsg;
            send_response_error<dbc::node_start_task_rsp>(NODE_START_TASK_RSP, node_req_msg->header, E_DEFAULT, "check authority failed: " + result.errmsg);
            return;
        }

        if (found_other_running_domains()) {
            send_response_error<dbc::node_start_task_rsp>(NODE_START_TASK_RSP, node_req_msg->header, E_DEFAULT, "start task failed, please try again in a minute");
            return;
        }

        task_start(node_req_msg->header, data, result);
    } else {
        node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
        network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
    }
}

void node_request_service::task_start(const network::base_header& header,
                                      const std::shared_ptr<dbc::node_start_task_req_data>& data, 
                                      const AuthoriseResult& result) {
    int ret_code = ERR_SUCCESS;
    std::string ret_msg = "ok";

    auto fresult = TaskMgr::instance().startTask(result.rent_wallet, data->task_id);
    ret_code = fresult.errcode;
    ret_msg = fresult.errmsg;

    if (ret_code == ERR_SUCCESS) {
        send_response_ok<dbc::node_start_task_rsp>(NODE_START_TASK_RSP, header);
    } else {
        send_response_error<dbc::node_start_task_rsp>(NODE_START_TASK_RSP, header, ret_code, ret_msg);
    }
}

// shutdown task
void node_request_service::on_node_shutdown_task_req(const std::shared_ptr<network::message>& msg) {
    auto node_req_msg = std::dynamic_pointer_cast<dbc::node_shutdown_task_req>(msg->get_content());
    if (node_req_msg == nullptr) {
        return;
    }

    if (!check_req_header_nonce(node_req_msg->header.nonce)) {
        return;
    }

    if (Server::NodeType != NODE_TYPE::ComputeNode) {
        node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
        network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
        return;
    }

    if (!check_req_header(msg)) {
        LOG_ERROR << "request header check failed";
        node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
        network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
        return;
    }

    // decrypt
    std::string pub_key = node_req_msg->header.exten_info["pub_key"];
    std::string priv_key = ConfManager::instance().GetPrivKey();
    if (pub_key.empty() || priv_key.empty()) {
        LOG_ERROR << "pub_key or priv_key is empty";
        node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
        network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
        return;
    }

    std::shared_ptr<dbc::node_shutdown_task_req_data> data = std::make_shared<dbc::node_shutdown_task_req_data>();
    try {
        std::string ori_message;
        bool succ = decrypt_data(node_req_msg->body.data, pub_key, priv_key, ori_message);
        if (!succ || ori_message.empty()) {
            node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
            network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
            return;
        }

        std::shared_ptr<byte_buf> task_buf = std::make_shared<byte_buf>();
        task_buf->write_to_byte_buf(ori_message.c_str(), ori_message.size());
        network::binary_protocol proto(task_buf.get());
        data->read(&proto);
    } catch (std::exception &e) {
        node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
        network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
        return;
    }

    std::vector<std::string> req_peer_nodes = data->peer_nodes_list;
    bool hit_self = hit_node(req_peer_nodes, ConfManager::instance().GetNodeId());
    if (hit_self) {
        AuthorityParams params;
        params.wallet = data->wallet;
        params.nonce = data->nonce;
        params.sign = data->sign;
        params.multisig_wallets = data->multisig_wallets;
        params.multisig_threshold = data->multisig_threshold;
        params.multisig_signs = data->multisig_signs;
        params.session_id = data->session_id;
        params.session_id_sign = data->session_id_sign;
        AuthoriseResult result;
        check_authority(params, result);
        if (!result.success) {
            LOG_ERROR << "check authority failed: " << result.errmsg;
            send_response_error<dbc::node_shutdown_task_rsp>(NODE_SHUTDOWN_TASK_RSP, node_req_msg->header, E_DEFAULT, "check authority failed: " + result.errmsg);
            return;
        }

        task_shutdown(node_req_msg->header, data, result);
    } else {
        node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
        network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
    }
}

void node_request_service::task_shutdown(const network::base_header& header,
                                     const std::shared_ptr<dbc::node_shutdown_task_req_data>& data, const AuthoriseResult& result) {
    int ret_code = ERR_SUCCESS;
    std::string ret_msg = "ok";
    
    auto fresult = TaskMgr::instance().shutdownTask(result.rent_wallet, data->task_id);
    ret_code = fresult.errcode;
    ret_msg = fresult.errmsg;

    if (ret_code == ERR_SUCCESS) {
        send_response_ok<dbc::node_shutdown_task_rsp>(NODE_SHUTDOWN_TASK_RSP, header);
    } else {
        send_response_error<dbc::node_shutdown_task_rsp>(NODE_SHUTDOWN_TASK_RSP, header, ret_code, ret_msg);
    }
}

// poweroff task
void node_request_service::on_node_poweroff_task_req(const std::shared_ptr<network::message>& msg) {
	auto node_req_msg = std::dynamic_pointer_cast<dbc::node_poweroff_task_req>(msg->get_content());
	if (node_req_msg == nullptr) {
		return;
	}

	if (!check_req_header_nonce(node_req_msg->header.nonce)) {
		return;
	}

	if (Server::NodeType != NODE_TYPE::ComputeNode) {
		node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
		network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
		return;
	}

	if (!check_req_header(msg)) {
		LOG_ERROR << "request header check failed";
		node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
		network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
		return;
	}

	// decrypt
	std::string pub_key = node_req_msg->header.exten_info["pub_key"];
	std::string priv_key = ConfManager::instance().GetPrivKey();
	if (pub_key.empty() || priv_key.empty()) {
		LOG_ERROR << "pub_key or priv_key is empty";
		node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
		network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
		return;
	}

	std::shared_ptr<dbc::node_poweroff_task_req_data> data = std::make_shared<dbc::node_poweroff_task_req_data>();
	try {
		std::string ori_message;
		bool succ = decrypt_data(node_req_msg->body.data, pub_key, priv_key, ori_message);
		if (!succ || ori_message.empty()) {
			node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
			network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
			return;
		}

		std::shared_ptr<byte_buf> task_buf = std::make_shared<byte_buf>();
		task_buf->write_to_byte_buf(ori_message.c_str(), ori_message.size());
		network::binary_protocol proto(task_buf.get());
		data->read(&proto);
	}
	catch (std::exception& e) {
		node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
		network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
		return;
	}

	std::vector<std::string> req_peer_nodes = data->peer_nodes_list;
	bool hit_self = hit_node(req_peer_nodes, ConfManager::instance().GetNodeId());
	if (hit_self) {
		AuthorityParams params;
		params.wallet = data->wallet;
		params.nonce = data->nonce;
		params.sign = data->sign;
		params.multisig_wallets = data->multisig_wallets;
		params.multisig_threshold = data->multisig_threshold;
		params.multisig_signs = data->multisig_signs;
		params.session_id = data->session_id;
		params.session_id_sign = data->session_id_sign;
		AuthoriseResult result;
		check_authority(params, result);
		if (!result.success) {
			LOG_ERROR << "check authority failed: " << result.errmsg;
			send_response_error<dbc::node_poweroff_task_rsp>(NODE_POWEROFF_TASK_RSP, node_req_msg->header, E_DEFAULT, "check authority failed: " + result.errmsg);
			return;
		}

		task_poweroff(node_req_msg->header, data, result);
	}
	else {
		node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
		network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
	}
}

void node_request_service::task_poweroff(const network::base_header& header,
	const std::shared_ptr<dbc::node_poweroff_task_req_data>& data, const AuthoriseResult& result) {
	int ret_code = ERR_SUCCESS;
	std::string ret_msg = "ok";

	auto fresult = TaskMgr::instance().poweroffTask(result.rent_wallet, data->task_id);
	ret_code = fresult.errcode;
	ret_msg = fresult.errmsg;

	if (ret_code == ERR_SUCCESS) {
		send_response_ok<dbc::node_poweroff_task_rsp>(NODE_POWEROFF_TASK_RSP, header);
	}
	else {
		send_response_error<dbc::node_poweroff_task_rsp>(NODE_POWEROFF_TASK_RSP, header, ret_code, ret_msg);
	}
}

// stop task
void node_request_service::on_node_stop_task_req(const std::shared_ptr<network::message>& msg) {
	auto node_req_msg = std::dynamic_pointer_cast<dbc::node_stop_task_req>(msg->get_content());
	if (node_req_msg == nullptr) {
		return;
	}

	if (!check_req_header_nonce(node_req_msg->header.nonce)) {
		return;
	}

	if (Server::NodeType != NODE_TYPE::ComputeNode) {
		node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
		network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
		return;
	}

	if (!check_req_header(msg)) {
		LOG_ERROR << "request header check failed";
		node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
		network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
		return;
	}

	// decrypt
	std::string pub_key = node_req_msg->header.exten_info["pub_key"];
	std::string priv_key = ConfManager::instance().GetPrivKey();
	if (pub_key.empty() || priv_key.empty()) {
		LOG_ERROR << "pub_key or priv_key is empty";
		node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
		network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
		return;
	}

	std::shared_ptr<dbc::node_stop_task_req_data> data = std::make_shared<dbc::node_stop_task_req_data>();
	try {
		std::string ori_message;
		bool succ = decrypt_data(node_req_msg->body.data, pub_key, priv_key, ori_message);
		if (!succ || ori_message.empty()) {
			node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
			network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
			return;
		}

		std::shared_ptr<byte_buf> task_buf = std::make_shared<byte_buf>();
		task_buf->write_to_byte_buf(ori_message.c_str(), ori_message.size());
		network::binary_protocol proto(task_buf.get());
		data->read(&proto);
	}
	catch (std::exception& e) {
		node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
		network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
		return;
	}

	std::vector<std::string> req_peer_nodes = data->peer_nodes_list;
	bool hit_self = hit_node(req_peer_nodes, ConfManager::instance().GetNodeId());
	if (hit_self) {
		AuthorityParams params;
		params.wallet = data->wallet;
		params.nonce = data->nonce;
		params.sign = data->sign;
		params.multisig_wallets = data->multisig_wallets;
		params.multisig_threshold = data->multisig_threshold;
		params.multisig_signs = data->multisig_signs;
		params.session_id = data->session_id;
		params.session_id_sign = data->session_id_sign;
		AuthoriseResult result;
		check_authority(params, result);
		if (!result.success) {
			LOG_ERROR << "check authority failed: " << result.errmsg;
			send_response_error<dbc::node_stop_task_rsp>(NODE_STOP_TASK_RSP, node_req_msg->header, E_DEFAULT, "check authority failed: " + result.errmsg);
			return;
		}

		task_stop(node_req_msg->header, data, result);
	}
	else {
		node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
		network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
	}
}

void node_request_service::task_stop(const network::base_header& header,
	const std::shared_ptr<dbc::node_stop_task_req_data>& data, const AuthoriseResult& result) {
	int ret_code = ERR_SUCCESS;
	std::string ret_msg = "ok";

	auto fresult = TaskMgr::instance().poweroffTask(result.rent_wallet, data->task_id);
	ret_code = fresult.errcode;
	ret_msg = fresult.errmsg;

	if (ret_code == ERR_SUCCESS) {
		send_response_ok<dbc::node_stop_task_rsp>(NODE_STOP_TASK_RSP, header);
	}
	else {
		send_response_error<dbc::node_stop_task_rsp>(NODE_STOP_TASK_RSP, header, ret_code, ret_msg);
	}
}

// 重启task
void node_request_service::on_node_restart_task_req(const std::shared_ptr<network::message>& msg) {
    auto node_req_msg = std::dynamic_pointer_cast<dbc::node_restart_task_req>(msg->get_content());
    if (node_req_msg == nullptr) {
        return;
    }

    if (!check_req_header_nonce(node_req_msg->header.nonce)) {
        return;
    }

    if (Server::NodeType != NODE_TYPE::ComputeNode) {
        node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
        network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
        return;
    }

    if (!check_req_header(msg)) {
        LOG_ERROR << "request header check failed";
        node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
        network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
        return;
    }

    // decrypt
    std::string pub_key = node_req_msg->header.exten_info["pub_key"];
    std::string priv_key = ConfManager::instance().GetPrivKey();
    if (pub_key.empty() || priv_key.empty()) {
        LOG_ERROR << "pub_key or priv_key is empty";
        node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
        network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
        return;
    }

    std::shared_ptr<dbc::node_restart_task_req_data> data = std::make_shared<dbc::node_restart_task_req_data>();
    try {
        std::string ori_message;
        bool succ = decrypt_data(node_req_msg->body.data, pub_key, priv_key, ori_message);
        if (!succ || ori_message.empty()) {
            node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
            network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
            return;
        }

        std::shared_ptr<byte_buf> task_buf = std::make_shared<byte_buf>();
        task_buf->write_to_byte_buf(ori_message.c_str(), ori_message.size());
        network::binary_protocol proto(task_buf.get());
        data->read(&proto);
    } catch (std::exception &e) {
        node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
        network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
        return;
    }

    std::vector<std::string> req_peer_nodes = data->peer_nodes_list;
    bool hit_self = hit_node(req_peer_nodes, ConfManager::instance().GetNodeId());
    if (hit_self) {
        AuthorityParams params;
        params.wallet = data->wallet;
        params.nonce = data->nonce;
        params.sign = data->sign;
        params.multisig_wallets = data->multisig_wallets;
        params.multisig_threshold = data->multisig_threshold;
        params.multisig_signs = data->multisig_signs;
        params.session_id = data->session_id;
        params.session_id_sign = data->session_id_sign;
        AuthoriseResult result;
        check_authority(params, result);
        if (!result.success) {
            LOG_ERROR << "check authority failed: " << result.errmsg;
            send_response_error<dbc::node_restart_task_rsp>(NODE_RESTART_TASK_RSP, node_req_msg->header, E_DEFAULT, "check authority failed: " + result.errmsg);
            return;
        }

        if (found_other_running_domains()) {
            send_response_error<dbc::node_restart_task_rsp>(NODE_RESTART_TASK_RSP, node_req_msg->header, E_DEFAULT, "restart task failed, please try again in a minute");
            return;
        }

        task_restart(node_req_msg->header, data, result);
    } else {
        node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
        network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
    }
}

void node_request_service::task_restart(const network::base_header& header,
                                        const std::shared_ptr<dbc::node_restart_task_req_data>& data, 
                                        const AuthoriseResult& result) {
    int ret_code = ERR_SUCCESS;
    std::string ret_msg = "ok";

    auto fresult = TaskMgr::instance().restartTask(result.rent_wallet, data->task_id, data->force_reboot != 0);
    ret_code = fresult.errcode;
    ret_msg = fresult.errmsg;

    if (ret_code == ERR_SUCCESS) {
        send_response_ok<dbc::node_restart_task_rsp>(NODE_RESTART_TASK_RSP, header);
    } else {
        send_response_error<dbc::node_restart_task_rsp>(NODE_RESTART_TASK_RSP, header, ret_code, ret_msg);
    }
}

// 重置task
void node_request_service::on_node_reset_task_req(const std::shared_ptr<network::message>& msg) {
    auto node_req_msg = std::dynamic_pointer_cast<dbc::node_reset_task_req>(msg->get_content());
    if (node_req_msg == nullptr) {
        return;
    }

    if (!check_req_header_nonce(node_req_msg->header.nonce)) {
        return;
    }

    if (Server::NodeType != NODE_TYPE::ComputeNode) {
        node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
        network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
        return;
    }

    if (!check_req_header(msg)) {
        LOG_ERROR << "request header check failed";
        node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
        network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
        return;
    }

    // decrypt
    std::string pub_key = node_req_msg->header.exten_info["pub_key"];
    std::string priv_key = ConfManager::instance().GetPrivKey();
    if (pub_key.empty() || priv_key.empty()) {
        LOG_ERROR << "pub_key or priv_key is empty";
        node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
        network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
        return;
    }

    std::shared_ptr<dbc::node_reset_task_req_data> data = std::make_shared<dbc::node_reset_task_req_data>();
    try {
        std::string ori_message;
        bool succ = decrypt_data(node_req_msg->body.data, pub_key, priv_key, ori_message);
        if (!succ || ori_message.empty()) {
            node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
            network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
            return;
        }

        std::shared_ptr<byte_buf> task_buf = std::make_shared<byte_buf>();
        task_buf->write_to_byte_buf(ori_message.c_str(), ori_message.size());
        network::binary_protocol proto(task_buf.get());
        data->read(&proto);
    } catch (std::exception &e) {
        node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
        network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
        return;
    }

    std::vector<std::string> req_peer_nodes = data->peer_nodes_list;
    bool hit_self = hit_node(req_peer_nodes, ConfManager::instance().GetNodeId());
    if (hit_self) {
        AuthorityParams params;
        params.wallet = data->wallet;
        params.nonce = data->nonce;
        params.sign = data->sign;
        params.multisig_wallets = data->multisig_wallets;
        params.multisig_threshold = data->multisig_threshold;
        params.multisig_signs = data->multisig_signs;
        params.session_id = data->session_id;
        params.session_id_sign = data->session_id_sign;
        AuthoriseResult result;
        check_authority(params, result);
        if (!result.success) {
            LOG_ERROR << "check authority failed: " << result.errmsg;
            send_response_error<dbc::node_reset_task_rsp>(NODE_RESET_TASK_RSP, node_req_msg->header, E_DEFAULT, "check authority failed: " + result.errmsg);
            return;
        }

        task_reset(node_req_msg->header, data, result);
    } else {
        node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
        network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
    }
}

void node_request_service::task_reset(const network::base_header& header,
                                      const std::shared_ptr<dbc::node_reset_task_req_data>& data, const AuthoriseResult& result) {
    int ret_code = ERR_SUCCESS;
    std::string ret_msg = "ok";

    auto fresult = TaskMgr::instance().resetTask(result.rent_wallet, data->task_id);
    ret_code = fresult.errcode;
    ret_msg = fresult.errmsg;

    if (ret_code == ERR_SUCCESS) {
        send_response_ok<dbc::node_reset_task_rsp>(NODE_RESET_TASK_RSP, header);
    } else {
        send_response_error<dbc::node_reset_task_rsp>(NODE_RESET_TASK_RSP, header, ret_code, ret_msg);
    }
}

// 删除task
void node_request_service::on_node_delete_task_req(const std::shared_ptr<network::message>& msg) {
    auto node_req_msg = std::dynamic_pointer_cast<dbc::node_delete_task_req>(msg->get_content());
    if (node_req_msg == nullptr) {
        return;
    }

    if (!check_req_header_nonce(node_req_msg->header.nonce)) {
        return;
    }

    if (Server::NodeType != NODE_TYPE::ComputeNode) {
        node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
        network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
        return;
    }

    if (!check_req_header(msg)) {
        LOG_ERROR << "request header check failed";
        node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
        network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
        return;
    }

    // decrypt
    std::string pub_key = node_req_msg->header.exten_info["pub_key"];
    std::string priv_key = ConfManager::instance().GetPrivKey();
    if (pub_key.empty() || priv_key.empty()) {
        LOG_ERROR << "pub_key or priv_key is empty";
        node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
        network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
        return;
    }

    std::shared_ptr<dbc::node_delete_task_req_data> data = std::make_shared<dbc::node_delete_task_req_data>();
    try {
        std::string ori_message;
        bool succ = decrypt_data(node_req_msg->body.data, pub_key, priv_key, ori_message);
        if (!succ || ori_message.empty()) {
            node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
            network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
            return;
        }

        std::shared_ptr<byte_buf> task_buf = std::make_shared<byte_buf>();
        task_buf->write_to_byte_buf(ori_message.c_str(), ori_message.size());
        network::binary_protocol proto(task_buf.get());
        data->read(&proto);
    } catch (std::exception &e) {
        node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
        network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
        return;
    }

    std::vector<std::string> req_peer_nodes = data->peer_nodes_list;
    bool hit_self = hit_node(req_peer_nodes, ConfManager::instance().GetNodeId());
    if (hit_self) {
        AuthorityParams params;
        params.wallet = data->wallet;
        params.nonce = data->nonce;
        params.sign = data->sign;
        params.multisig_wallets = data->multisig_wallets;
        params.multisig_threshold = data->multisig_threshold;
        params.multisig_signs = data->multisig_signs;
        params.session_id = data->session_id;
        params.session_id_sign = data->session_id_sign;
        AuthoriseResult result;
        check_authority(params, result);
        if (!result.success) {
            LOG_ERROR << "check authority failed: " << result.errmsg;
            send_response_error<dbc::node_delete_task_rsp>(NODE_DELETE_TASK_RSP, node_req_msg->header, E_DEFAULT, "check authority failed: " + result.errmsg);
            return;
        }

        task_delete(node_req_msg->header, data, result);
    } else {
        node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
        network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
    }
}

void node_request_service::task_delete(const network::base_header& header,
                                       const std::shared_ptr<dbc::node_delete_task_req_data>& data, const AuthoriseResult& result) {
    int ret_code = ERR_SUCCESS;
    std::string ret_msg = "ok";

    auto fresult = TaskMgr::instance().deleteTask(result.rent_wallet, data->task_id);
    ret_code = fresult.errcode;
    ret_msg = fresult.errmsg;

    if (ret_code == ERR_SUCCESS) {
        send_response_ok<dbc::node_delete_task_rsp>(NODE_DELETE_TASK_RSP, header);
    } else {
        send_response_error<dbc::node_delete_task_rsp>(NODE_DELETE_TASK_RSP, header, ret_code, ret_msg);
    }
}

// 查询task日志
void node_request_service::on_node_task_logs_req(const std::shared_ptr<network::message>& msg) {
    auto node_req_msg = std::dynamic_pointer_cast<dbc::node_task_logs_req>(msg->get_content());
    if (node_req_msg == nullptr) {
        return;
    }

    if (!check_req_header_nonce(node_req_msg->header.nonce)) {
        return;
    }

    if (Server::NodeType != NODE_TYPE::ComputeNode) {
        node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
        network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
        return;
    }

    if (!check_req_header(msg)) {
        LOG_ERROR << "request header check failed";
        node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
        network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
        return;
    }

    // decrypt
    std::string pub_key = node_req_msg->header.exten_info["pub_key"];
    std::string priv_key = ConfManager::instance().GetPrivKey();
    if (pub_key.empty() || priv_key.empty()) {
        LOG_ERROR << "pub_key or priv_key is empty";
        node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
        network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
        return;
    }

    std::shared_ptr<dbc::node_task_logs_req_data> data = std::make_shared<dbc::node_task_logs_req_data>();
    try {
        std::string ori_message;
        bool succ = decrypt_data(node_req_msg->body.data, pub_key, priv_key, ori_message);
        if (!succ || ori_message.empty()) {
            node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
            network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
            return;
        }

        std::shared_ptr<byte_buf> task_buf = std::make_shared<byte_buf>();
        task_buf->write_to_byte_buf(ori_message.c_str(), ori_message.size());
        network::binary_protocol proto(task_buf.get());
        data->read(&proto);
    } catch (std::exception &e) {
        node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
        network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
        return;
    }

    std::vector<std::string> req_peer_nodes = data->peer_nodes_list;
    bool hit_self = hit_node(req_peer_nodes, ConfManager::instance().GetNodeId());
    if (hit_self) {
        AuthorityParams params;
        params.wallet = data->wallet;
        params.nonce = data->nonce;
        params.sign = data->sign;
        params.multisig_wallets = data->multisig_wallets;
        params.multisig_threshold = data->multisig_threshold;
        params.multisig_signs = data->multisig_signs;
        params.session_id = data->session_id;
        params.session_id_sign = data->session_id_sign;
        AuthoriseResult result;
        check_authority(params, result);
        if (!result.success) {
            LOG_ERROR << "check authority failed: " << result.errmsg;
            send_response_error<dbc::node_task_logs_rsp>(NODE_TASK_LOGS_RSP, node_req_msg->header, E_DEFAULT, "check authority failed: " + result.errmsg);
            return;
        }

        task_logs(node_req_msg->header, data, result);
    } else {
        node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
        network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
    }
}

void node_request_service::task_logs(const network::base_header& header,
                                     const std::shared_ptr<dbc::node_task_logs_req_data>& data, const AuthoriseResult& result) {
    int ret_code = ERR_SUCCESS;
    std::string ret_msg = "ok";

    QUERY_LOG_DIRECTION head_or_tail = (QUERY_LOG_DIRECTION) data->head_or_tail;
    int32_t number_of_lines = data->number_of_lines;

    if (QUERY_LOG_DIRECTION::Head != head_or_tail && QUERY_LOG_DIRECTION::Tail != head_or_tail) {
        send_response_error<dbc::node_task_logs_rsp>(NODE_TASK_LOGS_RSP, header, E_DEFAULT, "request head_or_tail is invalid");
        LOG_ERROR << "req_head_or_tail is invalid:" << (int32_t) head_or_tail;
        return;
    }

    if (number_of_lines > MAX_NUMBER_OF_LINES || number_of_lines < 0) {
        send_response_error<dbc::node_task_logs_rsp>(NODE_TASK_LOGS_RSP, header, E_DEFAULT, "req_number_of_lines is invalid:" + number_of_lines);
        LOG_ERROR << "req_number_of_lines is invalid:" << number_of_lines;
        return;
    }

    std::string log_content;
    auto fresult = TaskMgr::instance().getTaskLog(data->task_id, head_or_tail, number_of_lines, log_content);
    ret_code = fresult.errcode;
    ret_msg = fresult.errmsg;

    if (ret_code == ERR_SUCCESS) {
        std::stringstream ss;
        ss << "{";
        ss << "\"errcode\":" << ERR_SUCCESS;
        ss << ", \"message\":";
        ss << "{";
        ss << "\"task_id\":" << "\"" << data->task_id << "\"";
        ss << ", \"log_content\":" << "[" << log_content << "]";
        ss << "}";
        ss << "}";

        const std::map<std::string, std::string>& mp = header.exten_info;
        auto it = mp.find("pub_key");
        if (it != mp.end()) {
            std::string pub_key = it->second;
            std::string priv_key = ConfManager::instance().GetPrivKey();

            if (!pub_key.empty() && !priv_key.empty()) {
                std::string s_data = encrypt_data((unsigned char*) ss.str().c_str(), ss.str().size(), pub_key, priv_key);
                send_response_json<dbc::node_task_logs_rsp>(NODE_TASK_LOGS_RSP, header, s_data);
            } else {
                LOG_ERROR << "pub_key or priv_key is empty";
                send_response_error<dbc::node_task_logs_rsp>(NODE_TASK_LOGS_RSP, header, E_DEFAULT, "pub_key or priv_key is empty");
            }
        } else {
            LOG_ERROR << "request no pub_key";
            send_response_error<dbc::node_task_logs_rsp>(NODE_TASK_LOGS_RSP, header, E_DEFAULT, "request no pub_key");
        }
    } else {
        send_response_error<dbc::node_task_logs_rsp>(NODE_TASK_LOGS_RSP, header, ret_code, ret_msg);
    }
}

// 修改task
void node_request_service::on_node_modify_task_req(const std::shared_ptr<network::message>& msg) {
	auto node_req_msg = std::dynamic_pointer_cast<dbc::node_modify_task_req>(msg->get_content());
	if (node_req_msg == nullptr) {
		return;
	}

	if (!check_req_header_nonce(node_req_msg->header.nonce)) {
		return;
	}

	if (Server::NodeType != NODE_TYPE::ComputeNode) {
		node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
		network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
		return;
	}

	if (!check_req_header(msg)) {
		LOG_ERROR << "request header check failed";
		node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
		network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
		return;
	}

	// decrypt
	std::string pub_key = node_req_msg->header.exten_info["pub_key"];
	std::string priv_key = ConfManager::instance().GetPrivKey();
	if (pub_key.empty() || priv_key.empty()) {
		LOG_ERROR << "pub_key or priv_key is empty";
		node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
		network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
		return;
	}

	std::shared_ptr<dbc::node_modify_task_req_data> data = std::make_shared<dbc::node_modify_task_req_data>();
	try {
		std::string ori_message;
		bool succ = decrypt_data(node_req_msg->body.data, pub_key, priv_key, ori_message);
		if (!succ || ori_message.empty()) {
			node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
			network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
			return;
		}

		std::shared_ptr<byte_buf> task_buf = std::make_shared<byte_buf>();
		task_buf->write_to_byte_buf(ori_message.c_str(), ori_message.size());
		network::binary_protocol proto(task_buf.get());
		data->read(&proto);
	}
	catch (std::exception& e) {
		node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
		network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
		return;
	}

	std::vector<std::string> req_peer_nodes = data->peer_nodes_list;
	bool hit_self = hit_node(req_peer_nodes, ConfManager::instance().GetNodeId());
	if (hit_self) {
		AuthorityParams params;
		params.wallet = data->wallet;
		params.nonce = data->nonce;
		params.sign = data->sign;
		params.multisig_wallets = data->multisig_wallets;
		params.multisig_threshold = data->multisig_threshold;
		params.multisig_signs = data->multisig_signs;
		params.session_id = data->session_id;
		params.session_id_sign = data->session_id_sign;
		AuthoriseResult result;
		check_authority(params, result);
		if (!result.success) {
			LOG_ERROR << "check authority failed: " << result.errmsg;
			send_response_error<dbc::node_modify_task_rsp>(NODE_MODIFY_TASK_RSP, node_req_msg->header, E_DEFAULT, "check authority failed: " + result.errmsg);
			return;
		}

		task_modify(node_req_msg->header, data, result);
	}
	else {
		node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
		network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
	}
}

void node_request_service::task_modify(const network::base_header& header,
                                       const std::shared_ptr<dbc::node_modify_task_req_data>& data, const AuthoriseResult& result) {
	int ret_code = ERR_SUCCESS;
	std::string ret_msg = "ok";

	auto fresult = TaskMgr::instance().modifyTask(result.rent_wallet, data);
    send_response_error<dbc::node_modify_task_rsp>(NODE_MODIFY_TASK_RSP, header, fresult.errcode, fresult.errmsg);
}

// 设置task密码
void node_request_service::on_node_passwd_task_req(const std::shared_ptr<network::message>& msg) {
	auto node_req_msg = std::dynamic_pointer_cast<dbc::node_passwd_task_req>(msg->get_content());
	if (node_req_msg == nullptr) {
		return;
	}

	if (!check_req_header_nonce(node_req_msg->header.nonce)) {
		return;
	}

	if (Server::NodeType != NODE_TYPE::ComputeNode) {
		node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
		network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
		return;
	}

	if (!check_req_header(msg)) {
		LOG_ERROR << "request header check failed";
		node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
		network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
		return;
	}

	// decrypt
	std::string pub_key = node_req_msg->header.exten_info["pub_key"];
	std::string priv_key = ConfManager::instance().GetPrivKey();
	if (pub_key.empty() || priv_key.empty()) {
		LOG_ERROR << "pub_key or priv_key is empty";
		node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
		network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
		return;
	}

	std::shared_ptr<dbc::node_passwd_task_req_data> data = std::make_shared<dbc::node_passwd_task_req_data>();
	try {
		std::string ori_message;
		bool succ = decrypt_data(node_req_msg->body.data, pub_key, priv_key, ori_message);
		if (!succ || ori_message.empty()) {
			node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
			network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
			return;
		}

		std::shared_ptr<byte_buf> task_buf = std::make_shared<byte_buf>();
		task_buf->write_to_byte_buf(ori_message.c_str(), ori_message.size());
		network::binary_protocol proto(task_buf.get());
		data->read(&proto);
	}
	catch (std::exception& e) {
		node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
		network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
		return;
	}

	std::vector<std::string> req_peer_nodes = data->peer_nodes_list;
	bool hit_self = hit_node(req_peer_nodes, ConfManager::instance().GetNodeId());
	if (hit_self) {
		AuthorityParams params;
		params.wallet = data->wallet;
		params.nonce = data->nonce;
		params.sign = data->sign;
		params.multisig_wallets = data->multisig_wallets;
		params.multisig_threshold = data->multisig_threshold;
		params.multisig_signs = data->multisig_signs;
		params.session_id = data->session_id;
		params.session_id_sign = data->session_id_sign;
		AuthoriseResult result;
		check_authority(params, result);
		if (!result.success) {
			LOG_ERROR << "check authority failed: " << result.errmsg;
			send_response_error<dbc::node_passwd_task_rsp>(NODE_PASSWD_TASK_RSP, node_req_msg->header, E_DEFAULT, "check authority failed: " + result.errmsg);
			return;
		}

		task_passwd(node_req_msg->header, data, result);
	}
	else {
		node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
		network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
	}
}

void node_request_service::task_passwd(const network::base_header& header,
                                       const std::shared_ptr<dbc::node_passwd_task_req_data>& data, const AuthoriseResult& result) {
	int ret_code = ERR_SUCCESS;
	std::string ret_msg = "ok";

	auto fresult = TaskMgr::instance().passwdTask(result.rent_wallet, data);
	ret_code = fresult.errcode;
	ret_msg = fresult.errmsg;

	if (ret_code == ERR_SUCCESS) {
		send_response_ok<dbc::node_passwd_task_rsp>(NODE_PASSWD_TASK_RSP, header);
	} else {
		send_response_error<dbc::node_passwd_task_rsp>(NODE_PASSWD_TASK_RSP, header, ret_code, ret_msg);
	}
}

// 镜像列表
void node_request_service::on_node_list_images_req(const std::shared_ptr<network::message>& msg) {
	auto node_req_msg = std::dynamic_pointer_cast<dbc::node_list_images_req>(msg->get_content());
	if (node_req_msg == nullptr) {
		return;
	}

	if (!check_req_header_nonce(node_req_msg->header.nonce)) {
		return;
	}

	if (Server::NodeType != NODE_TYPE::ComputeNode) {
		node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
		network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
		return;
	}

	if (!check_req_header(msg)) {
		LOG_ERROR << "request header check failed";
		node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
		network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
		return;
	}

	// decrypt
	std::string pub_key = node_req_msg->header.exten_info["pub_key"];
	std::string priv_key = ConfManager::instance().GetPrivKey();
	if (pub_key.empty() || priv_key.empty()) {
		LOG_ERROR << "pub_key or priv_key is empty";
		node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
		network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
		return;
	}

	std::shared_ptr<dbc::node_list_images_req_data> data = std::make_shared<dbc::node_list_images_req_data>();
	try {
		std::string ori_message;
		bool succ = decrypt_data(node_req_msg->body.data, pub_key, priv_key, ori_message);
		if (!succ || ori_message.empty()) {
			node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
			network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
			return;
		}

		std::shared_ptr<byte_buf> task_buf = std::make_shared<byte_buf>();
		task_buf->write_to_byte_buf(ori_message.c_str(), ori_message.size());
		network::binary_protocol proto(task_buf.get());
		data->read(&proto);
	}
	catch (std::exception& e) {
		node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
		network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
		return;
	}

	std::vector<std::string> req_peer_nodes = data->peer_nodes_list;
	bool hit_self = hit_node(req_peer_nodes, ConfManager::instance().GetNodeId());
	if (hit_self) {
		AuthorityParams params;
		params.wallet = data->wallet;
		params.nonce = data->nonce;
		params.sign = data->sign;
		params.multisig_wallets = data->multisig_wallets;
		params.multisig_threshold = data->multisig_threshold;
		params.multisig_signs = data->multisig_signs;
		params.session_id = data->session_id;
		params.session_id_sign = data->session_id_sign;
		AuthoriseResult result;
		check_authority(params, result);

		list_images(node_req_msg->header, data, result);
	}
	else {
		node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
		network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
	}
}

void node_request_service::list_images(const network::base_header& header,
	const std::shared_ptr<dbc::node_list_images_req_data>& data,
	const AuthoriseResult& result) {
	int ret_code = ERR_SUCCESS;
	std::string ret_msg = "ok";

	std::vector<ImageFile> images;
	auto fresult = TaskMgr::instance().listImages(data, result, images);
	ret_code = fresult.errcode;
	ret_msg = fresult.errmsg;

	std::stringstream ss;
	if (ret_code == ERR_SUCCESS) {
		ss << "{";
		ss << "\"errcode\":" << ret_code;
		ss << ", \"message\":" << "[";
		int index = 0;
		for (const auto& image : images) {
			if (index > 0) {
				ss << ",";
			}

			ss << "{";
			ss << "\"image_name:\":" << "\"" << image.name << "\"";
			ss << ",\"image_size\":" << "\"" << scale_size(image.size / 1024L) << "\"";
			ss << "}";

			++index;
		}
		ss << "]";
		ss << "}";
	}
	else {
		ss << "{";
		ss << "\"errcode\":" << ret_code;
		ss << ", \"message\":" << "\"" << ret_msg << "\"";
		ss << "}";
	}

	const std::map<std::string, std::string>& mp = header.exten_info;
	auto it = mp.find("pub_key");
	if (it != mp.end()) {
		std::string pub_key = it->second;
		std::string priv_key = ConfManager::instance().GetPrivKey();

		if (!pub_key.empty() && !priv_key.empty()) {
			std::string s_data = encrypt_data((unsigned char*)ss.str().c_str(), ss.str().size(), pub_key, priv_key);
			send_response_json<dbc::node_list_images_rsp>(NODE_LIST_IMAGES_RSP, header, s_data);
		}
		else {
			LOG_ERROR << "pub_key or priv_key is empty";
			send_response_error<dbc::node_list_images_rsp>(NODE_LIST_IMAGES_RSP, header, E_DEFAULT, "pub_key or priv_key is empty");
		}
	}
	else {
		LOG_ERROR << "request no pub_key";
		send_response_error<dbc::node_list_images_rsp>(NODE_LIST_IMAGES_RSP, header, E_DEFAULT, "request no pub_key");
	}
}

// 下载镜像
void node_request_service::on_node_download_image_req(const std::shared_ptr<network::message>& msg) {
	auto node_req_msg = std::dynamic_pointer_cast<dbc::node_download_image_req>(msg->get_content());
	if (node_req_msg == nullptr) {
		return;
	}

	if (!check_req_header_nonce(node_req_msg->header.nonce)) {
		return;
	}

	if (Server::NodeType != NODE_TYPE::ComputeNode) {
		node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
		network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
		return;
	}

	if (!check_req_header(msg)) {
		LOG_ERROR << "request header check failed";
		node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
		network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
		return;
	}

	// decrypt
	std::string pub_key = node_req_msg->header.exten_info["pub_key"];
	std::string priv_key = ConfManager::instance().GetPrivKey();
	if (pub_key.empty() || priv_key.empty()) {
		LOG_ERROR << "pub_key or priv_key is empty";
		node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
		network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
		return;
	}

	std::shared_ptr<dbc::node_download_image_req_data> data = std::make_shared<dbc::node_download_image_req_data>();
	try {
		std::string ori_message;
		bool succ = decrypt_data(node_req_msg->body.data, pub_key, priv_key, ori_message);
		if (!succ || ori_message.empty()) {
			node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
			network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
			return;
		}

		std::shared_ptr<byte_buf> task_buf = std::make_shared<byte_buf>();
		task_buf->write_to_byte_buf(ori_message.c_str(), ori_message.size());
		network::binary_protocol proto(task_buf.get());
		data->read(&proto);
	}
	catch (std::exception& e) {
		node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
		network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
		return;
	}

	std::vector<std::string> req_peer_nodes = data->peer_nodes_list;
	bool hit_self = hit_node(req_peer_nodes, ConfManager::instance().GetNodeId());
	if (hit_self) {
		AuthorityParams params;
		params.wallet = data->wallet;
		params.nonce = data->nonce;
		params.sign = data->sign;
		params.multisig_wallets = data->multisig_wallets;
		params.multisig_threshold = data->multisig_threshold;
		params.multisig_signs = data->multisig_signs;
		params.session_id = data->session_id;
		params.session_id_sign = data->session_id_sign;
		AuthoriseResult result;
		check_authority(params, result);
		if (!result.success) {
			LOG_ERROR << "check authority failed: " << result.errmsg;
			send_response_error<dbc::node_download_image_rsp>(NODE_DOWNLOAD_IMAGE_RSP, node_req_msg->header, E_DEFAULT, "check authority failed: " + result.errmsg);
			return;
		}

		download_image(node_req_msg->header, data, result);
	}
	else {
		node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
		network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
	}
}

void node_request_service::download_image(const network::base_header& header,
	const std::shared_ptr<dbc::node_download_image_req_data>& data,
	const AuthoriseResult& result) {
	int ret_code = ERR_SUCCESS;
	std::string ret_msg = "ok";

	auto fresult = TaskMgr::instance().downloadImage(result.rent_wallet, data);
	ret_code = fresult.errcode;
	ret_msg = fresult.errmsg;

	if (ret_code == ERR_SUCCESS) {
		send_response_ok<dbc::node_download_image_rsp>(NODE_DOWNLOAD_IMAGE_RSP, header);
	}
	else {
		send_response_error<dbc::node_download_image_rsp>(NODE_DOWNLOAD_IMAGE_RSP, header, ret_code, ret_msg);
	}
}

// 下载进度
void node_request_service::on_node_download_image_progress_req(const std::shared_ptr<network::message>& msg) {
	auto node_req_msg = std::dynamic_pointer_cast<dbc::node_download_image_progress_req>(msg->get_content());
	if (node_req_msg == nullptr) {
		return;
	}

	if (!check_req_header_nonce(node_req_msg->header.nonce)) {
		return;
	}

	if (Server::NodeType != NODE_TYPE::ComputeNode) {
		node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
		network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
		return;
	}

	if (!check_req_header(msg)) {
		LOG_ERROR << "request header check failed";
		node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
		network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
		return;
	}

	// decrypt
	std::string pub_key = node_req_msg->header.exten_info["pub_key"];
	std::string priv_key = ConfManager::instance().GetPrivKey();
	if (pub_key.empty() || priv_key.empty()) {
		LOG_ERROR << "pub_key or priv_key is empty";
		node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
		network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
		return;
	}

	std::shared_ptr<dbc::node_download_image_progress_req_data> data = std::make_shared<dbc::node_download_image_progress_req_data>();
	try {
		std::string ori_message;
		bool succ = decrypt_data(node_req_msg->body.data, pub_key, priv_key, ori_message);
		if (!succ || ori_message.empty()) {
			node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
			network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
			return;
		}

		std::shared_ptr<byte_buf> task_buf = std::make_shared<byte_buf>();
		task_buf->write_to_byte_buf(ori_message.c_str(), ori_message.size());
		network::binary_protocol proto(task_buf.get());
		data->read(&proto);
	}
	catch (std::exception& e) {
		node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
		network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
		return;
	}

	std::vector<std::string> req_peer_nodes = data->peer_nodes_list;
	bool hit_self = hit_node(req_peer_nodes, ConfManager::instance().GetNodeId());
	if (hit_self) {
		AuthorityParams params;
		params.wallet = data->wallet;
		params.nonce = data->nonce;
		params.sign = data->sign;
		params.multisig_wallets = data->multisig_wallets;
		params.multisig_threshold = data->multisig_threshold;
		params.multisig_signs = data->multisig_signs;
		params.session_id = data->session_id;
		params.session_id_sign = data->session_id_sign;
		AuthoriseResult result;
		check_authority(params, result);
		if (!result.success) {
			LOG_ERROR << "check authority failed: " << result.errmsg;
			send_response_error<dbc::node_download_image_progress_rsp>(NODE_DOWNLOAD_IMAGE_PROGRESS_RSP, node_req_msg->header, E_DEFAULT, "check authority failed: " + result.errmsg);
			return;
		}

		download_image_progress(node_req_msg->header, data, result);
	}
	else {
		node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
		network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
	}
}

void node_request_service::download_image_progress(const network::base_header& header,
	const std::shared_ptr<dbc::node_download_image_progress_req_data>& data,
	const AuthoriseResult& result) {
	int ret_code = ERR_SUCCESS;
	std::string ret_msg = "ok";

	auto fresult = TaskMgr::instance().downloadImageProgress(result.rent_wallet, data);
	ret_code = fresult.errcode;
	ret_msg = fresult.errmsg;

	if (ret_code == ERR_SUCCESS) {
		const std::map<std::string, std::string>& mp = header.exten_info;
		auto it = mp.find("pub_key");
		if (it != mp.end()) {
			std::string pub_key = it->second;
			std::string priv_key = ConfManager::instance().GetPrivKey();

			if (!pub_key.empty() && !priv_key.empty()) {
				std::string s_data = encrypt_data((unsigned char*)ret_msg.c_str(), ret_msg.size(), pub_key, priv_key);
				send_response_json<dbc::node_download_image_progress_rsp>(NODE_DOWNLOAD_IMAGE_PROGRESS_RSP, header, s_data);
			}
			else {
				LOG_ERROR << "pub_key or priv_key is empty";
				send_response_error<dbc::node_download_image_progress_rsp>(NODE_DOWNLOAD_IMAGE_PROGRESS_RSP, header, E_DEFAULT, "pub_key or priv_key is empty");
			}
		}
		else {
			LOG_ERROR << "request no pub_key";
			send_response_error<dbc::node_download_image_progress_rsp>(NODE_DOWNLOAD_IMAGE_PROGRESS_RSP, header, E_DEFAULT, "request no pub_key");
		}
	}
	else {
		send_response_error<dbc::node_download_image_progress_rsp>(NODE_DOWNLOAD_IMAGE_PROGRESS_RSP, header, ret_code, ret_msg);
	}
}

// 停止下载
void node_request_service::on_node_stop_download_image_req(const std::shared_ptr<network::message>& msg) {
	auto node_req_msg = std::dynamic_pointer_cast<dbc::node_stop_download_image_req>(msg->get_content());
	if (node_req_msg == nullptr) {
		return;
	}

	if (!check_req_header_nonce(node_req_msg->header.nonce)) {
		return;
	}

	if (Server::NodeType != NODE_TYPE::ComputeNode) {
		node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
		network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
		return;
	}

	if (!check_req_header(msg)) {
		LOG_ERROR << "request header check failed";
		node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
		network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
		return;
	}

	// decrypt
	std::string pub_key = node_req_msg->header.exten_info["pub_key"];
	std::string priv_key = ConfManager::instance().GetPrivKey();
	if (pub_key.empty() || priv_key.empty()) {
		LOG_ERROR << "pub_key or priv_key is empty";
		node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
		network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
		return;
	}

	std::shared_ptr<dbc::node_stop_download_image_req_data> data = std::make_shared<dbc::node_stop_download_image_req_data>();
	try {
		std::string ori_message;
		bool succ = decrypt_data(node_req_msg->body.data, pub_key, priv_key, ori_message);
		if (!succ || ori_message.empty()) {
			node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
			network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
			return;
		}

		std::shared_ptr<byte_buf> task_buf = std::make_shared<byte_buf>();
		task_buf->write_to_byte_buf(ori_message.c_str(), ori_message.size());
		network::binary_protocol proto(task_buf.get());
		data->read(&proto);
	}
	catch (std::exception& e) {
		node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
		network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
		return;
	}

	std::vector<std::string> req_peer_nodes = data->peer_nodes_list;
	bool hit_self = hit_node(req_peer_nodes, ConfManager::instance().GetNodeId());
	if (hit_self) {
		AuthorityParams params;
		params.wallet = data->wallet;
		params.nonce = data->nonce;
		params.sign = data->sign;
		params.multisig_wallets = data->multisig_wallets;
		params.multisig_threshold = data->multisig_threshold;
		params.multisig_signs = data->multisig_signs;
		params.session_id = data->session_id;
		params.session_id_sign = data->session_id_sign;
		AuthoriseResult result;
		check_authority(params, result);
		if (!result.success) {
			LOG_ERROR << "check authority failed: " << result.errmsg;
			send_response_error<dbc::node_stop_download_image_rsp>(NODE_STOP_DOWNLOAD_IMAGE_RSP, node_req_msg->header, E_DEFAULT, "check authority failed: " + result.errmsg);
			return;
		}

		stop_download_image(node_req_msg->header, data, result);
	}
	else {
		node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
		network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
	}
}

void node_request_service::stop_download_image(const network::base_header& header,
	const std::shared_ptr<dbc::node_stop_download_image_req_data>& data,
	const AuthoriseResult& result) {
	int ret_code = ERR_SUCCESS;
	std::string ret_msg = "ok";

	auto fresult = TaskMgr::instance().stopDownloadImage(result.rent_wallet, data);
	ret_code = fresult.errcode;
	ret_msg = fresult.errmsg;

	if (ret_code == ERR_SUCCESS) {
		const std::map<std::string, std::string>& mp = header.exten_info;
		auto it = mp.find("pub_key");
		if (it != mp.end()) {
			std::string pub_key = it->second;
			std::string priv_key = ConfManager::instance().GetPrivKey();

			if (!pub_key.empty() && !priv_key.empty()) {
				std::string s_data = encrypt_data((unsigned char*)ret_msg.c_str(), ret_msg.size(), pub_key, priv_key);
				send_response_json<dbc::node_stop_download_image_rsp>(NODE_STOP_DOWNLOAD_IMAGE_RSP, header, s_data);
			}
			else {
				LOG_ERROR << "pub_key or priv_key is empty";
				send_response_error<dbc::node_stop_download_image_rsp>(NODE_STOP_DOWNLOAD_IMAGE_RSP, header, E_DEFAULT, "pub_key or priv_key is empty");
			}
		}
		else {
			LOG_ERROR << "request no pub_key";
			send_response_error<dbc::node_stop_download_image_rsp>(NODE_STOP_DOWNLOAD_IMAGE_RSP, header, E_DEFAULT, "request no pub_key");
		}
	}
	else {
		send_response_error<dbc::node_stop_download_image_rsp>(NODE_STOP_DOWNLOAD_IMAGE_RSP, header, ret_code, ret_msg);
	}
}

// 上传镜像
void node_request_service::on_node_upload_image_req(const std::shared_ptr<network::message>& msg) {
	auto node_req_msg = std::dynamic_pointer_cast<dbc::node_upload_image_req>(msg->get_content());
	if (node_req_msg == nullptr) {
		return;
	}

	if (!check_req_header_nonce(node_req_msg->header.nonce)) {
		return;
	}

	if (Server::NodeType != NODE_TYPE::ComputeNode) {
		node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
		network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
		return;
	}

	if (!check_req_header(msg)) {
		LOG_ERROR << "request header check failed";
		node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
		network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
		return;
	}

	// decrypt
	std::string pub_key = node_req_msg->header.exten_info["pub_key"];
	std::string priv_key = ConfManager::instance().GetPrivKey();
	if (pub_key.empty() || priv_key.empty()) {
		LOG_ERROR << "pub_key or priv_key is empty";
		node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
		network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
		return;
	}

	std::shared_ptr<dbc::node_upload_image_req_data> data = std::make_shared<dbc::node_upload_image_req_data>();
	try {
		std::string ori_message;
		bool succ = decrypt_data(node_req_msg->body.data, pub_key, priv_key, ori_message);
		if (!succ || ori_message.empty()) {
			node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
			network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
			return;
		}

		std::shared_ptr<byte_buf> task_buf = std::make_shared<byte_buf>();
		task_buf->write_to_byte_buf(ori_message.c_str(), ori_message.size());
		network::binary_protocol proto(task_buf.get());
		data->read(&proto);
	}
	catch (std::exception& e) {
		node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
		network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
		return;
	}

	std::vector<std::string> req_peer_nodes = data->peer_nodes_list;
	bool hit_self = hit_node(req_peer_nodes, ConfManager::instance().GetNodeId());
	if (hit_self) {
		AuthorityParams params;
		params.wallet = data->wallet;
		params.nonce = data->nonce;
		params.sign = data->sign;
		params.multisig_wallets = data->multisig_wallets;
		params.multisig_threshold = data->multisig_threshold;
		params.multisig_signs = data->multisig_signs;
		params.session_id = data->session_id;
		params.session_id_sign = data->session_id_sign;
		AuthoriseResult result;
		check_authority(params, result);
		if (!result.success) {
			LOG_ERROR << "check authority failed: " << result.errmsg;
			send_response_error<dbc::node_upload_image_rsp>(NODE_UPLOAD_IMAGE_RSP, node_req_msg->header, E_DEFAULT, "check authority failed: " + result.errmsg);
			return;
		}

		upload_image(node_req_msg->header, data, result);
	}
	else {
		node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
		network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
	}
}

void node_request_service::upload_image(const network::base_header& header,
	const std::shared_ptr<dbc::node_upload_image_req_data>& data,
	const AuthoriseResult& result) {
	int ret_code = ERR_SUCCESS;
	std::string ret_msg = "ok";

	auto fresult = TaskMgr::instance().uploadImage(result.rent_wallet, data);
	ret_code = fresult.errcode;
	ret_msg = fresult.errmsg;

	if (ret_code == ERR_SUCCESS) {
		send_response_ok<dbc::node_upload_image_rsp>(NODE_UPLOAD_IMAGE_RSP, header);
	}
	else {
		send_response_error<dbc::node_upload_image_rsp>(NODE_UPLOAD_IMAGE_RSP, header, ret_code, ret_msg);
	}
}

// 上传进度
void node_request_service::on_node_upload_image_progress_req(const std::shared_ptr<network::message>& msg) {
	auto node_req_msg = std::dynamic_pointer_cast<dbc::node_upload_image_progress_req>(msg->get_content());
	if (node_req_msg == nullptr) {
		return;
	}

	if (!check_req_header_nonce(node_req_msg->header.nonce)) {
		return;
	}

	if (Server::NodeType != NODE_TYPE::ComputeNode) {
		node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
		network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
		return;
	}

	if (!check_req_header(msg)) {
		LOG_ERROR << "request header check failed";
		node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
		network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
		return;
	}

	// decrypt
	std::string pub_key = node_req_msg->header.exten_info["pub_key"];
	std::string priv_key = ConfManager::instance().GetPrivKey();
	if (pub_key.empty() || priv_key.empty()) {
		LOG_ERROR << "pub_key or priv_key is empty";
		node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
		network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
		return;
	}

	std::shared_ptr<dbc::node_upload_image_progress_req_data> data = std::make_shared<dbc::node_upload_image_progress_req_data>();
	try {
		std::string ori_message;
		bool succ = decrypt_data(node_req_msg->body.data, pub_key, priv_key, ori_message);
		if (!succ || ori_message.empty()) {
			node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
			network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
			return;
		}

		std::shared_ptr<byte_buf> task_buf = std::make_shared<byte_buf>();
		task_buf->write_to_byte_buf(ori_message.c_str(), ori_message.size());
		network::binary_protocol proto(task_buf.get());
		data->read(&proto);
	}
	catch (std::exception& e) {
		node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
		network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
		return;
	}

	std::vector<std::string> req_peer_nodes = data->peer_nodes_list;
	bool hit_self = hit_node(req_peer_nodes, ConfManager::instance().GetNodeId());
	if (hit_self) {
		AuthorityParams params;
		params.wallet = data->wallet;
		params.nonce = data->nonce;
		params.sign = data->sign;
		params.multisig_wallets = data->multisig_wallets;
		params.multisig_threshold = data->multisig_threshold;
		params.multisig_signs = data->multisig_signs;
		params.session_id = data->session_id;
		params.session_id_sign = data->session_id_sign;
		AuthoriseResult result;
		check_authority(params, result);
		if (!result.success) {
			LOG_ERROR << "check authority failed: " << result.errmsg;
			send_response_error<dbc::node_upload_image_progress_rsp>(NODE_UPLOAD_IMAGE_PROGRESS_RSP, node_req_msg->header, E_DEFAULT, "check authority failed: " + result.errmsg);
			return;
		}

		upload_image_progress(node_req_msg->header, data, result);
	}
	else {
		node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
		network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
	}
}

void node_request_service::upload_image_progress(const network::base_header& header,
	const std::shared_ptr<dbc::node_upload_image_progress_req_data>& data,
	const AuthoriseResult& result) {
	int ret_code = ERR_SUCCESS;
	std::string ret_msg = "ok";

	auto fresult = TaskMgr::instance().uploadImageProgress(result.rent_wallet, data);
	ret_code = fresult.errcode;
	ret_msg = fresult.errmsg;

	if (ret_code == ERR_SUCCESS) {
		const std::map<std::string, std::string>& mp = header.exten_info;
		auto it = mp.find("pub_key");
		if (it != mp.end()) {
			std::string pub_key = it->second;
			std::string priv_key = ConfManager::instance().GetPrivKey();

			if (!pub_key.empty() && !priv_key.empty()) {
				std::string s_data = encrypt_data((unsigned char*)ret_msg.c_str(), ret_msg.size(), pub_key, priv_key);
				send_response_json<dbc::node_upload_image_progress_rsp>(NODE_UPLOAD_IMAGE_PROGRESS_RSP, header, s_data);
			}
			else {
				LOG_ERROR << "pub_key or priv_key is empty";
				send_response_error<dbc::node_upload_image_progress_rsp>(NODE_UPLOAD_IMAGE_PROGRESS_RSP, header, E_DEFAULT, "pub_key or priv_key is empty");
			}
		}
		else {
			LOG_ERROR << "request no pub_key";
			send_response_error<dbc::node_upload_image_progress_rsp>(NODE_UPLOAD_IMAGE_PROGRESS_RSP, header, E_DEFAULT, "request no pub_key");
		}
	}
	else {
		send_response_error<dbc::node_upload_image_progress_rsp>(NODE_UPLOAD_IMAGE_PROGRESS_RSP, header, ret_code, ret_msg);
	}
}

// 停止上传
void node_request_service::on_node_stop_upload_image_req(const std::shared_ptr<network::message>& msg) {
	auto node_req_msg = std::dynamic_pointer_cast<dbc::node_stop_upload_image_req>(msg->get_content());
	if (node_req_msg == nullptr) {
		return;
	}

	if (!check_req_header_nonce(node_req_msg->header.nonce)) {
		return;
	}

	if (Server::NodeType != NODE_TYPE::ComputeNode) {
		node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
		network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
		return;
	}

	if (!check_req_header(msg)) {
		LOG_ERROR << "request header check failed";
		node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
		network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
		return;
	}

	// decrypt
	std::string pub_key = node_req_msg->header.exten_info["pub_key"];
	std::string priv_key = ConfManager::instance().GetPrivKey();
	if (pub_key.empty() || priv_key.empty()) {
		LOG_ERROR << "pub_key or priv_key is empty";
		node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
		network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
		return;
	}

	std::shared_ptr<dbc::node_stop_upload_image_req_data> data = std::make_shared<dbc::node_stop_upload_image_req_data>();
	try {
		std::string ori_message;
		bool succ = decrypt_data(node_req_msg->body.data, pub_key, priv_key, ori_message);
		if (!succ || ori_message.empty()) {
			node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
			network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
			return;
		}

		std::shared_ptr<byte_buf> task_buf = std::make_shared<byte_buf>();
		task_buf->write_to_byte_buf(ori_message.c_str(), ori_message.size());
		network::binary_protocol proto(task_buf.get());
		data->read(&proto);
	}
	catch (std::exception& e) {
		node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
		network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
		return;
	}

	std::vector<std::string> req_peer_nodes = data->peer_nodes_list;
	bool hit_self = hit_node(req_peer_nodes, ConfManager::instance().GetNodeId());
	if (hit_self) {
		AuthorityParams params;
		params.wallet = data->wallet;
		params.nonce = data->nonce;
		params.sign = data->sign;
		params.multisig_wallets = data->multisig_wallets;
		params.multisig_threshold = data->multisig_threshold;
		params.multisig_signs = data->multisig_signs;
		params.session_id = data->session_id;
		params.session_id_sign = data->session_id_sign;
		AuthoriseResult result;
		check_authority(params, result);
		if (!result.success) {
			LOG_ERROR << "check authority failed: " << result.errmsg;
			send_response_error<dbc::node_stop_upload_image_rsp>(NODE_STOP_UPLOAD_IMAGE_RSP, node_req_msg->header, E_DEFAULT, "check authority failed: " + result.errmsg);
			return;
		}

		stop_upload_image(node_req_msg->header, data, result);
	}
	else {
		node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
		network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
	}
}

void node_request_service::stop_upload_image(const network::base_header& header,
	const std::shared_ptr<dbc::node_stop_upload_image_req_data>& data,
	const AuthoriseResult& result) {
	int ret_code = ERR_SUCCESS;
	std::string ret_msg = "ok";

	auto fresult = TaskMgr::instance().stopUploadImage(result.rent_wallet, data);
	ret_code = fresult.errcode;
	ret_msg = fresult.errmsg;

	if (ret_code == ERR_SUCCESS) {
		const std::map<std::string, std::string>& mp = header.exten_info;
		auto it = mp.find("pub_key");
		if (it != mp.end()) {
			std::string pub_key = it->second;
			std::string priv_key = ConfManager::instance().GetPrivKey();

			if (!pub_key.empty() && !priv_key.empty()) {
				std::string s_data = encrypt_data((unsigned char*)ret_msg.c_str(), ret_msg.size(), pub_key, priv_key);
				send_response_json<dbc::node_stop_upload_image_rsp>(NODE_STOP_UPLOAD_IMAGE_RSP, header, s_data);
			}
			else {
				LOG_ERROR << "pub_key or priv_key is empty";
				send_response_error<dbc::node_stop_upload_image_rsp>(NODE_STOP_UPLOAD_IMAGE_RSP, header, E_DEFAULT, "pub_key or priv_key is empty");
			}
		}
		else {
			LOG_ERROR << "request no pub_key";
			send_response_error<dbc::node_stop_upload_image_rsp>(NODE_STOP_UPLOAD_IMAGE_RSP, header, E_DEFAULT, "request no pub_key");
		}
	}
	else {
		send_response_error<dbc::node_stop_upload_image_rsp>(NODE_STOP_UPLOAD_IMAGE_RSP, header, ret_code, ret_msg);
	}
}

// 删除镜像
void node_request_service::on_node_delete_image_req(const std::shared_ptr<network::message>& msg) {
	auto node_req_msg = std::dynamic_pointer_cast<dbc::node_delete_image_req>(msg->get_content());
	if (node_req_msg == nullptr) {
		return;
	}

	if (!check_req_header_nonce(node_req_msg->header.nonce)) {
		return;
	}

	if (Server::NodeType != NODE_TYPE::ComputeNode) {
		node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
		network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
		return;
	}

	if (!check_req_header(msg)) {
		LOG_ERROR << "request header check failed";
		node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
		network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
		return;
	}

	// decrypt
	std::string pub_key = node_req_msg->header.exten_info["pub_key"];
	std::string priv_key = ConfManager::instance().GetPrivKey();
	if (pub_key.empty() || priv_key.empty()) {
		LOG_ERROR << "pub_key or priv_key is empty";
		node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
		network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
		return;
	}

	std::shared_ptr<dbc::node_delete_image_req_data> data = std::make_shared<dbc::node_delete_image_req_data>();
	try {
		std::string ori_message;
		bool succ = decrypt_data(node_req_msg->body.data, pub_key, priv_key, ori_message);
		if (!succ || ori_message.empty()) {
			node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
			network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
			return;
		}

		std::shared_ptr<byte_buf> task_buf = std::make_shared<byte_buf>();
		task_buf->write_to_byte_buf(ori_message.c_str(), ori_message.size());
		network::binary_protocol proto(task_buf.get());
		data->read(&proto);
	}
	catch (std::exception& e) {
		node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
		network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
		return;
	}

	std::vector<std::string> req_peer_nodes = data->peer_nodes_list;
	bool hit_self = hit_node(req_peer_nodes, ConfManager::instance().GetNodeId());
	if (hit_self) {
		AuthorityParams params;
		params.wallet = data->wallet;
		params.nonce = data->nonce;
		params.sign = data->sign;
		params.multisig_wallets = data->multisig_wallets;
		params.multisig_threshold = data->multisig_threshold;
		params.multisig_signs = data->multisig_signs;
		params.session_id = data->session_id;
		params.session_id_sign = data->session_id_sign;
		AuthoriseResult result;
		check_authority(params, result);
		if (!result.success) {
			LOG_ERROR << "check authority failed: " << result.errmsg;
			send_response_error<dbc::node_delete_image_rsp>(NODE_DELETE_IMAGE_RSP, node_req_msg->header, E_DEFAULT, "check authority failed: " + result.errmsg);
			return;
		}

		delete_image(node_req_msg->header, data, result);
	}
	else {
		node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
		network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
	}
}

void node_request_service::delete_image(const network::base_header& header,
	const std::shared_ptr<dbc::node_delete_image_req_data>& data,
	const AuthoriseResult& result) {
	int ret_code = ERR_SUCCESS;
	std::string ret_msg = "ok";

	auto fresult = TaskMgr::instance().deleteImage(result.rent_wallet, data);
	ret_code = fresult.errcode;
	ret_msg = fresult.errmsg;

	if (ret_code == ERR_SUCCESS) {
		send_response_ok<dbc::node_upload_image_rsp>(NODE_UPLOAD_IMAGE_RSP, header);
	}
	else {
		send_response_error<dbc::node_upload_image_rsp>(NODE_UPLOAD_IMAGE_RSP, header, ret_code, ret_msg);
	}
}

// 快照列表
void node_request_service::on_node_list_snapshot_req(const std::shared_ptr<network::message>& msg) {
    auto node_req_msg = std::dynamic_pointer_cast<dbc::node_list_snapshot_req>(msg->get_content());
    if (node_req_msg == nullptr) {
        return;
    }

    if (!check_req_header_nonce(node_req_msg->header.nonce)) {
        return;
    }

    if (Server::NodeType != NODE_TYPE::ComputeNode) {
        node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
        network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
        return;
    }

    if (!check_req_header(msg)) {
        LOG_ERROR << "request header check failed";
        node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
        network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
        return;
    }

    // decrypt
    std::string pub_key = node_req_msg->header.exten_info["pub_key"];
    std::string priv_key = ConfManager::instance().GetPrivKey();
    if (pub_key.empty() || priv_key.empty()) {
        LOG_ERROR << "pub_key or priv_key is empty";
        node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
        network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
        return;
    }

    std::shared_ptr<dbc::node_list_snapshot_req_data> data = std::make_shared<dbc::node_list_snapshot_req_data>();
    try {
        std::string ori_message;
        bool succ = decrypt_data(node_req_msg->body.data, pub_key, priv_key, ori_message);
        if (!succ || ori_message.empty()) {
            node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
            network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
            return;
        }

        std::shared_ptr<byte_buf> snapshot_buf = std::make_shared<byte_buf>();
        snapshot_buf->write_to_byte_buf(ori_message.c_str(), ori_message.size());
        network::binary_protocol proto(snapshot_buf.get());
        data->read(&proto);
    }
    catch (std::exception& e) {
        node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
        network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
        return;
    }

    std::vector<std::string> req_peer_nodes = data->peer_nodes_list;
    bool hit_self = hit_node(req_peer_nodes, ConfManager::instance().GetNodeId());
    if (hit_self) {
        AuthorityParams params;
        params.wallet = data->wallet;
        params.nonce = data->nonce;
        params.sign = data->sign;
        params.multisig_wallets = data->multisig_wallets;
        params.multisig_threshold = data->multisig_threshold;
        params.multisig_signs = data->multisig_signs;
        params.session_id = data->session_id;
        params.session_id_sign = data->session_id_sign;
        AuthoriseResult result;
        check_authority(params, result);
        if (!result.success) {
            LOG_ERROR << "check authority failed: " << result.errmsg;
            send_response_error<dbc::node_list_snapshot_rsp>(NODE_LIST_SNAPSHOT_RSP, node_req_msg->header, E_DEFAULT, "check authority failed: " + result.errmsg);
            return;
        }

        snapshot_list(node_req_msg->header, data, result);
    }
    else {
        node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
        network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
    }
}

void node_request_service::snapshot_list(const network::base_header& header,
    const std::shared_ptr<dbc::node_list_snapshot_req_data>& data, const AuthoriseResult& result) {
    int ret_code = ERR_SUCCESS;
    std::string ret_msg = "ok";

    std::stringstream ss_snapshots;
 
    auto snapshotinfo = TaskDiskMgr::instance().getSnapshotInfo(data->task_id);

    if (data->snapshot_name.empty()) {
		std::vector<dbc::snapshot_info> snapshots;
		TaskDiskMgr::instance().listSnapshots(data->task_id, snapshots);

        ss_snapshots << "[";
        int idx = 0;
        for (auto &snapshot : snapshots) {
            if (idx > 0)
                ss_snapshots << ",";

            ss_snapshots << "{";
            ss_snapshots << "\"snapshot_name\":" << "\"" << snapshot.name << "\"";
            ss_snapshots << ", \"description\":" << "\"" << snapshot.desc << "\"";
            ss_snapshots << ", \"snapshot_file\":" << "\"" << snapshot.file << "\"";

            struct tm _tm{};
            time_t tt = snapshot.create_time;
            localtime_r(&tt, &_tm);
            char buf[256] = {0};
            memset(buf, 0, sizeof(char) * 256);
            strftime(buf, sizeof(char) * 256, "%Y-%m-%d %H:%M:%S", &_tm);
            ss_snapshots << ", \"create_time\":" << "\"" << buf << "\"";
            
            if (snapshotinfo)
                ss_snapshots << ", \"status\":" << "\""
                    << snapshot_status_string(snapshotinfo->getSnapshotStatus(snapshot.name)) << "\"";
            ss_snapshots << "}";

            idx++;
        }
        ss_snapshots << "]";
    }
    else {
        dbc::snapshot_info info;
        bool found = TaskDiskMgr::instance().getSnapshot(data->task_id, data->snapshot_name, info);
        if (found) {
            ss_snapshots << "{";
            ss_snapshots << "\"snapshot_name\":" << "\"" << info.name << "\"";
            ss_snapshots << ", \"description\":" << "\"" << info.desc << "\"";
            ss_snapshots << ", \"snapshot_file\":" << "\"" << info.file << "\"";

            struct tm _tm{};
            time_t tt = info.create_time;
            localtime_r(&tt, &_tm);
            char buf[256] = {0};
            memset(buf, 0, sizeof(char) * 256);
            strftime(buf, sizeof(char) * 256, "%Y-%m-%d %H:%M:%S", &_tm);
            ss_snapshots << ", \"create_time\":" << "\"" << buf << "\"";

			if (snapshotinfo)
				ss_snapshots << ", \"status\":" << "\""
				<< snapshot_status_string(snapshotinfo->getSnapshotStatus(info.name)) << "\"";
            ss_snapshots << "}";
        }
        else {
            ret_code = ERR_ERROR;
            ret_msg = "snapshot_name not exist";
        }
    }
 
    std::stringstream ss;
    ss << "{";
    if (ret_code == ERR_SUCCESS) {
        ss << "\"errcode\":" << ERR_SUCCESS;
        ss << ", \"message\":" << ss_snapshots.str();
    }
    else {
        ss << "\"errcode\":" << ret_code;
        ss << ", \"message\":" << "\"" << ret_msg << "\"";
    }
    ss << "}";

    const std::map<std::string, std::string>& mp = header.exten_info;
    auto it = mp.find("pub_key");
    if (it != mp.end()) {
        std::string pub_key = it->second;
        std::string priv_key = ConfManager::instance().GetPrivKey();

        if (!pub_key.empty() && !priv_key.empty()) {
            std::string s_data = encrypt_data((unsigned char*)ss.str().c_str(), ss.str().size(), pub_key, priv_key);
            send_response_json<dbc::node_list_snapshot_rsp>(NODE_LIST_SNAPSHOT_RSP, header, s_data);
        }
        else {
            LOG_ERROR << "pub_key or priv_key is empty";
            send_response_error<dbc::node_list_snapshot_rsp>(NODE_LIST_SNAPSHOT_RSP, header, E_DEFAULT, "pub_key or priv_key is empty");
        }
    }
    else {
        LOG_ERROR << "request no pub_key";
        send_response_error<dbc::node_list_snapshot_rsp>(NODE_LIST_SNAPSHOT_RSP, header, E_DEFAULT, "request no pub_key");
    }
}

// 创建快照
void node_request_service::on_node_create_snapshot_req(const std::shared_ptr<network::message>& msg) {
    auto node_req_msg = std::dynamic_pointer_cast<dbc::node_create_snapshot_req>(msg->get_content());
    if (node_req_msg == nullptr) {
        return;
    }

    if (!check_req_header_nonce(node_req_msg->header.nonce)) {
        return;
    }

    if (Server::NodeType != NODE_TYPE::ComputeNode) {
        node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
        network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
        return;
    }

    if (!check_req_header(msg)) {
        LOG_ERROR << "request header check failed";
        node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
        network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
        return;
    }

    // decrypt
    std::string pub_key = node_req_msg->header.exten_info["pub_key"];
    std::string priv_key = ConfManager::instance().GetPrivKey();
    if (pub_key.empty() || priv_key.empty()) {
        LOG_ERROR << "pub_key or priv_key is empty";
        node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
        network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
        return;
    }

    std::shared_ptr<dbc::node_create_snapshot_req_data> data = std::make_shared<dbc::node_create_snapshot_req_data>();
    try {
        std::string ori_message;
        bool succ = decrypt_data(node_req_msg->body.data, pub_key, priv_key, ori_message);
        if (!succ || ori_message.empty()) {
            node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
            network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
            return;
        }

        std::shared_ptr<byte_buf> snapshot_buf = std::make_shared<byte_buf>();
        snapshot_buf->write_to_byte_buf(ori_message.c_str(), ori_message.size());
        network::binary_protocol proto(snapshot_buf.get());
        data->read(&proto);
    }
    catch (std::exception& e) {
        node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
        network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
        return;
    }

    std::vector<std::string> req_peer_nodes = data->peer_nodes_list;
    bool hit_self = hit_node(req_peer_nodes, ConfManager::instance().GetNodeId());
    if (hit_self) {
        AuthorityParams params;
        params.wallet = data->wallet;
        params.nonce = data->nonce;
        params.sign = data->sign;
        params.multisig_wallets = data->multisig_wallets;
        params.multisig_threshold = data->multisig_threshold;
        params.multisig_signs = data->multisig_signs;
        params.session_id = data->session_id;
        params.session_id_sign = data->session_id_sign;
        AuthoriseResult result;
        check_authority(params, result);
        if (!result.success || result.user_role == USER_ROLE::Unknown || result.user_role == USER_ROLE::Verifier) {
            LOG_ERROR << "check authority failed: " << result.errmsg;
            send_response_error<dbc::node_create_snapshot_rsp>(NODE_CREATE_SNAPSHOT_RSP, node_req_msg->header, E_DEFAULT, "check authority failed: " + result.errmsg);
            return;
        }

        snapshot_create(node_req_msg->header, data, result);
    }
    else {
        node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
        network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
    }
}

void node_request_service::snapshot_create(const network::base_header& header,
    const std::shared_ptr<dbc::node_create_snapshot_req_data>& data, const AuthoriseResult& result) {
    FResult fret = FResultOk;
    
	// 解析请求参数
	rapidjson::Document doc;
	doc.Parse(data->additional.c_str());
	if (!doc.IsObject()) {
		LOG_ERROR << "parse xml failed";
		send_response_error<dbc::node_create_snapshot_rsp>(NODE_CREATE_SNAPSHOT_RSP, header, E_DEFAULT, "parse xml failed");
		return;
	}

	std::string s_snapshot_name;
	JSON_PARSE_STRING(doc, "snapshot_name", s_snapshot_name);
	if (s_snapshot_name.empty()) {
		LOG_ERROR << "'snapshot_name' is empty";
		send_response_error<dbc::node_create_snapshot_rsp>(NODE_CREATE_SNAPSHOT_RSP, header, E_DEFAULT, "'snapshot_name' is empty");
		return;
	}

	std::string s_desc;
	JSON_PARSE_STRING(doc, "desc", s_desc);

	ImageServer imgsvr;
	imgsvr.from_string(data->image_server);

    // 执行逻辑
    std::stringstream ss;

    fret = TaskDiskMgr::instance().createAndUploadSnapshot(data->task_id, s_snapshot_name, imgsvr, s_desc);
	if (fret.errcode == ERR_SUCCESS) {
		ss << "{";
		ss << "\"errcode\":" << ERR_SUCCESS;
		ss << ",\"message\":" << "\"ok\"";
		ss << "}";
	}
	else {
		ss << "{";
		ss << "\"errcode\":" << fret.errcode;
		ss << ",\"message\":" << "\"" << fret.errmsg << "\"";
		ss << "}";
	}

    // 返回结果
    const std::map<std::string, std::string>& mp = header.exten_info;
    auto it = mp.find("pub_key");
    if (it != mp.end()) {
        std::string pub_key = it->second;
        std::string priv_key = ConfManager::instance().GetPrivKey();

        if (!pub_key.empty() && !priv_key.empty()) {
            std::string s_data = encrypt_data((unsigned char*)ss.str().c_str(), ss.str().size(), pub_key, priv_key);
            send_response_json<dbc::node_create_snapshot_rsp>(NODE_CREATE_SNAPSHOT_RSP, header, s_data);
        }
        else {
            LOG_ERROR << "pub_key or priv_key is empty";
            send_response_error<dbc::node_create_snapshot_rsp>(NODE_CREATE_SNAPSHOT_RSP, header, E_DEFAULT, "pub_key or priv_key is empty");
        }
    }
    else {
        LOG_ERROR << "request no pub_key";
        send_response_error<dbc::node_create_snapshot_rsp>(NODE_CREATE_SNAPSHOT_RSP, header, E_DEFAULT, "request no pub_key");
    }
}

// 删除快照
void node_request_service::on_node_delete_snapshot_req(const std::shared_ptr<network::message>& msg) {
    auto node_req_msg = std::dynamic_pointer_cast<dbc::node_delete_snapshot_req>(msg->get_content());
    if (node_req_msg == nullptr) {
        return;
    }

    if (!check_req_header_nonce(node_req_msg->header.nonce)) {
        return;
    }

    if (Server::NodeType != NODE_TYPE::ComputeNode) {
        node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
        network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
        return;
    }

    if (!check_req_header(msg)) {
        LOG_ERROR << "request header check failed";
        node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
        network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
        return;
    }

    // decrypt
    std::string pub_key = node_req_msg->header.exten_info["pub_key"];
    std::string priv_key = ConfManager::instance().GetPrivKey();
    if (pub_key.empty() || priv_key.empty()) {
        LOG_ERROR << "pub_key or priv_key is empty";
        node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
        network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
        return;
    }

    std::shared_ptr<dbc::node_delete_snapshot_req_data> data = std::make_shared<dbc::node_delete_snapshot_req_data>();
    try {
        std::string ori_message;
        bool succ = decrypt_data(node_req_msg->body.data, pub_key, priv_key, ori_message);
        if (!succ || ori_message.empty()) {
            node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
            network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
            return;
        }

        std::shared_ptr<byte_buf> snapshot_buf = std::make_shared<byte_buf>();
        snapshot_buf->write_to_byte_buf(ori_message.c_str(), ori_message.size());
        network::binary_protocol proto(snapshot_buf.get());
        data->read(&proto);
    }
    catch (std::exception& e) {
        node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
        network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
        return;
    }

    std::vector<std::string> req_peer_nodes = data->peer_nodes_list;
    bool hit_self = hit_node(req_peer_nodes, ConfManager::instance().GetNodeId());
    if (hit_self) {
        AuthorityParams params;
        params.wallet = data->wallet;
        params.nonce = data->nonce;
        params.sign = data->sign;
        params.multisig_wallets = data->multisig_wallets;
        params.multisig_threshold = data->multisig_threshold;
        params.multisig_signs = data->multisig_signs;
        params.session_id = data->session_id;
        params.session_id_sign = data->session_id_sign;
        AuthoriseResult result;
        check_authority(params, result);
        if (!result.success || result.user_role == USER_ROLE::Unknown || result.user_role == USER_ROLE::Verifier) {
            LOG_ERROR << "check authority failed: " << result.errmsg;
            send_response_error<dbc::node_delete_snapshot_rsp>(NODE_DELETE_SNAPSHOT_RSP, node_req_msg->header, E_DEFAULT, "check authority failed: " + result.errmsg);
            return;
        }

        snapshot_delete(node_req_msg->header, data, result);
    }
    else {
        node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
        network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
    }
}

void node_request_service::snapshot_delete(const network::base_header& header,
    const std::shared_ptr<dbc::node_delete_snapshot_req_data>& data, const AuthoriseResult& result) {
    FResult fret = FResultOk;

    std::stringstream ss;

    dbc::snapshot_info snapinfo;
    bool found = TaskDiskMgr::instance().getSnapshot(data->task_id, data->snapshot_name, snapinfo);
    if (found) {
        TaskDiskMgr::instance().terminateUploadSnapshot(data->task_id, data->snapshot_name);

		ss << "{";
		ss << "\"errcode\":" << ERR_SUCCESS;
		ss << ",\"message\":" << "\"ok\"";
		ss << "}";
    }
    else {
		ss << "{";
		ss << "\"errcode\":" << ERR_ERROR;
		ss << ",\"message\":" << "\"" << data->snapshot_name << " not exist" << "\"";
		ss << "}";
    }
	
	// 返回结果
	const std::map<std::string, std::string>& mp = header.exten_info;
	auto it = mp.find("pub_key");
	if (it != mp.end()) {
		std::string pub_key = it->second;
		std::string priv_key = ConfManager::instance().GetPrivKey();

		if (!pub_key.empty() && !priv_key.empty()) {
			std::string s_data = encrypt_data((unsigned char*)ss.str().c_str(), ss.str().size(), pub_key, priv_key);
			send_response_json<dbc::node_delete_snapshot_rsp>(NODE_DELETE_SNAPSHOT_RSP, header, s_data);
		}
		else {
			LOG_ERROR << "pub_key or priv_key is empty";
			send_response_error<dbc::node_delete_snapshot_rsp>(NODE_DELETE_SNAPSHOT_RSP, header, E_DEFAULT, "pub_key or priv_key is empty");
		}
	}
	else {
		LOG_ERROR << "request no pub_key";
		send_response_error<dbc::node_delete_snapshot_rsp>(NODE_DELETE_SNAPSHOT_RSP, header, E_DEFAULT, "request no pub_key");
	}
}

// task的磁盘列表
void node_request_service::on_node_list_disk_req(const std::shared_ptr<network::message>& msg) {
	auto node_req_msg = std::dynamic_pointer_cast<dbc::node_list_disk_req>(msg->get_content());
	if (node_req_msg == nullptr) {
		return;
	}

	if (!check_req_header_nonce(node_req_msg->header.nonce)) {
		return;
	}

	if (Server::NodeType != NODE_TYPE::ComputeNode) {
		node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
		network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
		return;
	}

	if (!check_req_header(msg)) {
		LOG_ERROR << "request header check failed";
		node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
		network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
		return;
	}

	// decrypt
	std::string pub_key = node_req_msg->header.exten_info["pub_key"];
	std::string priv_key = ConfManager::instance().GetPrivKey();
	if (pub_key.empty() || priv_key.empty()) {
		LOG_ERROR << "pub_key or priv_key is empty";
		node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
		network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
		return;
	}

	std::shared_ptr<dbc::node_list_disk_req_data> data = std::make_shared<dbc::node_list_disk_req_data>();
	try {
		std::string ori_message;
		bool succ = decrypt_data(node_req_msg->body.data, pub_key, priv_key, ori_message);
		if (!succ || ori_message.empty()) {
			node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
			network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
			return;
		}

		std::shared_ptr<byte_buf> snapshot_buf = std::make_shared<byte_buf>();
		snapshot_buf->write_to_byte_buf(ori_message.c_str(), ori_message.size());
		network::binary_protocol proto(snapshot_buf.get());
		data->read(&proto);
	}
	catch (std::exception& e) {
		node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
		network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
		return;
	}

	std::vector<std::string> req_peer_nodes = data->peer_nodes_list;
	bool hit_self = hit_node(req_peer_nodes, ConfManager::instance().GetNodeId());
	if (hit_self) {
		AuthorityParams params;
		params.wallet = data->wallet;
		params.nonce = data->nonce;
		params.sign = data->sign;
		params.multisig_wallets = data->multisig_wallets;
		params.multisig_threshold = data->multisig_threshold;
		params.multisig_signs = data->multisig_signs;
		params.session_id = data->session_id;
		params.session_id_sign = data->session_id_sign;
		AuthoriseResult result;
		check_authority(params, result);
		if (!result.success || result.user_role == USER_ROLE::Unknown || result.user_role == USER_ROLE::Verifier) {
			LOG_ERROR << "check authority failed: " << result.errmsg;
			send_response_error<dbc::node_list_disk_rsp>(NODE_LIST_DISK_RSP, node_req_msg->header, E_DEFAULT, "check authority failed: " + result.errmsg);
			return;
		}

		do_list_disk(node_req_msg->header, data, result);
	}
	else {
		node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
		network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
	}
}

void node_request_service::do_list_disk(const network::base_header& header,
    const std::shared_ptr<dbc::node_list_disk_req_data>& data, const AuthoriseResult& result) {
    FResult fret = FResultOk;

    auto taskinfo = TaskInfoMgr::instance().getTaskInfo(data->task_id);
    if (taskinfo == nullptr) {
        send_response_error<dbc::node_list_disk_rsp>(NODE_LIST_DISK_RSP, header, E_DEFAULT, "task not exist");
        return;
    }

    if (!VmClient::instance().IsExistDomain(data->task_id)) {
        send_response_error<dbc::node_list_disk_rsp>(NODE_LIST_DISK_RSP, header, E_DEFAULT, "task not exist");
        return;
    }

    std::string bios_mode = taskinfo->getBiosMode();
    if (bios_mode == "pxe") {
		send_response_error<dbc::node_list_disk_rsp>(NODE_LIST_DISK_RSP, header, E_DEFAULT, "pxe not support disk");
		return;
    }

    std::map<std::string, std::shared_ptr<DiskInfo>> mpdisks;
    TaskDiskMgr::instance().listDisks(data->task_id, mpdisks);

    std::stringstream ss;
    ss << "{";
    ss << "\"errcode\":" << ERR_SUCCESS;
    ss << ",\"message\":" << "[";
    int count = 0;
    for (auto& iter : mpdisks) {
        if (count > 0)
            ss << ",";

        ss << "{";
        ss << "\"name\":" << "\"" << iter.second->getName() << "\"";
        ss << ",\"file\":" << "\"" << iter.second->getSourceFile() << "\"";
        ss << ",\"size\":" << "\"" << scale_size(iter.second->getVirtualSize() / 1024L) << "\"";
        ss << "}";

        count++;
    }
    ss << "]";
    ss << "}";

    const std::map<std::string, std::string>& mp = header.exten_info;
    auto it = mp.find("pub_key");
    if (it != mp.end()) {
        std::string pub_key = it->second;
        std::string priv_key = ConfManager::instance().GetPrivKey();

        if (!pub_key.empty() && !priv_key.empty()) {
            std::string s_data = encrypt_data((unsigned char*)ss.str().c_str(), ss.str().size(), pub_key, priv_key);
            send_response_json<dbc::node_list_disk_rsp>(NODE_LIST_DISK_RSP, header, s_data);
        }
        else {
            LOG_ERROR << "pub_key or priv_key is empty";
            send_response_error<dbc::node_list_disk_rsp>(NODE_LIST_DISK_RSP, header, E_DEFAULT, "pub_key or priv_key is empty");
        }
    }
    else {
        LOG_ERROR << "request no pub_key";
        send_response_error<dbc::node_list_disk_rsp>(NODE_LIST_DISK_RSP, header, E_DEFAULT, "request no pub_key");
    }
}

// 磁盘扩容
void node_request_service::on_node_resize_disk_req(const std::shared_ptr<network::message>& msg) {
	auto node_req_msg = std::dynamic_pointer_cast<dbc::node_resize_disk_req>(msg->get_content());
	if (node_req_msg == nullptr) {
		return;
	}

	if (!check_req_header_nonce(node_req_msg->header.nonce)) {
		return;
	}

	if (Server::NodeType != NODE_TYPE::ComputeNode) {
		node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
		network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
		return;
	}

	if (!check_req_header(msg)) {
		LOG_ERROR << "request header check failed";
		node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
		network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
		return;
	}

	// decrypt
	std::string pub_key = node_req_msg->header.exten_info["pub_key"];
	std::string priv_key = ConfManager::instance().GetPrivKey();
	if (pub_key.empty() || priv_key.empty()) {
		LOG_ERROR << "pub_key or priv_key is empty";
		node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
		network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
		return;
	}

	std::shared_ptr<dbc::node_resize_disk_req_data> data = std::make_shared<dbc::node_resize_disk_req_data>();
	try {
		std::string ori_message;
		bool succ = decrypt_data(node_req_msg->body.data, pub_key, priv_key, ori_message);
		if (!succ || ori_message.empty()) {
			node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
			network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
			return;
		}

		std::shared_ptr<byte_buf> snapshot_buf = std::make_shared<byte_buf>();
		snapshot_buf->write_to_byte_buf(ori_message.c_str(), ori_message.size());
		network::binary_protocol proto(snapshot_buf.get());
		data->read(&proto);
	}
	catch (std::exception& e) {
		node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
		network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
		return;
	}

	std::vector<std::string> req_peer_nodes = data->peer_nodes_list;
	bool hit_self = hit_node(req_peer_nodes, ConfManager::instance().GetNodeId());
	if (hit_self) {
		AuthorityParams params;
		params.wallet = data->wallet;
		params.nonce = data->nonce;
		params.sign = data->sign;
		params.multisig_wallets = data->multisig_wallets;
		params.multisig_threshold = data->multisig_threshold;
		params.multisig_signs = data->multisig_signs;
		params.session_id = data->session_id;
		params.session_id_sign = data->session_id_sign;
		AuthoriseResult result;
		check_authority(params, result);
		if (!result.success || result.user_role == USER_ROLE::Unknown || result.user_role == USER_ROLE::Verifier) {
			LOG_ERROR << "check authority failed: " << result.errmsg;
			send_response_error<dbc::node_resize_disk_rsp>(NODE_RESIZE_DISK_RSP, node_req_msg->header, E_DEFAULT, "check authority failed: " + result.errmsg);
			return;
		}

		do_resize_disk(node_req_msg->header, data, result);
	}
	else {
		node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
		network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
	}
}

void node_request_service::do_resize_disk(const network::base_header& header, 
    const std::shared_ptr<dbc::node_resize_disk_req_data>& data, const AuthoriseResult& result) {
	FResult fret = FResultOk;

	auto taskinfo = TaskInfoMgr::instance().getTaskInfo(data->task_id);
	if (taskinfo == nullptr) {
		send_response_error<dbc::node_resize_disk_rsp>(NODE_RESIZE_DISK_RSP, header, E_DEFAULT, "task not exist");
		return;
	}

	if (!VmClient::instance().IsExistDomain(data->task_id)) {
		send_response_error<dbc::node_resize_disk_rsp>(NODE_RESIZE_DISK_RSP, header, E_DEFAULT, "task not exist");
		return;
	}

	std::string bios_mode = taskinfo->getBiosMode();
	if (bios_mode == "pxe") {
		send_response_error<dbc::node_resize_disk_rsp>(NODE_RESIZE_DISK_RSP, header, E_DEFAULT, "pxe not support disk");
		return;
	}

    // 解析请求参数
	rapidjson::Document doc;
	doc.Parse(data->additional.c_str());
	if (!doc.IsObject()) {
		LOG_ERROR << "parse xml failed";
		send_response_error<dbc::node_resize_disk_rsp>(NODE_RESIZE_DISK_RSP, header, E_DEFAULT, "parse xml failed");
        return;
    }

    std::string s_disk;
    JSON_PARSE_STRING(doc, "disk", s_disk);
    if (s_disk.empty()) {
		LOG_ERROR << "'disk' is empty";
		send_response_error<dbc::node_resize_disk_rsp>(NODE_RESIZE_DISK_RSP, header, E_DEFAULT, "'disk' is empty");
        return;
    }

	int n_size; //G
	JSON_PARSE_INT(doc, "size", n_size);
	if (n_size <= 0) {
		LOG_ERROR << "'size' is invalid";
		send_response_error<dbc::node_resize_disk_rsp>(NODE_RESIZE_DISK_RSP, header, E_DEFAULT, "'size' is invalid");
		return;
	}

    // 执行逻辑
    std::stringstream ss;

    fret = TaskDiskMgr::instance().resizeDisk(data->task_id, s_disk, n_size * 1024L * 1024L);
	if (fret.errcode == ERR_SUCCESS) {
		ss << "{";
		ss << "\"errcode\":" << ERR_SUCCESS;
		ss << ",\"message\":" << "\"ok\"";
		ss << "}";
	}
	else {
		ss << "{";
		ss << "\"errcode\":" << fret.errcode;
		ss << ",\"message\":" << "\"" << fret.errmsg << "\"";
		ss << "}";
	}

    // 返回结果
	const std::map<std::string, std::string>& mp = header.exten_info;
	auto it = mp.find("pub_key");
	if (it != mp.end()) {
		std::string pub_key = it->second;
		std::string priv_key = ConfManager::instance().GetPrivKey();

		if (!pub_key.empty() && !priv_key.empty()) {
			std::string s_data = encrypt_data((unsigned char*)ss.str().c_str(), ss.str().size(), pub_key, priv_key);
			send_response_json<dbc::node_resize_disk_rsp>(NODE_RESIZE_DISK_RSP, header, s_data);
		}
		else {
			LOG_ERROR << "pub_key or priv_key is empty";
			send_response_error<dbc::node_resize_disk_rsp>(NODE_RESIZE_DISK_RSP, header, E_DEFAULT, "pub_key or priv_key is empty");
		}
	}
	else {
		LOG_ERROR << "request no pub_key";
		send_response_error<dbc::node_resize_disk_rsp>(NODE_RESIZE_DISK_RSP, header, E_DEFAULT, "request no pub_key");
	}
}

// 增加新数据盘
void node_request_service::on_node_add_disk_req(const std::shared_ptr<network::message>& msg) {
	auto node_req_msg = std::dynamic_pointer_cast<dbc::node_add_disk_req>(msg->get_content());
	if (node_req_msg == nullptr) {
		return;
	}

	if (!check_req_header_nonce(node_req_msg->header.nonce)) {
		return;
	}

	if (Server::NodeType != NODE_TYPE::ComputeNode) {
		node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
		network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
		return;
	}

	if (!check_req_header(msg)) {
		LOG_ERROR << "request header check failed";
		node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
		network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
		return;
	}

	// decrypt
	std::string pub_key = node_req_msg->header.exten_info["pub_key"];
	std::string priv_key = ConfManager::instance().GetPrivKey();
	if (pub_key.empty() || priv_key.empty()) {
		LOG_ERROR << "pub_key or priv_key is empty";
		node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
		network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
		return;
	}

	std::shared_ptr<dbc::node_add_disk_req_data> data = std::make_shared<dbc::node_add_disk_req_data>();
	try {
		std::string ori_message;
		bool succ = decrypt_data(node_req_msg->body.data, pub_key, priv_key, ori_message);
		if (!succ || ori_message.empty()) {
			node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
			network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
			return;
		}

		std::shared_ptr<byte_buf> snapshot_buf = std::make_shared<byte_buf>();
		snapshot_buf->write_to_byte_buf(ori_message.c_str(), ori_message.size());
		network::binary_protocol proto(snapshot_buf.get());
		data->read(&proto);
	}
	catch (std::exception& e) {
		node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
		network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
		return;
	}

	std::vector<std::string> req_peer_nodes = data->peer_nodes_list;
	bool hit_self = hit_node(req_peer_nodes, ConfManager::instance().GetNodeId());
	if (hit_self) {
		AuthorityParams params;
		params.wallet = data->wallet;
		params.nonce = data->nonce;
		params.sign = data->sign;
		params.multisig_wallets = data->multisig_wallets;
		params.multisig_threshold = data->multisig_threshold;
		params.multisig_signs = data->multisig_signs;
		params.session_id = data->session_id;
		params.session_id_sign = data->session_id_sign;
		AuthoriseResult result;
		check_authority(params, result);
		if (!result.success || result.user_role == USER_ROLE::Unknown || result.user_role == USER_ROLE::Verifier) {
			LOG_ERROR << "check authority failed: " << result.errmsg;
			send_response_error<dbc::node_add_disk_rsp>(NODE_ADD_DISK_RSP, node_req_msg->header, E_DEFAULT, "check authority failed: " + result.errmsg);
			return;
		}

		do_add_disk(node_req_msg->header, data, result);
	}
	else {
		node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
		network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
	}
}

void node_request_service::do_add_disk(const network::base_header& header, 
    const std::shared_ptr<dbc::node_add_disk_req_data>& data, const AuthoriseResult& result) {
	FResult fret = FResultOk;

	auto taskinfo = TaskInfoMgr::instance().getTaskInfo(data->task_id);
	if (taskinfo == nullptr) {
		send_response_error<dbc::node_add_disk_rsp>(NODE_ADD_DISK_RSP, header, E_DEFAULT, "task not exist");
		return;
	}

	if (!VmClient::instance().IsExistDomain(data->task_id)) {
		send_response_error<dbc::node_add_disk_rsp>(NODE_ADD_DISK_RSP, header, E_DEFAULT, "task not exist");
		return;
	}

	std::string bios_mode = taskinfo->getBiosMode();
	if (bios_mode == "pxe") {
		send_response_error<dbc::node_add_disk_rsp>(NODE_ADD_DISK_RSP, header, E_DEFAULT, "pxe not support disk");
		return;
	}

	// 解析请求参数
	rapidjson::Document doc;
	doc.Parse(data->additional.c_str());
	if (!doc.IsObject()) {
		LOG_ERROR << "parse xml failed";
		send_response_error<dbc::node_add_disk_rsp>(NODE_ADD_DISK_RSP, header, E_DEFAULT, "parse xml failed");
		return;
	}

	int n_size; //G
	JSON_PARSE_INT(doc, "size", n_size);
	if (n_size <= 0) {
		LOG_ERROR << "'size' is invalid";
		send_response_error<dbc::node_add_disk_rsp>(NODE_ADD_DISK_RSP, header, E_DEFAULT, "'size' is invalid");
		return;
	}

	std::string s_mount_dir;
	JSON_PARSE_STRING(doc, "mount_dir", s_mount_dir);
    if (s_mount_dir.empty()) {
        s_mount_dir = "/data";
    }

    if (!bfs::exists(s_mount_dir)) {
        LOG_ERROR << "mount_dir:" << s_mount_dir << " is not exist";
		send_response_error<dbc::node_add_disk_rsp>(NODE_ADD_DISK_RSP, header, E_DEFAULT, s_mount_dir + " is not exist");
		return;
    }

	// 执行逻辑
	std::stringstream ss;
     
    fret = TaskDiskMgr::instance().addNewDisk(data->task_id, n_size * 1024L * 1024L, s_mount_dir);
	if (fret.errcode == ERR_SUCCESS) {
		ss << "{";
		ss << "\"errcode\":" << ERR_SUCCESS;
		ss << ",\"message\":" << "\"ok\"";
		ss << "}";
	}
	else {
		ss << "{";
		ss << "\"errcode\":" << fret.errcode;
		ss << ",\"message\":" << "\"" << fret.errmsg << "\"";
		ss << "}";
	}

	// 返回结果
	const std::map<std::string, std::string>& mp = header.exten_info;
	auto it = mp.find("pub_key");
	if (it != mp.end()) {
		std::string pub_key = it->second;
		std::string priv_key = ConfManager::instance().GetPrivKey();

		if (!pub_key.empty() && !priv_key.empty()) {
			std::string s_data = encrypt_data((unsigned char*)ss.str().c_str(), ss.str().size(), pub_key, priv_key);
			send_response_json<dbc::node_add_disk_rsp>(NODE_ADD_DISK_RSP, header, s_data);
		}
		else {
			LOG_ERROR << "pub_key or priv_key is empty";
			send_response_error<dbc::node_add_disk_rsp>(NODE_ADD_DISK_RSP, header, E_DEFAULT, "pub_key or priv_key is empty");
		}
	}
	else {
		LOG_ERROR << "request no pub_key";
		send_response_error<dbc::node_add_disk_rsp>(NODE_ADD_DISK_RSP, header, E_DEFAULT, "request no pub_key");
	}
}

// 删除数据盘
void node_request_service::on_node_delete_disk_req(const std::shared_ptr<network::message>& msg) {
	auto node_req_msg = std::dynamic_pointer_cast<dbc::node_delete_disk_req>(msg->get_content());
	if (node_req_msg == nullptr) {
		return;
	}

	if (!check_req_header_nonce(node_req_msg->header.nonce)) {
		return;
	}

	if (Server::NodeType != NODE_TYPE::ComputeNode) {
		node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
		network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
		return;
	}

	if (!check_req_header(msg)) {
		LOG_ERROR << "request header check failed";
		node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
		network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
		return;
	}

	// decrypt
	std::string pub_key = node_req_msg->header.exten_info["pub_key"];
	std::string priv_key = ConfManager::instance().GetPrivKey();
	if (pub_key.empty() || priv_key.empty()) {
		LOG_ERROR << "pub_key or priv_key is empty";
		node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
		network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
		return;
	}

	std::shared_ptr<dbc::node_delete_disk_req_data> data = std::make_shared<dbc::node_delete_disk_req_data>();
	try {
		std::string ori_message;
		bool succ = decrypt_data(node_req_msg->body.data, pub_key, priv_key, ori_message);
		if (!succ || ori_message.empty()) {
			node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
			network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
			return;
		}

		std::shared_ptr<byte_buf> snapshot_buf = std::make_shared<byte_buf>();
		snapshot_buf->write_to_byte_buf(ori_message.c_str(), ori_message.size());
		network::binary_protocol proto(snapshot_buf.get());
		data->read(&proto);
	}
	catch (std::exception& e) {
		node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
		network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
		return;
	}

	std::vector<std::string> req_peer_nodes = data->peer_nodes_list;
	bool hit_self = hit_node(req_peer_nodes, ConfManager::instance().GetNodeId());
	if (hit_self) {
		AuthorityParams params;
		params.wallet = data->wallet;
		params.nonce = data->nonce;
		params.sign = data->sign;
		params.multisig_wallets = data->multisig_wallets;
		params.multisig_threshold = data->multisig_threshold;
		params.multisig_signs = data->multisig_signs;
		params.session_id = data->session_id;
		params.session_id_sign = data->session_id_sign;
		AuthoriseResult result;
		check_authority(params, result);
		if (!result.success || result.user_role == USER_ROLE::Unknown || result.user_role == USER_ROLE::Verifier) {
			LOG_ERROR << "check authority failed: " << result.errmsg;
			send_response_error<dbc::node_delete_disk_rsp>(NODE_DELETE_DISK_RSP, node_req_msg->header, E_DEFAULT, "check authority failed: " + result.errmsg);
			return;
		}

		do_delete_disk(node_req_msg->header, data, result);
	}
	else {
		node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
		network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
	}
}

void node_request_service::do_delete_disk(const network::base_header& header, 
    const std::shared_ptr<dbc::node_delete_disk_req_data>& data, const AuthoriseResult& result) {
	FResult fret = FResultOk;

	auto taskinfo = TaskInfoMgr::instance().getTaskInfo(data->task_id);
	if (taskinfo == nullptr) {
		send_response_error<dbc::node_delete_disk_rsp>(NODE_DELETE_DISK_RSP, header, E_DEFAULT, "task not exist");
		return;
	}

	if (!VmClient::instance().IsExistDomain(data->task_id)) {
		send_response_error<dbc::node_delete_disk_rsp>(NODE_DELETE_DISK_RSP, header, E_DEFAULT, "task not exist");
		return;
	}

	std::string bios_mode = taskinfo->getBiosMode();
	if (bios_mode == "pxe") {
		send_response_error<dbc::node_delete_disk_rsp>(NODE_DELETE_DISK_RSP, header, E_DEFAULT, "pxe not support disk");
		return;
	}

	// 解析请求参数
	rapidjson::Document doc;
	doc.Parse(data->additional.c_str());
	if (!doc.IsObject()) {
		LOG_ERROR << "parse xml failed";
		send_response_error<dbc::node_delete_disk_rsp>(NODE_DELETE_DISK_RSP, header, E_DEFAULT, "parse xml failed");
		return;
	}

	std::string s_disk;
	JSON_PARSE_STRING(doc, "disk", s_disk);
	if (s_disk.empty()) {
		LOG_ERROR << "'disk' is empty";
		send_response_error<dbc::node_delete_disk_rsp>(NODE_DELETE_DISK_RSP, header, E_DEFAULT, "'disk' is empty");
		return;
	}

	// 执行逻辑
	std::stringstream ss;

    fret = TaskDiskMgr::instance().deleteDisk(data->task_id, s_disk);
	if (fret.errcode == ERR_SUCCESS) {
		ss << "{";
		ss << "\"errcode\":" << ERR_SUCCESS;
		ss << ",\"message\":" << "\"ok\"";
		ss << "}";
	}
	else {
		ss << "{";
		ss << "\"errcode\":" << fret.errcode;
		ss << ",\"message\":" << "\"" << fret.errmsg << "\"";
		ss << "}";
	}

	// 返回结果
	const std::map<std::string, std::string>& mp = header.exten_info;
	auto it = mp.find("pub_key");
	if (it != mp.end()) {
		std::string pub_key = it->second;
		std::string priv_key = ConfManager::instance().GetPrivKey();

		if (!pub_key.empty() && !priv_key.empty()) {
			std::string s_data = encrypt_data((unsigned char*)ss.str().c_str(), ss.str().size(), pub_key, priv_key);
			send_response_json<dbc::node_delete_disk_rsp>(NODE_DELETE_DISK_RSP, header, s_data);
		}
		else {
			LOG_ERROR << "pub_key or priv_key is empty";
			send_response_error<dbc::node_delete_disk_rsp>(NODE_DELETE_DISK_RSP, header, E_DEFAULT, "pub_key or priv_key is empty");
		}
	}
	else {
		LOG_ERROR << "request no pub_key";
		send_response_error<dbc::node_delete_disk_rsp>(NODE_DELETE_DISK_RSP, header, E_DEFAULT, "request no pub_key");
	}
}

// 查询节点信息
void node_request_service::on_node_query_node_info_req(const std::shared_ptr<network::message>& msg) {
    auto node_req_msg = std::dynamic_pointer_cast<dbc::node_query_node_info_req>(msg->get_content());
    if (node_req_msg == nullptr) {
        return;
    }

    if (!check_req_header_nonce(node_req_msg->header.nonce)) {
        return;
    }

    if (Server::NodeType != NODE_TYPE::ComputeNode) {
        node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
        network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
        return;
    }

    if (!check_req_header(msg)) {
        LOG_ERROR << "request header check failed";
        node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
        network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
        return;
    }

    // decrypt
    std::string pub_key = node_req_msg->header.exten_info["pub_key"];
    std::string priv_key = ConfManager::instance().GetPrivKey();
    if (pub_key.empty() || priv_key.empty()) {
        LOG_ERROR << "pub_key or priv_key is empty";
        node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
        network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
        return;
    }

    std::shared_ptr<dbc::node_query_node_info_req_data> data = std::make_shared<dbc::node_query_node_info_req_data>();
    try {
        std::string ori_message;
        bool succ = decrypt_data(node_req_msg->body.data, pub_key, priv_key, ori_message);
        if (!succ || ori_message.empty()) {
            node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
            network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
            return;
        }

        std::shared_ptr<byte_buf> task_buf = std::make_shared<byte_buf>();
        task_buf->write_to_byte_buf(ori_message.c_str(), ori_message.size());
        network::binary_protocol proto(task_buf.get());
        data->read(&proto);
    }
    catch (std::exception& e) {
        node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
        network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
        return;
    }

    std::vector<std::string> req_peer_nodes = data->peer_nodes_list;
    bool hit_self = hit_node(req_peer_nodes, ConfManager::instance().GetNodeId());
    if (hit_self) {
        query_node_info(node_req_msg->header, data);
    }
    else {
        node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
        network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
    }
}

void node_request_service::query_node_info(const network::base_header& header,
    const std::shared_ptr<dbc::node_query_node_info_req_data>& data) {
    std::stringstream ss;
    ss << "{";
    ss << "\"errcode\":" << 0;
    ss << ",\"message\":" << "{";
    ss << "\"version\":" << "\"" << dbcversion() << "\"";
    ss << ",\"ip\":" << "\"" << hide_ip_addr(SystemInfo::instance().GetPublicip()) << "\"";
    ss << ",\"os\":" << "\"" << SystemInfo::instance().GetOsName() << "\"";
    cpu_info tmp_cpuinfo = SystemInfo::instance().GetCpuInfo();
    ss << ",\"cpu\":" << "{";
    ss << "\"type\":" << "\"" << tmp_cpuinfo.cpu_name << "\"";
    ss << ",\"hz\":" << "\"" << tmp_cpuinfo.mhz << "\"";
    ss << ",\"cores\":" << "\"" << tmp_cpuinfo.cores << "\"";
    ss << ",\"used_usage\":" << "\"" << f2s(SystemInfo::instance().GetCpuUsage() * 100) << "%" << "\"";
    ss << "}";

    ss << ",\"gpu\":" << "{";
    const std::map<std::string, gpu_info> gpu_infos = SystemInfo::instance().GetGpuInfo();
    int gpu_count = gpu_infos.size();
    ss << "\"gpu_count\":" << "\"" << gpu_count << "\"";
    int gpu_used = 0;
    auto taskinfos = TaskInfoMgr::instance().getAllTaskInfos();
    for (const auto& taskinfo : taskinfos) {
        if (taskinfo.second->getTaskStatus() != TaskStatus::TS_Task_Shutoff 
            && taskinfo.second->getTaskStatus() < TaskStatus::TS_Error) {
            auto gpus = TaskGpuMgr::instance().getTaskGpus(taskinfo.second->getTaskId());
            gpu_used += gpus.size();
        }
    }
    ss << ",\"gpu_used\":" << "\"" << gpu_used << "\"";
    ss << "}";

    mem_info tmp_meminfo = SystemInfo::instance().GetMemInfo();
    ss << ",\"mem\":" << "{";
    ss << "\"size\":" << "\"" << size2GB(tmp_meminfo.total) << "\"";
    ss << ",\"free\":" << "\"" << size2GB(tmp_meminfo.free) << "\"";
    ss << ",\"used_usage\":" << "\"" << f2s(tmp_meminfo.usage * 100) << "%" << "\"";
    ss << "}";
     
    disk_info tmp_diskinfo;
	SystemInfo::instance().GetDiskInfo("/data", tmp_diskinfo);
    ss << ",\"disk_system\":" << "{";
    ss << "\"type\":" << "\"" << (tmp_diskinfo.disk_type == DISK_SSD ? "SSD" : "HDD") << "\"";
    ss << ",\"size\":" << "\"" << g_disk_system_size << "G\"";
    ss << "}";
 
    ss << ",\"disk_data\":" << "[";
    auto mpdisks = SystemInfo::instance().GetDiskInfo();
    int disk_count = 0;
    for (auto iter_disk : mpdisks) {
        if (disk_count > 0) ss << ",";

        ss << "{";
        ss << "\"path\":" << "\"" << iter_disk.first << "\"";
		ss << ",\"type\":" << "\"" << (iter_disk.second.disk_type == DISK_SSD ? "SSD" : "HDD") << "\"";
		ss << ",\"size\":" << "\"" << size2GB(iter_disk.second.total) << "\"";
        int64_t disk_free = 0;
        if (iter_disk.first == "/data") {
            disk_free = std::min(iter_disk.second.total - g_disk_system_size * 1024L * 1024L, iter_disk.second.available);
            disk_free = std::max(0L, disk_free);
        }
        else {
            disk_free = iter_disk.second.available;
        }
		ss << ",\"free\":" << "\"" << size2GB(disk_free) << "\"";
		ss << ",\"used_usage\":" << "\"" << f2s(iter_disk.second.usage * 100) << "%" << "\"";
        ss << "}";

        disk_count++;
    }
    ss << "]";

    std::vector<ImageFile> images;
    ImageServer imgsvr;
    imgsvr.from_string(data->image_server);
    ImageMgr::instance().listLocalShareImages(imgsvr, images);
    ss << ",\"images\":" << "[";
    for (int i = 0; i < images.size(); ++i) {
        if (i > 0) ss << ",";

        ss << "{";
        ss << "\"image_name\":" << "\"" << images[i].name << "\"";
        ss << ",\"image_size\":" << "\"" << scale_size(images[i].size / 1024L) << "\"";
        ss << "}";
    }
    ss << "]";
    /*
    int32_t count = TaskMgr::instance().GetRunningTaskSize();
    std::string state;
    if (count <= 0) {
        state = "idle";
    }
    else {
        state = "busy(" + std::to_string(count) + ")";
    }
    ss << ",\"state\":" << "\"" << state << "\"";
    */
    ss << "}";
    ss << "}";

    const std::map<std::string, std::string>& mp = header.exten_info;
    auto it = mp.find("pub_key");
    if (it != mp.end()) {
        std::string pub_key = it->second;
        std::string priv_key = ConfManager::instance().GetPrivKey();

        if (!pub_key.empty() && !priv_key.empty()) {
            std::string s_data = encrypt_data((unsigned char*)ss.str().c_str(), ss.str().size(), pub_key, priv_key);
            send_response_json<dbc::node_query_node_info_rsp>(NODE_QUERY_NODE_INFO_RSP, header, s_data);
        }
        else {
            LOG_ERROR << "pub_key or priv_key is empty";
            send_response_error<dbc::node_query_node_info_rsp>(NODE_QUERY_NODE_INFO_RSP, header, E_DEFAULT, "pub_key or priv_key is empty");
        }
    }
    else {
        LOG_ERROR << "request no pub_key";
        send_response_error<dbc::node_query_node_info_rsp>(NODE_QUERY_NODE_INFO_RSP, header, E_DEFAULT, "request no pub_key");
    }
}

// 查询session_id
void node_request_service::on_node_session_id_req(const std::shared_ptr<network::message> &msg) {
    auto node_req_msg = std::dynamic_pointer_cast<dbc::node_session_id_req>(msg->get_content());
    if (node_req_msg == nullptr) {
        return;
    }

    if (!check_req_header_nonce(node_req_msg->header.nonce)) {
        return;
    }

    if (Server::NodeType != NODE_TYPE::ComputeNode) {
        node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
        network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
        return;
    }

    if (!check_req_header(msg)) {
        LOG_ERROR << "request header check failed";
        node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
        network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
        return;
    }

    // decrypt
    std::string pub_key = node_req_msg->header.exten_info["pub_key"];
    std::string priv_key = ConfManager::instance().GetPrivKey();
    if (pub_key.empty() || priv_key.empty()) {
        LOG_ERROR << "pub_key or priv_key is empty";
        node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
        network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
        return;
    }

    std::shared_ptr<dbc::node_session_id_req_data> data = std::make_shared<dbc::node_session_id_req_data>();
    try {
        std::string ori_message;
        bool succ = decrypt_data(node_req_msg->body.data, pub_key, priv_key, ori_message);
        if (!succ || ori_message.empty()) {
            node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
            network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
            return;
        }

        std::shared_ptr<byte_buf> task_buf = std::make_shared<byte_buf>();
        task_buf->write_to_byte_buf(ori_message.c_str(), ori_message.size());
        network::binary_protocol proto(task_buf.get());
        data->read(&proto);
    } catch (std::exception &e) {
        node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
        network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
        return;
    }

    std::vector<std::string> req_peer_nodes = data->peer_nodes_list;
    bool hit_self = hit_node(req_peer_nodes, ConfManager::instance().GetNodeId());
    if (hit_self) {
        AuthorityParams params;
        params.wallet = data->wallet;
        params.nonce = data->nonce;
        params.sign = data->sign;
        params.multisig_wallets = data->multisig_wallets;
        params.multisig_threshold = data->multisig_threshold;
        params.multisig_signs = data->multisig_signs;
        AuthoriseResult result;
        check_authority(params, result);
        if (!result.success) {
            LOG_ERROR << "check authority failed: " << result.errmsg;
            send_response_error<dbc::node_session_id_rsp>(NODE_SESSION_ID_RSP, node_req_msg->header, E_DEFAULT, "check authority failed: " + result.errmsg);
            return;
        }

        node_session_id(node_req_msg->header, data, result);
    } else {
        node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
        network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
    }
}

void node_request_service::node_session_id(const network::base_header &header,
                                           const std::shared_ptr<dbc::node_session_id_req_data> &data, const AuthoriseResult& result) {
    if (result.machine_status == MACHINE_STATUS::Rented && result.user_role == USER_ROLE::WalletRenter) {
        std::string session_id = TaskMgr::instance().getSessionId(result.rent_wallet);
        if (session_id.empty()) {
            send_response_error<dbc::node_session_id_rsp>(NODE_SESSION_ID_RSP, header, E_DEFAULT, "no session id");
        } else {
            std::stringstream ss;
            ss << "{";
            ss << "\"errcode\":" << ERR_SUCCESS;
            ss << ", \"message\":{";
            ss << "\"session_id\":\"" << session_id << "\"";
            ss << "}";
            ss << "}";

            const std::map<std::string, std::string>& mp = header.exten_info;
            auto it = mp.find("pub_key");
            if (it != mp.end()) {
                std::string pub_key = it->second;
                std::string priv_key = ConfManager::instance().GetPrivKey();

                if (!pub_key.empty() && !priv_key.empty()) {
                    std::string s_data = encrypt_data((unsigned char*) ss.str().c_str(), ss.str().size(), pub_key, priv_key);
                    send_response_json<dbc::node_session_id_rsp>(NODE_SESSION_ID_RSP, header, s_data);
                } else {
                    LOG_ERROR << "pub_key or priv_key is empty";
                    send_response_error<dbc::node_session_id_rsp>(NODE_SESSION_ID_RSP, header, E_DEFAULT, "pub_key or priv_key is empty");
                }
            } else {
                LOG_ERROR << "request no pub_key";
                send_response_error<dbc::node_session_id_rsp>(NODE_SESSION_ID_RSP, header, E_DEFAULT, "request no pub_key");
            }
        }
    } else {
        LOG_INFO << "you are not renter of this machine";
        send_response_error<dbc::node_session_id_rsp>(NODE_SESSION_ID_RSP, header, E_DEFAULT, "check authority failed");
    }
}

// 释放内存
void node_request_service::on_node_free_memory_req(const std::shared_ptr<network::message>& msg) {
	auto node_req_msg = std::dynamic_pointer_cast<dbc::node_free_memory_req>(msg->get_content());
	if (node_req_msg == nullptr) {
		return;
	}

	if (!check_req_header_nonce(node_req_msg->header.nonce)) {
		return;
	}

	if (Server::NodeType != NODE_TYPE::ComputeNode) {
		node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
		network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
		return;
	}

	if (!check_req_header(msg)) {
		LOG_ERROR << "request header check failed";
		node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
		network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
		return;
	}

	// decrypt
	std::string pub_key = node_req_msg->header.exten_info["pub_key"];
	std::string priv_key = ConfManager::instance().GetPrivKey();
	if (pub_key.empty() || priv_key.empty()) {
		LOG_ERROR << "pub_key or priv_key is empty";
		node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
		network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
		return;
	}

	std::shared_ptr<dbc::node_free_memory_req_data> data = std::make_shared<dbc::node_free_memory_req_data>();
	try {
		std::string ori_message;
		bool succ = decrypt_data(node_req_msg->body.data, pub_key, priv_key, ori_message);
		if (!succ || ori_message.empty()) {
			node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
			network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
			return;
		}

		std::shared_ptr<byte_buf> task_buf = std::make_shared<byte_buf>();
		task_buf->write_to_byte_buf(ori_message.c_str(), ori_message.size());
		network::binary_protocol proto(task_buf.get());
		data->read(&proto);
	}
	catch (std::exception& e) {
		node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
		network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
		return;
	}

	std::vector<std::string> req_peer_nodes = data->peer_nodes_list;
	bool hit_self = hit_node(req_peer_nodes, ConfManager::instance().GetNodeId());
	if (hit_self) {
		AuthorityParams params;
		params.wallet = data->wallet;
		params.nonce = data->nonce;
		params.sign = data->sign;
		params.multisig_wallets = data->multisig_wallets;
		params.multisig_threshold = data->multisig_threshold;
		params.multisig_signs = data->multisig_signs;
		params.session_id = data->session_id;
		params.session_id_sign = data->session_id_sign;
		AuthoriseResult result;
		check_authority(params, result);
		if (!result.success) {
			LOG_ERROR << "check authority failed: " << result.errmsg;
			send_response_error<dbc::node_free_memory_rsp>(NODE_FREE_MEMORY_RSP, node_req_msg->header, E_DEFAULT, "check authority failed: " + result.errmsg);
			return;
		}

		node_free_memory(node_req_msg->header, data, result);
	}
	else {
		node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
		network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
	}
}

void node_request_service::node_free_memory(const network::base_header& header,
	const std::shared_ptr<dbc::node_free_memory_req_data>& data, const AuthoriseResult& result) {
    
    run_shell("echo 3 > /proc/sys/vm/drop_caches");
    
    send_response_ok<dbc::node_free_memory_rsp>(NODE_FREE_MEMORY_RSP, header);
}

void node_request_service::on_training_task_timer(const std::shared_ptr<core_timer>& timer) {
    //TaskMgr::instance().ProcessTask();
}

void node_request_service::on_prune_task_timer(const std::shared_ptr<core_timer>& timer) {
    //TaskMgr::instance().PruneTask();
}

void node_request_service::on_timer_service_broadcast(const std::shared_ptr<core_timer>& timer)
{
    auto s_map_size = ServiceInfoManager::instance().size();
    if (s_map_size == 0) {
        return;
    }

    if (Server::NodeType == NODE_TYPE::ComputeNode) {
        /*
        int32_t count = TaskMgr::instance().GetRunningTaskSize();
        std::string state;
        if (count <= 0) {
            state = "idle";
        } else {
            state = "busy(" + std::to_string(count) + ")";
        }

        ServiceInfoManager::instance().update(ConfManager::instance().GetNodeId(), "state", state);
        */
        ServiceInfoManager::instance().update(ConfManager::instance().GetNodeId(), "version", dbcversion());
        ServiceInfoManager::instance().update_time_stamp(ConfManager::instance().GetNodeId());
    }

    ServiceInfoManager::instance().remove_unlived_nodes(TIME_SERVICE_INFO_LIST_EXPIRED);

    auto service_info_map = ServiceInfoManager::instance().get_change_set();
    if(!service_info_map.empty()) {
        auto service_broadcast_req = create_service_broadcast_req_msg(service_info_map);
        if (service_broadcast_req != nullptr) {
            network::connection_manager::instance().broadcast_message(service_broadcast_req);
        }
    }
}

std::shared_ptr<network::message> node_request_service::create_service_broadcast_req_msg(
    const std::map <std::string, std::shared_ptr<dbc::node_service_info>>& mp) {
    auto req_content = std::make_shared<dbc::service_broadcast_req>();
    // header
    req_content->header.__set_magic(ConfManager::instance().GetNetFlag());
    req_content->header.__set_msg_name(SERVICE_BROADCAST_REQ);
    req_content->header.__set_nonce(util::create_nonce());
    req_content->header.__set_session_id(util::create_session_id());
    std::vector<std::string> path;
    path.push_back(ConfManager::instance().GetNodeId());
    req_content->header.__set_path(path);
    std::map<std::string, std::string> exten_info;
    std::string sign_message = req_content->header.nonce + req_content->header.session_id;
    std::string signature = util::sign(sign_message, ConfManager::instance().GetNodePrivateKey());
    if (signature.empty())  return nullptr;
    exten_info["sign"] = signature;
    exten_info["node_id"] = ConfManager::instance().GetNodeId();
    exten_info["pub_key"] = ConfManager::instance().GetPubKey();
    req_content->header.__set_exten_info(exten_info);
    // body
    std::map <std::string, dbc::node_service_info> service_infos;
    for (auto it : mp) {
        service_infos.insert({ it.first, *it.second });
    }
    req_content->body.__set_node_service_info_map(service_infos);

    auto req_msg = std::make_shared<network::message>();
    req_msg->set_name(SERVICE_BROADCAST_REQ);
    req_msg->set_content(req_content);
    return req_msg;
}

void node_request_service::on_net_service_broadcast_req(const std::shared_ptr<network::message> &msg) {
    auto node_req_msg = std::dynamic_pointer_cast<dbc::service_broadcast_req>(msg->get_content());
    if (node_req_msg == nullptr) {
        return;
    }

    if (!check_req_header_nonce(node_req_msg->header.nonce)) {
        return;
    }

    if (!check_req_header(msg)) {
        LOG_ERROR << "req header check failed";
        return;
    }

    std::string sign_msg = node_req_msg->header.nonce + node_req_msg->header.session_id;
    if (!util::verify_sign(node_req_msg->header.exten_info["sign"], sign_msg, node_req_msg->header.exten_info["node_id"])) {
        LOG_ERROR << "verify sign error." << node_req_msg->header.exten_info["origin_id"];
        return;
    }

    std::map<std::string, dbc::node_service_info>& mp = node_req_msg->body.node_service_info_map;
    std::map<std::string, std::shared_ptr<dbc::node_service_info>> service_infos;
    for (auto& it : mp) {
        std::shared_ptr<dbc::node_service_info> ptr = std::make_shared<dbc::node_service_info>();
        *ptr = it.second;
        service_infos.insert({ it.first, ptr });
    }
    ServiceInfoManager::instance().add(service_infos);
}

// monitor
void node_request_service::on_node_list_monitor_server_req(const std::shared_ptr<network::message>& msg) {
    auto node_req_msg = std::dynamic_pointer_cast<dbc::node_list_monitor_server_req>(msg->get_content());
    if (node_req_msg == nullptr) {
        return;
    }

    if (!check_req_header_nonce(node_req_msg->header.nonce)) {
        return;
    }

    if (Server::NodeType != NODE_TYPE::ComputeNode) {
        node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
        network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
        return;
    }

    if (!check_req_header(msg)) {
        LOG_ERROR << "request header check failed";
        node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
        network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
        return;
    }

    // decrypt
    std::string pub_key = node_req_msg->header.exten_info["pub_key"];
    std::string priv_key = ConfManager::instance().GetPrivKey();
    if (pub_key.empty() || priv_key.empty()) {
        LOG_ERROR << "pub_key or priv_key is empty";
        node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
        network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
        return;
    }

    std::shared_ptr<dbc::node_list_monitor_server_req_data> data = std::make_shared<dbc::node_list_monitor_server_req_data>();
    try {
        std::string ori_message;
        bool succ = decrypt_data(node_req_msg->body.data, pub_key, priv_key, ori_message);
        if (!succ || ori_message.empty()) {
            node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
            network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
            return;
        }

        std::shared_ptr<byte_buf> buf = std::make_shared<byte_buf>();
        buf->write_to_byte_buf(ori_message.c_str(), ori_message.size());
        network::binary_protocol proto(buf.get());
        data->read(&proto);
    } catch (std::exception &e) {
        node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
        network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
        return;
    }

    std::vector<std::string> req_peer_nodes = data->peer_nodes_list;
    bool hit_self = hit_node(req_peer_nodes, ConfManager::instance().GetNodeId());
    if (hit_self) {
        AuthorityParams params;
        params.wallet = data->wallet;
        params.nonce = data->nonce;
        params.sign = data->sign;
        params.multisig_wallets = data->multisig_wallets;
        params.multisig_threshold = data->multisig_threshold;
        params.multisig_signs = data->multisig_signs;
        params.session_id = data->session_id;
        params.session_id_sign = data->session_id_sign;
        AuthoriseResult result;
        check_authority(params, result);
        if (!result.success || result.user_role == USER_ROLE::Unknown || result.user_role == USER_ROLE::Verifier) {
            LOG_ERROR << "check authority failed: " << result.errmsg;
            send_response_error<dbc::node_list_monitor_server_rsp>(NODE_LIST_MONITOR_SERVER_RSP, node_req_msg->header, E_DEFAULT, "check authority failed: " + result.errmsg);
            return;
        }

        monitor_server_list(node_req_msg->header, data, result);
    } else {
        node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
        network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
    }
}

void node_request_service::monitor_server_list(const network::base_header& header,
                                       const std::shared_ptr<dbc::node_list_monitor_server_req_data>& data, const AuthoriseResult& result) {
    int ret_code = ERR_SUCCESS;
    std::string ret_msg = "ok";

    std::stringstream ss_tasks;
    ss_tasks << "{\"servers\":[";
    std::vector<monitor_server> server_list;
    node_monitor_service::instance().listMonitorServer(result.rent_wallet, server_list);
    int idx = 0;
    for (const auto &server : server_list) {
        if (idx > 0)
            ss_tasks << ",";

        ss_tasks << "\"" << server.host << ":" << server.port << "\"";
        idx++;
    }
    ss_tasks << "]}";
    
    std::stringstream ss;
    ss << "{";
    if (ret_code == ERR_SUCCESS) {
        ss << "\"errcode\":" << ERR_SUCCESS;
        ss << ", \"message\":" << ss_tasks.str();
    } else {
        ss << "\"errcode\":" << ret_code;
        ss << ", \"message\":" << "\"" << ret_msg << "\"";
    }
    ss << "}";

    const std::map<std::string, std::string>& mp = header.exten_info;
    auto it = mp.find("pub_key");
    if (it != mp.end()) {
        std::string pub_key = it->second;
        std::string priv_key = ConfManager::instance().GetPrivKey();

        if (!pub_key.empty() && !priv_key.empty()) {
            std::string s_data = encrypt_data((unsigned char*) ss.str().c_str(), ss.str().size(), pub_key, priv_key);
            send_response_json<dbc::node_list_monitor_server_rsp>(NODE_LIST_MONITOR_SERVER_RSP, header, s_data);
        } else {
            LOG_ERROR << "pub_key or priv_key is empty";
            send_response_error<dbc::node_list_monitor_server_rsp>(NODE_LIST_MONITOR_SERVER_RSP, header, E_DEFAULT, "pub_key or priv_key is empty");
        }
    } else {
        LOG_ERROR << "request no pub_key";
        send_response_error<dbc::node_list_monitor_server_rsp>(NODE_LIST_MONITOR_SERVER_RSP, header, E_DEFAULT, "request no pub_key");
    }
}

void node_request_service::on_node_set_monitor_server_req(const std::shared_ptr<network::message>& msg) {
    auto node_req_msg = std::dynamic_pointer_cast<dbc::node_set_monitor_server_req>(msg->get_content());
    if (node_req_msg == nullptr) {
        return;
    }

    if (!check_req_header_nonce(node_req_msg->header.nonce)) {
        return;
    }

    if (Server::NodeType != NODE_TYPE::ComputeNode) {
        node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
        network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
        return;
    }

    if (!check_req_header(msg)) {
        LOG_ERROR << "request header check failed";
        node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
        network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
        return;
    }

    // decrypt
    std::string pub_key = node_req_msg->header.exten_info["pub_key"];
    std::string priv_key = ConfManager::instance().GetPrivKey();
    if (pub_key.empty() || priv_key.empty()) {
        LOG_ERROR << "pub_key or priv_key is empty";
        node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
        network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
        return;
    }

    std::shared_ptr<dbc::node_set_monitor_server_req_data> data = std::make_shared<dbc::node_set_monitor_server_req_data>();
    try {
        std::string ori_message;
        bool succ = decrypt_data(node_req_msg->body.data, pub_key, priv_key, ori_message);
        if (!succ || ori_message.empty()) {
            node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
            network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
            return;
        }

        std::shared_ptr<byte_buf> buf = std::make_shared<byte_buf>();
        buf->write_to_byte_buf(ori_message.c_str(), ori_message.size());
        network::binary_protocol proto(buf.get());
        data->read(&proto);
    } catch (std::exception &e) {
        node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
        network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
        return;
    }

    std::vector<std::string> req_peer_nodes = data->peer_nodes_list;
    bool hit_self = hit_node(req_peer_nodes, ConfManager::instance().GetNodeId());
    if (hit_self) {
        AuthorityParams params;
        params.wallet = data->wallet;
        params.nonce = data->nonce;
        params.sign = data->sign;
        params.multisig_wallets = data->multisig_wallets;
        params.multisig_threshold = data->multisig_threshold;
        params.multisig_signs = data->multisig_signs;
        params.session_id = data->session_id;
        params.session_id_sign = data->session_id_sign;
        AuthoriseResult result;
        check_authority(params, result);
        if (!result.success || result.user_role == USER_ROLE::Unknown || result.user_role == USER_ROLE::Verifier) {
            LOG_ERROR << "check authority failed: " << result.errmsg;
            send_response_error<dbc::node_set_monitor_server_rsp>(NODE_SET_MONITOR_SERVER_RSP, node_req_msg->header, E_DEFAULT, "check authority failed: " + result.errmsg);
            return;
        }

        monitor_server_set(node_req_msg->header, data, result);
    } else {
        node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
        network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
    }
}

void node_request_service::monitor_server_set(const network::base_header& header,
                                       const std::shared_ptr<dbc::node_set_monitor_server_req_data>& data, const AuthoriseResult& result) {
    int ret_code = ERR_SUCCESS;
    std::string ret_msg = "ok";

    auto fresult = node_monitor_service::instance().setMonitorServer(result.rent_wallet, data->additional);
    ret_code = fresult.errcode;
    ret_msg = fresult.errmsg;

    if (ret_code == ERR_SUCCESS) {
        send_response_ok<dbc::node_set_monitor_server_rsp>(NODE_SET_MONITOR_SERVER_RSP, header);
    } else {
        send_response_error<dbc::node_set_monitor_server_rsp>(NODE_SET_MONITOR_SERVER_RSP, header, ret_code, ret_msg);
    }
}

// network
void node_request_service::on_node_list_lan_req(const std::shared_ptr<network::message>& msg) {
    auto node_req_msg = std::dynamic_pointer_cast<dbc::node_list_lan_req>(msg->get_content());
    if (node_req_msg == nullptr) {
        return;
    }

    if (!check_req_header_nonce(node_req_msg->header.nonce)) {
        return;
    }

    if (Server::NodeType != NODE_TYPE::ComputeNode) {
        node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
        network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
        return;
    }

    if (!check_req_header(msg)) {
        LOG_ERROR << "request header check failed";
        node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
        network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
        return;
    }

    // decrypt
    std::string pub_key = node_req_msg->header.exten_info["pub_key"];
    std::string priv_key = ConfManager::instance().GetPrivKey();
    if (pub_key.empty() || priv_key.empty()) {
        LOG_ERROR << "pub_key or priv_key is empty";
        node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
        network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
        return;
    }

    std::shared_ptr<dbc::node_list_lan_req_data> data = std::make_shared<dbc::node_list_lan_req_data>();
    try {
        std::string ori_message;
        bool succ = decrypt_data(node_req_msg->body.data, pub_key, priv_key, ori_message);
        if (!succ || ori_message.empty()) {
            node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
            network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
            return;
        }

        std::shared_ptr<byte_buf> buf = std::make_shared<byte_buf>();
        buf->write_to_byte_buf(ori_message.c_str(), ori_message.size());
        network::binary_protocol proto(buf.get());
        data->read(&proto);
    } catch (std::exception &e) {
        node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
        network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
        return;
    }

    std::vector<std::string> req_peer_nodes = data->peer_nodes_list;
    bool hit_self = hit_node(req_peer_nodes, ConfManager::instance().GetNodeId());
    if (hit_self) {
        AuthorityParams params;
        params.wallet = data->wallet;
        params.nonce = data->nonce;
        params.sign = data->sign;
        params.multisig_wallets = data->multisig_wallets;
        params.multisig_threshold = data->multisig_threshold;
        params.multisig_signs = data->multisig_signs;
        params.session_id = data->session_id;
        params.session_id_sign = data->session_id_sign;
        AuthoriseResult result;
        check_authority(params, result);
        if (!result.success || result.user_role == USER_ROLE::Unknown || result.user_role == USER_ROLE::Verifier) {
            LOG_ERROR << "check authority failed: " << result.errmsg;
            send_response_error<dbc::node_list_lan_rsp>(NODE_LIST_LAN_RSP, node_req_msg->header, E_DEFAULT, "check authority failed: " + result.errmsg);
            return;
        }

        list_lan(node_req_msg->header, data, result);
    } else {
        node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
        network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
    }
}

void node_request_service::list_lan(const network::base_header& header, const std::shared_ptr<dbc::node_list_lan_req_data>& data, const AuthoriseResult& result) {
    // send_response_error<dbc::node_list_lan_rsp>(NODE_LIST_LAN_RSP, header, ERR_SUCCESS, "AAAAAAAAAAAA");
    int ret_code = ERR_SUCCESS;
    std::string ret_msg = "ok";

    std::stringstream ss_networks;
    if (data->network_id.empty()) {
        ss_networks << "[";
        const std::map<std::string, std::shared_ptr<dbc::networkInfo>> networks = VxlanManager::instance().GetNetworks();
        int idx = 0;
        for (const auto &it : networks) {
            if (idx > 30) break;
            if (idx > 0) ss_networks << ",";

            ss_networks << "{";
            ss_networks << "\"network_name\":" << "\"" << it.second->networkId << "\"";
            ss_networks << ",\"bridge_name\":" << "\"" << it.second->bridgeName << "\"";
            ss_networks << ",\"vxlan_name\":" << "\"" << it.second->vxlanName << "\"";
            ss_networks << ",\"vxlan_vni\":" << "\"" << it.second->vxlanVni << "\"";
            ss_networks << ",\"ip_cidr\":" << "\"" << it.second->ipCidr << "\"";
            ss_networks << ",\"ip_start\":" << "\"" << it.second->ipStart << "\"";
            ss_networks << ",\"ip_end\":" << "\"" << it.second->ipEnd << "\"";
            ss_networks << ",\"dhcp_interface\":" << "\"" << it.second->dhcpInterface << "\"";
            ss_networks << ",\"machine_id\":" << "\"" << it.second->machineId << "\"";
            ss_networks << ",\"rent_wallet\":" << "\"" << it.second->rentWallet << "\"";
            ss_networks << ",\"members\":" << "[";
            int memberCounts = 0;
            for (const auto& task_id : it.second->members) {
                if (memberCounts > 0) ss_networks << ",";
                ss_networks << "\"" << task_id << "\"";
                memberCounts++;
            }
            ss_networks << "]";
            struct tm _tm{};
            time_t tt = it.second->lastUseTime;
            localtime_r(&tt, &_tm);
            char buf[256] = {0};
            memset(buf, 0, sizeof(char) * 256);
            strftime(buf, sizeof(char) * 256, "%Y-%m-%d %H:%M:%S", &_tm);
            ss_networks << ",\"lastUseTime\":" << "\"" << buf << "\"";
            ss_networks << ",\"native_flags\":" << "\"" << it.second->nativeFlags << "\"";
            ss_networks << "}";

            idx++;
        }
        ss_networks << "]";
    } else {
        std::shared_ptr<dbc::networkInfo> network = VxlanManager::instance().GetNetwork(data->network_id);
        if (network) {
            ss_networks << "{";
            ss_networks << "\"network_name\":" << "\"" << network->networkId << "\"";
            ss_networks << ",\"bridge_name\":" << "\"" << network->bridgeName << "\"";
            ss_networks << ",\"vxlan_name\":" << "\"" << network->vxlanName << "\"";
            ss_networks << ",\"vxlan_vni\":" << "\"" << network->vxlanVni << "\"";
            ss_networks << ",\"ip_cidr\":" << "\"" << network->ipCidr << "\"";
            ss_networks << ",\"ip_start\":" << "\"" << network->ipStart << "\"";
            ss_networks << ",\"ip_end\":" << "\"" << network->ipEnd << "\"";
            ss_networks << ",\"dhcp_interface\":" << "\"" << network->dhcpInterface << "\"";
            ss_networks << ",\"machine_id\":" << "\"" << network->machineId << "\"";
            ss_networks << ",\"rent_wallet\":" << "\"" << network->rentWallet << "\"";
            ss_networks << ",\"members\":" << "[";
            int memberCounts = 0;
            for (const auto& task_id : network->members) {
                if (memberCounts > 0) ss_networks << ",";
                ss_networks << "\"" << task_id << "\"";
                memberCounts++;
            }
            ss_networks << "]";
            struct tm _tm{};
            time_t tt = network->lastUseTime;
            localtime_r(&tt, &_tm);
            char buf[256] = {0};
            memset(buf, 0, sizeof(char) * 256);
            strftime(buf, sizeof(char) * 256, "%Y-%m-%d %H:%M:%S", &_tm);
            ss_networks << ",\"lastUseTime\":" << "\"" << buf << "\"";
            ss_networks << ",\"native_flags\":" << "\"" << network->nativeFlags << "\"";
            ss_networks << "}";
        } else {
            ret_code = E_DEFAULT;
            ret_msg = "network name not exist";
        }
    }

    std::stringstream ss;
    ss << "{";
    if (ret_code == ERR_SUCCESS) {
        ss << "\"errcode\":" << ERR_SUCCESS;
        ss << ", \"message\":" << ss_networks.str();
    } else {
        ss << "\"errcode\":" << ret_code;
        ss << ", \"message\":" << "\"" << ret_msg << "\"";
    }
    ss << "}";

    const std::map<std::string, std::string>& mp = header.exten_info;
    auto it = mp.find("pub_key");
    if (it != mp.end()) {
        std::string pub_key = it->second;
        std::string priv_key = ConfManager::instance().GetPrivKey();

        if (!pub_key.empty() && !priv_key.empty()) {
            std::string s_data = encrypt_data((unsigned char*) ss.str().c_str(), ss.str().size(), pub_key, priv_key);
            send_response_json<dbc::node_list_lan_rsp>(NODE_LIST_LAN_RSP, header, s_data);
        } else {
            LOG_ERROR << "pub_key or priv_key is empty";
            send_response_error<dbc::node_list_lan_rsp>(NODE_LIST_LAN_RSP, header, E_DEFAULT, "pub_key or priv_key is empty");
        }
    } else {
        LOG_ERROR << "request no pub_key";
        send_response_error<dbc::node_list_lan_rsp>(NODE_LIST_LAN_RSP, header, E_DEFAULT, "request no pub_key");
    }
}

void node_request_service::on_node_create_lan_req(const std::shared_ptr<network::message>& msg) {
    auto node_req_msg = std::dynamic_pointer_cast<dbc::node_create_lan_req>(msg->get_content());
    if (node_req_msg == nullptr) {
        return;
    }

    if (!check_req_header_nonce(node_req_msg->header.nonce)) {
        return;
    }

    if (Server::NodeType != NODE_TYPE::ComputeNode) {
        node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
        network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
        return;
    }

    if (!check_req_header(msg)) {
        LOG_ERROR << "request header check failed";
        node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
        network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
        return;
    }

    // decrypt
    std::string pub_key = node_req_msg->header.exten_info["pub_key"];
    std::string priv_key = ConfManager::instance().GetPrivKey();
    if (pub_key.empty() || priv_key.empty()) {
        LOG_ERROR << "pub_key or priv_key is empty";
        node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
        network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
        return;
    }

    std::shared_ptr<dbc::node_create_lan_req_data> data = std::make_shared<dbc::node_create_lan_req_data>();
    try {
        std::string ori_message;
        bool succ = decrypt_data(node_req_msg->body.data, pub_key, priv_key, ori_message);
        if (!succ || ori_message.empty()) {
            node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
            network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
            return;
        }

        std::shared_ptr<byte_buf> buf = std::make_shared<byte_buf>();
        buf->write_to_byte_buf(ori_message.c_str(), ori_message.size());
        network::binary_protocol proto(buf.get());
        data->read(&proto);
    } catch (std::exception &e) {
        node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
        network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
        return;
    }

    std::vector<std::string> req_peer_nodes = data->peer_nodes_list;
    bool hit_self = hit_node(req_peer_nodes, ConfManager::instance().GetNodeId());
    if (hit_self) {
        AuthorityParams params;
        params.wallet = data->wallet;
        params.nonce = data->nonce;
        params.sign = data->sign;
        params.multisig_wallets = data->multisig_wallets;
        params.multisig_threshold = data->multisig_threshold;
        params.multisig_signs = data->multisig_signs;
        params.session_id = data->session_id;
        params.session_id_sign = data->session_id_sign;
        AuthoriseResult result;
        check_authority(params, result);
        if (!result.success || result.user_role == USER_ROLE::Unknown || result.user_role == USER_ROLE::Verifier) {
            LOG_ERROR << "check authority failed: " << result.errmsg;
            send_response_error<dbc::node_create_lan_rsp>(NODE_CREATE_LAN_RSP, node_req_msg->header, E_DEFAULT, "check authority failed: " + result.errmsg);
            return;
        }

        create_lan(node_req_msg->header, data, result);
    } else {
        node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
        network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
    }
}

void node_request_service::create_lan(const network::base_header& header, const std::shared_ptr<dbc::node_create_lan_req_data>& data, const AuthoriseResult& result) {
    // send_response_error<dbc::node_create_lan_rsp>(NODE_CREATE_LAN_RSP, header, ERR_SUCCESS, "AAAAAAAAAAAA");
    // send_response_ok<dbc::node_create_lan_rsp>(NODE_CREATE_LAN_RSP, header);
    FResult fret = {ERR_ERROR, "invalid additional param"};
    rapidjson::Document doc;
    rapidjson::ParseResult ok = doc.Parse(data->additional.c_str());
    if (ok && doc.IsObject()) {
        std::string networkName, vxlanVni, ipCidr;
        JSON_PARSE_STRING(doc, "network_name", networkName);
        // JSON_PARSE_STRING(doc, "vxlan_vni", vxlanVni);
        JSON_PARSE_STRING(doc, "ip_cidr", ipCidr);
        fret = VxlanManager::instance().CreateNetworkServer(networkName, ipCidr, result.rent_wallet);
    }

    if (fret.errcode == ERR_SUCCESS) {
        send_response_ok<dbc::node_create_lan_rsp>(NODE_CREATE_LAN_RSP, header);
    } else {
        send_response_error<dbc::node_create_lan_rsp>(NODE_CREATE_LAN_RSP, header, E_DEFAULT, fret.errmsg);
    }
}

void node_request_service::on_node_delete_lan_req(const std::shared_ptr<network::message>& msg) {
    auto node_req_msg = std::dynamic_pointer_cast<dbc::node_delete_lan_req>(msg->get_content());
    if (node_req_msg == nullptr) {
        return;
    }

    if (!check_req_header_nonce(node_req_msg->header.nonce)) {
        return;
    }

    if (Server::NodeType != NODE_TYPE::ComputeNode) {
        node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
        network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
        return;
    }

    if (!check_req_header(msg)) {
        LOG_ERROR << "request header check failed";
        node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
        network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
        return;
    }

    // decrypt
    std::string pub_key = node_req_msg->header.exten_info["pub_key"];
    std::string priv_key = ConfManager::instance().GetPrivKey();
    if (pub_key.empty() || priv_key.empty()) {
        LOG_ERROR << "pub_key or priv_key is empty";
        node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
        network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
        return;
    }

    std::shared_ptr<dbc::node_delete_lan_req_data> data = std::make_shared<dbc::node_delete_lan_req_data>();
    try {
        std::string ori_message;
        bool succ = decrypt_data(node_req_msg->body.data, pub_key, priv_key, ori_message);
        if (!succ || ori_message.empty()) {
            node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
            network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
            return;
        }

        std::shared_ptr<byte_buf> buf = std::make_shared<byte_buf>();
        buf->write_to_byte_buf(ori_message.c_str(), ori_message.size());
        network::binary_protocol proto(buf.get());
        data->read(&proto);
    } catch (std::exception &e) {
        node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
        network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
        return;
    }

    std::vector<std::string> req_peer_nodes = data->peer_nodes_list;
    bool hit_self = hit_node(req_peer_nodes, ConfManager::instance().GetNodeId());
    if (hit_self) {
        AuthorityParams params;
        params.wallet = data->wallet;
        params.nonce = data->nonce;
        params.sign = data->sign;
        params.multisig_wallets = data->multisig_wallets;
        params.multisig_threshold = data->multisig_threshold;
        params.multisig_signs = data->multisig_signs;
        params.session_id = data->session_id;
        params.session_id_sign = data->session_id_sign;
        AuthoriseResult result;
        check_authority(params, result);
        if (!result.success || result.user_role == USER_ROLE::Unknown || result.user_role == USER_ROLE::Verifier) {
            LOG_ERROR << "check authority failed: " << result.errmsg;
            send_response_error<dbc::node_delete_lan_rsp>(NODE_DELETE_LAN_RSP, node_req_msg->header, E_DEFAULT, "check authority failed: " + result.errmsg);
            return;
        }

        delete_lan(node_req_msg->header, data, result);
    } else {
        node_req_msg->header.path.push_back(ConfManager::instance().GetNodeId());
        network::connection_manager::instance().broadcast_message(msg, msg->header.src_sid);
    }
}

void node_request_service::delete_lan(const network::base_header& header, const std::shared_ptr<dbc::node_delete_lan_req_data>& data, const AuthoriseResult& result) {
    // send_response_error<dbc::node_delete_lan_rsp>(NODE_DELETE_LAN_RSP, header, ERR_SUCCESS, "BBBBBBBBBBB");
    // send_response_ok<dbc::node_delete_lan_rsp>(NODE_DELETE_LAN_RSP, header);
    int ret_code = ERR_SUCCESS;
    std::string ret_msg = "ok";

    FResult fret = VxlanManager::instance().DeleteNetwork(data->network_id, result.rent_wallet);
    if (fret.errcode == ERR_SUCCESS) {
        send_response_ok<dbc::node_delete_lan_rsp>(NODE_DELETE_LAN_RSP, header);
    } else {
        send_response_error<dbc::node_delete_lan_rsp>(NODE_DELETE_LAN_RSP, header, E_DEFAULT, fret.errmsg);
    }
}