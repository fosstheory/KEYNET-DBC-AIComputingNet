/*********************************************************************************
*  Copyright (c) 2017-2018 DeepBrainChain core team
*  Distributed under the MIT software license, see the accompanying
*  file COPYING or http://www.opensource.org/licenses/mit-license.php
* file name        ��matrix_client_socket_channel_handler.cpp
* description    ��dbc socket channel handler for dbc network protocol layer
* date                  : 2018.02.27
* author            ��Bruce Feng
**********************************************************************************/
#include "matrix_server_socket_channel_handler.h"

namespace matrix
{
    namespace service_core
    {

        matrix_server_socket_channel_handler::matrix_server_socket_channel_handler(channel *ch)
            : matrix_socket_channel_handler(ch)
            , m_lost_shake_hand_count(0)
            , m_lost_shake_hand_count_max(LOST_SHAKE_HAND_COUNT_MAX)
            , m_wait_ver_req_timer(*(ch->get_io_service()))
        {
            m_shake_hand_timer_handler =
                [&](const boost::system::error_code & error)
            {
                if (error)
                {
                    //aborted, maybe cancel triggered
                    if (boost::asio::error::operation_aborted == error.value())
                    {
                        LOG_DEBUG << "matrix client socket channel handler timer aborted." << m_channel->id().to_string();
                        return;
                    }

                    LOG_ERROR << "matrix server socket channel handler timer error: " << error.value() << " " << error.message() << m_channel->id().to_string();
                    m_channel->on_error();
                    return;
                }

                //lost shake hand too many times
                if (++m_lost_shake_hand_count >= m_lost_shake_hand_count_max)
                {
                    LOG_ERROR << "matrix server socket channel handler lost shake hand count error timers: " << m_lost_shake_hand_count;
                    m_channel->on_error();
                    return;
                }

                //async wait
                m_shake_hand_timer.expires_from_now(std::chrono::seconds(SHAKE_HAND_INTERVAL));
                m_shake_hand_timer.async_wait(m_shake_hand_timer_handler);
            };

            m_wait_ver_req_timer_handler =
                [&](const boost::system::error_code & error)
            {
                if (error)
                {
                    //aborted, maybe cancel triggered
                    if (boost::asio::error::operation_aborted == error.value())
                    {
                        LOG_DEBUG << "matrix server socket channel handler wait ver req timer aborted." << m_channel->id().to_string();
                        return;
                    }

                    LOG_ERROR << "matrix server socket channel handler wait ver req timer error: " << error.value() << " " << error.message() << m_channel->id().to_string();
                    m_channel->on_error();
                    return;
                }

                //time out and disconnect tcp socket
                if (m_wait_ver_req_timer.expires_at() < std::chrono::steady_clock::now())
                {
                    LOG_ERROR << "matrix server socket channel handler connect successfully but no message received, " << m_channel->id().to_string();

                    stop_wait_ver_req_timer();
                    m_channel->on_error();
                    return;
                }

                //restart timer for not time out
                start_wait_ver_req_timer();
            };
        }

        int32_t matrix_server_socket_channel_handler::stop()
        {
            //release timer
            stop_wait_ver_req_timer();
            stop_shake_hand_timer();

            return E_SUCCESS;
        }

        int32_t matrix_server_socket_channel_handler::on_after_msg_received(message &msg)
        {
            if (false == m_login_success)
            {
                if (VER_REQ != msg.get_name())
                {
                    LOG_ERROR << "matrix server socket channel received error message: " << msg.get_name() << " , while not login success" << msg.header.src_sid.to_string();
                    m_channel->on_error();
                    return E_SUCCESS;
                }
            }

            if (VER_REQ == msg.get_name())
            {
                if (true == m_login_success)
                {
                    LOG_ERROR << "matrix server socket channel received duplicated VER_REQ message" << msg.header.src_sid.to_string();
                    m_channel->on_error();
                    return E_SUCCESS;
                }
                else
                {
                    stop_wait_ver_req_timer();
                    return E_SUCCESS;
                }
            }

            //send shake hand resp in handler
            if (SHAKE_HAND_REQ == msg.get_name())
            {
                send_shake_hand_resp();
            }

            m_lost_shake_hand_count = 0;
            return E_SUCCESS;
        }

        int32_t matrix_server_socket_channel_handler::on_after_msg_sent(message &msg)
        {
            if (VER_RESP == msg.get_name())
            {
                m_login_success = true;
                start_shake_hand_timer();
                LOG_DEBUG << "matrix server socket channel handler start shake hand timer, socket number: " << m_channel->id().get_id();

                //public new node
                using msg_new_node = matrix::service_core::msg_new_peer_node;
                shared_ptr<msg_new_node> msg_node = std::make_shared<msg_new_node>();
                msg_node->sid = m_channel->id();
                msg_node->node_id = "node_id";//todo ...

                std::shared_ptr<matrix::core::message> msg = std::make_shared<matrix::core::message>();
                msg->set_name(P2P_NEW_PEER_NODE);
                msg->set_content(msg_node);

                TOPIC_MANAGER->publish<int32_t>(P2P_NEW_PEER_NODE, msg);

                return E_SUCCESS;
            }

            //reset
            m_lost_shake_hand_count = 0;
            return E_SUCCESS;
        }

        int32_t matrix_server_socket_channel_handler::on_before_msg_receive()
        {
            start_wait_ver_req_timer();
            return E_SUCCESS;
        }

        void matrix_server_socket_channel_handler::send_shake_hand_resp()
        {
            //shake_hand_req 
            std::shared_ptr<message> resp_msg = std::make_shared<message>();
            std::shared_ptr<matrix::service_core::shake_hand_resp> req_content = std::make_shared<matrix::service_core::shake_hand_resp>();

            //header
            //req_content->header.length = 0;
            req_content->header.magic = TEST_NET;
            req_content->header.msg_name = SHAKE_HAND_RESP;
            req_content->header.check_sum = 0;
            req_content->header.session_id = 0;

            resp_msg->set_content(std::dynamic_pointer_cast<base>(req_content));
            resp_msg->set_name(SHAKE_HAND_RESP);
            m_channel->write(resp_msg);
        }

        void matrix_server_socket_channel_handler::start_wait_ver_req_timer()
        {
            m_wait_ver_req_timer.expires_from_now(std::chrono::seconds(DEFAULT_WAIT_VER_REQ_INTERVAL));
            m_wait_ver_req_timer.async_wait(m_wait_ver_req_timer_handler);

            LOG_DEBUG << "matrix server socket channel handler start wait ver req timer, " << m_channel->id().to_string();
        }

        void matrix_server_socket_channel_handler::stop_wait_ver_req_timer()
        {
            boost::system::error_code error;

            m_wait_ver_req_timer.cancel(error);
            if (error)
            {
                LOG_ERROR << "matrix server socket channel handler cancel timer error: " << error;
            }
            else
            {
                LOG_DEBUG << "matrix server socket channel handler stop wait ver req timer, " << m_channel->id().to_string();
            }
        }

    }

}