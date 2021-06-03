/*********************************************************************************
*  Copyright (c) 2017-2018 DeepBrainChain core team
*  Distributed under the MIT software license, see the accompanying
*  file COPYING or http://www.opensource.org/licenses/mit-license.php
* file name         :   ai_power_provider_service.cpp
* description     :     init and the callback of thrift matrix cmd
* date                  :   2020.09.16
* author             :   Feng
**********************************************************************************/
#include <boost/property_tree/json_parser.hpp>
#include <cassert>
#include <boost/exception/all.hpp>
#include "server.h"
#include "api_call_handler.h"
#include "conf_manager.h"
#include "ai_power_provider_service.h"
#include "service_message_id.h"
#include "matrix_types.h"
#include "matrix_coder.h"
#include "ip_validator.h"
#include "port_validator.h"
#include "task_common_def.h"
#include "util.h"
#include <boost/thread/thread.hpp>
#include "service_topic.h"
#include <ctime>
#include <boost/format.hpp>
#include "url_validator.h"
#include "time_util.h"
#include <stdlib.h>
#include <boost/algorithm/string/join.hpp>
#include "ai_crypter.h"

using namespace std;
using namespace matrix::core;
using namespace matrix::service_core;
using namespace ai::dbc;

namespace ai {
    namespace dbc {
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
                    matrix::core::string_util::trim(rt);
                }
            } catch (...) {

            }

            return rt;
        }

        ai_power_provider_service::ai_power_provider_service()
                : m_training_task_timer_id(INVALID_TIMER_ID), m_prune_task_timer_id(INVALID_TIMER_ID) {

        }

        int32_t ai_power_provider_service::service_init(bpo::variables_map &options) {
            int32_t ret = E_SUCCESS;

            //oss验证
            m_oss_task_mng = std::make_shared<oss_task_manager>();
            ret = m_oss_task_mng->init();
            if (E_SUCCESS != ret) {
                LOG_ERROR << "ai power provider service init oss error";
                return ret;
            }

            m_container_worker = std::make_shared<container_worker>();
            ret = m_container_worker->init();
            if (E_SUCCESS != ret) {
                LOG_ERROR << "ai power provider service init container worker error";
                return ret;
            }

            m_vm_worker = std::make_shared<vm_worker>();
            ret = m_vm_worker->init();
            if (E_SUCCESS != ret) {
                LOG_ERROR << "ai power provider service init vm worker error";
                return ret;
            }

            m_user_task_ptr = std::make_shared<user_task_scheduling>(m_container_worker, m_vm_worker);
            if (E_SUCCESS != m_user_task_ptr->init(options)) {
                return E_DEFAULT;
            }

            m_user_task_ptr->set_auth_handler(
                    std::bind(&oss_task_manager::auth_task, m_oss_task_mng, std::placeholders::_1));

            m_idle_task_ptr = std::make_shared<idle_task_scheduling>(m_container_worker, m_vm_worker);
            if (E_SUCCESS != m_idle_task_ptr->init()) {
                return E_DEFAULT;
            }
            m_idle_task_ptr->set_fetch_handler(std::bind(&oss_task_manager::fetch_idle_task, m_oss_task_mng));

            //user_task_scheduling is repo for all ai user tasks's schedling
            //before task running, task should auth by oss
            //after auth success, dbc can exec task. Before exec task, dbc should stop idle task.
            //set_stop_idle_task_handler is used to reg stop_idle_task_hadler.
            m_user_task_ptr->set_stop_idle_task_handler(std::bind(&idle_task_scheduling::stop_task, m_idle_task_ptr));

            return E_SUCCESS;
        }

        int32_t ai_power_provider_service::service_exit() {
            remove_timer(m_training_task_timer_id);
            remove_timer(m_prune_task_timer_id);
            return E_SUCCESS;
        }

        void ai_power_provider_service::init_subscription() {
            //ai training
            SUBSCRIBE_BUS_MESSAGE(AI_TRAINING_NOTIFICATION_REQ);

            //stop training
            SUBSCRIBE_BUS_MESSAGE(STOP_TRAINING_REQ);

            //list training req
            SUBSCRIBE_BUS_MESSAGE(LIST_TRAINING_REQ);

            //task logs req
            SUBSCRIBE_BUS_MESSAGE(LOGS_REQ);

            //task queue size query
            SUBSCRIBE_BUS_MESSAGE(typeid(service::get_task_queue_size_req_msg).name());
        }

        void ai_power_provider_service::init_invoker() {
            invoker_type invoker;

            //ai training
            BIND_MESSAGE_INVOKER(AI_TRAINING_NOTIFICATION_REQ, &ai_power_provider_service::on_start_training_req);

            //stop training
            BIND_MESSAGE_INVOKER(STOP_TRAINING_REQ, &ai_power_provider_service::on_stop_training_req);

            //list training req
            BIND_MESSAGE_INVOKER(LIST_TRAINING_REQ, &ai_power_provider_service::on_list_training_req);

            //task logs req
            BIND_MESSAGE_INVOKER(LOGS_REQ, &ai_power_provider_service::on_logs_req);

            //task queue size req
            BIND_MESSAGE_INVOKER(typeid(service::get_task_queue_size_req_msg).name(),
                                 &ai_power_provider_service::on_get_task_queue_size_req);
        }

        void ai_power_provider_service::init_timer() {
            m_timer_invokers[AI_TRAINING_TASK_TIMER] = std::bind(&ai_power_provider_service::on_training_task_timer,
                                                                 this, std::placeholders::_1);
            //配置文件：timer_ai_training_task_schedule_in_second（15秒）
            m_training_task_timer_id = this->add_timer(AI_TRAINING_TASK_TIMER,
                                                       CONF_MANAGER->get_timer_ai_training_task_schedule_in_second() *
                                                       1000);

            m_timer_invokers[AI_PRUNE_TASK_TIMER] = std::bind(&ai_power_provider_service::on_prune_task_timer, this,
                                                              std::placeholders::_1);
            m_prune_task_timer_id = this->add_timer(AI_PRUNE_TASK_TIMER, AI_PRUNE_TASK_TIMER_INTERVAL); //10分钟

            assert(INVALID_TIMER_ID != m_training_task_timer_id);
            assert(INVALID_TIMER_ID != m_prune_task_timer_id);
        }

        int32_t ai_power_provider_service::on_start_training_req(std::shared_ptr<message> &msg) {
            std::shared_ptr<matrix::service_core::start_training_req> req =
                    std::dynamic_pointer_cast<matrix::service_core::start_training_req>(msg->get_content());
            if (req == nullptr) return E_DEFAULT;
            LOG_INFO << "start: " << req->body.task_id << endl;

            if (!id_generator::check_base58_id(req->header.nonce)) {
                LOG_ERROR << "ai power provider service nonce error ";
                return E_DEFAULT;
            }

            if (!id_generator::check_base58_id(req->body.task_id)) {
                LOG_ERROR << "ai power provider service task_id error ";
                return E_DEFAULT;
            }

            if (req->body.entry_file.empty()) {
                LOG_ERROR << "entry_file non exist in task.";
                return E_DEFAULT;
            }

            if (req->body.entry_file.size() > MAX_ENTRY_FILE_NAME_LEN) {
                LOG_ERROR << "entry_file name lenth is too long." << req->body.entry_file.size();
                return E_DEFAULT;
            }

            if (req->body.training_engine.empty()) {
                LOG_ERROR << "training_engine not exist in task";
                return E_DEFAULT;
            }

            if (req->body.training_engine.size() > MAX_ENGINE_IMGE_NAME_LEN) {
                LOG_ERROR << "engine image lenth is too long." << req->body.training_engine.size();
                return E_DEFAULT;
            }

            if (!check_task_engine(req->body.training_engine)) {
                LOG_ERROR << "engine name is error." << req->body.training_engine.size();
                return E_DEFAULT;
            }

            std::string sign_msg = req->body.task_id + req->body.code_dir + req->header.nonce;
            if (req->header.exten_info.size() < 3) {
                LOG_ERROR << "exten info error.";
                return E_DEFAULT;
            }

            if ((E_SUCCESS != check_sign(sign_msg, req->header.exten_info["sign"], req->header.exten_info["origin_id"],
                                         req->header.exten_info["sign_algo"]))
                &&
                (!ai_crypto_util::verify_sign(sign_msg, req->header.exten_info, req->header.exten_info["origin_id"]))) {
                LOG_ERROR << "sign error." << req->header.exten_info["origin_id"];
                return E_DEFAULT;
            }

            const std::vector<std::string> &peer_nodes = req->body.peer_nodes_list;
            auto it = peer_nodes.begin();
            for (; it != peer_nodes.end(); it++) {
                if (id_generator::check_node_id((*it)) != true) {
                    LOG_ERROR << "ai power provider service node_id error " << (*it);
                    return E_DEFAULT;
                }

                if ((*it) == CONF_MANAGER->get_node_id()) {
                    LOG_DEBUG << "ai power provider service found self node id in on start training: "
                              << req->body.task_id << " node id: " << (*it);
                    break;
                }
            }

            //not find self; or find self, but designate more than one node
            if (it == peer_nodes.end() || peer_nodes.size() > 1) {
                LOG_DEBUG << "ai power provider service found start training req " << req->body.task_id
                          << " is not self and exit function";
                //relay start training in network
                LOG_INFO
                        << "0 ai power provider service relay broadcast start training req to neighbor peer nodes: "
                        << req->body.task_id;
                CONNECTION_MANAGER->broadcast_message(msg, msg->header.src_sid);
                /*
                if (CONF_MANAGER->get_node_id() == "2gfpp3MAB489TcFSWfwvyXcgJKUcDWybSuPsi88SZQF"
                    || CONF_MANAGER->get_node_id() == "2gfpp3MAB4K4z1f3vyEUergJmjG64kXPFXNspBbhn3n"
                    || CONF_MANAGER->get_node_id() == "2gfpp3MAB4Hta5wtuFv9Ef7Cg9ZkUamgxYh3UjTYsQ8"
                    || CONF_MANAGER->get_node_id() == "2gfpp3MAB4CdhXs56P4UwtTauGcWcMbnBJhF6qWAWQD"
                    || CONF_MANAGER->get_node_id() == "2gfpp3MAB43V9jndyUdKAeU6qAQ1kFRw3WX3rqqfmKj"
                    || CONF_MANAGER->get_node_id() == "2gfpp3MAB4Bx3W6xH62YicCj2rsaBCk6ujeC11MooaQ"
                    || CONF_MANAGER->get_node_id() == "2gfpp3MAB488Ut5nduiiZ33dJmvS6txstdRPeHZ7Kkf"
                    || CONF_MANAGER->get_node_id() == "2gfpp3MAB3x6jBqi8S2jaoC6yAc5MSVvjUDStUdLqpm"
                    || CONF_MANAGER->get_node_id() == "2gfpp3MAB3vc4tVLHb1D4iacrTmzsQ3NzGE7HS5pnfw"
                    || CONF_MANAGER->get_node_id() == "2gfpp3MAB432cJRdKhLd4cGZwpZjoE5PtxvEmHhWbp1"
                    || CONF_MANAGER->get_node_id() == "2gfpp3MAB49V1bPqEXyuef2pThQWqN6sFij71m7epkd"
                    || CONF_MANAGER->get_node_id() == "2gfpp3MAB42X3BJhQjmMe6HAFivnMVU5QXVC1nFfAK5"
                    || CONF_MANAGER->get_node_id() == "2gfpp3MAB4AMdYxL74z7kq6y1zD7AgJtwRmuv4mUf9p"
                    || CONF_MANAGER->get_node_id() == "2gfpp3MAB414gDp7YY9afeVwptEp4hf8MWtfE8Pp3zE"
                    || CONF_MANAGER->get_node_id() == "2gfpp3MAB3zQYp35EGQeGwMcEAvE2PBsk9shwL7gyWA"
                    || CONF_MANAGER->get_node_id() == "2gfpp3MAB4HYkz2ezFwxa36mpCZzTGxXgg8TNrPynYo"
                    || CONF_MANAGER->get_node_id() == "2gfpp3MAB3vRQuPD23q8sBNHDn7jknCiqcx4NmoRWQY"
                    || CONF_MANAGER->get_node_id() == "2gfpp3MAB4GaGfS4fj1Z8rXjkHLcF4ifNB2EMWPpKL2"
                    || CONF_MANAGER->get_node_id() == "2gfpp3MAB49qg5LJPE3pAXQULDP4aK2TQDc93edaG5i"
                    || CONF_MANAGER->get_node_id() == "2gfpp3MAB4F8a5dTsjo8qduNuWoQVWw1XJeRYL7rKiC"
                    || CONF_MANAGER->get_node_id() == "2gfpp3MAB49pWqfvgTvKXArEU6wrp9UCYZi9hoR6sw1"
                    || CONF_MANAGER->get_node_id() == "2gfpp3MAB4Gxw9JXAkccZJCsLqZJ9T77W5JjQdbhLK7"
                    || CONF_MANAGER->get_node_id() == "2gfpp3MAB3yFm6vwKzL2jtiTPbzZRiHXuZFQM7i6aZ8"
                    || CONF_MANAGER->get_node_id() == "2gfpp3MAB4C8RrFLupsM4SBBiDfHWmPSZ29zSiPDMx6"
                    || CONF_MANAGER->get_node_id() == "2gfpp3MAB47xpUSdgv2oMbeYvbMTfnF7TRhRPtwMDpM") {
                    LOG_INFO
                        << "0 ai power provider service relay broadcast start training req to neighbor peer nodes: "
                        << req->body.task_id;
                    CONNECTION_MANAGER->broadcast_message(msg, msg->header.src_sid);
                } else {
                    LOG_INFO
                        << "1 ai power provider service relay broadcast start training req to neighbor peer nodes: "
                        << req->body.task_id;
                    srand((int) time(0));
                    int32_t count = (rand() % (10 - 1) + 1);
                    if (count == 6) {
                        CONNECTION_MANAGER->broadcast_message(msg, msg->header.src_sid);
                    }
                }
                */
            }

            if (it == peer_nodes.end()) {
                return E_SUCCESS; //not find self, return
            }

            // reboot node
            if (req->body.code_dir == std::string(NODE_REBOOT) && CONF_MANAGER->get_enable_node_reboot()) {
                return node_reboot(req);
            }

            // restart a user task
            if (req->body.code_dir == std::string(TASK_RESTART)) {
                bool is_docker = false;
                if (req->body.server_specification.find("docker") != std::string::npos) {
                    is_docker = true;
                }

                return task_restart(req, is_docker);
            }

            return task_start(req);
        }

        int32_t ai_power_provider_service::on_stop_training_req(std::shared_ptr<message> &msg) {
            std::shared_ptr<matrix::service_core::stop_training_req> req = std::dynamic_pointer_cast<matrix::service_core::stop_training_req>(
                    msg->get_content());
            if (req == nullptr) return E_DEFAULT;
            LOG_INFO << "stop: " << req->body.task_id << endl;

            if (!id_generator::check_base58_id(req->header.nonce)) {
                LOG_ERROR << "ai power provider service on_stop_training_req nonce error ";
                return E_DEFAULT;
            }

            if (!id_generator::check_base58_id(req->body.task_id)) {
                LOG_ERROR << "ai power provider service on_stop_training_req task_id error ";
                return E_DEFAULT;
            }

            std::string sign_msg = req->body.task_id + req->header.nonce;
            if (req->header.exten_info.size() < 3) {
                LOG_ERROR << "exten info error.";
                return E_DEFAULT;
            }

            if ((E_SUCCESS != check_sign(sign_msg, req->header.exten_info["sign"], req->header.exten_info["origin_id"],
                                         req->header.exten_info["sign_algo"]))
                &&
                (!ai_crypto_util::verify_sign(sign_msg, req->header.exten_info, req->header.exten_info["origin_id"]))) {
                LOG_ERROR << "sign error." << req->header.exten_info["origin_id"];
                return E_DEFAULT;
            }

            const std::string &task_id = std::dynamic_pointer_cast<stop_training_req>(msg->get_content())->body.task_id;
            auto sp_task = m_user_task_ptr->find_task(task_id);
            if (sp_task) {
                bool is_docker = false;
                if (sp_task->server_specification.find("docker") != sp_task->server_specification.npos) {
                    is_docker = true;
                }

                if (sp_task->status == task_stopped) {
                    m_user_task_ptr->stop_task_only_id(task_id, is_docker);//强制停止
                } else {
                    m_user_task_ptr->stop_task(sp_task, task_stopped, is_docker);
                }

                return E_SUCCESS;
            } else {
                LOG_INFO << "stop training, not found task: " << task_id << endl;

                LOG_INFO << "0 stop training, broadcast_message task: " << task_id << endl;
                CONNECTION_MANAGER->broadcast_message(msg, msg->header.src_sid);

                // relay on stop_training to network
                // not support task running on multiple nodes
                // LOG_DEBUG << "ai power provider service relay broadcast stop_training req to neighbor peer nodes: " << req->body.task_id;
                // LOG_INFO << "force stop training:" << task_id << endl;
                // return  m_user_task_ptr->stop_task_only_id(task_id);//强制停止
                /*
                if (CONF_MANAGER->get_node_id() == "2gfpp3MAB489TcFSWfwvyXcgJKUcDWybSuPsi88SZQF"
                    || CONF_MANAGER->get_node_id() == "2gfpp3MAB4K4z1f3vyEUergJmjG64kXPFXNspBbhn3n"
                    || CONF_MANAGER->get_node_id() == "2gfpp3MAB4Hta5wtuFv9Ef7Cg9ZkUamgxYh3UjTYsQ8"
                    || CONF_MANAGER->get_node_id() == "2gfpp3MAB4CdhXs56P4UwtTauGcWcMbnBJhF6qWAWQD"
                    || CONF_MANAGER->get_node_id() == "2gfpp3MAB43V9jndyUdKAeU6qAQ1kFRw3WX3rqqfmKj"
                    || CONF_MANAGER->get_node_id() == "2gfpp3MAB4Bx3W6xH62YicCj2rsaBCk6ujeC11MooaQ"
                    || CONF_MANAGER->get_node_id() == "2gfpp3MAB488Ut5nduiiZ33dJmvS6txstdRPeHZ7Kkf"
                    || CONF_MANAGER->get_node_id() == "2gfpp3MAB3x6jBqi8S2jaoC6yAc5MSVvjUDStUdLqpm"
                    || CONF_MANAGER->get_node_id() == "2gfpp3MAB3vc4tVLHb1D4iacrTmzsQ3NzGE7HS5pnfw"
                    || CONF_MANAGER->get_node_id() == "2gfpp3MAB432cJRdKhLd4cGZwpZjoE5PtxvEmHhWbp1"
                    || CONF_MANAGER->get_node_id() == "2gfpp3MAB49V1bPqEXyuef2pThQWqN6sFij71m7epkd"
                    || CONF_MANAGER->get_node_id() == "2gfpp3MAB4AMdYxL74z7kq6y1zD7AgJtwRmuv4mUf9p"
                    || CONF_MANAGER->get_node_id() == "2gfpp3MAB414gDp7YY9afeVwptEp4hf8MWtfE8Pp3zE"
                    || CONF_MANAGER->get_node_id() == "2gfpp3MAB3zQYp35EGQeGwMcEAvE2PBsk9shwL7gyWA"
                    || CONF_MANAGER->get_node_id() == "2gfpp3MAB4HYkz2ezFwxa36mpCZzTGxXgg8TNrPynYo"
                    || CONF_MANAGER->get_node_id() == "2gfpp3MAB3vRQuPD23q8sBNHDn7jknCiqcx4NmoRWQY"
                    || CONF_MANAGER->get_node_id() == "2gfpp3MAB4GaGfS4fj1Z8rXjkHLcF4ifNB2EMWPpKL2"
                    || CONF_MANAGER->get_node_id() == "2gfpp3MAB49qg5LJPE3pAXQULDP4aK2TQDc93edaG5i"
                    || CONF_MANAGER->get_node_id() == "2gfpp3MAB4F8a5dTsjo8qduNuWoQVWw1XJeRYL7rKiC"
                    || CONF_MANAGER->get_node_id() == "2gfpp3MAB49pWqfvgTvKXArEU6wrp9UCYZi9hoR6sw1"
                    || CONF_MANAGER->get_node_id() == "2gfpp3MAB4Gxw9JXAkccZJCsLqZJ9T77W5JjQdbhLK7"
                    || CONF_MANAGER->get_node_id() == "2gfpp3MAB3yFm6vwKzL2jtiTPbzZRiHXuZFQM7i6aZ8"
                    || CONF_MANAGER->get_node_id() == "2gfpp3MAB4C8RrFLupsM4SBBiDfHWmPSZ29zSiPDMx6"
                    || CONF_MANAGER->get_node_id() == "2gfpp3MAB47xpUSdgv2oMbeYvbMTfnF7TRhRPtwMDpM") {
                    LOG_INFO << "0 stop training, broadcast_message task: " << task_id << endl;
                    CONNECTION_MANAGER->broadcast_message(msg, msg->header.src_sid);
                } else {
                    srand((int) time(0));
                    int32_t count = (rand() % (10 - 1) + 1);
                    if (count == 6) {
                        LOG_INFO << "1 stop training, broadcast_message task: " << task_id << endl;
                        CONNECTION_MANAGER->broadcast_message(msg, msg->header.src_sid);
                    }
                }
                */
            }

            return E_SUCCESS;
        }

        int32_t ai_power_provider_service::on_list_training_req(std::shared_ptr<message> &msg) {
            std::shared_ptr<list_training_req> req_content = std::dynamic_pointer_cast<list_training_req>(
                    msg->get_content());
            if (req_content == nullptr) return E_DEFAULT;
            LOG_DEBUG << "list training, nonce: " << req_content->header.nonce << ", session: "
                      << req_content->header.session_id;

            if (!id_generator::check_base58_id(req_content->header.nonce)) {
                LOG_ERROR << "ai power provider service nonce error ";
                return E_DEFAULT;
            }

            if (!id_generator::check_base58_id(req_content->header.session_id)) {
                LOG_ERROR << "ai power provider service sessionid error ";
                return E_DEFAULT;
            }

            if (req_content->body.task_list.empty()) {
                LOG_ERROR << "ai power provider service recv empty list training tasks";
                return E_DEFAULT;
            }

            for (auto & it : req_content->body.task_list) {
                if (!id_generator::check_base58_id(it)) {
                    LOG_ERROR << "ai power provider service taskid error: " << it;
                    return E_DEFAULT;
                }
            }

            std::string sign_msg = boost::algorithm::join(req_content->body.task_list, "") + req_content->header.nonce +
                                   req_content->header.session_id;
            if (!ai_crypto_util::verify_sign(sign_msg, req_content->header.exten_info,
                                             req_content->header.exten_info["origin_id"])) {
                LOG_ERROR << "fake message. " << req_content->header.exten_info["origin_id"];
                return E_DEFAULT;
            }

            //relay list_training to network(maybe task running on multiple nodes, no mater I took this task)
            req_content->header.path.push_back(CONF_MANAGER->get_node_id()); //add this node id into path

            LOG_INFO << "0 broadcast ai power provider service training task";
            CONNECTION_MANAGER->broadcast_message(msg, msg->header.src_sid);

            /*
            if (CONF_MANAGER->get_node_id() == "2gfpp3MAB489TcFSWfwvyXcgJKUcDWybSuPsi88SZQF"
                || CONF_MANAGER->get_node_id() == "2gfpp3MAB4K4z1f3vyEUergJmjG64kXPFXNspBbhn3n"
                || CONF_MANAGER->get_node_id() == "2gfpp3MAB4Hta5wtuFv9Ef7Cg9ZkUamgxYh3UjTYsQ8"
                || CONF_MANAGER->get_node_id() == "2gfpp3MAB4CdhXs56P4UwtTauGcWcMbnBJhF6qWAWQD"
                || CONF_MANAGER->get_node_id() == "2gfpp3MAB43V9jndyUdKAeU6qAQ1kFRw3WX3rqqfmKj"
                || CONF_MANAGER->get_node_id() == "2gfpp3MAB4Bx3W6xH62YicCj2rsaBCk6ujeC11MooaQ"
                || CONF_MANAGER->get_node_id() == "2gfpp3MAB488Ut5nduiiZ33dJmvS6txstdRPeHZ7Kkf"
                || CONF_MANAGER->get_node_id() == "2gfpp3MAB3x6jBqi8S2jaoC6yAc5MSVvjUDStUdLqpm"
                || CONF_MANAGER->get_node_id() == "2gfpp3MAB3vc4tVLHb1D4iacrTmzsQ3NzGE7HS5pnfw"
                || CONF_MANAGER->get_node_id() == "2gfpp3MAB432cJRdKhLd4cGZwpZjoE5PtxvEmHhWbp1"
                || CONF_MANAGER->get_node_id() == "2gfpp3MAB49V1bPqEXyuef2pThQWqN6sFij71m7epkd"
                || CONF_MANAGER->get_node_id() == "2gfpp3MAB42X3BJhQjmMe6HAFivnMVU5QXVC1nFfAK5"
                || CONF_MANAGER->get_node_id() == "2gfpp3MAB4AMdYxL74z7kq6y1zD7AgJtwRmuv4mUf9p"
                || CONF_MANAGER->get_node_id() == "2gfpp3MAB414gDp7YY9afeVwptEp4hf8MWtfE8Pp3zE"
                || CONF_MANAGER->get_node_id() == "2gfpp3MAB3zQYp35EGQeGwMcEAvE2PBsk9shwL7gyWA"
                || CONF_MANAGER->get_node_id() == "2gfpp3MAB4HYkz2ezFwxa36mpCZzTGxXgg8TNrPynYo"
                || CONF_MANAGER->get_node_id() == "2gfpp3MAB3vRQuPD23q8sBNHDn7jknCiqcx4NmoRWQY"
                || CONF_MANAGER->get_node_id() == "2gfpp3MAB4GaGfS4fj1Z8rXjkHLcF4ifNB2EMWPpKL2"
                || CONF_MANAGER->get_node_id() == "2gfpp3MAB49qg5LJPE3pAXQULDP4aK2TQDc93edaG5i"
                || CONF_MANAGER->get_node_id() == "2gfpp3MAB4F8a5dTsjo8qduNuWoQVWw1XJeRYL7rKiC"
                || CONF_MANAGER->get_node_id() == "2gfpp3MAB49pWqfvgTvKXArEU6wrp9UCYZi9hoR6sw1"
                || CONF_MANAGER->get_node_id() == "2gfpp3MAB4Gxw9JXAkccZJCsLqZJ9T77W5JjQdbhLK7"
                || CONF_MANAGER->get_node_id() == "2gfpp3MAB3yFm6vwKzL2jtiTPbzZRiHXuZFQM7i6aZ8"
                || CONF_MANAGER->get_node_id() == "2gfpp3MAB4C8RrFLupsM4SBBiDfHWmPSZ29zSiPDMx6"
                || CONF_MANAGER->get_node_id() == "2gfpp3MAB47xpUSdgv2oMbeYvbMTfnF7TRhRPtwMDpM") {
                LOG_INFO << "0 broadcast ai power provider service training task";
                CONNECTION_MANAGER->broadcast_message(msg, msg->header.src_sid);
            } else {
                srand((int) time(0));
                int32_t count = (rand() % (10 - 1) + 1);
                if (count == 6) {
                    LOG_INFO << "1 broadcast ai power provider service training task";
                    CONNECTION_MANAGER->broadcast_message(msg, msg->header.src_sid);
                }
            }
            */

            if (0 == m_user_task_ptr->get_total_user_task_size()) {
                LOG_INFO << "ai power provider service training task is empty";
                return E_SUCCESS;
            }

            std::vector<matrix::service_core::task_status> status_list;
            for (auto it = req_content->body.task_list.begin(); it != req_content->body.task_list.end(); ++it) {
                auto task = m_user_task_ptr->find_task(*it);
                if (nullptr == task) {
                    continue;
                }

                matrix::service_core::task_status ts;
                ts.task_id = task->task_id;

                /*
                if (use_sign_verify()) {
                    if (task->ai_user_node_id != req_content->header.exten_info["origin_id"]) {
                        return E_DEFAULT;
                    }
                }
                */

                ts.status = task->status;
                status_list.push_back(ts);

                if (status_list.size() >= MAX_LIST_TASK_COUNT) {
                    break;
                }
            }

            if (status_list.size() > 0) {
                std::shared_ptr<matrix::service_core::list_training_resp> rsp_content = std::make_shared<matrix::service_core::list_training_resp>();
                //content header
                rsp_content->header.__set_magic(CONF_MANAGER->get_net_flag());
                rsp_content->header.__set_msg_name(LIST_TRAINING_RESP);
                rsp_content->header.__set_nonce(id_generator::generate_nonce());
                rsp_content->header.__set_session_id(req_content->header.session_id);

                rsp_content->header.__set_path(req_content->header.path); // for efficient resp msg transport

                rsp_content->body.task_status_list.swap(status_list);

                std::string task_status_msg = "";
                for (auto t : rsp_content->body.task_status_list) {
                    task_status_msg = task_status_msg + t.task_id + boost::str(boost::format("%d") % t.status);
                }
                std::string sign_msg = rsp_content->header.nonce + rsp_content->header.session_id + task_status_msg;
                std::map<std::string, std::string> exten_info;
                exten_info["origin_id"] = CONF_MANAGER->get_node_id();
                if (E_SUCCESS != ai_crypto_util::extra_sign_info(sign_msg, exten_info)) {
                    return E_DEFAULT;
                }
                rsp_content->header.__set_exten_info(exten_info);

                //resp msg
                std::shared_ptr<message> resp_msg = std::make_shared<message>();
                resp_msg->set_name(LIST_TRAINING_RESP);
                resp_msg->set_content(rsp_content);
                CONNECTION_MANAGER->send_resp_message(resp_msg);

                status_list.clear();

                LOG_INFO << "on_list_training_req send resp, nonce: " << rsp_content->header.nonce << ", session: "
                         << rsp_content->header.session_id
                         << "task cnt: " << rsp_content->body.task_status_list.size();
            }

            return E_SUCCESS;
        }

        int32_t ai_power_provider_service::on_logs_req(const std::shared_ptr<message> &msg) {
            std::shared_ptr<logs_req> req_content = std::dynamic_pointer_cast<logs_req>(msg->get_content());
            if (req_content == nullptr) return E_DEFAULT;
            LOG_INFO << "reqlogs: " << req_content->body.task_id << endl;

            if (!id_generator::check_base58_id(req_content->header.nonce)) {
                LOG_ERROR << "ai power provider service nonce error ";
                return E_DEFAULT;
            }

            if (!id_generator::check_base58_id(req_content->header.session_id)) {
                LOG_ERROR << "ai power provider service session_id error ";
                return E_DEFAULT;
            }

            if (!id_generator::check_base58_id(req_content->body.task_id)) {
                LOG_ERROR << "taskid error ";
                return E_DEFAULT;
            }

            const std::string &task_id = req_content->body.task_id;

            if (GET_LOG_HEAD != req_content->body.head_or_tail && GET_LOG_TAIL != req_content->body.head_or_tail) {
                LOG_ERROR << "ai power provider service on logs req log direction error: " << task_id;
                return E_DEFAULT;
            }

            if (req_content->body.number_of_lines > MAX_NUMBER_OF_LINES || req_content->body.number_of_lines < 0) {
                LOG_ERROR << "ai power provider service on logs req number of lines error: "
                          << req_content->body.number_of_lines;
                return E_DEFAULT;
            }

            //verify sign
            std::string sign_req_msg =
                    req_content->body.task_id + req_content->header.nonce + req_content->header.session_id;
            if (!ai_crypto_util::verify_sign(sign_req_msg, req_content->header.exten_info,
                                             req_content->header.exten_info["origin_id"])) {
                LOG_ERROR << "fake message. " << req_content->header.exten_info["origin_id"];
                return E_DEFAULT;
            }

            //check task id and get container
            auto task = m_user_task_ptr->find_task(task_id);
            if (nullptr == task) {
                req_content->header.path.push_back(CONF_MANAGER->get_node_id());

                LOG_DEBUG << "0 broadcast_message ai power provider service on logs req does not have task"
                          << task_id;
                CONNECTION_MANAGER->broadcast_message(msg, msg->header.src_sid);

                /*
                if (CONF_MANAGER->get_node_id() == "2gfpp3MAB489TcFSWfwvyXcgJKUcDWybSuPsi88SZQF"
                    || CONF_MANAGER->get_node_id() == "2gfpp3MAB4K4z1f3vyEUergJmjG64kXPFXNspBbhn3n"
                    || CONF_MANAGER->get_node_id() == "2gfpp3MAB4Hta5wtuFv9Ef7Cg9ZkUamgxYh3UjTYsQ8"
                    || CONF_MANAGER->get_node_id() == "2gfpp3MAB4CdhXs56P4UwtTauGcWcMbnBJhF6qWAWQD"
                    || CONF_MANAGER->get_node_id() == "2gfpp3MAB43V9jndyUdKAeU6qAQ1kFRw3WX3rqqfmKj"
                    || CONF_MANAGER->get_node_id() == "2gfpp3MAB4Bx3W6xH62YicCj2rsaBCk6ujeC11MooaQ"
                    || CONF_MANAGER->get_node_id() == "2gfpp3MAB488Ut5nduiiZ33dJmvS6txstdRPeHZ7Kkf"
                    || CONF_MANAGER->get_node_id() == "2gfpp3MAB3x6jBqi8S2jaoC6yAc5MSVvjUDStUdLqpm"
                    || CONF_MANAGER->get_node_id() == "2gfpp3MAB3vc4tVLHb1D4iacrTmzsQ3NzGE7HS5pnfw"
                    || CONF_MANAGER->get_node_id() == "2gfpp3MAB432cJRdKhLd4cGZwpZjoE5PtxvEmHhWbp1"
                    || CONF_MANAGER->get_node_id() == "2gfpp3MAB49V1bPqEXyuef2pThQWqN6sFij71m7epkd"
                    || CONF_MANAGER->get_node_id() == "2gfpp3MAB42X3BJhQjmMe6HAFivnMVU5QXVC1nFfAK5"
                    || CONF_MANAGER->get_node_id() == "2gfpp3MAB4AMdYxL74z7kq6y1zD7AgJtwRmuv4mUf9p"
                    || CONF_MANAGER->get_node_id() == "2gfpp3MAB414gDp7YY9afeVwptEp4hf8MWtfE8Pp3zE"
                    || CONF_MANAGER->get_node_id() == "2gfpp3MAB3zQYp35EGQeGwMcEAvE2PBsk9shwL7gyWA"
                    || CONF_MANAGER->get_node_id() == "2gfpp3MAB4HYkz2ezFwxa36mpCZzTGxXgg8TNrPynYo"
                    || CONF_MANAGER->get_node_id() == "2gfpp3MAB3vRQuPD23q8sBNHDn7jknCiqcx4NmoRWQY"
                    || CONF_MANAGER->get_node_id() == "2gfpp3MAB4GaGfS4fj1Z8rXjkHLcF4ifNB2EMWPpKL2"
                    || CONF_MANAGER->get_node_id() == "2gfpp3MAB49qg5LJPE3pAXQULDP4aK2TQDc93edaG5i"
                    || CONF_MANAGER->get_node_id() == "2gfpp3MAB4F8a5dTsjo8qduNuWoQVWw1XJeRYL7rKiC"
                    || CONF_MANAGER->get_node_id() == "2gfpp3MAB49pWqfvgTvKXArEU6wrp9UCYZi9hoR6sw1"
                    || CONF_MANAGER->get_node_id() == "2gfpp3MAB4Gxw9JXAkccZJCsLqZJ9T77W5JjQdbhLK7"
                    || CONF_MANAGER->get_node_id() == "2gfpp3MAB3yFm6vwKzL2jtiTPbzZRiHXuZFQM7i6aZ8"
                    || CONF_MANAGER->get_node_id() == "2gfpp3MAB4C8RrFLupsM4SBBiDfHWmPSZ29zSiPDMx6"
                    || CONF_MANAGER->get_node_id() == "2gfpp3MAB47xpUSdgv2oMbeYvbMTfnF7TRhRPtwMDpM") {
                    LOG_DEBUG << "0 broadcast_message ai power provider service on logs req does not have task"
                              << task_id;
                    CONNECTION_MANAGER->broadcast_message(msg, msg->header.src_sid);
                } else {
                    srand((int) time(0));
                    int32_t count = (rand() % (10 - 1) + 1);
                    if (count == 6) {
                        LOG_DEBUG << "1 broadcast_message ai power provider service on logs req does not have task"
                                  << task_id;
                        CONNECTION_MANAGER->broadcast_message(msg, msg->header.src_sid);
                    }
                }
                */

                return E_SUCCESS;
            }

            /*
            if (use_sign_verify()) {
                if (task->ai_user_node_id != req_content->header.exten_info["origin_id"]) {
                    return E_DEFAULT;
                }
            }
            */

            const std::string &container_id = task->container_id;

            std::shared_ptr<container_logs_req> container_req = std::make_shared<container_logs_req>();
            container_req->container_id = container_id;
            container_req->head_or_tail = req_content->body.head_or_tail;
            container_req->number_of_lines =
                    (req_content->body.number_of_lines) == 0 ? MAX_NUMBER_OF_LINES : req_content->body.number_of_lines;
            container_req->timestamps = true;

            std::string log_content;
            std::shared_ptr<container_logs_resp> container_resp = nullptr;
            if (!container_id.empty()) {
                container_resp = CONTAINER_WORKER_IF->get_container_log(container_req);
                if (nullptr == container_resp) {
                    LOG_ERROR << "ai power provider service get container logs error: " << task_id;
                    log_content = "get log content error.";
                    time_t cur = time_util::get_time_stamp_ms();
                    if (task->status >= task_stopped &&
                        cur - task->end_time > CONF_MANAGER->get_prune_container_stop_interval()) {
                        log_content += " the task may have been cleared";
                    }
                }
            } else if (task->status == task_queueing) {
                log_content = "task is waiting for schedule";
            } else if (task->status == task_pulling_image) {
                log_content = m_user_task_ptr->get_pull_log(task->training_engine);
            } else {
                log_content = "task abnormal. get log content error";
            }

            //添加密钥
            {
                leveldb::DB *db = nullptr;
                leveldb::Options  options;
                options.create_if_missing = true;
                boost::filesystem::path pwd_db_path = env_manager::get_db_path();
                if (fs::exists(pwd_db_path)) {
                    pwd_db_path /= fs::path("pwd.db");
                    leveldb::Status status = leveldb::DB::Open(options, pwd_db_path.generic_string(), &db);
                    if (status.ok()) {
                        std::string strpwd;
                        db->Get(leveldb::ReadOptions(), task_id, &strpwd);
                        log_content += "vmpwd:\"" + strpwd + "\"";
                    }
                }
            }

            std::shared_ptr<matrix::service_core::logs_resp> rsp_content = std::make_shared<matrix::service_core::logs_resp>();
            rsp_content->header.__set_magic(CONF_MANAGER->get_net_flag());
            rsp_content->header.__set_msg_name(LOGS_RESP);
            rsp_content->header.__set_nonce(id_generator::generate_nonce());
            rsp_content->header.__set_session_id(req_content->header.session_id);
            rsp_content->header.__set_path(req_content->header.path);

            peer_node_log log;
            log.__set_peer_node_id(CONF_MANAGER->get_node_id());

            log_content = (nullptr == container_resp) ? log_content : format_logs(container_resp->log_content,
                                                                                  req_content->body.number_of_lines);

            std::map<std::string, std::string> exten_info;

            // jimmy: encrypt log content with ecdh
            CPubKey cpk_remote;
            bool encrypt_ok = false;

            ai_ecdh_crypter crypter(static_cast<secp256k1_context *>(get_context_sign()));

            if (!ai_crypto_util::derive_pub_key_bysign(sign_req_msg, req_content->header.exten_info, cpk_remote)) {
                LOG_ERROR << "fail to extract the pub key from signature";
            } else {
                ai_ecdh_cipher cipher;
                if (!crypter.encrypt(cpk_remote, log_content, cipher)) {
                    LOG_ERROR << "fail to encrypt log content";
                }

                encrypt_ok = true;
                log.__set_log_content(cipher.m_data);
                exten_info["ecdh_pub"] = cipher.m_pub;
            }

            if (!encrypt_ok) {
                log.__set_log_content(log_content);
            }

            if (GET_LOG_HEAD == req_content->body.head_or_tail) {
                log.log_content = log.log_content.substr(0, MAX_LOG_CONTENT_SIZE);
            } else {
                size_t log_lenth = log.log_content.length();
                if (log_lenth > MAX_LOG_CONTENT_SIZE) {
                    log.log_content = log.log_content.substr(log_lenth - MAX_LOG_CONTENT_SIZE, MAX_LOG_CONTENT_SIZE);
                }
            }

            rsp_content->body.__set_log(log);

            //add sign
            std::string sign_msg =
                    CONF_MANAGER->get_node_id() + rsp_content->header.nonce + rsp_content->header.session_id +
                    rsp_content->body.log.log_content;

            if (E_SUCCESS != ai_crypto_util::extra_sign_info(sign_msg, exten_info)) {
                return E_DEFAULT;
            }

            rsp_content->header.__set_exten_info(exten_info);

            //resp msg
            std::shared_ptr<message> resp_msg = std::make_shared<message>();
            resp_msg->set_name(LOGS_RESP);
            resp_msg->set_content(rsp_content);
            CONNECTION_MANAGER->send_resp_message(resp_msg);

            return E_SUCCESS;
        }

        int32_t ai_power_provider_service::task_restart(std::shared_ptr<matrix::service_core::start_training_req> req,
                                                        bool is_docker) {
            if (req == nullptr) return E_DEFAULT;
            LOG_INFO << "restart: " << req->body.task_id << endl;

            if (m_user_task_ptr->get_user_cur_task_size() >= AI_TRAINING_MAX_TASK_COUNT) {
                LOG_ERROR << "ai power provider service on start training too many tasks, task id: "
                          << req->body.task_id;
                return E_DEFAULT;
            }

            auto task = m_user_task_ptr->find_task(req->body.task_id);
            if (nullptr == task) {
                task = std::make_shared<ai_training_task>();

                if (is_docker) {
                    std::shared_ptr<container_inspect_response> resp = CONTAINER_WORKER_IF->inspect_container(
                            req->body.task_id);
                    if (nullptr != resp && !resp->id.empty()) {
                        task->__set_container_id(resp->id);
                    }
                }

                task->__set_task_id(req->body.task_id);
                task->__set_select_mode(req->body.select_mode);
                task->__set_master(req->body.master);
                task->__set_peer_nodes_list(req->body.peer_nodes_list);
                task->__set_server_count(req->body.server_count);
                task->__set_training_engine(req->body.training_engine);
                task->__set_code_dir(req->body.code_dir);
                task->__set_entry_file(req->body.entry_file);
                task->__set_data_dir(req->body.data_dir);
                task->__set_checkpoint_dir(req->body.checkpoint_dir);
                task->__set_hyper_parameters(req->body.hyper_parameters);
                task->__set_ai_user_node_id(req->header.exten_info["origin_id"]);
                task->__set_error_times(0);
                task->__set_gpus(get_gpu_spec(task->server_specification));
                task->__set_received_time_stamp(std::time(nullptr));
                task->__set_status(task_queueing);
                task->__set_server_specification(req->body.server_specification);
                m_user_task_ptr->add_task(task);

                return E_SUCCESS;
            } else {
                task->__set_error_times(0);
                task->__set_received_time_stamp(std::time(nullptr));
                task->__set_status(task_queueing);
                task->__set_server_specification(req->body.server_specification);
                m_user_task_ptr->add_task(task);

                return E_SUCCESS;
            }
        }

        int32_t ai_power_provider_service::node_reboot(std::shared_ptr<matrix::service_core::start_training_req> req) {
            if (req == nullptr) return E_DEFAULT;
            LOG_INFO << "reboot: " << req->body.task_id << endl;

            std::shared_ptr<ai_training_task> task = std::make_shared<ai_training_task>();
            task->__set_task_id(req->body.task_id);
            task->__set_select_mode(req->body.select_mode);
            task->__set_master(req->body.master);
            task->__set_peer_nodes_list(req->body.peer_nodes_list);
            task->__set_server_specification(req->body.server_specification);
            task->__set_server_count(req->body.server_count);
            task->__set_training_engine(req->body.training_engine);
            task->__set_code_dir(req->body.code_dir);
            task->__set_entry_file(req->body.entry_file);
            task->__set_data_dir(req->body.data_dir);
            task->__set_checkpoint_dir(req->body.checkpoint_dir);
            task->__set_hyper_parameters(req->body.hyper_parameters);
            task->__set_ai_user_node_id(req->header.exten_info["origin_id"]);
            task->__set_error_times(0);
            task->__set_container_id("");
            task->__set_received_time_stamp(std::time(nullptr));
            task->__set_status(task_queueing);
            // task->__set_memory(req->body.memory);
            // task->__set_memory_swap(req->body.memory_swap);
            m_urgent_task = task;

            return E_SUCCESS;
        }

        std::string get_is_update(const std::string &s) {
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
            } catch (...) {

            }

            return operation;
        }

        //start new task or update task
        int32_t ai_power_provider_service::task_start(std::shared_ptr<matrix::service_core::start_training_req> req)
        {
            if (req == nullptr) return E_DEFAULT;
            LOG_INFO << "reboot: " << req->body.task_id << endl;

            if (m_user_task_ptr->get_user_cur_task_size() >= AI_TRAINING_MAX_TASK_COUNT) {
                LOG_ERROR << "ai power provider service on start training too many tasks, task id: "
                          << req->body.task_id;
                return E_DEFAULT;
            }

            if (m_user_task_ptr->find_task(req->body.task_id)) {
                LOG_ERROR << "ai power provider service on start training already has task: " << req->body.task_id;
                return E_DEFAULT;
            }

            std::shared_ptr<ai_training_task> task = std::make_shared<ai_training_task>();
            task->__set_task_id(req->body.task_id);
            task->__set_select_mode(req->body.select_mode);
            task->__set_master(req->body.master);
            task->__set_peer_nodes_list(req->body.peer_nodes_list);
            task->__set_server_specification(req->body.server_specification);
            task->__set_server_count(req->body.server_count);
            task->__set_training_engine(req->body.training_engine);
            task->__set_code_dir(req->body.code_dir);
            task->__set_entry_file(req->body.entry_file);
            task->__set_data_dir(req->body.data_dir);
            task->__set_checkpoint_dir(req->body.checkpoint_dir);
            task->__set_hyper_parameters(req->body.hyper_parameters);
            task->__set_ai_user_node_id(req->header.exten_info["origin_id"]);
            task->__set_error_times(0);

            std::string update = "update_docker";
            std::string update1 = "update_vm";
            const std::string &server_specification = task->server_specification;

            if (update.compare(get_is_update(server_specification)) != 0 &&
                update1.compare(get_is_update(server_specification)) != 0) {
                task->__set_gpus(get_gpu_spec(server_specification));
            }

            // reuse container where container name is specificed in training requester msg.
            //      As we know, dbc names a container with the task id value when create the container.
            //      So the input container name also refer to a task id.
            std::string ref_container_id;
            {
                std::string task_id = get_task_id(req);
                auto ref_task2 = m_user_task_ptr->find_task(task_id);
                if (ref_task2 != nullptr) {
                    // update
                    if (update.compare(get_is_update(server_specification)) == 0 ||
                        update1.compare(get_is_update(server_specification)) == 0) {
                        ref_container_id = ref_task2->container_id;
                        task->__set_status(task_queueing);
                        task->__set_gpus(get_gpu_spec(task->server_specification));
                        task->__set_task_id(task_id); //update to old task id
                        task->__set_container_id(ref_container_id);
                        task->__set_received_time_stamp(std::time(nullptr));

                        return m_user_task_ptr->add_update_task(task);
                    }
                } else {
                    // only create new task
                    if (update.compare(get_is_update(task->server_specification)) != 0 &&
                        update1.compare(get_is_update(task->server_specification)) != 0) {
                        // task->__set_container_id(ref_container_id);
                        task->__set_received_time_stamp(std::time(nullptr));
                        task->__set_status(task_queueing);

                        return m_user_task_ptr->add_task(task);
                    } else {
                        std::shared_ptr<container_inspect_response> resp = CONTAINER_WORKER_IF->inspect_container(task_id);
                        if (nullptr != resp) {
                            if (!resp->id.empty()) {
                                //update to old task id
                                task->__set_container_id(resp->id);
                                task->__set_task_id(task_id);
                                task->__set_received_time_stamp(std::time(nullptr));
                                task->__set_status(task_queueing);

                                return m_user_task_ptr->add_update_task(task);
                            }
                        }
                    }
                }
            }

            return E_SUCCESS;
        }

        std::string
        ai_power_provider_service::get_task_id(std::shared_ptr<matrix::service_core::start_training_req> req) {
            if (nullptr == req) {
                return "";
            }

            auto customer_setting = req->body.server_specification;
            std::string task_id;
            if (!customer_setting.empty()) {
                std::stringstream ss;
                ss << customer_setting;
                boost::property_tree::ptree pt;

                try {
                    boost::property_tree::read_json(ss, pt);
                    if (pt.count("task_id") != 0) {
                        task_id = pt.get<std::string>("task_id");
                    }
                } catch (...) {

                }
            }

            return task_id;
        }

        std::string ai_power_provider_service::format_logs(const std::string &raw_logs, uint16_t max_lines) {
            //docker logs has special format with each line of log:
            // 0x01 0x00  0x00 0x00 0x00 0x00 0x00 0x38
            //we should remove it
            //and ends with 0x30 0x0d 0x0a 
            max_lines = (max_lines == 0) ? MAX_NUMBER_OF_LINES : max_lines;
            size_t size = raw_logs.size();
            vector<unsigned char> log_vector(size);

            int push_char_count = 0;
            const char *p = raw_logs.c_str();

            uint16_t line_count = 1;

            for (size_t i = 0; i < size;) {
                //0x30 0x0d 0x0a 
                if ((i + 2 < size)
                    && (0x30 == *p)
                    && (0x0d == *(p + 1))
                    && (0x0a == *(p + 2))) {
                    break;
                }

                if (max_lines != 0 && line_count > max_lines) {
                    break;
                }

                //skip: 0x01 0x00  0x00 0x00 0x00 0x00 0x00 0x38
                if ((i + 7 < size)
                    && ((0x01 == *p) || (0x02 == *p))
                    && (0x00 == *(p + 1))
                    && (0x00 == *(p + 2))
                    && (0x00 == *(p + 3))
                    && (0x00 == *(p + 4))
                    && (0x00 == *(p + 5))) {
                    i += 8;
                    p += 8;
                    continue;
                }

                log_vector[push_char_count] = *p++;

                if (log_vector[push_char_count] == '\n') {
                    line_count++;
                }

                ++push_char_count;
                i++;
            }

            std::string formatted_str;
            formatted_str.reserve(push_char_count);

            int i = 0;
            while (i < push_char_count) {
                formatted_str += log_vector[i++];
            }

            return formatted_str;
        }

        int32_t ai_power_provider_service::on_training_task_timer(std::shared_ptr<core_timer> timer) {
            if (m_user_task_ptr != nullptr && m_urgent_task) {
                m_user_task_ptr->process_urgent_task(m_urgent_task);
                m_urgent_task = nullptr;
            }

            if (m_idle_task_ptr != nullptr) {
                m_idle_task_ptr->update_idle_task();
            }

            if (m_user_task_ptr != nullptr) {
                if (0 == m_user_task_ptr->get_user_cur_task_size()) {
                    if (m_oss_task_mng->can_exec_idle_task()) {
                        m_idle_task_ptr->exec_task();
                    }
                } else {
                    m_user_task_ptr->process_task();
                }
            }

            static int count = 0;
            count++;
            if ((count % 10) == 0) {
                if (m_user_task_ptr != nullptr)
                    m_user_task_ptr->update_gpu_info_from_proc();
            }

            return E_SUCCESS;
        }

        int32_t ai_power_provider_service::on_get_task_queue_size_req(std::shared_ptr<message> &msg) {
            auto resp = std::make_shared<service::get_task_queue_size_resp_msg>();

            auto task_num = m_user_task_ptr->get_user_cur_task_size();

            if (m_idle_task_ptr && m_idle_task_ptr->get_status() == task_running) {
                task_num = -1; // task_num(-1) means idle task is running.
            }

            resp->set_task_size(task_num);
            resp->set_gpu_state(m_user_task_ptr->get_gpu_state());
            resp->set_active_tasks(m_user_task_ptr->get_active_tasks());

            auto resp_msg = std::dynamic_pointer_cast<message>(resp);

            TOPIC_MANAGER->publish<int32_t>(typeid(service::get_task_queue_size_resp_msg).name(), resp_msg);

            return E_SUCCESS;
        }

        int32_t ai_power_provider_service::check_sign(const std::string message, const std::string &sign,
                                                      const std::string &origin_id, const std::string &sign_algo) {
            if (sign_algo != ECDSA) {
                LOG_ERROR << "sign_algorithm error.";
                return E_DEFAULT;
            }

            if (origin_id.empty() || sign.empty()) {
                LOG_ERROR << "sign error.";
                return E_DEFAULT;
            }

            std::string derive_node_id;
            id_generator::derive_node_id_by_sign(message, sign, derive_node_id);
            if (derive_node_id != origin_id) {
                LOG_ERROR << "sign check error";
                return E_DEFAULT;
            }

            return E_SUCCESS;
        }

        //prune the task container that have been stopped.
        int32_t ai_power_provider_service::on_prune_task_timer(std::shared_ptr<core_timer> timer) {
            if (m_user_task_ptr != nullptr)
                return m_user_task_ptr->prune_task();
            else
                return E_SUCCESS;
        }
    }
}
