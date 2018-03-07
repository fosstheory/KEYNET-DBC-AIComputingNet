/*********************************************************************************
*  Copyright (c) 2017-2018 DeepBrainChain core team
*  Distributed under the MIT software license, see the accompanying
*  file COPYING or http://www.opensource.org/licenses/mit-license.php
* file name        ��matrix_server_socket_channel_handler.h
* description    ��dbc socket channel handler for dbc network protocol layer
* date                  : 2018.02.27
* author            ��Bruce Feng
**********************************************************************************/
#pragma once


#include<chrono>
#include "matrix_socket_channel_handler.h"


#define LOST_SHAKE_HAND_COUNT_MAX               10                  //max lost 10 shake hand req
#define DEFAULT_WAIT_VER_REQ_INTERVAL           5                   //wait VER_REQ interval while socket accepted


using namespace boost::asio;

namespace matrix
{
    namespace service_core
    {

        class matrix_server_socket_channel_handler : public matrix_socket_channel_handler
        {
        public:

            matrix_server_socket_channel_handler(channel *ch);

            virtual ~matrix_server_socket_channel_handler() = default;

            virtual int32_t stop();

            virtual int32_t on_after_msg_received(message &msg);

            virtual int32_t on_after_msg_sent(message &msg);

            virtual int32_t on_before_msg_receive();

            static socket_channel_handler * create(channel *ch) { return new matrix_server_socket_channel_handler(ch); }

        protected:

            //I think don't worry about multi thread visit
            void reset_lost_shake_hand_count() { m_lost_shake_hand_count = 0; }         

            void send_shake_hand_resp();

            virtual void start_wait_ver_req_timer();

            virtual void stop_wait_ver_req_timer();

        protected:

            uint16_t m_lost_shake_hand_count;

            uint16_t m_lost_shake_hand_count_max;

            bool m_login_success;

            steady_timer m_wait_ver_req_timer;

            std::function<timer_handler_type> m_wait_ver_req_timer_handler;

        };

    }

}


