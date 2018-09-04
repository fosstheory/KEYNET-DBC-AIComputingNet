/*********************************************************************************
*  Copyright (c) 2017-2018 DeepBrainChain core team
*  Distributed under the MIT software license, see the accompanying
*  file COPYING or http://www.opensource.org/licenses/mit-license.php
* file name         :   matrix_socket_channel_handler.cpp
* description     :   matrix socket channel handler
* date                  :   2018.01.28
* author             :   Bruce Feng
**********************************************************************************/
#include "matrix_socket_channel_handler.h"
#include "p2p_net_service.h"
#include "server.h"
#include "tcp_socket_channel.h"
#include "service_proto_filter.h"


namespace matrix
{
    namespace service_core
    {

        matrix_socket_channel_handler::matrix_socket_channel_handler(std::shared_ptr<channel> ch)
            : m_stopped(false)
            , m_coder(std::make_shared<matrix_coder>())
            , m_decoder(std::make_shared<matrix_coder>())
            , m_channel(ch)
            , m_shake_hand_timer(*(ch->get_io_service()))
            , m_has_message(false)
            , m_login_success(false)
            , m_sid(ch->id())
        {
            if (CONF_MANAGER->get_max_recv_speed() > 0)
            {
                int32_t recv_speed = CONF_MANAGER->get_max_recv_speed();
                int32_t cycle = 1;
                int32_t slice = 2;
                m_f_ctl = std::make_shared<flow_ctrl>(recv_speed, cycle, slice, ch->get_io_service());
                if (m_f_ctl != nullptr)
                {
                    m_f_ctl->start();
                }
                
            }
        }

        matrix_socket_channel_handler::~matrix_socket_channel_handler()
        {
            LOG_DEBUG << "socket channel handler destroyed, " << m_sid.to_string();
        }

        int32_t matrix_socket_channel_handler::stop()
        {
            m_stopped = true;
            stop_shake_hand_timer();
            if (CONF_MANAGER->get_max_recv_speed() > 0 && m_f_ctl != nullptr)
            {
                m_f_ctl->stop();
            }

            return E_SUCCESS;
        }

        int32_t matrix_socket_channel_handler::on_read(channel_handler_context &ctx, byte_buf &in)
        {
            if (in.get_valid_read_len() > 0)
            {
                decode_status recv_status = m_decoder->recv_message(in);

                if (recv_status != DECODE_SUCCESS)
                {
                    LOG_ERROR << "recv error msg." << m_sid.to_string();
                    return recv_status;
                }

                if (recv_status != DECODE_SUCCESS )
                {
                    return recv_status;
                }
                
                while (m_decoder->has_complete_message())
                {
                    shared_ptr<message> msg = std::make_shared<message>();
                    decode_status status = m_decoder->decode(ctx, msg);

                    //decode success
                    if (DECODE_SUCCESS == status)
                    {

                        LOG_DEBUG << "socket channel handler recv msg: " << msg->get_name() << m_sid.to_string();

                        //modify by regulus: fix ver_req duplication error. 
                        //callback
                        msg->header.src_sid = m_sid;

                        int32_t iprocRet = on_after_msg_received(*msg);
                        if (iprocRet != E_SUCCESS)
                        {
                            return iprocRet;
                        }

                        //send to bus
                        if (msg->get_name() != SHAKE_HAND_REQ
                            && msg->get_name() != SHAKE_HAND_RESP)
                        {
                            if (nullptr == msg || nullptr == msg->content)
                            {
                                LOG_ERROR << "decode error, msg is null" << m_sid.to_string();
                                return DECODE_ERROR;
                            }
                            const std::string & nonce = msg->content->header.__isset.nonce ? msg->content->header.nonce : DEFAULT_STRING;
                            //check msg duplicated
                            if (!service_proto_filter::get_mutable_instance().check_dup(nonce))
                            {
                                if (m_f_ctl != nullptr && m_f_ctl->over_speed(1))
                                {
                                    LOG_INFO << "over_speed";
                                    return E_SUCCESS;
                                }
                                TOPIC_MANAGER->publish<int32_t>(msg->get_name(), msg);
                                //LOG_INFO << "m_count "<< m_count << endl;
                                LOG_DEBUG << "matrix socket channel handler received msg: " << msg->get_name() << ", nonce: " << nonce << m_sid.to_string();
                            }
                            else
                            {
                                LOG_DEBUG << "matrix socket channel handler received duplicated msg: " << msg->get_name() << ", nonce: " << nonce << m_sid.to_string();
                            }
                        }

                        //has message
                        set_has_message(*msg);
                    }
                    //not enough, return and next time
                    else if (DECODE_UNKNOWN_MSG == status)
                    {
                        LOG_DEBUG << "unknow message: " << msg->get_name();
                    }
                    else if (DECODE_NEED_MORE_DATA == status)
                    {
                        LOG_DEBUG << "package contiue to receive";
                    }
                    //decode error
                    else
                    {
                        LOG_ERROR << "matrix socket channel handler on read error and call socket channel on_error, " << m_sid.to_string();
                        return status;
                    }
                }

            }
            return E_SUCCESS;
        }

        int32_t matrix_socket_channel_handler::on_write(channel_handler_context &ctx, message &msg, byte_buf &buf)
        {

            LOG_DEBUG << "socket channel handler send msg: " << msg.get_name() << m_sid.to_string();

            encode_status status = m_coder->encode(ctx, msg, buf);
            if (ENCODE_SUCCESS == status)
            {
                if (msg.get_name() != SHAKE_HAND_REQ
                    && msg.get_name() != SHAKE_HAND_RESP)
                {
                    const std::string & nonce = (msg.content)->header.__isset.nonce ? (msg.content)->header.nonce : DEFAULT_STRING;
                    //insert nonce to avoid receive msg sent by itself
                    service_proto_filter::get_mutable_instance().insert_nonce(nonce);

                    LOG_DEBUG << m_sid.to_string() << " matrix socket channel handler send msg: " << msg.get_name() << ", nonce: " << nonce << " buf message:" << buf.to_string();
                }

                //has message
                set_has_message(msg);

                return E_SUCCESS;
            }
            else if (BUFFER_IS_NOT_ENOUGH_TO_ENCODE == status)
            {
                //this should be found in coding phase
                LOG_ERROR << "matrix_socket_channel_handler encode error, buffer is not enough to encode: " << msg.get_name();
                return E_DEFAULT;
            }
            else
            {
                LOG_ERROR << "matrix_socket_channel_handler encode error: " << msg.get_name();
                return E_DEFAULT;
            }
        }

        int32_t matrix_socket_channel_handler::on_error()
        {
            if (auto ch = m_channel.lock())
            {
                ch->on_error();
            }
            return E_SUCCESS;
        }

        void matrix_socket_channel_handler::start_shake_hand_timer()
        {
            //m_shake_hand_timer.expires_from_now(std::chrono::seconds(SHAKE_HAND_INTERVAL));
            //m_shake_hand_timer.async_wait(m_shake_hand_timer_handler);
            start_shake_hand_timer_ext();
            reset_has_message();
        }

        void matrix_socket_channel_handler::stop_shake_hand_timer()
        {
            boost::system::error_code error;

            m_shake_hand_timer.cancel(error);
            if (error)
            {
                LOG_ERROR << "matrix socket channel handler cancel timer error: " << error;
            }

            //modify by regulus:fix can't free resource when client disconnect 
            //m_shake_hand_timer_handler = nullptr;

        }

        void matrix_socket_channel_handler::set_has_message(message &msg)
        {
            if (SHAKE_HAND_REQ == msg.get_name()
                || SHAKE_HAND_RESP == msg.get_name()
                || VER_REQ == msg.get_name()
                || VER_RESP == msg.get_name())
            {
                return;
            }

            m_has_message = true;
        }
    }

}