/*********************************************************************************
*  Copyright (c) 2017-2018 DeepBrainChain core team
*  Distributed under the MIT software license, see the accompanying
*  file COPYING or http://www.opensource.org/licenses/mit-license.php
* file name         :   ai_power_provider_service.cpp
* description     :   ai_power_provider_service
* date                  :   2018.01.28
* author             :   Bruce Feng
**********************************************************************************/
#include <cassert>
#include <boost/exception/all.hpp>
#include <iostream>
#include "server.h"
#include "api_call_handler.h"
#include "conf_manager.h"
#include "ai_power_provider_service.h"
#include "peer_node.h"
#include "service_message_id.h"
#include "matrix_types.h"
#include "matrix_coder.h"
#include "ip_validator.h"
#include "port_validator.h"
#include "task_common_def.h"
#include "util.h"
#include "utilstrencodings.h"
#include "base58.h"

#include "error/en.h"
#include <boost/thread/thread.hpp>
#include "utilstrencodings.h"
#include "service_topic.h"
#include <ctime>
#include <iostream>
#include <boost/format.hpp>
#include "url_validator.h"
#include "time_util.h"

using namespace std;
using namespace boost::asio::ip;
using namespace matrix::core;
using namespace matrix::service_core;
using namespace ai::dbc;

namespace ai
{
    namespace dbc
    {
        

        ai_power_provider_service::ai_power_provider_service()
            : m_prov_training_task_db()
            , m_container_ip(DEFAULT_LOCAL_IP)
            , m_container_port((uint16_t)std::stoi(DEFAULT_CONTAINER_LISTEN_PORT))
            , m_container_client(std::make_shared<container_client>(m_container_ip, m_container_port))
            , m_nvidia_client(std::make_shared<container_client>(m_container_ip, DEFAULT_NVIDIA_DOCKER_PORT))
            , m_training_task_timer_id(INVALID_TIMER_ID)
            , m_nv_config(nullptr)
            , m_nv_docker_version(NVIDIA_DOCKER_UNKNOWN)
    {

        }

        int32_t ai_power_provider_service::init_conf()
        {
            ip_validator ip_vdr;
            port_validator port_vdr;

            //container ip
            const std::string & container_ip = CONF_MANAGER->get_container_ip();

            variable_value val;
            val.value() = container_ip;

            if (false == ip_vdr.validate(val))
            {
                LOG_ERROR << "ai power provider init conf invalid container ip: " << container_ip;
                return E_DEFAULT;
            }
            m_container_ip = container_ip;

            //container port
            std::string s_port = CONF_MANAGER->get_container_port();
            val.value() = s_port;

            if (false == port_vdr.validate(val))
            {
                LOG_ERROR << "ai power provider init conf invalid container port: " << s_port;
                return E_DEFAULT;
            }
            else
            {
                try
                {
                    m_container_port = (uint16_t)std::stoi(s_port);
                }
                catch (const std::exception &e)
                {
                    LOG_ERROR << "ai power provider service init conf container port: " << s_port << ", " << e.what();
                    return E_DEFAULT;
                }
            }
            
            m_container_client->set_address(m_container_ip, m_container_port);
            m_nvidia_client->set_address(m_container_ip, DEFAULT_NVIDIA_DOCKER_PORT);

            return E_SUCCESS;
        }

        int32_t ai_power_provider_service::service_init(bpo::variables_map &options)
        {
            int32_t ret = E_SUCCESS;

            ret = init_conf();
            if (E_SUCCESS != ret)
            {
                LOG_ERROR << "ai power provider service init config error";
                return ret;
            }

            ret = init_db();
            if (E_SUCCESS != ret)
            {
                LOG_ERROR << "ai power provider service init db error";
                return ret;
            }

            ret = load_container();
            if (E_SUCCESS != ret)
            {
                LOG_ERROR << "ai power provider service load container config error";
                return ret;
            }

            ret = load_bill_config();
            if (E_SUCCESS != ret)
            {
                LOG_ERROR << "ai power provider service load bill config error";
                return ret;
            }
            return E_SUCCESS;
        }

        int32_t ai_power_provider_service::check_cpu_config(const double & cpu_info)
        {
            uint16_t cpu_num = std::thread::hardware_concurrency();
            if (cpu_info < 0)
            {
                LOG_ERROR << "cpus config error. cpus was smaller than 0";
                return E_DEFAULT;
            }

            double temp = cpu_info * m_nano_cpu;
            if (temp - (int64_t)temp > 0)
            {
                LOG_ERROR << "cpu num config error";
                return E_DEFAULT;
            }

            if (temp > cpu_num * m_nano_cpu)
            {
                LOG_ERROR << "cpu num error" << " system cpu is:" << cpu_num << " cpu config num is" << cpu_info;

                return E_DEFAULT;
            }

            return E_SUCCESS;
        }

        int32_t ai_power_provider_service::check_memory_config(int64_t memory, int64_t memory_swap, int64_t shm_size)
        {
            if (memory < 0 )
            {
                LOG_ERROR << "memory config error. memeory was smaller than 0";
                return E_DEFAULT;
            }

            if ( memory_swap < -1)
            {
                LOG_ERROR << "memory_swap config error.memory_swap was smaller than -1";
                return E_DEFAULT;
            }

            /*if (shm_size < 1)
            {
                LOG_ERROR << "shm_size config error. shm_size was smaller than 1";
                return E_DEFAULT;
            }*/

            if (((memory_swap != 0) && (memory_swap != -1))
                &&(memory > memory_swap))
            {
                LOG_ERROR << "config error:memory > memory_swap";
                return E_DEFAULT;
            }

            int64_t sys_mem = 0;
            int64_t sys_swap = 0;
            get_sys_mem(sys_mem, sys_swap);

            if (0==sys_mem || 0==sys_swap)
            {
                return E_SUCCESS;
            }

            if ( ((memory != 0) && (memory * m_g_bytes) > sys_mem)
                 || (memory != 0 && shm_size > memory))
            {
                LOG_ERROR << "check memory failed.";
                return E_DEFAULT;
            }


            if (((memory_swap != 0) || (memory_swap != -1))
               && (memory_swap * m_g_bytes) > sys_swap)
            {
                LOG_ERROR << "check memory_swap failed.";
                return E_DEFAULT;
            }

            if (memory != 0 && shm_size > memory)
            {
                LOG_ERROR << "check shm_size failed.";
                return E_DEFAULT;
            }

            if (shm_size * m_g_bytes > sys_mem)
            {
                LOG_ERROR << "check shm_size failed.";
                return E_DEFAULT;
            }
            return E_SUCCESS;
        }

        int32_t ai_power_provider_service::load_container()
        {
            //file path
            const fs::path &container_path = env_manager::get_container_path();
            bpo::options_description container_opts("container file options");

            container_opts.add_options()
                ("memory", bpo::value<int64_t>()->default_value(0), "")
                ("memory_swap", bpo::value<int64_t>()->default_value(-1), "")
                ("cpus", bpo::value<double>()->default_value(0), "")
                ("shm_size", bpo::value<int64_t>()->default_value(0), "")
                ("host_volum_dir", bpo::value<std::string>()->default_value(""), "")
                ("auto_pull_image", bpo::value<bool>()->default_value(true), "")
                ("engine_reg", bpo::value<std::string>()->default_value(""), "");

            try
            {
                //container.conf
                std::ifstream conf_ifs(container_path.generic_string());
                bpo::store(bpo::parse_config_file(conf_ifs, container_opts), m_container_args);

                bpo::notify(m_container_args);

                std::string host_dir = m_container_args["host_volum_dir"].as<std::string>();

                if (!host_dir.empty() && !fs::exists(host_dir))
                {
                    LOG_ERROR << "host volum dir is not exist. pls check";
                    return E_DEFAULT;
                }

                if (!host_dir.empty() && !fs::is_empty(host_dir))
                {
                    LOG_WARNING << "dangerous: host_volum_dir contain files. pls check";
                    //return E_DEFAULT;
                }

                int32_t ret = check_cpu_config( m_container_args["cpus"].as<double>());

                if (ret != E_SUCCESS)
                {
                    return ret;
                }

                ret = check_memory_config(m_container_args["memory"].as<int64_t>(), m_container_args["memory_swap"].as<int64_t>(), m_container_args["shm_size"].as<int64_t>());
                if (ret != E_SUCCESS)
                {
                    return ret;
                }
                m_auto_pull_image = m_container_args["auto_pull_image"].as<bool>();
                set_task_engine(m_container_args["engine_reg"].as<std::string>());
            }
            catch (const boost::exception & e)
            {
                LOG_ERROR << "parse container.conf error: " << diagnostic_information(e);
                return E_DEFAULT;
            }

            return E_SUCCESS;
        }

        int32_t ai_power_provider_service::load_bill_config()
        {
            std::string url = CONF_MANAGER->get_bill_url();
            if (url.empty())
            {
                return E_SUCCESS;
            }
            variable_value val;
            val.value() = url;
            if (true != url_validator().validate(val))
            {
                LOG_ERROR << "url config error";
                return E_DEFAULT;
            }

            std::string crt = CONF_MANAGER->get_bill_crt();
            if (! crt.empty() && !fs::exists(crt))
            {
                LOG_ERROR << "crt path error";
                return E_DEFAULT;
            }
            if (!url.empty())
            {
                m_bill_client = std::make_shared<bill_client>(url, crt);
                if (m_bill_client != nullptr)
                {
                    return E_SUCCESS;
                }
                else
                {
                    return E_DEFAULT;
                }
            }

            return E_SUCCESS;
        }

        int32_t ai_power_provider_service::service_exit()
        {
            remove_timer(m_training_task_timer_id);
            return E_SUCCESS;
        }

        void ai_power_provider_service::init_subscription()
        {
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

        void ai_power_provider_service::init_invoker()
        {
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
            BIND_MESSAGE_INVOKER(typeid(service::get_task_queue_size_req_msg).name(), &ai_power_provider_service::on_get_task_queue_size_req);
        }

        int32_t ai_power_provider_service::init_db()
        {
            leveldb::DB *db = nullptr;
            leveldb::Options  options;
            options.create_if_missing = true;
            try
            {
                //get db path
                fs::path task_db_path = env_manager::get_db_path();
                if (false == fs::exists(task_db_path))
                {
                    LOG_DEBUG << "db directory path does not exist and create db directory";
                    fs::create_directory(task_db_path);
                }

                //check db directory
                if (false == fs::is_directory(task_db_path))
                {
                    LOG_ERROR << "db directory path does not exist and exit";
                    return E_DEFAULT;
                }

                task_db_path /= fs::path("prov_training_task.db");
                LOG_DEBUG << "training task db path: " << task_db_path.generic_string();

                //open db
                leveldb::Status status = leveldb::DB::Open(options, task_db_path.generic_string(), &db);
                if (false == status.ok())
                {
                    LOG_ERROR << "ai power provider service init training task db error: " << status.ToString();
                    return E_DEFAULT;
                }

                //smart point auto close db
                m_prov_training_task_db.reset(db);

                //load task
                load_task_from_db();
                
                LOG_INFO << "ai power provider training db path:" << task_db_path;
            }
            catch (const std::exception & e)
            {
                LOG_ERROR << "create task provider db error: " << e.what();
                return E_DEFAULT;
            }
            catch (const boost::exception & e)
            {
                LOG_ERROR << "create task provider db error" << diagnostic_information(e);
                return E_DEFAULT;
            }

            return E_SUCCESS;
        }

        void ai_power_provider_service::init_timer()
        {
            m_timer_invokers[AI_TRAINING_TASK_TIMER] = std::bind(&ai_power_provider_service::on_training_task_timer, this, std::placeholders::_1);
            m_training_task_timer_id = this->add_timer(AI_TRAINING_TASK_TIMER, AI_TRAINING_TASK_TIMER_INTERVAL);
            assert(INVALID_TIMER_ID != m_training_task_timer_id);
        }

        int32_t ai_power_provider_service::on_start_training_req(std::shared_ptr<message> &msg)
        {
            std::shared_ptr<matrix::service_core::start_training_req> req = std::dynamic_pointer_cast<matrix::service_core::start_training_req>(msg->get_content());
            assert(nullptr != req);

            if (id_generator().check_base58_id(req->header.nonce) != true)
            {
                LOG_ERROR << "ai power provider service nonce error ";
                return E_DEFAULT;
            }

            if (id_generator().check_base58_id(req->body.task_id) != true)
            {
                LOG_ERROR << "ai power provider service task_id error ";
                return E_DEFAULT;
            }

            if (req->body.entry_file.empty())
            {
                LOG_ERROR << "entry_file non exist in task.";
                return E_DEFAULT;
            }

            if (req->body.entry_file.size() > MAX_ENTRY_FILE_NAME_LEN)
            {

                LOG_ERROR << "entry_file name lenth is too long." << req->body.entry_file.size();
                return E_DEFAULT;
            }

            if (req->body.training_engine.size() > MAX_ENGINE_IMGE_NAME_LEN)
            {
                LOG_ERROR << "engine image lenth is too long." << req->body.training_engine.size();
                return E_DEFAULT;
            }

            if (check_task_engine(req->body.training_engine) != true)
            {
                LOG_ERROR << "engine name is error." << req->body.training_engine.size();
                return E_DEFAULT;
            }

            std::string message = req->body.task_id + req->body.code_dir + req->header.nonce;
            if (req->header.exten_info.size()<3)
            {
                LOG_ERROR << "exten info error.";
                return E_DEFAULT;
            }

            if (E_SUCCESS != check_sign(message, req->header.exten_info["sign"], req->header.exten_info["origin_id"], req->header.exten_info["sign_algo"]))
            {
                LOG_ERROR << "sign error.";
                return E_DEFAULT;
            }

            //check node id
            const std::vector<std::string> &peer_nodes = req->body.peer_nodes_list;
            auto it = peer_nodes.begin();
            for (; it != peer_nodes.end(); it++)
            {
                if (id_generator().check_node_id((*it)) != true)
                {
                    LOG_ERROR <<"ai power provider service node_id error " << (*it);
                    return E_DEFAULT;
                }
                
                if ((*it) == CONF_MANAGER->get_node_id())
                {
                    LOG_DEBUG << "ai power provider service found self node id in on start training: " << req->body.task_id << " node id: " << (*it);
                    break;
                }
            }

            //not find self; or find self, but designate more than one node
            if (it == peer_nodes.end() || peer_nodes.size() > 1)
            {
                LOG_DEBUG << "ai power provider service found start training req " << req->body.task_id << " is not self and exit function";
                //relay start training in network
                LOG_DEBUG << "ai power provider service relay broadcast start training req to neighbor peer nodes: " << req->body.task_id;
                CONNECTION_MANAGER->broadcast_message(msg, msg->header.src_sid);
            }

            if (it == peer_nodes.end())
            {
                return E_SUCCESS;//not find self, return
            }

            if (nullptr == m_prov_training_task_db)
            {
                LOG_ERROR << "ai power provider service training task db is nullptr, task id: " << req->body.task_id;
                return E_DEFAULT;
            }

            if (m_queueing_tasks.size() >= AI_TRAINING_MAX_TASK_COUNT)
            {
                LOG_ERROR << "ai power provider service on start training too many tasks, task id: " << req->body.task_id;
                return E_DEFAULT;
            }

            //check task id exists 
            std::string task_value;
            leveldb::Status status = m_prov_training_task_db->Get(leveldb::ReadOptions(), req->body.task_id, &task_value);
            if (status.ok())                //already exists and directly return
            {
                LOG_ERROR << "ai power provider service on start training already had task: " << req->body.task_id;
                return E_DEFAULT;
            }

            assert(0 == m_training_tasks.count(req->body.task_id));

            if (m_training_tasks.count(req->body.task_id) != 0)
            {
                LOG_ERROR << "ai power provider service on start training already had task: " << req->body.task_id;
                return E_DEFAULT;
            }

            std::shared_ptr<ai_training_task> task = std::make_shared<ai_training_task>();
            assert(nullptr != task);

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
            
            //flush to db
            write_task_to_db(task);
            LOG_INFO << "ai power provider service flush task to db: " << req->body.task_id;
            m_queueing_tasks.push_back(task);
            m_training_tasks[task->task_id] = task;
            LOG_INFO << "ai power provider service task(" << req->body.task_id << ") is in task_queueing";

            return E_SUCCESS;
        }

        int32_t ai_power_provider_service::on_stop_training_req(std::shared_ptr<message> &msg)
        {
            std::shared_ptr<matrix::service_core::stop_training_req> req = std::dynamic_pointer_cast<matrix::service_core::stop_training_req>(msg->get_content());
            assert(nullptr != req);

            if (id_generator().check_base58_id(req->header.nonce) != true)
            {
                LOG_ERROR << "ai power provider service on_stop_training_req nonce error ";
                return E_DEFAULT;
            }

            if (id_generator().check_base58_id(req->body.task_id) != true)
            {
                LOG_ERROR << "ai power provider service on_stop_training_req task_id error ";
                return E_DEFAULT;
            }
            std::string message = req->body.task_id  + req->header.nonce;
            if (req->header.exten_info.size()<3)
            {
                LOG_ERROR << "exten info error.";
                return E_DEFAULT;
            }
            if (E_SUCCESS != check_sign(message, req->header.exten_info["sign"], req->header.exten_info["origin_id"], req->header.exten_info["sign_algo"]))
            {
                LOG_ERROR << "sign error.";
                return E_DEFAULT;
            }

            //relay on stop_training to network(maybe task running on multiple nodes)
            LOG_DEBUG << "ai power provider service relay broadcast stop_training req to neighbor peer nodes: " << req->body.task_id;
            CONNECTION_MANAGER->broadcast_message(msg, msg->header.src_sid);

            //check task_id
            if (0 == m_queueing_tasks.size())
            {
                LOG_DEBUG << "training queuing task is empty";
                return E_SUCCESS;
            }

            std::shared_ptr<ai_training_task> sp_task;
            const std::string& task_id = std::dynamic_pointer_cast<stop_training_req>(msg->get_content())->body.task_id;

            //find from queue
            auto it = m_queueing_tasks.begin();
            for (; it != m_queueing_tasks.end(); it++)
            {
                if ((*it)->task_id == task_id)
                {
                    sp_task = *it;
                    break;
                }
            }

            if (sp_task) //found
            {
                if (sp_task->ai_user_node_id != req->header.exten_info["origin_id"])
                {
                    LOG_ERROR << "bad user try to stop task" << endl;
                    return E_DEFAULT;
                }
                LOG_INFO << "stop training, task_status: " << to_training_task_status_string(sp_task->status) << endl;                
                stop_task(sp_task, task_stopped);
                auth_task(sp_task);               
            }
            else
            {
                LOG_DEBUG << "stop training, not found task: " << task_id << endl;
            }

            return E_SUCCESS;
        }

        int32_t ai_power_provider_service::on_list_training_req(std::shared_ptr<message> &msg)
        {
            std::shared_ptr<list_training_req> req_content = std::dynamic_pointer_cast<list_training_req>(msg->get_content());

            if (id_generator().check_base58_id(req_content->header.nonce) != true)
            {
                LOG_ERROR << "ai power provider service nonce error ";
                return E_DEFAULT;
            }

            if (id_generator().check_base58_id(req_content->header.session_id) != true)
            {
                LOG_ERROR << "ai power provider service sessionid error ";
                return E_DEFAULT;
            }

            assert(nullptr != req_content);
            LOG_DEBUG << "on_list_training_req recv req, nonce: " << req_content->header.nonce << ", session: " << req_content->header.session_id;

            if (req_content->body.task_list.size() == 0)
            {
                LOG_ERROR << "ai power provider service recv empty list training tasks";
                return E_DEFAULT;
            }

            for (auto it = req_content->body.task_list.begin(); it != req_content->body.task_list.end(); ++it)
            {
                if (id_generator().check_base58_id(*it) != true)
                {
                    LOG_ERROR << "ai power provider service taskid error: " << *it;
                    return E_DEFAULT;
                }
                
            }

            //relay list_training to network(maybe task running on multiple nodes, no mater I took this task)

            req_content->header.path.push_back(CONF_MANAGER->get_node_id()); //add this node id into path

            CONNECTION_MANAGER->broadcast_message(msg, msg->header.src_sid);

            if (0 == m_training_tasks.size())
            {
                LOG_INFO << "ai power provider service training task is empty";
                return E_SUCCESS;
            }

            std::vector<matrix::service_core::task_status> status_list;
            for (auto it = req_content->body.task_list.begin(); it != req_content->body.task_list.end(); ++it)
            {
                //find task
                auto it_task = m_training_tasks.find(*it);
                if (it_task == m_training_tasks.end())
                {
                    continue;
                }

                matrix::service_core::task_status ts;
                ts.task_id = it_task->second->task_id;
                
                
                ts.status = it_task->second->status;
                status_list.push_back(ts);
                LOG_DEBUG << "on_list_training_req task: " << ts.task_id << "--" << to_training_task_status_string(ts.status);

                if (status_list.size() >= MAX_LIST_TASK_COUNT)
                {
                    std::shared_ptr<matrix::service_core::list_training_resp> rsp_content = std::make_shared<matrix::service_core::list_training_resp>();
                    //content header
                    rsp_content->header.__set_magic(CONF_MANAGER->get_net_flag());
                    rsp_content->header.__set_msg_name(LIST_TRAINING_RESP);
                    rsp_content->header.__set_nonce(id_generator().generate_nonce());
                    rsp_content->header.__set_session_id(req_content->header.session_id);

                    rsp_content->header.__set_path(req_content->header.path); // for efficient resp msg transport

                    rsp_content->body.task_status_list.swap(status_list);
                    status_list.clear(); 

                    //resp msg
                    std::shared_ptr<message> resp_msg = std::make_shared<message>();
                    resp_msg->set_name(LIST_TRAINING_RESP);
                    resp_msg->set_content(rsp_content);
                    CONNECTION_MANAGER->send_resp_message(resp_msg);

                    LOG_DEBUG << "on_list_training_req send resp, nonce: " << rsp_content->header.nonce << ", session: " << rsp_content->header.session_id
                        << "task cnt: " << rsp_content->body.task_status_list.size();
                 }
            }
            if (status_list.size() > 0)
            {
                std::shared_ptr<matrix::service_core::list_training_resp> rsp_content = std::make_shared<matrix::service_core::list_training_resp>();
                //content header
                rsp_content->header.__set_magic(CONF_MANAGER->get_net_flag());
                rsp_content->header.__set_msg_name(LIST_TRAINING_RESP);
                rsp_content->header.__set_nonce(id_generator().generate_nonce());
                rsp_content->header.__set_session_id(req_content->header.session_id);

                rsp_content->header.__set_path(req_content->header.path); // for efficient resp msg transport

                rsp_content->body.task_status_list.swap(status_list);

                //resp msg
                std::shared_ptr<message> resp_msg = std::make_shared<message>();
                resp_msg->set_name(LIST_TRAINING_RESP);
                resp_msg->set_content(rsp_content);
                CONNECTION_MANAGER->send_resp_message(resp_msg);

                status_list.clear();

                LOG_DEBUG << "on_list_training_req send resp, nonce: " << rsp_content->header.nonce << ", session: " << rsp_content->header.session_id
                    << "task cnt: " << rsp_content->body.task_status_list.size();
            }

            return E_SUCCESS;
        }

        int32_t ai_power_provider_service::on_logs_req(const std::shared_ptr<message> &msg)
        {
            std::shared_ptr<logs_req> req_content = std::dynamic_pointer_cast<logs_req>(msg->get_content());
            assert(nullptr != req_content);

            if (id_generator().check_base58_id(req_content->header.nonce) != true)
            {
                LOG_ERROR << "ai power provider service nonce error ";
                return E_DEFAULT;
            }

    
            if (id_generator().check_base58_id(req_content->header.session_id) != true)
            {
                LOG_ERROR << "ai power provider service session_id error ";
                return E_DEFAULT;
            }
    
            if (id_generator().check_base58_id(req_content->body.task_id) != true)
            {
                LOG_ERROR << "taskid error ";
                return E_DEFAULT;
            }
            

            const std::string &task_id = req_content->body.task_id;

            //check log direction
            if (GET_LOG_HEAD != req_content->body.head_or_tail && GET_LOG_TAIL != req_content->body.head_or_tail)
            {
                LOG_ERROR << "ai power provider service on logs req log direction error: " << task_id;
                return E_DEFAULT;
            }

            //check number of lines
            if (req_content->body.number_of_lines > MAX_NUMBER_OF_LINES || req_content->body.number_of_lines < 0)
            {
                LOG_ERROR << "ai power provider service on logs req number of lines error: " << req_content->body.number_of_lines;
                return E_DEFAULT;
            }

            //check task id
            /*std::string task_value;
            leveldb::Status status = m_prov_training_task_db->Get(leveldb::ReadOptions(), task_id, &task_value);
            if (!status.ok())
            {
            //relay msg to network
            LOG_DEBUG << "ai power provider service on logs req does not have task: " << task_id;
            CONNECTION_MANAGER->broadcast_message(msg);
            return E_SUCCESS;
            }*/

            //check task id and get container
            auto it = m_training_tasks.find(task_id);
            if (it == m_training_tasks.end())
            {
                //relay msg to network
                LOG_DEBUG << "ai power provider service on logs req does not have task: " << task_id;

                req_content->header.path.push_back(CONF_MANAGER->get_node_id()); //add this node id into path

                CONNECTION_MANAGER->broadcast_message(msg, msg->header.src_sid);

                return E_SUCCESS;
            }

            //get container logs
            const std::string &container_id = it->second->container_id;
            

            std::shared_ptr<container_logs_req> container_req = std::make_shared<container_logs_req>();
            container_req->container_id = container_id;
            container_req->head_or_tail = req_content->body.head_or_tail;
            container_req->number_of_lines = (req_content->body.number_of_lines) ==0 ? MAX_NUMBER_OF_LINES:req_content->body.number_of_lines;
            container_req->timestamps = true;

            std::string log_content;
            std::shared_ptr<container_logs_resp> container_resp = nullptr;
            if (!container_id.empty())
            {
                container_resp = m_container_client->get_container_log(container_req);
                if (nullptr == container_resp)
                {
                    LOG_ERROR << "ai power provider service get container logs error: " << task_id;
                    log_content = "get log content error";
                }
            }
            else if (it->second->status == task_queueing)
            {
                log_content = "task is waiting for schedule";
            }
            else if (it->second->status == task_pulling_image)
            {
                if (m_pull_image_mng!=nullptr)
                {
                    log_content = m_pull_image_mng->get_out_log(it->second->training_engine);
                }
            }
            else
            {
                log_content = "task abnormal. get log content error";
            }


            //response content
            std::shared_ptr<matrix::service_core::logs_resp> rsp_content = std::make_shared<matrix::service_core::logs_resp>();

            //content header
            rsp_content->header.__set_magic(CONF_MANAGER->get_net_flag());
            rsp_content->header.__set_msg_name(LOGS_RESP);
            rsp_content->header.__set_nonce(id_generator().generate_nonce());
            rsp_content->header.__set_session_id(req_content->header.session_id);

            rsp_content->header.__set_path(req_content->header.path); // for efficient resp msg transport

            //content body
            peer_node_log log;
            log.__set_peer_node_id(CONF_MANAGER->get_node_id());
            log.__set_log_content((nullptr == container_resp) ? log_content : std::move(format_logs(container_resp->log_content, req_content->body.number_of_lines)));
            if (GET_LOG_HEAD == req_content->body.head_or_tail)
            {
                log.log_content = log.log_content.substr(0, MAX_LOG_CONTENT_SIZE);
            }
            else
            {
                size_t log_lenth = log.log_content.length();
                if (log_lenth > MAX_LOG_CONTENT_SIZE)
                {
                    log.log_content = log.log_content.substr(log_lenth - MAX_LOG_CONTENT_SIZE, MAX_LOG_CONTENT_SIZE);
                }

            }

            rsp_content->body.__set_log(log);

            //resp msg
            std::shared_ptr<message> resp_msg = std::make_shared<message>();
            resp_msg->set_name(LOGS_RESP);
            resp_msg->set_content(rsp_content);
            //CONNECTION_MANAGER->broadcast_message(resp_msg);
            CONNECTION_MANAGER->send_resp_message(resp_msg);

            return E_SUCCESS;
        }

        std::string ai_power_provider_service::format_logs(const std::string  &raw_logs, uint16_t max_lines)
        {
            //docker logs has special format with each line of log:
            // 0x01 0x00  0x00 0x00 0x00 0x00 0x00 0x38
            //we should remove it
            //and ends with 0x30 0x0d 0x0a 
            max_lines = (max_lines==0) ? MAX_NUMBER_OF_LINES : max_lines;
            size_t size = raw_logs.size();
            vector<unsigned char> log_vector(size);

            int push_char_count = 0;
            const char *p = raw_logs.c_str();

            uint16_t line_count = 1;

            for (size_t i = 0; i < size; )
            {
                //0x30 0x0d 0x0a 
                if ((i + 2 < size)
                    && (0x30 == *p)
                    && (0x0d == *(p + 1))
                    && (0x0a == *(p + 2)))
                {
                    break;
                }

                if (max_lines != 0 && line_count > max_lines)
                {
                    break;
                }

                //skip: 0x01 0x00  0x00 0x00 0x00 0x00 0x00 0x38
                if ((i + 7 < size)
                    && ((0x01 == *p) || (0x02 == *p))
                    && (0x00 == *(p + 1))
                    && (0x00 == *(p + 2))
                    && (0x00 == *(p + 3))
                    && (0x00 == *(p + 4))
                    && (0x00 == *(p + 5)))
                {
                    i += 8;
                    p += 8;
                    continue;
                }

                log_vector[push_char_count] = *p++;

                if (log_vector[push_char_count] == '\n')
                {
                    line_count++;
                }

                ++push_char_count;
                i++;
            }

            std::string formatted_str;
            formatted_str.reserve(push_char_count);

            int i = 0;
            while (i < push_char_count)
            {
                formatted_str += log_vector[i++];
            }

            return formatted_str;
        }
        
        int32_t ai_power_provider_service::auth_task(std::shared_ptr<ai_training_task> task)
        {
            if (false ==task_need_auth(task))
            {
                return E_SUCCESS;
            }
            
            LOG_INFO << "auth task:" << task->task_id;

            if (0 == task->start_time)
            {
                task->__set_start_time(time_util::get_time_stamp_ms());
            }

            task->__set_end_time(time_util::get_time_stamp_ms());

            if (nullptr == m_bill_client)
            {
                LOG_WARNING << "bill system is not config.";
                return E_SUCCESS;
            }

            std::shared_ptr<auth_task_req> task_req = create_auth_task_req(task);
            if (nullptr == task_req)
            {
                return E_SUCCESS;
            }

            if (task_req->sign.empty())
            {
                LOG_DEBUG << "sign error";
                return E_DEFAULT;
            }
            //m_task_timer_map.clear();
            m_auth_time_interval =  DEFAULT_AUTH_REPORT_CYTLE * 60 * 1000;
            if (m_bill_client != nullptr)
            {
                std::shared_ptr<auth_task_resp> resp = m_bill_client->post_auth_task(task_req);
                if (nullptr == resp)
                {
                    LOG_WARNING << "bill system can not arrive." << " Next auth time:" <<  DEFAULT_AUTH_REPORT_CYTLE << "m";
                    return E_SUCCESS;
                }

                if (AUTH_SUCCESS == resp->status  && resp->contract_state == "Active" 
                    && (task->status < task_stopped))
                {
                    LOG_INFO << "auth success " << " next auth time:" << resp->report_cycle << "m";
                    //m_task_timer_map[task->task_id] = resp->report_cycle * 60 * 1000;
                    m_auth_time_interval = resp->report_cycle * 60 * 1000;
                    return E_SUCCESS;
                }

                if (AUTH_SUCCESS == resp->status  && resp->contract_state == "Active" )
                {
                    return E_SUCCESS;
                }
                
                //if status=-1, means the rsp message is not real auth_resp
                if (AUTH_NET_ERROR == resp->status)
                {
                    LOG_WARNING << "bill system can not arrive." << " Next auth time:" <<  DEFAULT_AUTH_REPORT_CYTLE << "m";
                    return E_SUCCESS;
                }

                LOG_ERROR << "auth failed " << " auth_status:" << resp->status << " contract_state:" << resp->contract_state;

            }
            else
            {
                return E_SUCCESS;
            }

            
            return E_DEFAULT;
        }

        std::shared_ptr<auth_task_req> ai_power_provider_service::create_auth_task_req(std::shared_ptr<ai_training_task> task)
        {
            std::shared_ptr<auth_task_req> req = std::make_shared<auth_task_req>();
            req->ai_user_node_id = task->ai_user_node_id;
            req->mining_node_id = CONF_MANAGER->get_node_id();
            req->task_id = task->task_id;

            req->start_time = boost::str(boost::format("%d") % task->start_time);
            int8_t status = task->status;
            if (task_queueing == status || task_pulling_image == status)
            {
                status = task_running;
            }

            if (status >= task_abnormally_closed)
            {
                status = task_abnormally_closed;
            }

            req->task_state = to_training_task_status_string(status);
            req->sign_type = ECDSA;

            req->end_time = boost::str(boost::format("%d") % task->end_time);
            std::string message = req->mining_node_id + req->ai_user_node_id + req->task_id + req->start_time + req->task_state + req->end_time;

            LOG_DEBUG << "sign message:" << message;

            req->sign = id_generator().sign(message, CONF_MANAGER->get_node_private_key());
            return req;
        }

        int32_t ai_power_provider_service::on_training_task_timer(std::shared_ptr<core_timer> timer)
        {
            if (0 == m_queueing_tasks.size())
            {
                LOG_DEBUG << "training queuing task is empty";
                return E_SUCCESS;
            }
            //check first task in queue
            std::shared_ptr<ai_training_task> task = m_queueing_tasks.front();
            if (task_queueing == task->status)
            {  
                start_exec_training_task(task);
                return E_SUCCESS;
            }
            else if (task_pulling_image == task->status)
            {
                check_pull_image_state(task);
                return E_SUCCESS;
            }
            else if (task_running == task->status)
            {
                LOG_DEBUG << "training start check ai training task: " << task->task_id << " status: " << to_training_task_status_string(task->status);;
                return check_training_task_status(task);
            }
            else
            {
                //pop from task queue
                m_queueing_tasks.pop_front();

                LOG_ERROR << "training start exec ai training task: " << task->task_id << " invalid status: " << to_training_task_status_string(task->status);
                return E_DEFAULT;
            }

            return E_SUCCESS;
        }

        std::shared_ptr<nvidia_config> ai_power_provider_service::get_nvidia_config_from_cli()
        {
            //already have
            if (nullptr != m_nv_config)
            {
                return m_nv_config;
            }

            if (nullptr == m_nvidia_client)
            {
                LOG_ERROR << "nvidia client is nullptr";
                return nullptr;
            }

            std::shared_ptr<nvidia_config_resp> resp = m_nvidia_client->get_nvidia_config();
            if (nullptr == resp)
            {
                LOG_ERROR << "nvidia client get nvidia config error";
                return nullptr;
            }

            std::vector<std::string> vec;
            string_util::split(resp->content, " ", vec);
            if (0 == vec.size())
            {
                LOG_ERROR << "nvidia client get nvidia config split content into vector error";
                return nullptr;
            }

            m_nv_config = std::make_shared<nvidia_config>();

            //get nv gpu config
            for (auto it = vec.begin(); it != vec.end(); it++)
            {
                std::vector<std::string> v;
                std::string::size_type pos = std::string::npos;

                //--volume-driver
                pos = it->find("--volume-driver");
                if (std::string::npos != pos)
                {
                    string_util::split(*it, "=", v);
                    if (DEFAULT_SPLIT_COUNT == v.size())
                    {
                        m_nv_config->driver_name = v[1];
                        LOG_DEBUG << "--volume-driver: " << m_nv_config->driver_name;
                    }
                    continue;
                }

                //--volume
                pos = it->find("--volume");
                if (std::string::npos != pos)
                {
                    string_util::split(*it, "=", v);
                    if (DEFAULT_SPLIT_COUNT == v.size())
                    {
                        m_nv_config->volume = v[1];
                        LOG_DEBUG << "--volume: " << m_nv_config->volume;
                    }
                    continue;
                }

                //--device
                pos = it->find("--device");
                if (std::string::npos != pos)
                {
                    string_util::split(*it, "=", v);
                    if (DEFAULT_SPLIT_COUNT == v.size())
                    {
                        m_nv_config->devices.push_back(v[1]);
                        LOG_DEBUG << "--device: " << v[1];
                    }
                    continue;
                }

            }

            /*m_nv_config->driver_name = "nvidia-docker";
            m_nv_config->volume = "nvidia_driver_384.111:/usr/local/nvidia:ro";

            m_nv_config->devices.push_back("/dev/nvidiactl");
            m_nv_config->devices.push_back("/dev/nvidia-uvm");
            m_nv_config->devices.push_back("/dev/nvidia-uvm-tools");
            m_nv_config->devices.push_back("/dev/nvidia0");*/

            return m_nv_config;
        }

        std::shared_ptr<container_config> ai_power_provider_service::get_container_config(std::shared_ptr<ai_training_task> task)
        {
            if (nullptr == task)
            {
                LOG_ERROR << "ai power provider service get container config task or nv_config is nullptr";
                return nullptr;
            }

            std::shared_ptr<container_config> config = std::make_shared<container_config>();

            //exec cmd: dbc_task.sh data_dir_hash code_dir_hash ai_training_python
            //dbc_task.sh Qme2UKa6yi9obw6MUcCRbpZBUmqMnGnznti4Rnzba5BQE3 QmbA8ThUawkUNtoV7yjso6V8B1TYeCgpXDhMAfYCekTNkr ai_training.py hyperparameters
            //download file + exec training 
            std::string exec_cmd = AI_TRAINING_TASK_SCRIPT_HOME;
            exec_cmd += AI_TRAINING_TASK_SCRIPT;

            std::string start_cmd = task->entry_file + " " + task->hyper_parameters;

            config->image = task->training_engine;
            config->cmd.push_back(exec_cmd);
            config->cmd.push_back(task->data_dir);
            config->cmd.push_back(task->code_dir);
            config->cmd.push_back(start_cmd);

            config->host_config.binds.push_back(AI_TRAINING_BIND_LOCALTIME);
            config->host_config.binds.push_back(AI_TRAINING_BIND_TIMEZONE);

            config->host_config.memory = m_container_args["memory"].as<int64_t>()* m_g_bytes;
            config->host_config.memory_swap = m_container_args["memory_swap"].as<int64_t>()  * m_g_bytes;
            config->host_config.nano_cpus = (int64_t) (m_container_args["cpus"].as<double>() * m_nano_cpu);

            std::string mount_dbc_data_dir = m_container_args["host_volum_dir"].as<std::string>();

            if (!mount_dbc_data_dir.empty())
            {
                mount_dbc_data_dir = mount_dbc_data_dir + ":" + "/dbc";
                config->host_config.binds.push_back(mount_dbc_data_dir);
            }

            std::string mount_dbc_utils_dir = env_manager::get_home_path().generic_string();

            if (!mount_dbc_utils_dir.empty())
            {
                // read only
                mount_dbc_utils_dir = mount_dbc_utils_dir + "/container:" + "/home/dbc_utils:ro";
                config->host_config.binds.push_back(mount_dbc_utils_dir);
            }


            auto nv_docker_ver = get_nv_docker_version();

            if (nv_docker_ver != NVIDIA_DOCKER_UNKNOWN)
            {
                LOG_DEBUG << "get common attributes of nvidia docker";
                config->env.push_back(AI_TRAINING_NVIDIA_VISIBLE_DEVICES);
                config->env.push_back(AI_TRAINING_NVIDIA_DRIVER_CAPABILITIES);
                config->host_config.share_memory = m_container_args["shm_size"].as<int64_t>() * m_g_bytes;
                config->host_config.ulimits.push_back(container_ulimits("memlock", -1, -1));
            }

            switch (nv_docker_ver)
            {
                case NVIDIA_DOCKER_ONE:
                {
                    std::shared_ptr<nvidia_config> nv_config = get_nvidia_config_from_cli();

                    if (!nv_config) break;

                    LOG_INFO << "nvidia docker 1.x";

                    //binds
                    config->host_config.binds.push_back(nv_config->volume);

                    //devices
                    for (auto it = nv_config->devices.begin(); it != nv_config->devices.end(); it++)
                    {
                        container_device dev;

                        dev.path_on_host = *it;
                        dev.path_in_container = *it;
                        dev.cgroup_permissions = AI_TRAINING_CGROUP_PERMISSIONS;

                        config->host_config.devices.push_back(dev);
                    }

                    //VolumeDriver
                    config->host_config.volume_driver = nv_config->driver_name;

                    //Mounts
                    /*container_mount nv_mount;
                    nv_mount.type = "volume";

                    std::vector<std::string> vec;
                    string_util::split(nv_config->driver_name, ":", vec);
                    if (vec.size() > 0)
                    {
                    nv_mount.name = vec[0];
                    }
                    else
                    {
                    LOG_ERROR << "container config get mounts name from nv_config error";
                    return nullptr;
                    }

                    nv_mount.source = AI_TRAINING_MOUNTS_SOURCE;
                    nv_mount.destination = AI_TRAINING_MOUNTS_DESTINATION;
                    nv_mount.driver = nv_config->driver_name;
                    nv_mount.mode = AI_TRAINING_MOUNTS_MODE;
                    nv_mount.rw = false;
                    nv_mount.propagation = DEFAULT_STRING;

                    config->host_config.mounts.push_back(nv_mount);*/
                    break;
                }
                case NVIDIA_DOCKER_TWO:
                {
                    LOG_INFO << "nvidia docker 2.x";
                    config->host_config.runtime = RUNTIME_NVIDIA;
                    break;
                }
                default:
                {
                    LOG_INFO << "not find nvidia docker";
                }
            }

            return config;
        }

        int32_t ai_power_provider_service::start_exec_training_task(std::shared_ptr<ai_training_task> task)
        {
            assert(nullptr != task);

            //judge retry times
            if (task->error_times > AI_TRAINING_MAX_RETRY_TIMES)
            {
                task->__set_status(task_abnormally_closed);                
                auth_task(task);

                //flush to db
                write_task_to_db(task);

                //pop from task queue
                m_queueing_tasks.pop_front();

                LOG_WARNING << "ai power provider service restart container too many times and close task, " << "task id: " << task->task_id << " container id: " << task->container_id;
                return E_DEFAULT;
            }

            m_auth_time_interval = 0;            
            if (E_SUCCESS == auth_task(task))
            {                    
                LOG_DEBUG << "training start exec ai training task: " << task->task_id << " status: " << to_training_task_status_string(task->status);;
            }
            else
            {
                LOG_ERROR << "auth failed. " << " drop task:" << task->task_id;
                stop_task(task, task_overdue_closed);
                return E_SUCCESS;
            }

            int32_t check_ret = m_container_client->exist_docker_image(task->training_engine);

            if (E_SUCCESS != check_ret)
            {
                if (E_IMAGE_NOT_FOUND == check_ret)
                {
                    pull_image(task);
                    return E_SUCCESS;
                }
                else
                {
                    LOG_ERROR << "check docker image error. image:" << task->training_engine;
                    stop_task(task, task_abnormally_closed);
                    auth_task(task);
                    return E_DEFAULT;
                }
            }

            //first time or contain id empty -> create container
            if (0 == task->error_times || task->container_id.empty())
            {
                //container config
                std::shared_ptr<container_config> config = get_container_config(task);
                if (nullptr == config)
                {
                    LOG_ERROR << "ai power provider service get container config error";
                    task->error_times++;
                    write_task_to_db(task);
                    return E_DEFAULT;
                }

                //create container                
                std::shared_ptr<container_create_resp> resp = m_container_client->create_container(config, task->task_id);
                if (nullptr == resp || resp->container_id.empty())
                {
                    task->error_times++;

                    //flush to db: update container id, check point 1
                    write_task_to_db(task);

                    LOG_ERROR << "ai power provider service create container error";
                    return E_DEFAULT;
                }
                else
                {
                    task->error_times = 0;

                    //update container id
                    task->container_id = resp->container_id;

                    //flush to db: update container id, check point 1
                    write_task_to_db(task);

                    LOG_INFO << "ai power provider service create container successfully, container id: " << task->container_id;
                }
            }

            //start container            
            int32_t ret = m_container_client->start_container(task->container_id);
            if (E_SUCCESS != ret)
            {
                task->error_times++;

                //flush to db: update status, check point 2
                write_task_to_db(task);

                LOG_ERROR << "ai power provider service start container error, task id: " << task->task_id << "  container id: " << task->container_id;
                return E_DEFAULT;
            }
            else
            {
                //update status
                task->status = task_running;

                //flush to db: update status, check point 2
                write_task_to_db(task);

                LOG_INFO << "ai power provider service start container successfully and update task status running, task id: " << task->task_id << "  container id: " << task->container_id;
                return E_SUCCESS;
            }
        }

        int32_t ai_power_provider_service::check_training_task_status(std::shared_ptr<ai_training_task> task)
        {
            //judge retry times
            if (task->error_times > AI_TRAINING_MAX_RETRY_TIMES)
            {
                task->__set_status(task_abnormally_closed);
                LOG_ERROR<<"task abnormal close, report to bill system." << " task:" << task->task_id;
                //report task state
                auth_task(task);
                //flush to db
                write_task_to_db(task);
                //pop from task queue
                m_queueing_tasks.pop_front();

                LOG_WARNING << "ai power provider service restart container too many times and close task, " << "task id: " << task->task_id << " container id: " << task->container_id;
                return E_DEFAULT;
            }

            //inspect container
            std::shared_ptr<container_inspect_response> resp = m_container_client->inspect_container(task->container_id);
            if (nullptr == resp)
            {
                LOG_ERROR << "ai power provider service check container error, container id: " << task->container_id;

                task->error_times++;
                //flush to db
                write_task_to_db(task);
                return E_DEFAULT;
            }

            if (true == resp->state.running)
            {
                LOG_DEBUG << "ai power provider service check container is running, " << "task id: " << task->task_id << " container id: " << task->container_id;

                int32_t ret =  auth_task(task);
                    
                if (E_SUCCESS != ret)
                {
                    LOG_ERROR << "auth failed. " << "drop task:" << task->task_id;
                    stop_task(task, task_overdue_closed);
                    //m_queueing_tasks.pop_front();
                    return E_SUCCESS;
                }
                    
                return E_SUCCESS;
            }
            
            else if (0 != resp->state.exit_code)
            {
                LOG_DEBUG << "ai power provider service restart container while inspect container not running, " << "task id: " << task->task_id << " container id: " << task->container_id << " exit_code" << resp->state.exit_code;

                task->__set_status(task_abnormally_closed);
                task->error_times++;
                LOG_ERROR<<"task abnormal close, report to bill system." << " task:" << task->task_id;
                auth_task(task);
                write_task_to_db(task);
                m_queueing_tasks.pop_front();
                
                //restart container
                //start_exec_training_task(task);
                return E_SUCCESS;
            }
            else
            {
                task->__set_status(task_succefully_closed);
                LOG_INFO << "ai power provider service inspect container closed, " << "task id: " << task->task_id << " container id: " << task->container_id;
                LOG_ERROR<<"task success close, report to bill system." << " task:" << task->task_id;

                int32_t ret = auth_task(task);
                //flush to db
                write_task_to_db(task);
                //pop from task queue
                m_queueing_tasks.pop_front();
                                
                if (ret != E_SUCCESS)
                {
                    LOG_DEBUG << "auth error";
                }
                return E_SUCCESS;
            }
        }

        int32_t ai_power_provider_service::write_task_to_db(std::shared_ptr<ai_training_task> task)
        {
            assert(nullptr != m_prov_training_task_db && nullptr != task);

            //serialization
            std::shared_ptr<byte_buf> out_buf(new byte_buf);
            binary_protocol proto(out_buf.get());
            task->write(&proto);

            //flush to db
            leveldb::WriteOptions write_options;
            write_options.sync = true;

            leveldb::Slice slice(out_buf->get_read_ptr(), out_buf->get_valid_read_len());
            m_prov_training_task_db->Put(write_options, task->task_id, slice);

            return E_SUCCESS;
        }

        int32_t ai_power_provider_service::load_task_from_db()
        {
            try
            {
                std::shared_ptr<ai_training_task> task;

                //iterate task in db
                std::unique_ptr<leveldb::Iterator> it;
                it.reset(m_prov_training_task_db->NewIterator(leveldb::ReadOptions()));
                for (it->SeekToFirst(); it->Valid(); it->Next())
                {
                    task = std::make_shared<ai_training_task>();

                    //deserialization
                    std::shared_ptr<byte_buf> task_buf(new byte_buf);
                    task_buf->write_to_byte_buf(it->value().data(), (uint32_t)it->value().size());            //may exception
                    binary_protocol proto(task_buf.get());
                    task->read(&proto);             //may exception

                    if (0 != m_training_tasks.count(task->task_id))
                    {
                        LOG_ERROR << "ai power provider service training task duplicated: " << task->task_id;
                        continue;
                    }

                    //insert training task
                    m_training_tasks.insert({ task->task_id, task });
                    LOG_DEBUG << "ai power provider service insert ai training task to task map, task id: " << task->task_id << " container_id: " << task->container_id << " task status: " << task->status;

                    //go next
                    if (task_running == task->status || task_queueing == task->status || task_pulling_image == task->status)
                    {
                        //add to queue
                        m_queueing_tasks.push_back(task);
                        LOG_DEBUG << "ai power provider service insert ai training task to task queue, task id: " << task->task_id << " container_id: " << task->container_id << " task status: " << task->status;
                    }
                }

                //sort by received_time_stamp
                m_queueing_tasks.sort(task_time_stamp_comparator());
            }
            catch (...)
            {
                LOG_ERROR << "ai power provider service load task from db exception";
                return E_DEFAULT;
            }

            return E_SUCCESS;
        }


        int32_t ai_power_provider_service::on_get_task_queue_size_req(std::shared_ptr<message> &msg)
        {
            LOG_DEBUG << "on_get_task_queue_size_req";

            auto resp = std::make_shared<service::get_task_queue_size_resp_msg>();

            resp->set(m_queueing_tasks.size());

            auto resp_msg = std::dynamic_pointer_cast<message>(resp);

            TOPIC_MANAGER->publish<int32_t>(typeid(service::get_task_queue_size_resp_msg).name(), resp_msg);

            return E_SUCCESS;
        }

        int32_t ai_power_provider_service::check_sign(const std::string message, const std::string &sign, const std::string &origin_id, const std::string & sign_algo)
        {
            if (sign_algo != ECDSA)
            {
                LOG_ERROR << "sign_algorithm error.";
                return E_DEFAULT;
            }
            if (origin_id.empty() || sign.empty())
            {
                LOG_ERROR << "sign error.";
                return E_DEFAULT;
            }

            std::string derive_node_id;
            id_generator().derive_node_id_by_sign(message, sign, derive_node_id);
            if (derive_node_id != origin_id)
            {
                LOG_ERROR << "sign check error";
                return E_DEFAULT;
            }

            return E_SUCCESS;
        }

        int32_t ai_power_provider_service::stop_task(std::shared_ptr<ai_training_task> task, training_task_status status)
        {
            m_queueing_tasks.remove(task);
            
            //task have been stopped
            if (task->status >= task_stopped)
            {
                return E_SUCCESS;
            }
            
            if (task_running == task->status)
            {
                int32_t ret = m_container_client->stop_container(task->container_id);
                if (E_SUCCESS != ret)
                {
                    LOG_ERROR << "ai power provider service stop container error, container id: " << task->container_id;
                }
            }

            if (task_pulling_image == task->status)
            {
                LOG_INFO <<"task is pulling image. terminate pull " << task->training_engine;
                end_pull(task);
            }

            task->__set_end_time(time_util::get_time_stamp_ms());
            task->__set_status(status);
            write_task_to_db(task);
                        
            return E_SUCCESS;
        }

        int32_t ai_power_provider_service::pull_image(std::shared_ptr<ai_training_task> task)
        {
            //if do not allow auto pull image, then pop task directly
            if (! m_auto_pull_image)
            {
                LOG_ERROR << "docker image do not exist, should config auto_pull_image=true, or pull image manually. task req image:" << task->training_engine;
                stop_task(task, task_noimage_closed);
                auth_task(task);
                return E_DEFAULT;
            }

            //create pull process
            if (m_docker_root_dir.empty())
            {
                std::shared_ptr<docker_info> docker_info_ptr = m_container_client->get_docker_info();
                if (!docker_info_ptr)
                {
                    LOG_ERROR << "docker get docker info faild. drop task" << task->task_id;
                    stop_task(task, task_abnormally_closed);
                    auth_task(task);
                    return E_DEFAULT;
                }
                m_docker_root_dir = docker_info_ptr->root_dir;
            }

            //if the partion do not have enough space, stop task
            if (get_disk_free(m_docker_root_dir) < m_min_disk_free)
            {
                LOG_ERROR << "docker get docker info faild. the disk have no enough space. the free space: " << get_disk_free(m_docker_root_dir) << "MB";
                stop_task(task, task_nospace_closed);
                auth_task(task);
                return E_DEFAULT;
            }

            if (nullptr == m_pull_image_mng)
            {
                m_pull_image_mng = std::make_shared<image_manager>();
            }

            if (PULLING == m_pull_image_mng->check_pull_state())
            {
                if (m_pull_image_mng->get_pull_image_name() == task->training_engine )
                {
                    return E_SUCCESS;
                }
                else
                {
                    //if pulling image is other task triggered, then terminate the pulling.
                    m_pull_image_mng->terminate_pull();
                }
            }

            if (!m_pull_image_mng || m_pull_image_mng->start_pull(task->training_engine) != E_SUCCESS)
            {
                LOG_DEBUG << "start docker pulling image failed. status" << to_training_task_status_string(task->status)
                          << " task id:" << task->task_id << " image engine:" << task->training_engine;
                stop_task(task, task_noimage_closed);
                auth_task(task);
                return E_DEFAULT;
            }

            //cache the time
            m_pull_image_mng->set_start_time(time_util::get_time_stamp_ms());

            if (task_queueing == task->status)
            {
                task->__set_status(task_pulling_image);
                LOG_DEBUG << "docker pulling image. change status to " << to_training_task_status_string(task->status) 
                          << " task id:" << task->task_id << " image engine:" << task->training_engine;
                write_task_to_db(task);
            }

            return E_PULLING_IMAGE;
        }

        int32_t ai_power_provider_service::check_pull_image(std::shared_ptr<ai_training_task> task)
        {
            if (nullptr == m_pull_image_mng)
            {
                return E_DEFAULT;
            }

            if (m_pull_image_mng->get_pull_image_name() != task->training_engine)
            {
                return E_DEFAULT;
            }

            if (PULLING == m_pull_image_mng->check_pull_state())
            {
                LOG_DEBUG << "pulling: docker pull " << task->training_engine;
                return E_SUCCESS;
            }
            else
            {
                if (m_container_client->exist_docker_image(task->training_engine) != E_SUCCESS)
                {
                    LOG_DEBUG << "docker pull image fail. engine: " << task->training_engine;
                    LOG_WARNING << "ai power provider service pull image failed";
                    return E_DEFAULT;
                }
                LOG_DEBUG << "docker pull image success. engine: " << task->training_engine;
                task->__set_status(task_queueing);
                LOG_DEBUG << "change task status to " << to_training_task_status_string(task->status) << " task id:" << task->task_id;
            }
            
            return E_SUCCESS;
        }

        //when pull is over, need detach the engine from the map
        int32_t ai_power_provider_service::end_pull(std::shared_ptr<ai_training_task> task)
        {
            if (! m_pull_image_mng)
            {
                return E_SUCCESS;
            }

            if (task->training_engine != m_pull_image_mng->get_pull_image_name())
            {
                LOG_ERROR << "pull image is not " << task->training_engine;
                return E_SUCCESS;
            }

            if (nullptr != m_pull_image_mng)
            {
                LOG_INFO << "terminate pull " << task->training_engine;
                m_pull_image_mng->terminate_pull();
            }
            
            return E_SUCCESS;
        }

        //check pull image state
        int32_t ai_power_provider_service::check_pull_image_state(std::shared_ptr<ai_training_task> task)
        {
            if (task->status != task_pulling_image)
            {
                LOG_ERROR << "task state is not task_pulling_image. task id:" << task->task_id;
                return E_DEFAULT;
            }
            
            if (m_pull_image_mng != nullptr && m_pull_image_mng->get_pull_image_name() == task->training_engine)
            {
                if ((time_util::get_time_stamp_ms() - m_pull_image_mng->get_start_time()) >= AI_PULLING_IMAGE_TIMER_INTERVAL)
                {
                    //pull error
                    LOG_ERROR << "pull image too long." << " engine image:" << task->training_engine;
                    stop_task(task, task_noimage_closed);
                    
                }
                else
                {
                    if (check_pull_image(task) != E_SUCCESS)
                    {
                        stop_task(task, task_noimage_closed);
                    }
                }

                int32_t ret = auth_task(task);
                
                if (E_SUCCESS != ret)
                {
                    LOG_ERROR << "auth failed. " << "drop task:" << task->task_id;
                    stop_task(task, task_overdue_closed);
                    return E_SUCCESS;
                }
            }
            else
            {
               //task state is pulling, but image is not pulling. so dbc may be restarted. 
                task->__set_status(task_queueing);
            }

            return E_SUCCESS;
        }
        
        bool ai_power_provider_service::task_need_auth(std::shared_ptr<ai_training_task> task)
        {
            if (task->status < task_stopped)
            {
                ////dbc maybe restart and the task is  running state, at this time, dbc should send auth req
                if (0 == m_auth_time_interval)
                {
                    return true;
                }

                int64_t interval = time_util::get_time_stamp_ms() - task->end_time;
                if (interval >= m_auth_time_interval)
                {
                    return true;
                }

                return false;
            }

            //final state should report
            if (task->status >= task_stopped)
            {
                return true;
            }
            
            return false;
        }

        /**
         *
         * @return nvidia docker version in the host
         */
        nvidia_docker_version ai_power_provider_service::get_nv_docker_version()
        {
            if (m_nv_docker_version != NVIDIA_DOCKER_UNKNOWN)
            {
                return m_nv_docker_version;
            }

            if (get_nvidia_config_from_cli() != nullptr)
            {
                // nvidia-docker 1.x added a plug-in on runc.
                m_nv_docker_version = NVIDIA_DOCKER_ONE;
            }
            else
            {
                // nvidia-docker 2.x add a new docker runtime named nvidia. Check if nvidia runtime exists.
                auto docker_info_ptr = m_container_client->get_docker_info();
                if (docker_info_ptr && docker_info_ptr->runtimes.count(RUNTIME_NVIDIA))
                {
                    m_nv_docker_version = NVIDIA_DOCKER_TWO;
                }
            }

            return m_nv_docker_version;
        }
    }

}
