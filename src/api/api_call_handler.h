/*********************************************************************************
*  Copyright (c) 2017-2018 DeepBrainChain core team
*  Distributed under the MIT software license, see the accompanying
*  file COPYING or http://www.opensource.org/licenses/mit-license.php
* file name        ��api_call_handler.h
* description    ��api call handler for command line, json rpc
* date                  : 2018.03.25
* author            ��
**********************************************************************************/

#pragma once

#include <list>
#include <boost/program_options.hpp>
#include "service_message.h"
#include "callback_wait.h"
#include "server.h"


using namespace std;
using namespace boost::program_options;
using namespace matrix::core;


#define DEFAULT_CMD_LINE_WAIT_MILLI_SECONDS                 std::chrono::milliseconds(30000)                    //unit: ms
#define GET_TYPE_NAME(TYPE)                                                        #TYPE

#define LIST_ALL_TASKS                                                                          0
#define LIST_SPECIFIC_TASKS                                                                 1


namespace ai
{
    namespace dbc
    {

        class cmd_start_training_req : public matrix::core::base
        {
        public:
            std::string task_file_path;
        };

        class cmd_start_training_resp : public matrix::core::base
        {
        public:
            int32_t result;
            std::string result_info;

            std::string task_id;
        };

        class cmd_stop_training_req : public matrix::core::base
        {
        public:
            std::string task_id;
        };

        class cmd_stop_training_resp : public matrix::core::base
        {
        public:
            int32_t result;
            std::string result_info;
        };

        class cmd_start_multi_training_req : public matrix::core::base
        {
        public:
            std::string mulit_task_file_path;
        };

        class cmd_start_multi_training_resp : public matrix::core::base
        {
        public:
            int32_t result;
            std::string result_info;

            std::list<std::string > task_list;
        };

        class cmd_list_training_req : public matrix::core::base
        {
        public:
            int8_t list_type;                                   //0: list all tasks; 1: list specific tasks
            std::list<std::string > task_list;
        };

        class cmd_task_status
        {
        public:
            std::string task_id;
            int8_t status;
        };

        class cmd_list_training_resp : public matrix::core::base
        {
        public:
            int32_t result;
            std::string result_info;

            std::list<cmd_task_status> task_status_list;
        };

        class cmd_network_address
        {
        public:
            std::string ip;
            int16_t port;
        };

        class cmd_peer_node_info
        {
        public:
            std::string peer_node_id;
            int32_t live_time_stamp;
            cmd_network_address addr;
            std::list<std::string> service_list;
        };

        class cmd_get_peer_nodes_req : public matrix::core::base
        {

        };

        class cmd_get_peer_nodes_resp : public matrix::core::base
        {
        public:
            int32_t result;
            std::string result_info;

            std::list<cmd_peer_node_info> peer_nodes_list;
        };

        class api_call_handler
        {
        public:

            api_call_handler() : m_wait(new callback_wait<>) 
            {
                init_subscription();
            }

            ~api_call_handler() = default;

            void init_subscription();

            template<typename req_type, typename resp_type>
            std::shared_ptr<resp_type> invoke(std::shared_ptr<req_type> req)
            {
                //construct message
                std::shared_ptr<message> msg(new message);
                msg->set_name(typeid(req_type).name());
                msg->set_content(std::dynamic_pointer_cast<base>(req));

                //publish
                TOPIC_MANAGER->publish<int32_t>(msg->get_name(), msg);

                //synchronous wait for resp
                m_wait->wait_for(DEFAULT_CMD_LINE_WAIT_MILLI_SECONDS);
                if (true == m_wait->flag())
                {
                    return std::dynamic_pointer_cast<resp_type>(m_resp);
                }
                else
                {
                    LOG_DEBUG << "api_call_handler time out: " << msg->get_name();
                    return nullptr;
                }

            }

        private:

            //std::mutex m_mutex;           currently no need. 

            std::shared_ptr<message> m_resp;

            std::shared_ptr<callback_wait<>> m_wait;

        };

    }

}
