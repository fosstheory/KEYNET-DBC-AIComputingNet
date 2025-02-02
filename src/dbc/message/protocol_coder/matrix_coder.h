#ifndef DBC_MATRIX_CODER_H
#define DBC_MATRIX_CODER_H

#include <unordered_map>
#include "message_to_byte_encoder.h"
#include "length_field_frame_decoder.h"
#include "network/protocol/protocol.h"
#include "../matrix_types.h"
#include "../lan_types.h"
#include "../message_id.h"

#define DEFAULT_DECODE_HEADER_LEN               24
#define MATRIX_MSG_MIN_READ_LENGTH              8
#define MAX_MATRIX_MSG_LEN                      (4 * 1024 * 1024)   //max 4M bytes

#define DECLARE_DECODE_INVOKER             decode_invoker_type invoker;
#define BIND_DECODE_INVOKER(MSG_TYPE)   \
    invoker = std::bind(&matrix_coder::decode_invoke<dbc::MSG_TYPE>, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3); \
    m_binary_decode_invokers[#MSG_TYPE] = invoker;

#define DECLARE_ENCODE_INVOKER             encode_invoker_type invoker;
#define BIND_ENCODE_INVOKER(MSG_TYPE)   \
    invoker = std::bind(&matrix_coder::encode_invoke<dbc::MSG_TYPE>, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4); \
    m_binary_encode_invokers[#MSG_TYPE] = invoker;

//do not use virtual function
class matrix_packet_header {
public:
    matrix_packet_header();

    void read(byte_buf &in);

    bool update(byte_buf &out);

    void write(byte_buf &out);

    int32_t packet_len;
    int32_t packet_type;
};

class matrix_coder : public message_to_byte_encoder, public length_field_frame_decoder {
    using decode_invoker_type = typename std::function<void(std::shared_ptr<network::message> &,
            network::base_header &, std::shared_ptr<network::protocol> &)>;
    using encode_invoker_type = typename std::function<void(network::channel_handler_context &,
                                                            std::shared_ptr<network::protocol> &,
                                                            network::message &,
                                                            byte_buf &)>;
public:
    matrix_coder();

    virtual ~matrix_coder() = default;

    encode_status encode(network::channel_handler_context &ctx, network::message &msg, byte_buf &out) override;

    decode_status recv_message(byte_buf &in) override;

    decode_status decode_frame(network::channel_handler_context &ctx, byte_buf &in, std::shared_ptr<network::message> &msg) override;

protected:
    void init_decode_proto();

    void init_decode_invoker();

    void init_encode_invoker();

    std::shared_ptr<network::protocol> get_protocol(int32_t type);

    decode_status
    decode_service_frame(network::channel_handler_context &ctx, byte_buf &in, 
        std::shared_ptr<network::message> &msg, std::shared_ptr<network::protocol> proto);

    decode_status decode_fast_forward(network::channel_handler_context &ctx, byte_buf &in, 
        std::shared_ptr<network::message> &msg, network::base_header &header, 
        std::shared_ptr<network::protocol> proto);

    template<typename msg_type>
    void
    decode_invoke(std::shared_ptr<network::message> &msg, network::base_header &msg_header, 
        std::shared_ptr<network::protocol> &proto);

    template<typename msg_type>
    void
    encode_invoke(network::channel_handler_context &ctx, std::shared_ptr<network::protocol> &proto, 
        network::message &msg, byte_buf &out);

    bool get_compress_enabled(network::channel_handler_context &ctx);

    int get_thrift_proto(network::channel_handler_context &ctx);

    std::string get_local_node_id(network::channel_handler_context &ctx);

protected:
    std::unordered_map<std::string, decode_invoker_type> m_binary_decode_invokers;

    std::unordered_map<std::string, encode_invoker_type> m_binary_encode_invokers;

    std::unordered_map<int32_t, std::shared_ptr<network::protocol>> m_decode_protos;
};

#endif
