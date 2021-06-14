#include "matrix_types.h"

#include <algorithm>
#include <ostream>

#include <TToString.h>

namespace matrix {
    namespace service_core {


        empty::~empty() throw() {
        }

        std::ostream &operator<<(std::ostream &out, const empty &obj) {
            obj.printTo(out);
            return out;
        }


        uint32_t empty::read(::apache::thrift::protocol::TProtocol *iprot) {

            ::apache::thrift::protocol::TInputRecursionTracker tracker(*iprot);
            uint32_t xfer = 0;
            std::string fname;
            ::apache::thrift::protocol::TType ftype;
            int16_t fid;

            xfer += iprot->readStructBegin(fname);

            using ::apache::thrift::protocol::TProtocolException;


            while (true) {
                xfer += iprot->readFieldBegin(fname, ftype, fid);
                if (ftype == ::apache::thrift::protocol::T_STOP) {
                    break;
                }
                xfer += iprot->skip(ftype);
                xfer += iprot->readFieldEnd();
            }

            xfer += iprot->readStructEnd();

            return xfer;
        }

        uint32_t empty::write(::apache::thrift::protocol::TProtocol *oprot) const {
            uint32_t xfer = 0;
            ::apache::thrift::protocol::TOutputRecursionTracker tracker(*oprot);
            xfer += oprot->writeStructBegin("empty");

            xfer += oprot->writeFieldStop();
            xfer += oprot->writeStructEnd();
            return xfer;
        }

        void swap(empty &a, empty &b) {
            using ::std::swap;
            (void) a;
            (void) b;
        }

        empty::empty(const empty &other0) {
            (void) other0;
        }

        empty &empty::operator=(const empty &other1) {
            (void) other1;
            return *this;
        }

        void empty::printTo(std::ostream &out) const {
            using ::apache::thrift::to_string;
            out << "empty(";
            out << ")";
        }


        network_address::~network_address() throw() {
        }


        void network_address::__set_ip(const std::string &val) {
            this->ip = val;
        }

        void network_address::__set_port(const int16_t val) {
            this->port = val;
        }

        std::ostream &operator<<(std::ostream &out, const network_address &obj) {
            obj.printTo(out);
            return out;
        }


        uint32_t network_address::read(::apache::thrift::protocol::TProtocol *iprot) {

            ::apache::thrift::protocol::TInputRecursionTracker tracker(*iprot);
            uint32_t xfer = 0;
            std::string fname;
            ::apache::thrift::protocol::TType ftype;
            int16_t fid;

            xfer += iprot->readStructBegin(fname);

            using ::apache::thrift::protocol::TProtocolException;

            bool isset_ip = false;
            bool isset_port = false;

            while (true) {
                xfer += iprot->readFieldBegin(fname, ftype, fid);
                if (ftype == ::apache::thrift::protocol::T_STOP) {
                    break;
                }
                switch (fid) {
                    case 1:
                        if (ftype == ::apache::thrift::protocol::T_STRING) {
                            xfer += iprot->readString(this->ip);
                            isset_ip = true;
                        } else {
                            xfer += iprot->skip(ftype);
                        }
                        break;
                    case 2:
                        if (ftype == ::apache::thrift::protocol::T_I16) {
                            xfer += iprot->readI16(this->port);
                            isset_port = true;
                        } else {
                            xfer += iprot->skip(ftype);
                        }
                        break;
                    default:
                        xfer += iprot->skip(ftype);
                        break;
                }
                xfer += iprot->readFieldEnd();
            }

            xfer += iprot->readStructEnd();

            if (!isset_ip)
                throw TProtocolException(TProtocolException::INVALID_DATA);
            if (!isset_port)
                throw TProtocolException(TProtocolException::INVALID_DATA);
            return xfer;
        }

        uint32_t network_address::write(::apache::thrift::protocol::TProtocol *oprot) const {
            uint32_t xfer = 0;
            ::apache::thrift::protocol::TOutputRecursionTracker tracker(*oprot);
            xfer += oprot->writeStructBegin("network_address");

            xfer += oprot->writeFieldBegin("ip", ::apache::thrift::protocol::T_STRING, 1);
            xfer += oprot->writeString(this->ip);
            xfer += oprot->writeFieldEnd();

            xfer += oprot->writeFieldBegin("port", ::apache::thrift::protocol::T_I16, 2);
            xfer += oprot->writeI16(this->port);
            xfer += oprot->writeFieldEnd();

            xfer += oprot->writeFieldStop();
            xfer += oprot->writeStructEnd();
            return xfer;
        }

        void swap(network_address &a, network_address &b) {
            using ::std::swap;
            swap(a.ip, b.ip);
            swap(a.port, b.port);
        }

        network_address::network_address(const network_address &other2) {
            ip = other2.ip;
            port = other2.port;
        }

        network_address &network_address::operator=(const network_address &other3) {
            ip = other3.ip;
            port = other3.port;
            return *this;
        }

        void network_address::printTo(std::ostream &out) const {
            using ::apache::thrift::to_string;
            out << "network_address(";
            out << "ip=" << to_string(ip);
            out << ", " << "port=" << to_string(port);
            out << ")";
        }


        task_status::~task_status() throw() {
        }


        void task_status::__set_task_id(const std::string &val) {
            this->task_id = val;
        }

        void task_status::__set_status(const int8_t val) {
            this->status = val;
        }

        std::ostream &operator<<(std::ostream &out, const task_status &obj) {
            obj.printTo(out);
            return out;
        }


        uint32_t task_status::read(::apache::thrift::protocol::TProtocol *iprot) {

            ::apache::thrift::protocol::TInputRecursionTracker tracker(*iprot);
            uint32_t xfer = 0;
            std::string fname;
            ::apache::thrift::protocol::TType ftype;
            int16_t fid;

            xfer += iprot->readStructBegin(fname);

            using ::apache::thrift::protocol::TProtocolException;

            bool isset_task_id = false;
            bool isset_status = false;

            while (true) {
                xfer += iprot->readFieldBegin(fname, ftype, fid);
                if (ftype == ::apache::thrift::protocol::T_STOP) {
                    break;
                }
                switch (fid) {
                    case 1:
                        if (ftype == ::apache::thrift::protocol::T_STRING) {
                            xfer += iprot->readString(this->task_id);
                            isset_task_id = true;
                        } else {
                            xfer += iprot->skip(ftype);
                        }
                        break;
                    case 2:
                        if (ftype == ::apache::thrift::protocol::T_BYTE) {
                            xfer += iprot->readByte(this->status);
                            isset_status = true;
                        } else {
                            xfer += iprot->skip(ftype);
                        }
                        break;
                    default:
                        xfer += iprot->skip(ftype);
                        break;
                }
                xfer += iprot->readFieldEnd();
            }

            xfer += iprot->readStructEnd();

            if (!isset_task_id)
                throw TProtocolException(TProtocolException::INVALID_DATA);
            if (!isset_status)
                throw TProtocolException(TProtocolException::INVALID_DATA);
            return xfer;
        }

        uint32_t task_status::write(::apache::thrift::protocol::TProtocol *oprot) const {
            uint32_t xfer = 0;
            ::apache::thrift::protocol::TOutputRecursionTracker tracker(*oprot);
            xfer += oprot->writeStructBegin("task_status");

            xfer += oprot->writeFieldBegin("task_id", ::apache::thrift::protocol::T_STRING, 1);
            xfer += oprot->writeString(this->task_id);
            xfer += oprot->writeFieldEnd();

            xfer += oprot->writeFieldBegin("status", ::apache::thrift::protocol::T_BYTE, 2);
            xfer += oprot->writeByte(this->status);
            xfer += oprot->writeFieldEnd();

            xfer += oprot->writeFieldStop();
            xfer += oprot->writeStructEnd();
            return xfer;
        }

        void swap(task_status &a, task_status &b) {
            using ::std::swap;
            swap(a.task_id, b.task_id);
            swap(a.status, b.status);
        }

        task_status::task_status(const task_status &other4) {
            task_id = other4.task_id;
            status = other4.status;
        }

        task_status &task_status::operator=(const task_status &other5) {
            task_id = other5.task_id;
            status = other5.status;
            return *this;
        }

        void task_status::printTo(std::ostream &out) const {
            using ::apache::thrift::to_string;
            out << "task_status(";
            out << "task_id=" << to_string(task_id);
            out << ", " << "status=" << to_string(status);
            out << ")";
        }


        peer_node_info::~peer_node_info() throw() {
        }


        void peer_node_info::__set_peer_node_id(const std::string &val) {
            this->peer_node_id = val;
        }

        void peer_node_info::__set_core_version(const int32_t val) {
            this->core_version = val;
        }

        void peer_node_info::__set_protocol_version(const int32_t val) {
            this->protocol_version = val;
        }

        void peer_node_info::__set_live_time_stamp(const int32_t val) {
            this->live_time_stamp = val;
        }

        void peer_node_info::__set_addr(const network_address &val) {
            this->addr = val;
        }

        void peer_node_info::__set_service_list(const std::vector<std::string> &val) {
            this->service_list = val;
            __isset.service_list = true;
        }

        std::ostream &operator<<(std::ostream &out, const peer_node_info &obj) {
            obj.printTo(out);
            return out;
        }


        uint32_t peer_node_info::read(::apache::thrift::protocol::TProtocol *iprot) {

            ::apache::thrift::protocol::TInputRecursionTracker tracker(*iprot);
            uint32_t xfer = 0;
            std::string fname;
            ::apache::thrift::protocol::TType ftype;
            int16_t fid;

            xfer += iprot->readStructBegin(fname);

            using ::apache::thrift::protocol::TProtocolException;

            bool isset_peer_node_id = false;
            bool isset_core_version = false;
            bool isset_protocol_version = false;
            bool isset_live_time_stamp = false;
            bool isset_addr = false;

            while (true) {
                xfer += iprot->readFieldBegin(fname, ftype, fid);
                if (ftype == ::apache::thrift::protocol::T_STOP) {
                    break;
                }
                switch (fid) {
                    case 1:
                        if (ftype == ::apache::thrift::protocol::T_STRING) {
                            xfer += iprot->readString(this->peer_node_id);
                            isset_peer_node_id = true;
                        } else {
                            xfer += iprot->skip(ftype);
                        }
                        break;
                    case 2:
                        if (ftype == ::apache::thrift::protocol::T_I32) {
                            xfer += iprot->readI32(this->core_version);
                            isset_core_version = true;
                        } else {
                            xfer += iprot->skip(ftype);
                        }
                        break;
                    case 3:
                        if (ftype == ::apache::thrift::protocol::T_I32) {
                            xfer += iprot->readI32(this->protocol_version);
                            isset_protocol_version = true;
                        } else {
                            xfer += iprot->skip(ftype);
                        }
                        break;
                    case 4:
                        if (ftype == ::apache::thrift::protocol::T_I32) {
                            xfer += iprot->readI32(this->live_time_stamp);
                            isset_live_time_stamp = true;
                        } else {
                            xfer += iprot->skip(ftype);
                        }
                        break;
                    case 5:
                        if (ftype == ::apache::thrift::protocol::T_STRUCT) {
                            xfer += this->addr.read(iprot);
                            isset_addr = true;
                        } else {
                            xfer += iprot->skip(ftype);
                        }
                        break;
                    case 6:
                        if (ftype == ::apache::thrift::protocol::T_LIST) {
                            {
                                this->service_list.clear();
                                uint32_t _size6;
                                ::apache::thrift::protocol::TType _etype9;
                                xfer += iprot->readListBegin(_etype9, _size6);
                                this->service_list.resize(_size6);
                                uint32_t _i10;
                                for (_i10 = 0; _i10 < _size6; ++_i10) {
                                    xfer += iprot->readString(this->service_list[_i10]);
                                }
                                xfer += iprot->readListEnd();
                            }
                            this->__isset.service_list = true;
                        } else {
                            xfer += iprot->skip(ftype);
                        }
                        break;
                    default:
                        xfer += iprot->skip(ftype);
                        break;
                }
                xfer += iprot->readFieldEnd();
            }

            xfer += iprot->readStructEnd();

            if (!isset_peer_node_id)
                throw TProtocolException(TProtocolException::INVALID_DATA);
            if (!isset_core_version)
                throw TProtocolException(TProtocolException::INVALID_DATA);
            if (!isset_protocol_version)
                throw TProtocolException(TProtocolException::INVALID_DATA);
            if (!isset_live_time_stamp)
                throw TProtocolException(TProtocolException::INVALID_DATA);
            if (!isset_addr)
                throw TProtocolException(TProtocolException::INVALID_DATA);
            return xfer;
        }

        uint32_t peer_node_info::write(::apache::thrift::protocol::TProtocol *oprot) const {
            uint32_t xfer = 0;
            ::apache::thrift::protocol::TOutputRecursionTracker tracker(*oprot);
            xfer += oprot->writeStructBegin("peer_node_info");

            xfer += oprot->writeFieldBegin("peer_node_id", ::apache::thrift::protocol::T_STRING, 1);
            xfer += oprot->writeString(this->peer_node_id);
            xfer += oprot->writeFieldEnd();

            xfer += oprot->writeFieldBegin("core_version", ::apache::thrift::protocol::T_I32, 2);
            xfer += oprot->writeI32(this->core_version);
            xfer += oprot->writeFieldEnd();

            xfer += oprot->writeFieldBegin("protocol_version", ::apache::thrift::protocol::T_I32, 3);
            xfer += oprot->writeI32(this->protocol_version);
            xfer += oprot->writeFieldEnd();

            xfer += oprot->writeFieldBegin("live_time_stamp", ::apache::thrift::protocol::T_I32, 4);
            xfer += oprot->writeI32(this->live_time_stamp);
            xfer += oprot->writeFieldEnd();

            xfer += oprot->writeFieldBegin("addr", ::apache::thrift::protocol::T_STRUCT, 5);
            xfer += this->addr.write(oprot);
            xfer += oprot->writeFieldEnd();

            if (this->__isset.service_list) {
                xfer += oprot->writeFieldBegin("service_list", ::apache::thrift::protocol::T_LIST, 6);
                {
                    xfer += oprot->writeListBegin(::apache::thrift::protocol::T_STRING,
                                                  static_cast<uint32_t>(this->service_list.size()));
                    std::vector<std::string>::const_iterator _iter11;
                    for (_iter11 = this->service_list.begin(); _iter11 != this->service_list.end(); ++_iter11) {
                        xfer += oprot->writeString((*_iter11));
                    }
                    xfer += oprot->writeListEnd();
                }
                xfer += oprot->writeFieldEnd();
            }
            xfer += oprot->writeFieldStop();
            xfer += oprot->writeStructEnd();
            return xfer;
        }

        void swap(peer_node_info &a, peer_node_info &b) {
            using ::std::swap;
            swap(a.peer_node_id, b.peer_node_id);
            swap(a.core_version, b.core_version);
            swap(a.protocol_version, b.protocol_version);
            swap(a.live_time_stamp, b.live_time_stamp);
            swap(a.addr, b.addr);
            swap(a.service_list, b.service_list);
            swap(a.__isset, b.__isset);
        }

        peer_node_info::peer_node_info(const peer_node_info &other12) {
            peer_node_id = other12.peer_node_id;
            core_version = other12.core_version;
            protocol_version = other12.protocol_version;
            live_time_stamp = other12.live_time_stamp;
            addr = other12.addr;
            service_list = other12.service_list;
            __isset = other12.__isset;
        }

        peer_node_info &peer_node_info::operator=(const peer_node_info &other13) {
            peer_node_id = other13.peer_node_id;
            core_version = other13.core_version;
            protocol_version = other13.protocol_version;
            live_time_stamp = other13.live_time_stamp;
            addr = other13.addr;
            service_list = other13.service_list;
            __isset = other13.__isset;
            return *this;
        }

        void peer_node_info::printTo(std::ostream &out) const {
            using ::apache::thrift::to_string;
            out << "peer_node_info(";
            out << "peer_node_id=" << to_string(peer_node_id);
            out << ", " << "core_version=" << to_string(core_version);
            out << ", " << "protocol_version=" << to_string(protocol_version);
            out << ", " << "live_time_stamp=" << to_string(live_time_stamp);
            out << ", " << "addr=" << to_string(addr);
            out << ", " << "service_list=";
            (__isset.service_list ? (out << to_string(service_list)) : (out << "<null>"));
            out << ")";
        }


        ver_req_body::~ver_req_body() throw() {
        }


        void ver_req_body::__set_node_id(const std::string &val) {
            this->node_id = val;
        }

        void ver_req_body::__set_core_version(const int32_t val) {
            this->core_version = val;
        }

        void ver_req_body::__set_protocol_version(const int32_t val) {
            this->protocol_version = val;
        }

        void ver_req_body::__set_time_stamp(const int64_t val) {
            this->time_stamp = val;
        }

        void ver_req_body::__set_addr_me(const network_address &val) {
            this->addr_me = val;
        }

        void ver_req_body::__set_addr_you(const network_address &val) {
            this->addr_you = val;
        }

        void ver_req_body::__set_start_height(const int64_t val) {
            this->start_height = val;
        }

        std::ostream &operator<<(std::ostream &out, const ver_req_body &obj) {
            obj.printTo(out);
            return out;
        }


        uint32_t ver_req_body::read(::apache::thrift::protocol::TProtocol *iprot) {

            ::apache::thrift::protocol::TInputRecursionTracker tracker(*iprot);
            uint32_t xfer = 0;
            std::string fname;
            ::apache::thrift::protocol::TType ftype;
            int16_t fid;

            xfer += iprot->readStructBegin(fname);

            using ::apache::thrift::protocol::TProtocolException;

            bool isset_node_id = false;
            bool isset_core_version = false;
            bool isset_protocol_version = false;
            bool isset_time_stamp = false;
            bool isset_addr_me = false;
            bool isset_addr_you = false;
            bool isset_start_height = false;

            while (true) {
                xfer += iprot->readFieldBegin(fname, ftype, fid);
                if (ftype == ::apache::thrift::protocol::T_STOP) {
                    break;
                }
                switch (fid) {
                    case 1:
                        if (ftype == ::apache::thrift::protocol::T_STRING) {
                            xfer += iprot->readString(this->node_id);
                            isset_node_id = true;
                        } else {
                            xfer += iprot->skip(ftype);
                        }
                        break;
                    case 2:
                        if (ftype == ::apache::thrift::protocol::T_I32) {
                            xfer += iprot->readI32(this->core_version);
                            isset_core_version = true;
                        } else {
                            xfer += iprot->skip(ftype);
                        }
                        break;
                    case 3:
                        if (ftype == ::apache::thrift::protocol::T_I32) {
                            xfer += iprot->readI32(this->protocol_version);
                            isset_protocol_version = true;
                        } else {
                            xfer += iprot->skip(ftype);
                        }
                        break;
                    case 4:
                        if (ftype == ::apache::thrift::protocol::T_I64) {
                            xfer += iprot->readI64(this->time_stamp);
                            isset_time_stamp = true;
                        } else {
                            xfer += iprot->skip(ftype);
                        }
                        break;
                    case 5:
                        if (ftype == ::apache::thrift::protocol::T_STRUCT) {
                            xfer += this->addr_me.read(iprot);
                            isset_addr_me = true;
                        } else {
                            xfer += iprot->skip(ftype);
                        }
                        break;
                    case 6:
                        if (ftype == ::apache::thrift::protocol::T_STRUCT) {
                            xfer += this->addr_you.read(iprot);
                            isset_addr_you = true;
                        } else {
                            xfer += iprot->skip(ftype);
                        }
                        break;
                    case 7:
                        if (ftype == ::apache::thrift::protocol::T_I64) {
                            xfer += iprot->readI64(this->start_height);
                            isset_start_height = true;
                        } else {
                            xfer += iprot->skip(ftype);
                        }
                        break;
                    default:
                        xfer += iprot->skip(ftype);
                        break;
                }
                xfer += iprot->readFieldEnd();
            }

            xfer += iprot->readStructEnd();

            if (!isset_node_id)
                throw TProtocolException(TProtocolException::INVALID_DATA);
            if (!isset_core_version)
                throw TProtocolException(TProtocolException::INVALID_DATA);
            if (!isset_protocol_version)
                throw TProtocolException(TProtocolException::INVALID_DATA);
            if (!isset_time_stamp)
                throw TProtocolException(TProtocolException::INVALID_DATA);
            if (!isset_addr_me)
                throw TProtocolException(TProtocolException::INVALID_DATA);
            if (!isset_addr_you)
                throw TProtocolException(TProtocolException::INVALID_DATA);
            if (!isset_start_height)
                throw TProtocolException(TProtocolException::INVALID_DATA);
            return xfer;
        }

        uint32_t ver_req_body::write(::apache::thrift::protocol::TProtocol *oprot) const {
            uint32_t xfer = 0;
            ::apache::thrift::protocol::TOutputRecursionTracker tracker(*oprot);
            xfer += oprot->writeStructBegin("ver_req_body");

            xfer += oprot->writeFieldBegin("node_id", ::apache::thrift::protocol::T_STRING, 1);
            xfer += oprot->writeString(this->node_id);
            xfer += oprot->writeFieldEnd();

            xfer += oprot->writeFieldBegin("core_version", ::apache::thrift::protocol::T_I32, 2);
            xfer += oprot->writeI32(this->core_version);
            xfer += oprot->writeFieldEnd();

            xfer += oprot->writeFieldBegin("protocol_version", ::apache::thrift::protocol::T_I32, 3);
            xfer += oprot->writeI32(this->protocol_version);
            xfer += oprot->writeFieldEnd();

            xfer += oprot->writeFieldBegin("time_stamp", ::apache::thrift::protocol::T_I64, 4);
            xfer += oprot->writeI64(this->time_stamp);
            xfer += oprot->writeFieldEnd();

            xfer += oprot->writeFieldBegin("addr_me", ::apache::thrift::protocol::T_STRUCT, 5);
            xfer += this->addr_me.write(oprot);
            xfer += oprot->writeFieldEnd();

            xfer += oprot->writeFieldBegin("addr_you", ::apache::thrift::protocol::T_STRUCT, 6);
            xfer += this->addr_you.write(oprot);
            xfer += oprot->writeFieldEnd();

            xfer += oprot->writeFieldBegin("start_height", ::apache::thrift::protocol::T_I64, 7);
            xfer += oprot->writeI64(this->start_height);
            xfer += oprot->writeFieldEnd();

            xfer += oprot->writeFieldStop();
            xfer += oprot->writeStructEnd();
            return xfer;
        }

        void swap(ver_req_body &a, ver_req_body &b) {
            using ::std::swap;
            swap(a.node_id, b.node_id);
            swap(a.core_version, b.core_version);
            swap(a.protocol_version, b.protocol_version);
            swap(a.time_stamp, b.time_stamp);
            swap(a.addr_me, b.addr_me);
            swap(a.addr_you, b.addr_you);
            swap(a.start_height, b.start_height);
        }

        ver_req_body::ver_req_body(const ver_req_body &other14) {
            node_id = other14.node_id;
            core_version = other14.core_version;
            protocol_version = other14.protocol_version;
            time_stamp = other14.time_stamp;
            addr_me = other14.addr_me;
            addr_you = other14.addr_you;
            start_height = other14.start_height;
        }

        ver_req_body &ver_req_body::operator=(const ver_req_body &other15) {
            node_id = other15.node_id;
            core_version = other15.core_version;
            protocol_version = other15.protocol_version;
            time_stamp = other15.time_stamp;
            addr_me = other15.addr_me;
            addr_you = other15.addr_you;
            start_height = other15.start_height;
            return *this;
        }

        void ver_req_body::printTo(std::ostream &out) const {
            using ::apache::thrift::to_string;
            out << "ver_req_body(";
            out << "node_id=" << to_string(node_id);
            out << ", " << "core_version=" << to_string(core_version);
            out << ", " << "protocol_version=" << to_string(protocol_version);
            out << ", " << "time_stamp=" << to_string(time_stamp);
            out << ", " << "addr_me=" << to_string(addr_me);
            out << ", " << "addr_you=" << to_string(addr_you);
            out << ", " << "start_height=" << to_string(start_height);
            out << ")";
        }


        ver_req::~ver_req() throw() {
        }


        void ver_req::__set_body(const ver_req_body &val) {
            this->body = val;
        }

        std::ostream &operator<<(std::ostream &out, const ver_req &obj) {
            obj.printTo(out);
            return out;
        }


        uint32_t ver_req::read(::apache::thrift::protocol::TProtocol *iprot) {

            ::apache::thrift::protocol::TInputRecursionTracker tracker(*iprot);
            uint32_t xfer = 0;
            std::string fname;
            ::apache::thrift::protocol::TType ftype;
            int16_t fid;

            xfer += iprot->readStructBegin(fname);

            using ::apache::thrift::protocol::TProtocolException;


            while (true) {
                xfer += iprot->readFieldBegin(fname, ftype, fid);
                if (ftype == ::apache::thrift::protocol::T_STOP) {
                    break;
                }
                switch (fid) {
                    case 1:
                        if (ftype == ::apache::thrift::protocol::T_STRUCT) {
                            xfer += this->body.read(iprot);
                            this->__isset.body = true;
                        } else {
                            xfer += iprot->skip(ftype);
                        }
                        break;
                    default:
                        xfer += iprot->skip(ftype);
                        break;
                }
                xfer += iprot->readFieldEnd();
            }

            xfer += iprot->readStructEnd();

            return xfer;
        }

        uint32_t ver_req::write(::apache::thrift::protocol::TProtocol *oprot) const {
            uint32_t xfer = 0;
            ::apache::thrift::protocol::TOutputRecursionTracker tracker(*oprot);
            xfer += oprot->writeStructBegin("ver_req");

            xfer += oprot->writeFieldBegin("body", ::apache::thrift::protocol::T_STRUCT, 1);
            xfer += this->body.write(oprot);
            xfer += oprot->writeFieldEnd();

            xfer += oprot->writeFieldStop();
            xfer += oprot->writeStructEnd();
            return xfer;
        }

        void swap(ver_req &a, ver_req &b) {
            using ::std::swap;
            swap(a.body, b.body);
            swap(a.__isset, b.__isset);
        }

        ver_req::ver_req(const ver_req &other16) {
            body = other16.body;
            __isset = other16.__isset;
        }

        ver_req &ver_req::operator=(const ver_req &other17) {
            body = other17.body;
            __isset = other17.__isset;
            return *this;
        }

        void ver_req::printTo(std::ostream &out) const {
            using ::apache::thrift::to_string;
            out << "ver_req(";
            out << "body=" << to_string(body);
            out << ")";
        }


        ver_resp_body::~ver_resp_body() throw() {
        }


        void ver_resp_body::__set_node_id(const std::string &val) {
            this->node_id = val;
        }

        void ver_resp_body::__set_core_version(const int32_t val) {
            this->core_version = val;
        }

        void ver_resp_body::__set_protocol_version(const int32_t val) {
            this->protocol_version = val;
        }

        std::ostream &operator<<(std::ostream &out, const ver_resp_body &obj) {
            obj.printTo(out);
            return out;
        }


        uint32_t ver_resp_body::read(::apache::thrift::protocol::TProtocol *iprot) {

            ::apache::thrift::protocol::TInputRecursionTracker tracker(*iprot);
            uint32_t xfer = 0;
            std::string fname;
            ::apache::thrift::protocol::TType ftype;
            int16_t fid;

            xfer += iprot->readStructBegin(fname);

            using ::apache::thrift::protocol::TProtocolException;

            bool isset_node_id = false;
            bool isset_core_version = false;
            bool isset_protocol_version = false;

            while (true) {
                xfer += iprot->readFieldBegin(fname, ftype, fid);
                if (ftype == ::apache::thrift::protocol::T_STOP) {
                    break;
                }
                switch (fid) {
                    case 1:
                        if (ftype == ::apache::thrift::protocol::T_STRING) {
                            xfer += iprot->readString(this->node_id);
                            isset_node_id = true;
                        } else {
                            xfer += iprot->skip(ftype);
                        }
                        break;
                    case 2:
                        if (ftype == ::apache::thrift::protocol::T_I32) {
                            xfer += iprot->readI32(this->core_version);
                            isset_core_version = true;
                        } else {
                            xfer += iprot->skip(ftype);
                        }
                        break;
                    case 3:
                        if (ftype == ::apache::thrift::protocol::T_I32) {
                            xfer += iprot->readI32(this->protocol_version);
                            isset_protocol_version = true;
                        } else {
                            xfer += iprot->skip(ftype);
                        }
                        break;
                    default:
                        xfer += iprot->skip(ftype);
                        break;
                }
                xfer += iprot->readFieldEnd();
            }

            xfer += iprot->readStructEnd();

            if (!isset_node_id)
                throw TProtocolException(TProtocolException::INVALID_DATA);
            if (!isset_core_version)
                throw TProtocolException(TProtocolException::INVALID_DATA);
            if (!isset_protocol_version)
                throw TProtocolException(TProtocolException::INVALID_DATA);
            return xfer;
        }

        uint32_t ver_resp_body::write(::apache::thrift::protocol::TProtocol *oprot) const {
            uint32_t xfer = 0;
            ::apache::thrift::protocol::TOutputRecursionTracker tracker(*oprot);
            xfer += oprot->writeStructBegin("ver_resp_body");

            xfer += oprot->writeFieldBegin("node_id", ::apache::thrift::protocol::T_STRING, 1);
            xfer += oprot->writeString(this->node_id);
            xfer += oprot->writeFieldEnd();

            xfer += oprot->writeFieldBegin("core_version", ::apache::thrift::protocol::T_I32, 2);
            xfer += oprot->writeI32(this->core_version);
            xfer += oprot->writeFieldEnd();

            xfer += oprot->writeFieldBegin("protocol_version", ::apache::thrift::protocol::T_I32, 3);
            xfer += oprot->writeI32(this->protocol_version);
            xfer += oprot->writeFieldEnd();

            xfer += oprot->writeFieldStop();
            xfer += oprot->writeStructEnd();
            return xfer;
        }

        void swap(ver_resp_body &a, ver_resp_body &b) {
            using ::std::swap;
            swap(a.node_id, b.node_id);
            swap(a.core_version, b.core_version);
            swap(a.protocol_version, b.protocol_version);
        }

        ver_resp_body::ver_resp_body(const ver_resp_body &other18) {
            node_id = other18.node_id;
            core_version = other18.core_version;
            protocol_version = other18.protocol_version;
        }

        ver_resp_body &ver_resp_body::operator=(const ver_resp_body &other19) {
            node_id = other19.node_id;
            core_version = other19.core_version;
            protocol_version = other19.protocol_version;
            return *this;
        }

        void ver_resp_body::printTo(std::ostream &out) const {
            using ::apache::thrift::to_string;
            out << "ver_resp_body(";
            out << "node_id=" << to_string(node_id);
            out << ", " << "core_version=" << to_string(core_version);
            out << ", " << "protocol_version=" << to_string(protocol_version);
            out << ")";
        }


        ver_resp::~ver_resp() throw() {
        }


        void ver_resp::__set_body(const ver_resp_body &val) {
            this->body = val;
        }

        std::ostream &operator<<(std::ostream &out, const ver_resp &obj) {
            obj.printTo(out);
            return out;
        }


        uint32_t ver_resp::read(::apache::thrift::protocol::TProtocol *iprot) {

            ::apache::thrift::protocol::TInputRecursionTracker tracker(*iprot);
            uint32_t xfer = 0;
            std::string fname;
            ::apache::thrift::protocol::TType ftype;
            int16_t fid;

            xfer += iprot->readStructBegin(fname);

            using ::apache::thrift::protocol::TProtocolException;


            while (true) {
                xfer += iprot->readFieldBegin(fname, ftype, fid);
                if (ftype == ::apache::thrift::protocol::T_STOP) {
                    break;
                }
                switch (fid) {
                    case 1:
                        if (ftype == ::apache::thrift::protocol::T_STRUCT) {
                            xfer += this->body.read(iprot);
                            this->__isset.body = true;
                        } else {
                            xfer += iprot->skip(ftype);
                        }
                        break;
                    default:
                        xfer += iprot->skip(ftype);
                        break;
                }
                xfer += iprot->readFieldEnd();
            }

            xfer += iprot->readStructEnd();

            return xfer;
        }

        uint32_t ver_resp::write(::apache::thrift::protocol::TProtocol *oprot) const {
            uint32_t xfer = 0;
            ::apache::thrift::protocol::TOutputRecursionTracker tracker(*oprot);
            xfer += oprot->writeStructBegin("ver_resp");

            xfer += oprot->writeFieldBegin("body", ::apache::thrift::protocol::T_STRUCT, 1);
            xfer += this->body.write(oprot);
            xfer += oprot->writeFieldEnd();

            xfer += oprot->writeFieldStop();
            xfer += oprot->writeStructEnd();
            return xfer;
        }

        void swap(ver_resp &a, ver_resp &b) {
            using ::std::swap;
            swap(a.body, b.body);
            swap(a.__isset, b.__isset);
        }

        ver_resp::ver_resp(const ver_resp &other20) {
            body = other20.body;
            __isset = other20.__isset;
        }

        ver_resp &ver_resp::operator=(const ver_resp &other21) {
            body = other21.body;
            __isset = other21.__isset;
            return *this;
        }

        void ver_resp::printTo(std::ostream &out) const {
            using ::apache::thrift::to_string;
            out << "ver_resp(";
            out << "body=" << to_string(body);
            out << ")";
        }


        shake_hand_req::~shake_hand_req() throw() {
        }


        void shake_hand_req::__set_body(const empty &val) {
            this->body = val;
        }

        std::ostream &operator<<(std::ostream &out, const shake_hand_req &obj) {
            obj.printTo(out);
            return out;
        }


        uint32_t shake_hand_req::read(::apache::thrift::protocol::TProtocol *iprot) {

            ::apache::thrift::protocol::TInputRecursionTracker tracker(*iprot);
            uint32_t xfer = 0;
            std::string fname;
            ::apache::thrift::protocol::TType ftype;
            int16_t fid;

            xfer += iprot->readStructBegin(fname);

            using ::apache::thrift::protocol::TProtocolException;


            while (true) {
                xfer += iprot->readFieldBegin(fname, ftype, fid);
                if (ftype == ::apache::thrift::protocol::T_STOP) {
                    break;
                }
                switch (fid) {
                    case 1:
                        if (ftype == ::apache::thrift::protocol::T_STRUCT) {
                            xfer += this->body.read(iprot);
                            this->__isset.body = true;
                        } else {
                            xfer += iprot->skip(ftype);
                        }
                        break;
                    default:
                        xfer += iprot->skip(ftype);
                        break;
                }
                xfer += iprot->readFieldEnd();
            }

            xfer += iprot->readStructEnd();

            return xfer;
        }

        uint32_t shake_hand_req::write(::apache::thrift::protocol::TProtocol *oprot) const {
            uint32_t xfer = 0;
            ::apache::thrift::protocol::TOutputRecursionTracker tracker(*oprot);
            xfer += oprot->writeStructBegin("shake_hand_req");

            xfer += oprot->writeFieldBegin("body", ::apache::thrift::protocol::T_STRUCT, 1);
            xfer += this->body.write(oprot);
            xfer += oprot->writeFieldEnd();

            xfer += oprot->writeFieldStop();
            xfer += oprot->writeStructEnd();
            return xfer;
        }

        void swap(shake_hand_req &a, shake_hand_req &b) {
            using ::std::swap;
            swap(a.body, b.body);
            swap(a.__isset, b.__isset);
        }

        shake_hand_req::shake_hand_req(const shake_hand_req &other22) {
            body = other22.body;
            __isset = other22.__isset;
        }

        shake_hand_req &shake_hand_req::operator=(const shake_hand_req &other23) {
            body = other23.body;
            __isset = other23.__isset;
            return *this;
        }

        void shake_hand_req::printTo(std::ostream &out) const {
            using ::apache::thrift::to_string;
            out << "shake_hand_req(";
            out << "body=" << to_string(body);
            out << ")";
        }


        shake_hand_resp::~shake_hand_resp() throw() {
        }


        void shake_hand_resp::__set_body(const empty &val) {
            this->body = val;
        }

        std::ostream &operator<<(std::ostream &out, const shake_hand_resp &obj) {
            obj.printTo(out);
            return out;
        }


        uint32_t shake_hand_resp::read(::apache::thrift::protocol::TProtocol *iprot) {

            ::apache::thrift::protocol::TInputRecursionTracker tracker(*iprot);
            uint32_t xfer = 0;
            std::string fname;
            ::apache::thrift::protocol::TType ftype;
            int16_t fid;

            xfer += iprot->readStructBegin(fname);

            using ::apache::thrift::protocol::TProtocolException;


            while (true) {
                xfer += iprot->readFieldBegin(fname, ftype, fid);
                if (ftype == ::apache::thrift::protocol::T_STOP) {
                    break;
                }
                switch (fid) {
                    case 1:
                        if (ftype == ::apache::thrift::protocol::T_STRUCT) {
                            xfer += this->body.read(iprot);
                            this->__isset.body = true;
                        } else {
                            xfer += iprot->skip(ftype);
                        }
                        break;
                    default:
                        xfer += iprot->skip(ftype);
                        break;
                }
                xfer += iprot->readFieldEnd();
            }

            xfer += iprot->readStructEnd();

            return xfer;
        }

        uint32_t shake_hand_resp::write(::apache::thrift::protocol::TProtocol *oprot) const {
            uint32_t xfer = 0;
            ::apache::thrift::protocol::TOutputRecursionTracker tracker(*oprot);
            xfer += oprot->writeStructBegin("shake_hand_resp");

            xfer += oprot->writeFieldBegin("body", ::apache::thrift::protocol::T_STRUCT, 1);
            xfer += this->body.write(oprot);
            xfer += oprot->writeFieldEnd();

            xfer += oprot->writeFieldStop();
            xfer += oprot->writeStructEnd();
            return xfer;
        }

        void swap(shake_hand_resp &a, shake_hand_resp &b) {
            using ::std::swap;
            swap(a.body, b.body);
            swap(a.__isset, b.__isset);
        }

        shake_hand_resp::shake_hand_resp(const shake_hand_resp &other24) {
            body = other24.body;
            __isset = other24.__isset;
        }

        shake_hand_resp &shake_hand_resp::operator=(const shake_hand_resp &other25) {
            body = other25.body;
            __isset = other25.__isset;
            return *this;
        }

        void shake_hand_resp::printTo(std::ostream &out) const {
            using ::apache::thrift::to_string;
            out << "shake_hand_resp(";
            out << "body=" << to_string(body);
            out << ")";
        }


        get_peer_nodes_req::~get_peer_nodes_req() throw() {
        }


        void get_peer_nodes_req::__set_body(const empty &val) {
            this->body = val;
        }

        std::ostream &operator<<(std::ostream &out, const get_peer_nodes_req &obj) {
            obj.printTo(out);
            return out;
        }


        uint32_t get_peer_nodes_req::read(::apache::thrift::protocol::TProtocol *iprot) {

            ::apache::thrift::protocol::TInputRecursionTracker tracker(*iprot);
            uint32_t xfer = 0;
            std::string fname;
            ::apache::thrift::protocol::TType ftype;
            int16_t fid;

            xfer += iprot->readStructBegin(fname);

            using ::apache::thrift::protocol::TProtocolException;


            while (true) {
                xfer += iprot->readFieldBegin(fname, ftype, fid);
                if (ftype == ::apache::thrift::protocol::T_STOP) {
                    break;
                }
                switch (fid) {
                    case 1:
                        if (ftype == ::apache::thrift::protocol::T_STRUCT) {
                            xfer += this->body.read(iprot);
                            this->__isset.body = true;
                        } else {
                            xfer += iprot->skip(ftype);
                        }
                        break;
                    default:
                        xfer += iprot->skip(ftype);
                        break;
                }
                xfer += iprot->readFieldEnd();
            }

            xfer += iprot->readStructEnd();

            return xfer;
        }

        uint32_t get_peer_nodes_req::write(::apache::thrift::protocol::TProtocol *oprot) const {
            uint32_t xfer = 0;
            ::apache::thrift::protocol::TOutputRecursionTracker tracker(*oprot);
            xfer += oprot->writeStructBegin("get_peer_nodes_req");

            xfer += oprot->writeFieldBegin("body", ::apache::thrift::protocol::T_STRUCT, 1);
            xfer += this->body.write(oprot);
            xfer += oprot->writeFieldEnd();

            xfer += oprot->writeFieldStop();
            xfer += oprot->writeStructEnd();
            return xfer;
        }

        void swap(get_peer_nodes_req &a, get_peer_nodes_req &b) {
            using ::std::swap;
            swap(a.body, b.body);
            swap(a.__isset, b.__isset);
        }

        get_peer_nodes_req::get_peer_nodes_req(const get_peer_nodes_req &other26) {
            body = other26.body;
            __isset = other26.__isset;
        }

        get_peer_nodes_req &get_peer_nodes_req::operator=(const get_peer_nodes_req &other27) {
            body = other27.body;
            __isset = other27.__isset;
            return *this;
        }

        void get_peer_nodes_req::printTo(std::ostream &out) const {
            using ::apache::thrift::to_string;
            out << "get_peer_nodes_req(";
            out << "body=" << to_string(body);
            out << ")";
        }


        get_peer_nodes_resp_body::~get_peer_nodes_resp_body() throw() {
        }


        void get_peer_nodes_resp_body::__set_peer_nodes_list(const std::vector<peer_node_info> &val) {
            this->peer_nodes_list = val;
        }

        std::ostream &operator<<(std::ostream &out, const get_peer_nodes_resp_body &obj) {
            obj.printTo(out);
            return out;
        }


        uint32_t get_peer_nodes_resp_body::read(::apache::thrift::protocol::TProtocol *iprot) {

            ::apache::thrift::protocol::TInputRecursionTracker tracker(*iprot);
            uint32_t xfer = 0;
            std::string fname;
            ::apache::thrift::protocol::TType ftype;
            int16_t fid;

            xfer += iprot->readStructBegin(fname);

            using ::apache::thrift::protocol::TProtocolException;

            bool isset_peer_nodes_list = false;

            while (true) {
                xfer += iprot->readFieldBegin(fname, ftype, fid);
                if (ftype == ::apache::thrift::protocol::T_STOP) {
                    break;
                }
                switch (fid) {
                    case 1:
                        if (ftype == ::apache::thrift::protocol::T_LIST) {
                            {
                                this->peer_nodes_list.clear();
                                uint32_t _size28;
                                ::apache::thrift::protocol::TType _etype31;
                                xfer += iprot->readListBegin(_etype31, _size28);
                                this->peer_nodes_list.resize(_size28);
                                uint32_t _i32;
                                for (_i32 = 0; _i32 < _size28; ++_i32) {
                                    xfer += this->peer_nodes_list[_i32].read(iprot);
                                }
                                xfer += iprot->readListEnd();
                            }
                            isset_peer_nodes_list = true;
                        } else {
                            xfer += iprot->skip(ftype);
                        }
                        break;
                    default:
                        xfer += iprot->skip(ftype);
                        break;
                }
                xfer += iprot->readFieldEnd();
            }

            xfer += iprot->readStructEnd();

            if (!isset_peer_nodes_list)
                throw TProtocolException(TProtocolException::INVALID_DATA);
            return xfer;
        }

        uint32_t get_peer_nodes_resp_body::write(::apache::thrift::protocol::TProtocol *oprot) const {
            uint32_t xfer = 0;
            ::apache::thrift::protocol::TOutputRecursionTracker tracker(*oprot);
            xfer += oprot->writeStructBegin("get_peer_nodes_resp_body");

            xfer += oprot->writeFieldBegin("peer_nodes_list", ::apache::thrift::protocol::T_LIST, 1);
            {
                xfer += oprot->writeListBegin(::apache::thrift::protocol::T_STRUCT,
                                              static_cast<uint32_t>(this->peer_nodes_list.size()));
                std::vector<peer_node_info>::const_iterator _iter33;
                for (_iter33 = this->peer_nodes_list.begin(); _iter33 != this->peer_nodes_list.end(); ++_iter33) {
                    xfer += (*_iter33).write(oprot);
                }
                xfer += oprot->writeListEnd();
            }
            xfer += oprot->writeFieldEnd();

            xfer += oprot->writeFieldStop();
            xfer += oprot->writeStructEnd();
            return xfer;
        }

        void swap(get_peer_nodes_resp_body &a, get_peer_nodes_resp_body &b) {
            using ::std::swap;
            swap(a.peer_nodes_list, b.peer_nodes_list);
        }

        get_peer_nodes_resp_body::get_peer_nodes_resp_body(const get_peer_nodes_resp_body &other34) {
            peer_nodes_list = other34.peer_nodes_list;
        }

        get_peer_nodes_resp_body &get_peer_nodes_resp_body::operator=(const get_peer_nodes_resp_body &other35) {
            peer_nodes_list = other35.peer_nodes_list;
            return *this;
        }

        void get_peer_nodes_resp_body::printTo(std::ostream &out) const {
            using ::apache::thrift::to_string;
            out << "get_peer_nodes_resp_body(";
            out << "peer_nodes_list=" << to_string(peer_nodes_list);
            out << ")";
        }


        get_peer_nodes_resp::~get_peer_nodes_resp() throw() {
        }


        void get_peer_nodes_resp::__set_body(const get_peer_nodes_resp_body &val) {
            this->body = val;
        }

        std::ostream &operator<<(std::ostream &out, const get_peer_nodes_resp &obj) {
            obj.printTo(out);
            return out;
        }


        uint32_t get_peer_nodes_resp::read(::apache::thrift::protocol::TProtocol *iprot) {

            ::apache::thrift::protocol::TInputRecursionTracker tracker(*iprot);
            uint32_t xfer = 0;
            std::string fname;
            ::apache::thrift::protocol::TType ftype;
            int16_t fid;

            xfer += iprot->readStructBegin(fname);

            using ::apache::thrift::protocol::TProtocolException;


            while (true) {
                xfer += iprot->readFieldBegin(fname, ftype, fid);
                if (ftype == ::apache::thrift::protocol::T_STOP) {
                    break;
                }
                switch (fid) {
                    case 1:
                        if (ftype == ::apache::thrift::protocol::T_STRUCT) {
                            xfer += this->body.read(iprot);
                            this->__isset.body = true;
                        } else {
                            xfer += iprot->skip(ftype);
                        }
                        break;
                    default:
                        xfer += iprot->skip(ftype);
                        break;
                }
                xfer += iprot->readFieldEnd();
            }

            xfer += iprot->readStructEnd();

            return xfer;
        }

        uint32_t get_peer_nodes_resp::write(::apache::thrift::protocol::TProtocol *oprot) const {
            uint32_t xfer = 0;
            ::apache::thrift::protocol::TOutputRecursionTracker tracker(*oprot);
            xfer += oprot->writeStructBegin("get_peer_nodes_resp");

            xfer += oprot->writeFieldBegin("body", ::apache::thrift::protocol::T_STRUCT, 1);
            xfer += this->body.write(oprot);
            xfer += oprot->writeFieldEnd();

            xfer += oprot->writeFieldStop();
            xfer += oprot->writeStructEnd();
            return xfer;
        }

        void swap(get_peer_nodes_resp &a, get_peer_nodes_resp &b) {
            using ::std::swap;
            swap(a.body, b.body);
            swap(a.__isset, b.__isset);
        }

        get_peer_nodes_resp::get_peer_nodes_resp(const get_peer_nodes_resp &other36) {
            body = other36.body;
            __isset = other36.__isset;
        }

        get_peer_nodes_resp &get_peer_nodes_resp::operator=(const get_peer_nodes_resp &other37) {
            body = other37.body;
            __isset = other37.__isset;
            return *this;
        }

        void get_peer_nodes_resp::printTo(std::ostream &out) const {
            using ::apache::thrift::to_string;
            out << "get_peer_nodes_resp(";
            out << "body=" << to_string(body);
            out << ")";
        }


        peer_nodes_broadcast_req_body::~peer_nodes_broadcast_req_body() throw() {
        }


        void peer_nodes_broadcast_req_body::__set_peer_nodes_list(const std::vector<peer_node_info> &val) {
            this->peer_nodes_list = val;
        }

        std::ostream &operator<<(std::ostream &out, const peer_nodes_broadcast_req_body &obj) {
            obj.printTo(out);
            return out;
        }


        uint32_t peer_nodes_broadcast_req_body::read(::apache::thrift::protocol::TProtocol *iprot) {

            ::apache::thrift::protocol::TInputRecursionTracker tracker(*iprot);
            uint32_t xfer = 0;
            std::string fname;
            ::apache::thrift::protocol::TType ftype;
            int16_t fid;

            xfer += iprot->readStructBegin(fname);

            using ::apache::thrift::protocol::TProtocolException;

            bool isset_peer_nodes_list = false;

            while (true) {
                xfer += iprot->readFieldBegin(fname, ftype, fid);
                if (ftype == ::apache::thrift::protocol::T_STOP) {
                    break;
                }
                switch (fid) {
                    case 1:
                        if (ftype == ::apache::thrift::protocol::T_LIST) {
                            {
                                this->peer_nodes_list.clear();
                                uint32_t _size38;
                                ::apache::thrift::protocol::TType _etype41;
                                xfer += iprot->readListBegin(_etype41, _size38);
                                this->peer_nodes_list.resize(_size38);
                                uint32_t _i42;
                                for (_i42 = 0; _i42 < _size38; ++_i42) {
                                    xfer += this->peer_nodes_list[_i42].read(iprot);
                                }
                                xfer += iprot->readListEnd();
                            }
                            isset_peer_nodes_list = true;
                        } else {
                            xfer += iprot->skip(ftype);
                        }
                        break;
                    default:
                        xfer += iprot->skip(ftype);
                        break;
                }
                xfer += iprot->readFieldEnd();
            }

            xfer += iprot->readStructEnd();

            if (!isset_peer_nodes_list)
                throw TProtocolException(TProtocolException::INVALID_DATA);
            return xfer;
        }

        uint32_t peer_nodes_broadcast_req_body::write(::apache::thrift::protocol::TProtocol *oprot) const {
            uint32_t xfer = 0;
            ::apache::thrift::protocol::TOutputRecursionTracker tracker(*oprot);
            xfer += oprot->writeStructBegin("peer_nodes_broadcast_req_body");

            xfer += oprot->writeFieldBegin("peer_nodes_list", ::apache::thrift::protocol::T_LIST, 1);
            {
                xfer += oprot->writeListBegin(::apache::thrift::protocol::T_STRUCT,
                                              static_cast<uint32_t>(this->peer_nodes_list.size()));
                std::vector<peer_node_info>::const_iterator _iter43;
                for (_iter43 = this->peer_nodes_list.begin(); _iter43 != this->peer_nodes_list.end(); ++_iter43) {
                    xfer += (*_iter43).write(oprot);
                }
                xfer += oprot->writeListEnd();
            }
            xfer += oprot->writeFieldEnd();

            xfer += oprot->writeFieldStop();
            xfer += oprot->writeStructEnd();
            return xfer;
        }

        void swap(peer_nodes_broadcast_req_body &a, peer_nodes_broadcast_req_body &b) {
            using ::std::swap;
            swap(a.peer_nodes_list, b.peer_nodes_list);
        }

        peer_nodes_broadcast_req_body::peer_nodes_broadcast_req_body(const peer_nodes_broadcast_req_body &other44) {
            peer_nodes_list = other44.peer_nodes_list;
        }

        peer_nodes_broadcast_req_body &
        peer_nodes_broadcast_req_body::operator=(const peer_nodes_broadcast_req_body &other45) {
            peer_nodes_list = other45.peer_nodes_list;
            return *this;
        }

        void peer_nodes_broadcast_req_body::printTo(std::ostream &out) const {
            using ::apache::thrift::to_string;
            out << "peer_nodes_broadcast_req_body(";
            out << "peer_nodes_list=" << to_string(peer_nodes_list);
            out << ")";
        }


        peer_nodes_broadcast_req::~peer_nodes_broadcast_req() throw() {
        }


        void peer_nodes_broadcast_req::__set_body(const peer_nodes_broadcast_req_body &val) {
            this->body = val;
        }

        std::ostream &operator<<(std::ostream &out, const peer_nodes_broadcast_req &obj) {
            obj.printTo(out);
            return out;
        }


        uint32_t peer_nodes_broadcast_req::read(::apache::thrift::protocol::TProtocol *iprot) {

            ::apache::thrift::protocol::TInputRecursionTracker tracker(*iprot);
            uint32_t xfer = 0;
            std::string fname;
            ::apache::thrift::protocol::TType ftype;
            int16_t fid;

            xfer += iprot->readStructBegin(fname);

            using ::apache::thrift::protocol::TProtocolException;


            while (true) {
                xfer += iprot->readFieldBegin(fname, ftype, fid);
                if (ftype == ::apache::thrift::protocol::T_STOP) {
                    break;
                }
                switch (fid) {
                    case 1:
                        if (ftype == ::apache::thrift::protocol::T_STRUCT) {
                            xfer += this->body.read(iprot);
                            this->__isset.body = true;
                        } else {
                            xfer += iprot->skip(ftype);
                        }
                        break;
                    default:
                        xfer += iprot->skip(ftype);
                        break;
                }
                xfer += iprot->readFieldEnd();
            }

            xfer += iprot->readStructEnd();

            return xfer;
        }

        uint32_t peer_nodes_broadcast_req::write(::apache::thrift::protocol::TProtocol *oprot) const {
            uint32_t xfer = 0;
            ::apache::thrift::protocol::TOutputRecursionTracker tracker(*oprot);
            xfer += oprot->writeStructBegin("peer_nodes_broadcast_req");

            xfer += oprot->writeFieldBegin("body", ::apache::thrift::protocol::T_STRUCT, 1);
            xfer += this->body.write(oprot);
            xfer += oprot->writeFieldEnd();

            xfer += oprot->writeFieldStop();
            xfer += oprot->writeStructEnd();
            return xfer;
        }

        void swap(peer_nodes_broadcast_req &a, peer_nodes_broadcast_req &b) {
            using ::std::swap;
            swap(a.body, b.body);
            swap(a.__isset, b.__isset);
        }

        peer_nodes_broadcast_req::peer_nodes_broadcast_req(const peer_nodes_broadcast_req &other46) {
            body = other46.body;
            __isset = other46.__isset;
        }

        peer_nodes_broadcast_req &peer_nodes_broadcast_req::operator=(const peer_nodes_broadcast_req &other47) {
            body = other47.body;
            __isset = other47.__isset;
            return *this;
        }

        void peer_nodes_broadcast_req::printTo(std::ostream &out) const {
            using ::apache::thrift::to_string;
            out << "peer_nodes_broadcast_req(";
            out << "body=" << to_string(body);
            out << ")";
        }

        node_create_task_req_body::~node_create_task_req_body() throw() {
        }

        void node_create_task_req_body::__set_task_id(const std::string& val) {
            this->task_id = val;
        }

        void node_create_task_req_body::__set_select_mode(const int8_t val) {
            this->select_mode = val;
        }

        void node_create_task_req_body::__set_master(const std::string& val) {
            this->master = val;
            __isset.master = true;
        }

        void node_create_task_req_body::__set_peer_nodes_list(const std::vector<std::string> & val) {
            this->peer_nodes_list = val;
        }

        void node_create_task_req_body::__set_server_specification(const std::string& val) {
            this->server_specification = val;
            __isset.server_specification = true;
        }

        void node_create_task_req_body::__set_server_count(const int32_t val) {
            this->server_count = val;
            __isset.server_count = true;
        }

        void node_create_task_req_body::__set_training_engine(const std::string& val) {
            this->training_engine = val;
        }

        void node_create_task_req_body::__set_code_dir(const std::string& val) {
            this->code_dir = val;
        }

        void node_create_task_req_body::__set_entry_file(const std::string& val) {
            this->entry_file = val;
        }

        void node_create_task_req_body::__set_data_dir(const std::string& val) {
            this->data_dir = val;
        }

        void node_create_task_req_body::__set_checkpoint_dir(const std::string& val) {
            this->checkpoint_dir = val;
        }

        void node_create_task_req_body::__set_hyper_parameters(const std::string& val) {
            this->hyper_parameters = val;
            __isset.hyper_parameters = true;
        }

        void node_create_task_req_body::__set_container_name(const std::string& val) {
            this->container_name = val;
            __isset.container_name = true;
        }

        void node_create_task_req_body::__set_memory(const int64_t val) {
            this->memory = val;
            __isset.memory = true;
        }

        void node_create_task_req_body::__set_memory_swap(const int64_t val) {
            this->memory_swap = val;
            __isset.memory_swap = true;
        }
        std::ostream& operator<<(std::ostream& out, const node_create_task_req_body& obj)
        {
            obj.printTo(out);
            return out;
        }

        uint32_t node_create_task_req_body::read(::apache::thrift::protocol::TProtocol* iprot) {

            ::apache::thrift::protocol::TInputRecursionTracker tracker(*iprot);
            uint32_t xfer = 0;
            std::string fname;
            ::apache::thrift::protocol::TType ftype;
            int16_t fid;

            xfer += iprot->readStructBegin(fname);

            using ::apache::thrift::protocol::TProtocolException;

            bool isset_task_id = false;
            bool isset_select_mode = false;
            bool isset_peer_nodes_list = false;
            bool isset_training_engine = false;
            bool isset_code_dir = false;
            bool isset_entry_file = false;
            bool isset_data_dir = false;
            bool isset_checkpoint_dir = false;

            while (true)
            {
                xfer += iprot->readFieldBegin(fname, ftype, fid);
                if (ftype == ::apache::thrift::protocol::T_STOP) {
                    break;
                }
                switch (fid)
                {
                    case 1:
                        if (ftype == ::apache::thrift::protocol::T_STRING) {
                            xfer += iprot->readString(this->task_id);
                            isset_task_id = true;
                        } else {
                            xfer += iprot->skip(ftype);
                        }
                        break;
                    case 2:
                        if (ftype == ::apache::thrift::protocol::T_BYTE) {
                            xfer += iprot->readByte(this->select_mode);
                            isset_select_mode = true;
                        } else {
                            xfer += iprot->skip(ftype);
                        }
                        break;
                    case 3:
                        if (ftype == ::apache::thrift::protocol::T_STRING) {
                            xfer += iprot->readString(this->master);
                            this->__isset.master = true;
                        } else {
                            xfer += iprot->skip(ftype);
                        }
                        break;
                    case 4:
                        if (ftype == ::apache::thrift::protocol::T_LIST) {
                            {
                                this->peer_nodes_list.clear();
                                uint32_t _size48;
                                ::apache::thrift::protocol::TType _etype51;
                                xfer += iprot->readListBegin(_etype51, _size48);
                                this->peer_nodes_list.resize(_size48);
                                uint32_t _i52;
                                for (_i52 = 0; _i52 < _size48; ++_i52)
                                {
                                    xfer += iprot->readString(this->peer_nodes_list[_i52]);
                                }
                                xfer += iprot->readListEnd();
                            }
                            isset_peer_nodes_list = true;
                        } else {
                            xfer += iprot->skip(ftype);
                        }
                        break;
                    case 5:
                        if (ftype == ::apache::thrift::protocol::T_STRING) {
                            xfer += iprot->readString(this->server_specification);
                            this->__isset.server_specification = true;
                        } else {
                            xfer += iprot->skip(ftype);
                        }
                        break;
                    case 6:
                        if (ftype == ::apache::thrift::protocol::T_I32) {
                            xfer += iprot->readI32(this->server_count);
                            this->__isset.server_count = true;
                        } else {
                            xfer += iprot->skip(ftype);
                        }
                        break;
                    case 7:
                        if (ftype == ::apache::thrift::protocol::T_STRING) {
                            xfer += iprot->readString(this->training_engine);
                            isset_training_engine = true;
                        } else {
                            xfer += iprot->skip(ftype);
                        }
                        break;
                    case 8:
                        if (ftype == ::apache::thrift::protocol::T_STRING) {
                            xfer += iprot->readString(this->code_dir);
                            isset_code_dir = true;
                        } else {
                            xfer += iprot->skip(ftype);
                        }
                        break;
                    case 9:
                        if (ftype == ::apache::thrift::protocol::T_STRING) {
                            xfer += iprot->readString(this->entry_file);
                            isset_entry_file = true;
                        } else {
                            xfer += iprot->skip(ftype);
                        }
                        break;
                    case 10:
                        if (ftype == ::apache::thrift::protocol::T_STRING) {
                            xfer += iprot->readString(this->data_dir);
                            isset_data_dir = true;
                        } else {
                            xfer += iprot->skip(ftype);
                        }
                        break;
                    case 11:
                        if (ftype == ::apache::thrift::protocol::T_STRING) {
                            xfer += iprot->readString(this->checkpoint_dir);
                            isset_checkpoint_dir = true;
                        } else {
                            xfer += iprot->skip(ftype);
                        }
                        break;
                    case 12:
                        if (ftype == ::apache::thrift::protocol::T_STRING) {
                            xfer += iprot->readString(this->hyper_parameters);
                            this->__isset.hyper_parameters = true;
                        } else {
                            xfer += iprot->skip(ftype);
                        }
                        break;
                    case 13:
                        if (ftype == ::apache::thrift::protocol::T_STRING) {
                            xfer += iprot->readString(this->container_name);
                            this->__isset.container_name = true;
                        } else {
                            xfer += iprot->skip(ftype);
                        }
                        break;
                    case 14:
                        if (ftype == ::apache::thrift::protocol::T_I64) {
                            xfer += iprot->readI64(this->memory);
                            this->__isset.memory = true;
                        } else {
                            xfer += iprot->skip(ftype);
                        }
                        break;
                    case 15:
                        if (ftype == ::apache::thrift::protocol::T_I64) {
                            xfer += iprot->readI64(this->memory_swap);
                            this->__isset.memory_swap = true;
                        } else {
                            xfer += iprot->skip(ftype);
                        }
                        break;
                    default:
                        xfer += iprot->skip(ftype);
                        break;
                }
                xfer += iprot->readFieldEnd();
            }

            xfer += iprot->readStructEnd();

            if (!isset_task_id)
                throw TProtocolException(TProtocolException::INVALID_DATA);
            if (!isset_select_mode)
                throw TProtocolException(TProtocolException::INVALID_DATA);
            if (!isset_peer_nodes_list)
                throw TProtocolException(TProtocolException::INVALID_DATA);
            if (!isset_training_engine)
                throw TProtocolException(TProtocolException::INVALID_DATA);
            if (!isset_code_dir)
                throw TProtocolException(TProtocolException::INVALID_DATA);
            if (!isset_entry_file)
                throw TProtocolException(TProtocolException::INVALID_DATA);
            if (!isset_data_dir)
                throw TProtocolException(TProtocolException::INVALID_DATA);
            if (!isset_checkpoint_dir)
                throw TProtocolException(TProtocolException::INVALID_DATA);
            return xfer;
        }

        uint32_t node_create_task_req_body::write(::apache::thrift::protocol::TProtocol* oprot) const {
            uint32_t xfer = 0;
            ::apache::thrift::protocol::TOutputRecursionTracker tracker(*oprot);
            xfer += oprot->writeStructBegin("node_create_task_req_body");

            xfer += oprot->writeFieldBegin("task_id", ::apache::thrift::protocol::T_STRING, 1);
            xfer += oprot->writeString(this->task_id);
            xfer += oprot->writeFieldEnd();

            xfer += oprot->writeFieldBegin("select_mode", ::apache::thrift::protocol::T_BYTE, 2);
            xfer += oprot->writeByte(this->select_mode);
            xfer += oprot->writeFieldEnd();

            if (this->__isset.master) {
                xfer += oprot->writeFieldBegin("master", ::apache::thrift::protocol::T_STRING, 3);
                xfer += oprot->writeString(this->master);
                xfer += oprot->writeFieldEnd();
            }
            xfer += oprot->writeFieldBegin("peer_nodes_list", ::apache::thrift::protocol::T_LIST, 4);
            {
                xfer += oprot->writeListBegin(::apache::thrift::protocol::T_STRING, static_cast<uint32_t>(this->peer_nodes_list.size()));
                std::vector<std::string> ::const_iterator _iter53;
                for (_iter53 = this->peer_nodes_list.begin(); _iter53 != this->peer_nodes_list.end(); ++_iter53)
                {
                    xfer += oprot->writeString((*_iter53));
                }
                xfer += oprot->writeListEnd();
            }
            xfer += oprot->writeFieldEnd();

            if (this->__isset.server_specification) {
                xfer += oprot->writeFieldBegin("server_specification", ::apache::thrift::protocol::T_STRING, 5);
                xfer += oprot->writeString(this->server_specification);
                xfer += oprot->writeFieldEnd();
            }
            if (this->__isset.server_count) {
                xfer += oprot->writeFieldBegin("server_count", ::apache::thrift::protocol::T_I32, 6);
                xfer += oprot->writeI32(this->server_count);
                xfer += oprot->writeFieldEnd();
            }
            xfer += oprot->writeFieldBegin("training_engine", ::apache::thrift::protocol::T_STRING, 7);
            xfer += oprot->writeString(this->training_engine);
            xfer += oprot->writeFieldEnd();

            xfer += oprot->writeFieldBegin("code_dir", ::apache::thrift::protocol::T_STRING, 8);
            xfer += oprot->writeString(this->code_dir);
            xfer += oprot->writeFieldEnd();

            xfer += oprot->writeFieldBegin("entry_file", ::apache::thrift::protocol::T_STRING, 9);
            xfer += oprot->writeString(this->entry_file);
            xfer += oprot->writeFieldEnd();

            xfer += oprot->writeFieldBegin("data_dir", ::apache::thrift::protocol::T_STRING, 10);
            xfer += oprot->writeString(this->data_dir);
            xfer += oprot->writeFieldEnd();

            xfer += oprot->writeFieldBegin("checkpoint_dir", ::apache::thrift::protocol::T_STRING, 11);
            xfer += oprot->writeString(this->checkpoint_dir);
            xfer += oprot->writeFieldEnd();

            if (this->__isset.hyper_parameters) {
                xfer += oprot->writeFieldBegin("hyper_parameters", ::apache::thrift::protocol::T_STRING, 12);
                xfer += oprot->writeString(this->hyper_parameters);
                xfer += oprot->writeFieldEnd();
            }
            if (this->__isset.container_name) {
                xfer += oprot->writeFieldBegin("container_name", ::apache::thrift::protocol::T_STRING, 13);
                xfer += oprot->writeString(this->container_name);
                xfer += oprot->writeFieldEnd();
            }
            if (this->__isset.memory) {
                xfer += oprot->writeFieldBegin("memory", ::apache::thrift::protocol::T_I64, 14);
                xfer += oprot->writeI64(this->memory);
                xfer += oprot->writeFieldEnd();
            }
            if (this->__isset.memory_swap) {
                xfer += oprot->writeFieldBegin("memory_swap", ::apache::thrift::protocol::T_I64, 15);
                xfer += oprot->writeI64(this->memory_swap);
                xfer += oprot->writeFieldEnd();
            }
            xfer += oprot->writeFieldStop();
            xfer += oprot->writeStructEnd();
            return xfer;
        }

        void swap(node_create_task_req_body &a, node_create_task_req_body &b) {
            using ::std::swap;
            swap(a.task_id, b.task_id);
            swap(a.select_mode, b.select_mode);
            swap(a.master, b.master);
            swap(a.peer_nodes_list, b.peer_nodes_list);
            swap(a.server_specification, b.server_specification);
            swap(a.server_count, b.server_count);
            swap(a.training_engine, b.training_engine);
            swap(a.code_dir, b.code_dir);
            swap(a.entry_file, b.entry_file);
            swap(a.data_dir, b.data_dir);
            swap(a.checkpoint_dir, b.checkpoint_dir);
            swap(a.hyper_parameters, b.hyper_parameters);
            swap(a.container_name, b.container_name);
            swap(a.memory, b.memory);
            swap(a.memory_swap, b.memory_swap);
            swap(a.__isset, b.__isset);
        }

        node_create_task_req_body::node_create_task_req_body(const node_create_task_req_body& other54) {
            task_id = other54.task_id;
            select_mode = other54.select_mode;
            master = other54.master;
            peer_nodes_list = other54.peer_nodes_list;
            server_specification = other54.server_specification;
            server_count = other54.server_count;
            training_engine = other54.training_engine;
            code_dir = other54.code_dir;
            entry_file = other54.entry_file;
            data_dir = other54.data_dir;
            checkpoint_dir = other54.checkpoint_dir;
            hyper_parameters = other54.hyper_parameters;
            container_name = other54.container_name;
            memory = other54.memory;
            memory_swap = other54.memory_swap;
            __isset = other54.__isset;
        }
        node_create_task_req_body& node_create_task_req_body::operator=(const node_create_task_req_body& other55) {
            task_id = other55.task_id;
            select_mode = other55.select_mode;
            master = other55.master;
            peer_nodes_list = other55.peer_nodes_list;
            server_specification = other55.server_specification;
            server_count = other55.server_count;
            training_engine = other55.training_engine;
            code_dir = other55.code_dir;
            entry_file = other55.entry_file;
            data_dir = other55.data_dir;
            checkpoint_dir = other55.checkpoint_dir;
            hyper_parameters = other55.hyper_parameters;
            container_name = other55.container_name;
            memory = other55.memory;
            memory_swap = other55.memory_swap;
            __isset = other55.__isset;
            return *this;
        }
        void node_create_task_req_body::printTo(std::ostream& out) const {
            using ::apache::thrift::to_string;
            out << "node_create_task_req_body(";
            out << "task_id=" << to_string(task_id);
            out << ", " << "select_mode=" << to_string(select_mode);
            out << ", " << "master="; (__isset.master ? (out << to_string(master)) : (out << "<null>"));
            out << ", " << "peer_nodes_list=" << to_string(peer_nodes_list);
            out << ", " << "server_specification="; (__isset.server_specification ? (out << to_string(server_specification)) : (out << "<null>"));
            out << ", " << "server_count="; (__isset.server_count ? (out << to_string(server_count)) : (out << "<null>"));
            out << ", " << "training_engine=" << to_string(training_engine);
            out << ", " << "code_dir=" << to_string(code_dir);
            out << ", " << "entry_file=" << to_string(entry_file);
            out << ", " << "data_dir=" << to_string(data_dir);
            out << ", " << "checkpoint_dir=" << to_string(checkpoint_dir);
            out << ", " << "hyper_parameters="; (__isset.hyper_parameters ? (out << to_string(hyper_parameters)) : (out << "<null>"));
            out << ", " << "container_name="; (__isset.container_name ? (out << to_string(container_name)) : (out << "<null>"));
            out << ", " << "memory="; (__isset.memory ? (out << to_string(memory)) : (out << "<null>"));
            out << ", " << "memory_swap="; (__isset.memory_swap ? (out << to_string(memory_swap)) : (out << "<null>"));
            out << ")";
        }

        node_create_task_req::~node_create_task_req() throw() {
        }

        void node_create_task_req::__set_body(const node_create_task_req_body& val) {
            this->body = val;
        }
        std::ostream& operator<<(std::ostream& out, const node_create_task_req& obj)
        {
            obj.printTo(out);
            return out;
        }

        uint32_t node_create_task_req::read(::apache::thrift::protocol::TProtocol* iprot) {

            ::apache::thrift::protocol::TInputRecursionTracker tracker(*iprot);
            uint32_t xfer = 0;
            std::string fname;
            ::apache::thrift::protocol::TType ftype;
            int16_t fid;

            xfer += iprot->readStructBegin(fname);

            using ::apache::thrift::protocol::TProtocolException;


            while (true)
            {
                xfer += iprot->readFieldBegin(fname, ftype, fid);
                if (ftype == ::apache::thrift::protocol::T_STOP) {
                    break;
                }
                switch (fid)
                {
                    case 1:
                        if (ftype == ::apache::thrift::protocol::T_STRUCT) {
                            xfer += this->body.read(iprot);
                            this->__isset.body = true;
                        } else {
                            xfer += iprot->skip(ftype);
                        }
                        break;
                    default:
                        xfer += iprot->skip(ftype);
                        break;
                }
                xfer += iprot->readFieldEnd();
            }

            xfer += iprot->readStructEnd();

            return xfer;
        }

        uint32_t node_create_task_req::write(::apache::thrift::protocol::TProtocol* oprot) const {
            uint32_t xfer = 0;
            ::apache::thrift::protocol::TOutputRecursionTracker tracker(*oprot);
            xfer += oprot->writeStructBegin("node_create_task_req");

            xfer += oprot->writeFieldBegin("body", ::apache::thrift::protocol::T_STRUCT, 1);
            xfer += this->body.write(oprot);
            xfer += oprot->writeFieldEnd();

            xfer += oprot->writeFieldStop();
            xfer += oprot->writeStructEnd();
            return xfer;
        }

        void swap(node_create_task_req &a, node_create_task_req &b) {
            using ::std::swap;
            swap(a.body, b.body);
            swap(a.__isset, b.__isset);
        }

        node_create_task_req::node_create_task_req(const node_create_task_req& other56) {
            body = other56.body;
            __isset = other56.__isset;
        }
        node_create_task_req& node_create_task_req::operator=(const node_create_task_req& other57) {
            body = other57.body;
            __isset = other57.__isset;
            return *this;
        }
        void node_create_task_req::printTo(std::ostream& out) const {
            using ::apache::thrift::to_string;
            out << "node_create_task_req(";
            out << "body=" << to_string(body);
            out << ")";
        }


        node_start_task_req_body::~node_start_task_req_body() throw() {
        }


        void node_start_task_req_body::__set_task_id(const std::string& val) {
            this->task_id = val;
        }

        void node_start_task_req_body::__set_select_mode(const int8_t val) {
            this->select_mode = val;
        }

        void node_start_task_req_body::__set_master(const std::string& val) {
            this->master = val;
            __isset.master = true;
        }

        void node_start_task_req_body::__set_peer_nodes_list(const std::vector<std::string> & val) {
            this->peer_nodes_list = val;
        }

        void node_start_task_req_body::__set_server_specification(const std::string& val) {
            this->server_specification = val;
            __isset.server_specification = true;
        }

        void node_start_task_req_body::__set_server_count(const int32_t val) {
            this->server_count = val;
            __isset.server_count = true;
        }

        void node_start_task_req_body::__set_training_engine(const std::string& val) {
            this->training_engine = val;
        }

        void node_start_task_req_body::__set_code_dir(const std::string& val) {
            this->code_dir = val;
        }

        void node_start_task_req_body::__set_entry_file(const std::string& val) {
            this->entry_file = val;
        }

        void node_start_task_req_body::__set_data_dir(const std::string& val) {
            this->data_dir = val;
        }

        void node_start_task_req_body::__set_checkpoint_dir(const std::string& val) {
            this->checkpoint_dir = val;
        }

        void node_start_task_req_body::__set_hyper_parameters(const std::string& val) {
            this->hyper_parameters = val;
            __isset.hyper_parameters = true;
        }

        void node_start_task_req_body::__set_container_name(const std::string& val) {
            this->container_name = val;
            __isset.container_name = true;
        }

        void node_start_task_req_body::__set_memory(const int64_t val) {
            this->memory = val;
            __isset.memory = true;
        }

        void node_start_task_req_body::__set_memory_swap(const int64_t val) {
            this->memory_swap = val;
            __isset.memory_swap = true;
        }
        std::ostream& operator<<(std::ostream& out, const node_start_task_req_body& obj)
        {
            obj.printTo(out);
            return out;
        }


        uint32_t node_start_task_req_body::read(::apache::thrift::protocol::TProtocol* iprot) {

            ::apache::thrift::protocol::TInputRecursionTracker tracker(*iprot);
            uint32_t xfer = 0;
            std::string fname;
            ::apache::thrift::protocol::TType ftype;
            int16_t fid;

            xfer += iprot->readStructBegin(fname);

            using ::apache::thrift::protocol::TProtocolException;

            bool isset_task_id = false;
            bool isset_select_mode = false;
            bool isset_peer_nodes_list = false;
            bool isset_training_engine = false;
            bool isset_code_dir = false;
            bool isset_entry_file = false;
            bool isset_data_dir = false;
            bool isset_checkpoint_dir = false;

            while (true)
            {
                xfer += iprot->readFieldBegin(fname, ftype, fid);
                if (ftype == ::apache::thrift::protocol::T_STOP) {
                    break;
                }
                switch (fid)
                {
                    case 1:
                        if (ftype == ::apache::thrift::protocol::T_STRING) {
                            xfer += iprot->readString(this->task_id);
                            isset_task_id = true;
                        } else {
                            xfer += iprot->skip(ftype);
                        }
                        break;
                    case 2:
                        if (ftype == ::apache::thrift::protocol::T_BYTE) {
                            xfer += iprot->readByte(this->select_mode);
                            isset_select_mode = true;
                        } else {
                            xfer += iprot->skip(ftype);
                        }
                        break;
                    case 3:
                        if (ftype == ::apache::thrift::protocol::T_STRING) {
                            xfer += iprot->readString(this->master);
                            this->__isset.master = true;
                        } else {
                            xfer += iprot->skip(ftype);
                        }
                        break;
                    case 4:
                        if (ftype == ::apache::thrift::protocol::T_LIST) {
                            {
                                this->peer_nodes_list.clear();
                                uint32_t _size58;
                                ::apache::thrift::protocol::TType _etype61;
                                xfer += iprot->readListBegin(_etype61, _size58);
                                this->peer_nodes_list.resize(_size58);
                                uint32_t _i62;
                                for (_i62 = 0; _i62 < _size58; ++_i62)
                                {
                                    xfer += iprot->readString(this->peer_nodes_list[_i62]);
                                }
                                xfer += iprot->readListEnd();
                            }
                            isset_peer_nodes_list = true;
                        } else {
                            xfer += iprot->skip(ftype);
                        }
                        break;
                    case 5:
                        if (ftype == ::apache::thrift::protocol::T_STRING) {
                            xfer += iprot->readString(this->server_specification);
                            this->__isset.server_specification = true;
                        } else {
                            xfer += iprot->skip(ftype);
                        }
                        break;
                    case 6:
                        if (ftype == ::apache::thrift::protocol::T_I32) {
                            xfer += iprot->readI32(this->server_count);
                            this->__isset.server_count = true;
                        } else {
                            xfer += iprot->skip(ftype);
                        }
                        break;
                    case 7:
                        if (ftype == ::apache::thrift::protocol::T_STRING) {
                            xfer += iprot->readString(this->training_engine);
                            isset_training_engine = true;
                        } else {
                            xfer += iprot->skip(ftype);
                        }
                        break;
                    case 8:
                        if (ftype == ::apache::thrift::protocol::T_STRING) {
                            xfer += iprot->readString(this->code_dir);
                            isset_code_dir = true;
                        } else {
                            xfer += iprot->skip(ftype);
                        }
                        break;
                    case 9:
                        if (ftype == ::apache::thrift::protocol::T_STRING) {
                            xfer += iprot->readString(this->entry_file);
                            isset_entry_file = true;
                        } else {
                            xfer += iprot->skip(ftype);
                        }
                        break;
                    case 10:
                        if (ftype == ::apache::thrift::protocol::T_STRING) {
                            xfer += iprot->readString(this->data_dir);
                            isset_data_dir = true;
                        } else {
                            xfer += iprot->skip(ftype);
                        }
                        break;
                    case 11:
                        if (ftype == ::apache::thrift::protocol::T_STRING) {
                            xfer += iprot->readString(this->checkpoint_dir);
                            isset_checkpoint_dir = true;
                        } else {
                            xfer += iprot->skip(ftype);
                        }
                        break;
                    case 12:
                        if (ftype == ::apache::thrift::protocol::T_STRING) {
                            xfer += iprot->readString(this->hyper_parameters);
                            this->__isset.hyper_parameters = true;
                        } else {
                            xfer += iprot->skip(ftype);
                        }
                        break;
                    case 13:
                        if (ftype == ::apache::thrift::protocol::T_STRING) {
                            xfer += iprot->readString(this->container_name);
                            this->__isset.container_name = true;
                        } else {
                            xfer += iprot->skip(ftype);
                        }
                        break;
                    case 14:
                        if (ftype == ::apache::thrift::protocol::T_I64) {
                            xfer += iprot->readI64(this->memory);
                            this->__isset.memory = true;
                        } else {
                            xfer += iprot->skip(ftype);
                        }
                        break;
                    case 15:
                        if (ftype == ::apache::thrift::protocol::T_I64) {
                            xfer += iprot->readI64(this->memory_swap);
                            this->__isset.memory_swap = true;
                        } else {
                            xfer += iprot->skip(ftype);
                        }
                        break;
                    default:
                        xfer += iprot->skip(ftype);
                        break;
                }
                xfer += iprot->readFieldEnd();
            }

            xfer += iprot->readStructEnd();

            if (!isset_task_id)
                throw TProtocolException(TProtocolException::INVALID_DATA);
            if (!isset_select_mode)
                throw TProtocolException(TProtocolException::INVALID_DATA);
            if (!isset_peer_nodes_list)
                throw TProtocolException(TProtocolException::INVALID_DATA);
            if (!isset_training_engine)
                throw TProtocolException(TProtocolException::INVALID_DATA);
            if (!isset_code_dir)
                throw TProtocolException(TProtocolException::INVALID_DATA);
            if (!isset_entry_file)
                throw TProtocolException(TProtocolException::INVALID_DATA);
            if (!isset_data_dir)
                throw TProtocolException(TProtocolException::INVALID_DATA);
            if (!isset_checkpoint_dir)
                throw TProtocolException(TProtocolException::INVALID_DATA);
            return xfer;
        }

        uint32_t node_start_task_req_body::write(::apache::thrift::protocol::TProtocol* oprot) const {
            uint32_t xfer = 0;
            ::apache::thrift::protocol::TOutputRecursionTracker tracker(*oprot);
            xfer += oprot->writeStructBegin("node_start_task_req_body");

            xfer += oprot->writeFieldBegin("task_id", ::apache::thrift::protocol::T_STRING, 1);
            xfer += oprot->writeString(this->task_id);
            xfer += oprot->writeFieldEnd();

            xfer += oprot->writeFieldBegin("select_mode", ::apache::thrift::protocol::T_BYTE, 2);
            xfer += oprot->writeByte(this->select_mode);
            xfer += oprot->writeFieldEnd();

            if (this->__isset.master) {
                xfer += oprot->writeFieldBegin("master", ::apache::thrift::protocol::T_STRING, 3);
                xfer += oprot->writeString(this->master);
                xfer += oprot->writeFieldEnd();
            }
            xfer += oprot->writeFieldBegin("peer_nodes_list", ::apache::thrift::protocol::T_LIST, 4);
            {
                xfer += oprot->writeListBegin(::apache::thrift::protocol::T_STRING, static_cast<uint32_t>(this->peer_nodes_list.size()));
                std::vector<std::string> ::const_iterator _iter63;
                for (_iter63 = this->peer_nodes_list.begin(); _iter63 != this->peer_nodes_list.end(); ++_iter63)
                {
                    xfer += oprot->writeString((*_iter63));
                }
                xfer += oprot->writeListEnd();
            }
            xfer += oprot->writeFieldEnd();

            if (this->__isset.server_specification) {
                xfer += oprot->writeFieldBegin("server_specification", ::apache::thrift::protocol::T_STRING, 5);
                xfer += oprot->writeString(this->server_specification);
                xfer += oprot->writeFieldEnd();
            }
            if (this->__isset.server_count) {
                xfer += oprot->writeFieldBegin("server_count", ::apache::thrift::protocol::T_I32, 6);
                xfer += oprot->writeI32(this->server_count);
                xfer += oprot->writeFieldEnd();
            }
            xfer += oprot->writeFieldBegin("training_engine", ::apache::thrift::protocol::T_STRING, 7);
            xfer += oprot->writeString(this->training_engine);
            xfer += oprot->writeFieldEnd();

            xfer += oprot->writeFieldBegin("code_dir", ::apache::thrift::protocol::T_STRING, 8);
            xfer += oprot->writeString(this->code_dir);
            xfer += oprot->writeFieldEnd();

            xfer += oprot->writeFieldBegin("entry_file", ::apache::thrift::protocol::T_STRING, 9);
            xfer += oprot->writeString(this->entry_file);
            xfer += oprot->writeFieldEnd();

            xfer += oprot->writeFieldBegin("data_dir", ::apache::thrift::protocol::T_STRING, 10);
            xfer += oprot->writeString(this->data_dir);
            xfer += oprot->writeFieldEnd();

            xfer += oprot->writeFieldBegin("checkpoint_dir", ::apache::thrift::protocol::T_STRING, 11);
            xfer += oprot->writeString(this->checkpoint_dir);
            xfer += oprot->writeFieldEnd();

            if (this->__isset.hyper_parameters) {
                xfer += oprot->writeFieldBegin("hyper_parameters", ::apache::thrift::protocol::T_STRING, 12);
                xfer += oprot->writeString(this->hyper_parameters);
                xfer += oprot->writeFieldEnd();
            }
            if (this->__isset.container_name) {
                xfer += oprot->writeFieldBegin("container_name", ::apache::thrift::protocol::T_STRING, 13);
                xfer += oprot->writeString(this->container_name);
                xfer += oprot->writeFieldEnd();
            }
            if (this->__isset.memory) {
                xfer += oprot->writeFieldBegin("memory", ::apache::thrift::protocol::T_I64, 14);
                xfer += oprot->writeI64(this->memory);
                xfer += oprot->writeFieldEnd();
            }
            if (this->__isset.memory_swap) {
                xfer += oprot->writeFieldBegin("memory_swap", ::apache::thrift::protocol::T_I64, 15);
                xfer += oprot->writeI64(this->memory_swap);
                xfer += oprot->writeFieldEnd();
            }
            xfer += oprot->writeFieldStop();
            xfer += oprot->writeStructEnd();
            return xfer;
        }

        void swap(node_start_task_req_body &a, node_start_task_req_body &b) {
            using ::std::swap;
            swap(a.task_id, b.task_id);
            swap(a.select_mode, b.select_mode);
            swap(a.master, b.master);
            swap(a.peer_nodes_list, b.peer_nodes_list);
            swap(a.server_specification, b.server_specification);
            swap(a.server_count, b.server_count);
            swap(a.training_engine, b.training_engine);
            swap(a.code_dir, b.code_dir);
            swap(a.entry_file, b.entry_file);
            swap(a.data_dir, b.data_dir);
            swap(a.checkpoint_dir, b.checkpoint_dir);
            swap(a.hyper_parameters, b.hyper_parameters);
            swap(a.container_name, b.container_name);
            swap(a.memory, b.memory);
            swap(a.memory_swap, b.memory_swap);
            swap(a.__isset, b.__isset);
        }

        node_start_task_req_body::node_start_task_req_body(const node_start_task_req_body& other64) {
            task_id = other64.task_id;
            select_mode = other64.select_mode;
            master = other64.master;
            peer_nodes_list = other64.peer_nodes_list;
            server_specification = other64.server_specification;
            server_count = other64.server_count;
            training_engine = other64.training_engine;
            code_dir = other64.code_dir;
            entry_file = other64.entry_file;
            data_dir = other64.data_dir;
            checkpoint_dir = other64.checkpoint_dir;
            hyper_parameters = other64.hyper_parameters;
            container_name = other64.container_name;
            memory = other64.memory;
            memory_swap = other64.memory_swap;
            __isset = other64.__isset;
        }
        node_start_task_req_body& node_start_task_req_body::operator=(const node_start_task_req_body& other65) {
            task_id = other65.task_id;
            select_mode = other65.select_mode;
            master = other65.master;
            peer_nodes_list = other65.peer_nodes_list;
            server_specification = other65.server_specification;
            server_count = other65.server_count;
            training_engine = other65.training_engine;
            code_dir = other65.code_dir;
            entry_file = other65.entry_file;
            data_dir = other65.data_dir;
            checkpoint_dir = other65.checkpoint_dir;
            hyper_parameters = other65.hyper_parameters;
            container_name = other65.container_name;
            memory = other65.memory;
            memory_swap = other65.memory_swap;
            __isset = other65.__isset;
            return *this;
        }
        void node_start_task_req_body::printTo(std::ostream& out) const {
            using ::apache::thrift::to_string;
            out << "node_start_task_req_body(";
            out << "task_id=" << to_string(task_id);
            out << ", " << "select_mode=" << to_string(select_mode);
            out << ", " << "master="; (__isset.master ? (out << to_string(master)) : (out << "<null>"));
            out << ", " << "peer_nodes_list=" << to_string(peer_nodes_list);
            out << ", " << "server_specification="; (__isset.server_specification ? (out << to_string(server_specification)) : (out << "<null>"));
            out << ", " << "server_count="; (__isset.server_count ? (out << to_string(server_count)) : (out << "<null>"));
            out << ", " << "training_engine=" << to_string(training_engine);
            out << ", " << "code_dir=" << to_string(code_dir);
            out << ", " << "entry_file=" << to_string(entry_file);
            out << ", " << "data_dir=" << to_string(data_dir);
            out << ", " << "checkpoint_dir=" << to_string(checkpoint_dir);
            out << ", " << "hyper_parameters="; (__isset.hyper_parameters ? (out << to_string(hyper_parameters)) : (out << "<null>"));
            out << ", " << "container_name="; (__isset.container_name ? (out << to_string(container_name)) : (out << "<null>"));
            out << ", " << "memory="; (__isset.memory ? (out << to_string(memory)) : (out << "<null>"));
            out << ", " << "memory_swap="; (__isset.memory_swap ? (out << to_string(memory_swap)) : (out << "<null>"));
            out << ")";
        }


        node_start_task_req::~node_start_task_req() throw() {
        }


        void node_start_task_req::__set_body(const node_start_task_req_body& val) {
            this->body = val;
        }
        std::ostream& operator<<(std::ostream& out, const node_start_task_req& obj)
        {
            obj.printTo(out);
            return out;
        }


        uint32_t node_start_task_req::read(::apache::thrift::protocol::TProtocol* iprot) {

            ::apache::thrift::protocol::TInputRecursionTracker tracker(*iprot);
            uint32_t xfer = 0;
            std::string fname;
            ::apache::thrift::protocol::TType ftype;
            int16_t fid;

            xfer += iprot->readStructBegin(fname);

            using ::apache::thrift::protocol::TProtocolException;


            while (true)
            {
                xfer += iprot->readFieldBegin(fname, ftype, fid);
                if (ftype == ::apache::thrift::protocol::T_STOP) {
                    break;
                }
                switch (fid)
                {
                    case 1:
                        if (ftype == ::apache::thrift::protocol::T_STRUCT) {
                            xfer += this->body.read(iprot);
                            this->__isset.body = true;
                        } else {
                            xfer += iprot->skip(ftype);
                        }
                        break;
                    default:
                        xfer += iprot->skip(ftype);
                        break;
                }
                xfer += iprot->readFieldEnd();
            }

            xfer += iprot->readStructEnd();

            return xfer;
        }

        uint32_t node_start_task_req::write(::apache::thrift::protocol::TProtocol* oprot) const {
            uint32_t xfer = 0;
            ::apache::thrift::protocol::TOutputRecursionTracker tracker(*oprot);
            xfer += oprot->writeStructBegin("node_start_task_req");

            xfer += oprot->writeFieldBegin("body", ::apache::thrift::protocol::T_STRUCT, 1);
            xfer += this->body.write(oprot);
            xfer += oprot->writeFieldEnd();

            xfer += oprot->writeFieldStop();
            xfer += oprot->writeStructEnd();
            return xfer;
        }

        void swap(node_start_task_req &a, node_start_task_req &b) {
            using ::std::swap;
            swap(a.body, b.body);
            swap(a.__isset, b.__isset);
        }

        node_start_task_req::node_start_task_req(const node_start_task_req& other66) {
            body = other66.body;
            __isset = other66.__isset;
        }
        node_start_task_req& node_start_task_req::operator=(const node_start_task_req& other67) {
            body = other67.body;
            __isset = other67.__isset;
            return *this;
        }
        void node_start_task_req::printTo(std::ostream& out) const {
            using ::apache::thrift::to_string;
            out << "node_start_task_req(";
            out << "body=" << to_string(body);
            out << ")";
        }


        node_restart_task_req_body::~node_restart_task_req_body() throw() {
        }


        void node_restart_task_req_body::__set_task_id(const std::string& val) {
            this->task_id = val;
        }

        void node_restart_task_req_body::__set_select_mode(const int8_t val) {
            this->select_mode = val;
        }

        void node_restart_task_req_body::__set_master(const std::string& val) {
            this->master = val;
            __isset.master = true;
        }

        void node_restart_task_req_body::__set_peer_nodes_list(const std::vector<std::string> & val) {
            this->peer_nodes_list = val;
        }

        void node_restart_task_req_body::__set_server_specification(const std::string& val) {
            this->server_specification = val;
            __isset.server_specification = true;
        }

        void node_restart_task_req_body::__set_server_count(const int32_t val) {
            this->server_count = val;
            __isset.server_count = true;
        }

        void node_restart_task_req_body::__set_training_engine(const std::string& val) {
            this->training_engine = val;
        }

        void node_restart_task_req_body::__set_code_dir(const std::string& val) {
            this->code_dir = val;
        }

        void node_restart_task_req_body::__set_entry_file(const std::string& val) {
            this->entry_file = val;
        }

        void node_restart_task_req_body::__set_data_dir(const std::string& val) {
            this->data_dir = val;
        }

        void node_restart_task_req_body::__set_checkpoint_dir(const std::string& val) {
            this->checkpoint_dir = val;
        }

        void node_restart_task_req_body::__set_hyper_parameters(const std::string& val) {
            this->hyper_parameters = val;
            __isset.hyper_parameters = true;
        }

        void node_restart_task_req_body::__set_container_name(const std::string& val) {
            this->container_name = val;
            __isset.container_name = true;
        }

        void node_restart_task_req_body::__set_memory(const int64_t val) {
            this->memory = val;
            __isset.memory = true;
        }

        void node_restart_task_req_body::__set_memory_swap(const int64_t val) {
            this->memory_swap = val;
            __isset.memory_swap = true;
        }
        std::ostream& operator<<(std::ostream& out, const node_restart_task_req_body& obj)
        {
            obj.printTo(out);
            return out;
        }


        uint32_t node_restart_task_req_body::read(::apache::thrift::protocol::TProtocol* iprot) {

            ::apache::thrift::protocol::TInputRecursionTracker tracker(*iprot);
            uint32_t xfer = 0;
            std::string fname;
            ::apache::thrift::protocol::TType ftype;
            int16_t fid;

            xfer += iprot->readStructBegin(fname);

            using ::apache::thrift::protocol::TProtocolException;

            bool isset_task_id = false;
            bool isset_select_mode = false;
            bool isset_peer_nodes_list = false;
            bool isset_training_engine = false;
            bool isset_code_dir = false;
            bool isset_entry_file = false;
            bool isset_data_dir = false;
            bool isset_checkpoint_dir = false;

            while (true)
            {
                xfer += iprot->readFieldBegin(fname, ftype, fid);
                if (ftype == ::apache::thrift::protocol::T_STOP) {
                    break;
                }
                switch (fid)
                {
                    case 1:
                        if (ftype == ::apache::thrift::protocol::T_STRING) {
                            xfer += iprot->readString(this->task_id);
                            isset_task_id = true;
                        } else {
                            xfer += iprot->skip(ftype);
                        }
                        break;
                    case 2:
                        if (ftype == ::apache::thrift::protocol::T_BYTE) {
                            xfer += iprot->readByte(this->select_mode);
                            isset_select_mode = true;
                        } else {
                            xfer += iprot->skip(ftype);
                        }
                        break;
                    case 3:
                        if (ftype == ::apache::thrift::protocol::T_STRING) {
                            xfer += iprot->readString(this->master);
                            this->__isset.master = true;
                        } else {
                            xfer += iprot->skip(ftype);
                        }
                        break;
                    case 4:
                        if (ftype == ::apache::thrift::protocol::T_LIST) {
                            {
                                this->peer_nodes_list.clear();
                                uint32_t _size68;
                                ::apache::thrift::protocol::TType _etype71;
                                xfer += iprot->readListBegin(_etype71, _size68);
                                this->peer_nodes_list.resize(_size68);
                                uint32_t _i72;
                                for (_i72 = 0; _i72 < _size68; ++_i72)
                                {
                                    xfer += iprot->readString(this->peer_nodes_list[_i72]);
                                }
                                xfer += iprot->readListEnd();
                            }
                            isset_peer_nodes_list = true;
                        } else {
                            xfer += iprot->skip(ftype);
                        }
                        break;
                    case 5:
                        if (ftype == ::apache::thrift::protocol::T_STRING) {
                            xfer += iprot->readString(this->server_specification);
                            this->__isset.server_specification = true;
                        } else {
                            xfer += iprot->skip(ftype);
                        }
                        break;
                    case 6:
                        if (ftype == ::apache::thrift::protocol::T_I32) {
                            xfer += iprot->readI32(this->server_count);
                            this->__isset.server_count = true;
                        } else {
                            xfer += iprot->skip(ftype);
                        }
                        break;
                    case 7:
                        if (ftype == ::apache::thrift::protocol::T_STRING) {
                            xfer += iprot->readString(this->training_engine);
                            isset_training_engine = true;
                        } else {
                            xfer += iprot->skip(ftype);
                        }
                        break;
                    case 8:
                        if (ftype == ::apache::thrift::protocol::T_STRING) {
                            xfer += iprot->readString(this->code_dir);
                            isset_code_dir = true;
                        } else {
                            xfer += iprot->skip(ftype);
                        }
                        break;
                    case 9:
                        if (ftype == ::apache::thrift::protocol::T_STRING) {
                            xfer += iprot->readString(this->entry_file);
                            isset_entry_file = true;
                        } else {
                            xfer += iprot->skip(ftype);
                        }
                        break;
                    case 10:
                        if (ftype == ::apache::thrift::protocol::T_STRING) {
                            xfer += iprot->readString(this->data_dir);
                            isset_data_dir = true;
                        } else {
                            xfer += iprot->skip(ftype);
                        }
                        break;
                    case 11:
                        if (ftype == ::apache::thrift::protocol::T_STRING) {
                            xfer += iprot->readString(this->checkpoint_dir);
                            isset_checkpoint_dir = true;
                        } else {
                            xfer += iprot->skip(ftype);
                        }
                        break;
                    case 12:
                        if (ftype == ::apache::thrift::protocol::T_STRING) {
                            xfer += iprot->readString(this->hyper_parameters);
                            this->__isset.hyper_parameters = true;
                        } else {
                            xfer += iprot->skip(ftype);
                        }
                        break;
                    case 13:
                        if (ftype == ::apache::thrift::protocol::T_STRING) {
                            xfer += iprot->readString(this->container_name);
                            this->__isset.container_name = true;
                        } else {
                            xfer += iprot->skip(ftype);
                        }
                        break;
                    case 14:
                        if (ftype == ::apache::thrift::protocol::T_I64) {
                            xfer += iprot->readI64(this->memory);
                            this->__isset.memory = true;
                        } else {
                            xfer += iprot->skip(ftype);
                        }
                        break;
                    case 15:
                        if (ftype == ::apache::thrift::protocol::T_I64) {
                            xfer += iprot->readI64(this->memory_swap);
                            this->__isset.memory_swap = true;
                        } else {
                            xfer += iprot->skip(ftype);
                        }
                        break;
                    default:
                        xfer += iprot->skip(ftype);
                        break;
                }
                xfer += iprot->readFieldEnd();
            }

            xfer += iprot->readStructEnd();

            if (!isset_task_id)
                throw TProtocolException(TProtocolException::INVALID_DATA);
            if (!isset_select_mode)
                throw TProtocolException(TProtocolException::INVALID_DATA);
            if (!isset_peer_nodes_list)
                throw TProtocolException(TProtocolException::INVALID_DATA);
            if (!isset_training_engine)
                throw TProtocolException(TProtocolException::INVALID_DATA);
            if (!isset_code_dir)
                throw TProtocolException(TProtocolException::INVALID_DATA);
            if (!isset_entry_file)
                throw TProtocolException(TProtocolException::INVALID_DATA);
            if (!isset_data_dir)
                throw TProtocolException(TProtocolException::INVALID_DATA);
            if (!isset_checkpoint_dir)
                throw TProtocolException(TProtocolException::INVALID_DATA);
            return xfer;
        }

        uint32_t node_restart_task_req_body::write(::apache::thrift::protocol::TProtocol* oprot) const {
            uint32_t xfer = 0;
            ::apache::thrift::protocol::TOutputRecursionTracker tracker(*oprot);
            xfer += oprot->writeStructBegin("node_restart_task_req_body");

            xfer += oprot->writeFieldBegin("task_id", ::apache::thrift::protocol::T_STRING, 1);
            xfer += oprot->writeString(this->task_id);
            xfer += oprot->writeFieldEnd();

            xfer += oprot->writeFieldBegin("select_mode", ::apache::thrift::protocol::T_BYTE, 2);
            xfer += oprot->writeByte(this->select_mode);
            xfer += oprot->writeFieldEnd();

            if (this->__isset.master) {
                xfer += oprot->writeFieldBegin("master", ::apache::thrift::protocol::T_STRING, 3);
                xfer += oprot->writeString(this->master);
                xfer += oprot->writeFieldEnd();
            }
            xfer += oprot->writeFieldBegin("peer_nodes_list", ::apache::thrift::protocol::T_LIST, 4);
            {
                xfer += oprot->writeListBegin(::apache::thrift::protocol::T_STRING, static_cast<uint32_t>(this->peer_nodes_list.size()));
                std::vector<std::string> ::const_iterator _iter73;
                for (_iter73 = this->peer_nodes_list.begin(); _iter73 != this->peer_nodes_list.end(); ++_iter73)
                {
                    xfer += oprot->writeString((*_iter73));
                }
                xfer += oprot->writeListEnd();
            }
            xfer += oprot->writeFieldEnd();

            if (this->__isset.server_specification) {
                xfer += oprot->writeFieldBegin("server_specification", ::apache::thrift::protocol::T_STRING, 5);
                xfer += oprot->writeString(this->server_specification);
                xfer += oprot->writeFieldEnd();
            }
            if (this->__isset.server_count) {
                xfer += oprot->writeFieldBegin("server_count", ::apache::thrift::protocol::T_I32, 6);
                xfer += oprot->writeI32(this->server_count);
                xfer += oprot->writeFieldEnd();
            }
            xfer += oprot->writeFieldBegin("training_engine", ::apache::thrift::protocol::T_STRING, 7);
            xfer += oprot->writeString(this->training_engine);
            xfer += oprot->writeFieldEnd();

            xfer += oprot->writeFieldBegin("code_dir", ::apache::thrift::protocol::T_STRING, 8);
            xfer += oprot->writeString(this->code_dir);
            xfer += oprot->writeFieldEnd();

            xfer += oprot->writeFieldBegin("entry_file", ::apache::thrift::protocol::T_STRING, 9);
            xfer += oprot->writeString(this->entry_file);
            xfer += oprot->writeFieldEnd();

            xfer += oprot->writeFieldBegin("data_dir", ::apache::thrift::protocol::T_STRING, 10);
            xfer += oprot->writeString(this->data_dir);
            xfer += oprot->writeFieldEnd();

            xfer += oprot->writeFieldBegin("checkpoint_dir", ::apache::thrift::protocol::T_STRING, 11);
            xfer += oprot->writeString(this->checkpoint_dir);
            xfer += oprot->writeFieldEnd();

            if (this->__isset.hyper_parameters) {
                xfer += oprot->writeFieldBegin("hyper_parameters", ::apache::thrift::protocol::T_STRING, 12);
                xfer += oprot->writeString(this->hyper_parameters);
                xfer += oprot->writeFieldEnd();
            }
            if (this->__isset.container_name) {
                xfer += oprot->writeFieldBegin("container_name", ::apache::thrift::protocol::T_STRING, 13);
                xfer += oprot->writeString(this->container_name);
                xfer += oprot->writeFieldEnd();
            }
            if (this->__isset.memory) {
                xfer += oprot->writeFieldBegin("memory", ::apache::thrift::protocol::T_I64, 14);
                xfer += oprot->writeI64(this->memory);
                xfer += oprot->writeFieldEnd();
            }
            if (this->__isset.memory_swap) {
                xfer += oprot->writeFieldBegin("memory_swap", ::apache::thrift::protocol::T_I64, 15);
                xfer += oprot->writeI64(this->memory_swap);
                xfer += oprot->writeFieldEnd();
            }
            xfer += oprot->writeFieldStop();
            xfer += oprot->writeStructEnd();
            return xfer;
        }

        void swap(node_restart_task_req_body &a, node_restart_task_req_body &b) {
            using ::std::swap;
            swap(a.task_id, b.task_id);
            swap(a.select_mode, b.select_mode);
            swap(a.master, b.master);
            swap(a.peer_nodes_list, b.peer_nodes_list);
            swap(a.server_specification, b.server_specification);
            swap(a.server_count, b.server_count);
            swap(a.training_engine, b.training_engine);
            swap(a.code_dir, b.code_dir);
            swap(a.entry_file, b.entry_file);
            swap(a.data_dir, b.data_dir);
            swap(a.checkpoint_dir, b.checkpoint_dir);
            swap(a.hyper_parameters, b.hyper_parameters);
            swap(a.container_name, b.container_name);
            swap(a.memory, b.memory);
            swap(a.memory_swap, b.memory_swap);
            swap(a.__isset, b.__isset);
        }

        node_restart_task_req_body::node_restart_task_req_body(const node_restart_task_req_body& other74) {
            task_id = other74.task_id;
            select_mode = other74.select_mode;
            master = other74.master;
            peer_nodes_list = other74.peer_nodes_list;
            server_specification = other74.server_specification;
            server_count = other74.server_count;
            training_engine = other74.training_engine;
            code_dir = other74.code_dir;
            entry_file = other74.entry_file;
            data_dir = other74.data_dir;
            checkpoint_dir = other74.checkpoint_dir;
            hyper_parameters = other74.hyper_parameters;
            container_name = other74.container_name;
            memory = other74.memory;
            memory_swap = other74.memory_swap;
            __isset = other74.__isset;
        }
        node_restart_task_req_body& node_restart_task_req_body::operator=(const node_restart_task_req_body& other75) {
            task_id = other75.task_id;
            select_mode = other75.select_mode;
            master = other75.master;
            peer_nodes_list = other75.peer_nodes_list;
            server_specification = other75.server_specification;
            server_count = other75.server_count;
            training_engine = other75.training_engine;
            code_dir = other75.code_dir;
            entry_file = other75.entry_file;
            data_dir = other75.data_dir;
            checkpoint_dir = other75.checkpoint_dir;
            hyper_parameters = other75.hyper_parameters;
            container_name = other75.container_name;
            memory = other75.memory;
            memory_swap = other75.memory_swap;
            __isset = other75.__isset;
            return *this;
        }
        void node_restart_task_req_body::printTo(std::ostream& out) const {
            using ::apache::thrift::to_string;
            out << "node_restart_task_req_body(";
            out << "task_id=" << to_string(task_id);
            out << ", " << "select_mode=" << to_string(select_mode);
            out << ", " << "master="; (__isset.master ? (out << to_string(master)) : (out << "<null>"));
            out << ", " << "peer_nodes_list=" << to_string(peer_nodes_list);
            out << ", " << "server_specification="; (__isset.server_specification ? (out << to_string(server_specification)) : (out << "<null>"));
            out << ", " << "server_count="; (__isset.server_count ? (out << to_string(server_count)) : (out << "<null>"));
            out << ", " << "training_engine=" << to_string(training_engine);
            out << ", " << "code_dir=" << to_string(code_dir);
            out << ", " << "entry_file=" << to_string(entry_file);
            out << ", " << "data_dir=" << to_string(data_dir);
            out << ", " << "checkpoint_dir=" << to_string(checkpoint_dir);
            out << ", " << "hyper_parameters="; (__isset.hyper_parameters ? (out << to_string(hyper_parameters)) : (out << "<null>"));
            out << ", " << "container_name="; (__isset.container_name ? (out << to_string(container_name)) : (out << "<null>"));
            out << ", " << "memory="; (__isset.memory ? (out << to_string(memory)) : (out << "<null>"));
            out << ", " << "memory_swap="; (__isset.memory_swap ? (out << to_string(memory_swap)) : (out << "<null>"));
            out << ")";
        }


        node_restart_task_req::~node_restart_task_req() throw() {
        }


        void node_restart_task_req::__set_body(const node_restart_task_req_body& val) {
            this->body = val;
        }
        std::ostream& operator<<(std::ostream& out, const node_restart_task_req& obj)
        {
            obj.printTo(out);
            return out;
        }


        uint32_t node_restart_task_req::read(::apache::thrift::protocol::TProtocol* iprot) {

            ::apache::thrift::protocol::TInputRecursionTracker tracker(*iprot);
            uint32_t xfer = 0;
            std::string fname;
            ::apache::thrift::protocol::TType ftype;
            int16_t fid;

            xfer += iprot->readStructBegin(fname);

            using ::apache::thrift::protocol::TProtocolException;


            while (true)
            {
                xfer += iprot->readFieldBegin(fname, ftype, fid);
                if (ftype == ::apache::thrift::protocol::T_STOP) {
                    break;
                }
                switch (fid)
                {
                    case 1:
                        if (ftype == ::apache::thrift::protocol::T_STRUCT) {
                            xfer += this->body.read(iprot);
                            this->__isset.body = true;
                        } else {
                            xfer += iprot->skip(ftype);
                        }
                        break;
                    default:
                        xfer += iprot->skip(ftype);
                        break;
                }
                xfer += iprot->readFieldEnd();
            }

            xfer += iprot->readStructEnd();

            return xfer;
        }

        uint32_t node_restart_task_req::write(::apache::thrift::protocol::TProtocol* oprot) const {
            uint32_t xfer = 0;
            ::apache::thrift::protocol::TOutputRecursionTracker tracker(*oprot);
            xfer += oprot->writeStructBegin("node_restart_task_req");

            xfer += oprot->writeFieldBegin("body", ::apache::thrift::protocol::T_STRUCT, 1);
            xfer += this->body.write(oprot);
            xfer += oprot->writeFieldEnd();

            xfer += oprot->writeFieldStop();
            xfer += oprot->writeStructEnd();
            return xfer;
        }

        void swap(node_restart_task_req &a, node_restart_task_req &b) {
            using ::std::swap;
            swap(a.body, b.body);
            swap(a.__isset, b.__isset);
        }

        node_restart_task_req::node_restart_task_req(const node_restart_task_req& other76) {
            body = other76.body;
            __isset = other76.__isset;
        }
        node_restart_task_req& node_restart_task_req::operator=(const node_restart_task_req& other77) {
            body = other77.body;
            __isset = other77.__isset;
            return *this;
        }
        void node_restart_task_req::printTo(std::ostream& out) const {
            using ::apache::thrift::to_string;
            out << "node_restart_task_req(";
            out << "body=" << to_string(body);
            out << ")";
        }


        node_stop_task_req_body::~node_stop_task_req_body() throw() {
        }


        void node_stop_task_req_body::__set_task_id(const std::string& val) {
            this->task_id = val;
        }
        std::ostream& operator<<(std::ostream& out, const node_stop_task_req_body& obj)
        {
            obj.printTo(out);
            return out;
        }


        uint32_t node_stop_task_req_body::read(::apache::thrift::protocol::TProtocol* iprot) {

            ::apache::thrift::protocol::TInputRecursionTracker tracker(*iprot);
            uint32_t xfer = 0;
            std::string fname;
            ::apache::thrift::protocol::TType ftype;
            int16_t fid;

            xfer += iprot->readStructBegin(fname);

            using ::apache::thrift::protocol::TProtocolException;

            bool isset_task_id = false;

            while (true)
            {
                xfer += iprot->readFieldBegin(fname, ftype, fid);
                if (ftype == ::apache::thrift::protocol::T_STOP) {
                    break;
                }
                switch (fid)
                {
                    case 1:
                        if (ftype == ::apache::thrift::protocol::T_STRING) {
                            xfer += iprot->readString(this->task_id);
                            isset_task_id = true;
                        } else {
                            xfer += iprot->skip(ftype);
                        }
                        break;
                    default:
                        xfer += iprot->skip(ftype);
                        break;
                }
                xfer += iprot->readFieldEnd();
            }

            xfer += iprot->readStructEnd();

            if (!isset_task_id)
                throw TProtocolException(TProtocolException::INVALID_DATA);
            return xfer;
        }

        uint32_t node_stop_task_req_body::write(::apache::thrift::protocol::TProtocol* oprot) const {
            uint32_t xfer = 0;
            ::apache::thrift::protocol::TOutputRecursionTracker tracker(*oprot);
            xfer += oprot->writeStructBegin("node_stop_task_req_body");

            xfer += oprot->writeFieldBegin("task_id", ::apache::thrift::protocol::T_STRING, 1);
            xfer += oprot->writeString(this->task_id);
            xfer += oprot->writeFieldEnd();

            xfer += oprot->writeFieldStop();
            xfer += oprot->writeStructEnd();
            return xfer;
        }

        void swap(node_stop_task_req_body &a, node_stop_task_req_body &b) {
            using ::std::swap;
            swap(a.task_id, b.task_id);
        }

        node_stop_task_req_body::node_stop_task_req_body(const node_stop_task_req_body& other78) {
            task_id = other78.task_id;
        }
        node_stop_task_req_body& node_stop_task_req_body::operator=(const node_stop_task_req_body& other79) {
            task_id = other79.task_id;
            return *this;
        }
        void node_stop_task_req_body::printTo(std::ostream& out) const {
            using ::apache::thrift::to_string;
            out << "node_stop_task_req_body(";
            out << "task_id=" << to_string(task_id);
            out << ")";
        }


        node_stop_task_req::~node_stop_task_req() throw() {
        }


        void node_stop_task_req::__set_body(const node_stop_task_req_body& val) {
            this->body = val;
        }
        std::ostream& operator<<(std::ostream& out, const node_stop_task_req& obj)
        {
            obj.printTo(out);
            return out;
        }


        uint32_t node_stop_task_req::read(::apache::thrift::protocol::TProtocol* iprot) {

            ::apache::thrift::protocol::TInputRecursionTracker tracker(*iprot);
            uint32_t xfer = 0;
            std::string fname;
            ::apache::thrift::protocol::TType ftype;
            int16_t fid;

            xfer += iprot->readStructBegin(fname);

            using ::apache::thrift::protocol::TProtocolException;


            while (true)
            {
                xfer += iprot->readFieldBegin(fname, ftype, fid);
                if (ftype == ::apache::thrift::protocol::T_STOP) {
                    break;
                }
                switch (fid)
                {
                    case 1:
                        if (ftype == ::apache::thrift::protocol::T_STRUCT) {
                            xfer += this->body.read(iprot);
                            this->__isset.body = true;
                        } else {
                            xfer += iprot->skip(ftype);
                        }
                        break;
                    default:
                        xfer += iprot->skip(ftype);
                        break;
                }
                xfer += iprot->readFieldEnd();
            }

            xfer += iprot->readStructEnd();

            return xfer;
        }

        uint32_t node_stop_task_req::write(::apache::thrift::protocol::TProtocol* oprot) const {
            uint32_t xfer = 0;
            ::apache::thrift::protocol::TOutputRecursionTracker tracker(*oprot);
            xfer += oprot->writeStructBegin("node_stop_task_req");

            xfer += oprot->writeFieldBegin("body", ::apache::thrift::protocol::T_STRUCT, 1);
            xfer += this->body.write(oprot);
            xfer += oprot->writeFieldEnd();

            xfer += oprot->writeFieldStop();
            xfer += oprot->writeStructEnd();
            return xfer;
        }

        void swap(node_stop_task_req &a, node_stop_task_req &b) {
            using ::std::swap;
            swap(a.body, b.body);
            swap(a.__isset, b.__isset);
        }

        node_stop_task_req::node_stop_task_req(const node_stop_task_req& other80) {
            body = other80.body;
            __isset = other80.__isset;
        }
        node_stop_task_req& node_stop_task_req::operator=(const node_stop_task_req& other81) {
            body = other81.body;
            __isset = other81.__isset;
            return *this;
        }
        void node_stop_task_req::printTo(std::ostream& out) const {
            using ::apache::thrift::to_string;
            out << "node_stop_task_req(";
            out << "body=" << to_string(body);
            out << ")";
        }


        list_training_req_body::~list_training_req_body() throw() {
        }


        void list_training_req_body::__set_task_list(const std::vector<std::string> &val) {
            this->task_list = val;
        }

        std::ostream &operator<<(std::ostream &out, const list_training_req_body &obj) {
            obj.printTo(out);
            return out;
        }


        uint32_t list_training_req_body::read(::apache::thrift::protocol::TProtocol *iprot) {

            ::apache::thrift::protocol::TInputRecursionTracker tracker(*iprot);
            uint32_t xfer = 0;
            std::string fname;
            ::apache::thrift::protocol::TType ftype;
            int16_t fid;

            xfer += iprot->readStructBegin(fname);

            using ::apache::thrift::protocol::TProtocolException;

            bool isset_task_list = false;

            while (true) {
                xfer += iprot->readFieldBegin(fname, ftype, fid);
                if (ftype == ::apache::thrift::protocol::T_STOP) {
                    break;
                }
                switch (fid) {
                    case 1:
                        if (ftype == ::apache::thrift::protocol::T_LIST) {
                            {
                                this->task_list.clear();
                                uint32_t _size62;
                                ::apache::thrift::protocol::TType _etype65;
                                xfer += iprot->readListBegin(_etype65, _size62);
                                this->task_list.resize(_size62);
                                uint32_t _i66;
                                for (_i66 = 0; _i66 < _size62; ++_i66) {
                                    xfer += iprot->readString(this->task_list[_i66]);
                                }
                                xfer += iprot->readListEnd();
                            }
                            isset_task_list = true;
                        } else {
                            xfer += iprot->skip(ftype);
                        }
                        break;
                    default:
                        xfer += iprot->skip(ftype);
                        break;
                }
                xfer += iprot->readFieldEnd();
            }

            xfer += iprot->readStructEnd();

            if (!isset_task_list)
                throw TProtocolException(TProtocolException::INVALID_DATA);
            return xfer;
        }

        uint32_t list_training_req_body::write(::apache::thrift::protocol::TProtocol *oprot) const {
            uint32_t xfer = 0;
            ::apache::thrift::protocol::TOutputRecursionTracker tracker(*oprot);
            xfer += oprot->writeStructBegin("list_training_req_body");

            xfer += oprot->writeFieldBegin("task_list", ::apache::thrift::protocol::T_LIST, 1);
            {
                xfer += oprot->writeListBegin(::apache::thrift::protocol::T_STRING,
                                              static_cast<uint32_t>(this->task_list.size()));
                std::vector<std::string>::const_iterator _iter67;
                for (_iter67 = this->task_list.begin(); _iter67 != this->task_list.end(); ++_iter67) {
                    xfer += oprot->writeString((*_iter67));
                }
                xfer += oprot->writeListEnd();
            }
            xfer += oprot->writeFieldEnd();

            xfer += oprot->writeFieldStop();
            xfer += oprot->writeStructEnd();
            return xfer;
        }

        void swap(list_training_req_body &a, list_training_req_body &b) {
            using ::std::swap;
            swap(a.task_list, b.task_list);
        }

        list_training_req_body::list_training_req_body(const list_training_req_body &other68) {
            task_list = other68.task_list;
        }

        list_training_req_body &list_training_req_body::operator=(const list_training_req_body &other69) {
            task_list = other69.task_list;
            return *this;
        }

        void list_training_req_body::printTo(std::ostream &out) const {
            using ::apache::thrift::to_string;
            out << "list_training_req_body(";
            out << "task_list=" << to_string(task_list);
            out << ")";
        }


        list_training_req::~list_training_req() throw() {
        }


        void list_training_req::__set_body(const list_training_req_body &val) {
            this->body = val;
        }

        std::ostream &operator<<(std::ostream &out, const list_training_req &obj) {
            obj.printTo(out);
            return out;
        }


        uint32_t list_training_req::read(::apache::thrift::protocol::TProtocol *iprot) {

            ::apache::thrift::protocol::TInputRecursionTracker tracker(*iprot);
            uint32_t xfer = 0;
            std::string fname;
            ::apache::thrift::protocol::TType ftype;
            int16_t fid;

            xfer += iprot->readStructBegin(fname);

            using ::apache::thrift::protocol::TProtocolException;


            while (true) {
                xfer += iprot->readFieldBegin(fname, ftype, fid);
                if (ftype == ::apache::thrift::protocol::T_STOP) {
                    break;
                }
                switch (fid) {
                    case 1:
                        if (ftype == ::apache::thrift::protocol::T_STRUCT) {
                            xfer += this->body.read(iprot);
                            this->__isset.body = true;
                        } else {
                            xfer += iprot->skip(ftype);
                        }
                        break;
                    default:
                        xfer += iprot->skip(ftype);
                        break;
                }
                xfer += iprot->readFieldEnd();
            }

            xfer += iprot->readStructEnd();

            return xfer;
        }

        uint32_t list_training_req::write(::apache::thrift::protocol::TProtocol *oprot) const {
            uint32_t xfer = 0;
            ::apache::thrift::protocol::TOutputRecursionTracker tracker(*oprot);
            xfer += oprot->writeStructBegin("list_training_req");

            xfer += oprot->writeFieldBegin("body", ::apache::thrift::protocol::T_STRUCT, 1);
            xfer += this->body.write(oprot);
            xfer += oprot->writeFieldEnd();

            xfer += oprot->writeFieldStop();
            xfer += oprot->writeStructEnd();
            return xfer;
        }

        void swap(list_training_req &a, list_training_req &b) {
            using ::std::swap;
            swap(a.body, b.body);
            swap(a.__isset, b.__isset);
        }

        list_training_req::list_training_req(const list_training_req &other70) {
            body = other70.body;
            __isset = other70.__isset;
        }

        list_training_req &list_training_req::operator=(const list_training_req &other71) {
            body = other71.body;
            __isset = other71.__isset;
            return *this;
        }

        void list_training_req::printTo(std::ostream &out) const {
            using ::apache::thrift::to_string;
            out << "list_training_req(";
            out << "body=" << to_string(body);
            out << ")";
        }


        list_training_resp_body::~list_training_resp_body() throw() {
        }


        void list_training_resp_body::__set_task_status_list(const std::vector<task_status> &val) {
            this->task_status_list = val;
        }

        std::ostream &operator<<(std::ostream &out, const list_training_resp_body &obj) {
            obj.printTo(out);
            return out;
        }


        uint32_t list_training_resp_body::read(::apache::thrift::protocol::TProtocol *iprot) {

            ::apache::thrift::protocol::TInputRecursionTracker tracker(*iprot);
            uint32_t xfer = 0;
            std::string fname;
            ::apache::thrift::protocol::TType ftype;
            int16_t fid;

            xfer += iprot->readStructBegin(fname);

            using ::apache::thrift::protocol::TProtocolException;

            bool isset_task_status_list = false;

            while (true) {
                xfer += iprot->readFieldBegin(fname, ftype, fid);
                if (ftype == ::apache::thrift::protocol::T_STOP) {
                    break;
                }
                switch (fid) {
                    case 1:
                        if (ftype == ::apache::thrift::protocol::T_LIST) {
                            {
                                this->task_status_list.clear();
                                uint32_t _size72;
                                ::apache::thrift::protocol::TType _etype75;
                                xfer += iprot->readListBegin(_etype75, _size72);
                                this->task_status_list.resize(_size72);
                                uint32_t _i76;
                                for (_i76 = 0; _i76 < _size72; ++_i76) {
                                    xfer += this->task_status_list[_i76].read(iprot);
                                }
                                xfer += iprot->readListEnd();
                            }
                            isset_task_status_list = true;
                        } else {
                            xfer += iprot->skip(ftype);
                        }
                        break;
                    default:
                        xfer += iprot->skip(ftype);
                        break;
                }
                xfer += iprot->readFieldEnd();
            }

            xfer += iprot->readStructEnd();

            if (!isset_task_status_list)
                throw TProtocolException(TProtocolException::INVALID_DATA);
            return xfer;
        }

        uint32_t list_training_resp_body::write(::apache::thrift::protocol::TProtocol *oprot) const {
            uint32_t xfer = 0;
            ::apache::thrift::protocol::TOutputRecursionTracker tracker(*oprot);
            xfer += oprot->writeStructBegin("list_training_resp_body");

            xfer += oprot->writeFieldBegin("task_status_list", ::apache::thrift::protocol::T_LIST, 1);
            {
                xfer += oprot->writeListBegin(::apache::thrift::protocol::T_STRUCT,
                                              static_cast<uint32_t>(this->task_status_list.size()));
                std::vector<task_status>::const_iterator _iter77;
                for (_iter77 = this->task_status_list.begin(); _iter77 != this->task_status_list.end(); ++_iter77) {
                    xfer += (*_iter77).write(oprot);
                }
                xfer += oprot->writeListEnd();
            }
            xfer += oprot->writeFieldEnd();

            xfer += oprot->writeFieldStop();
            xfer += oprot->writeStructEnd();
            return xfer;
        }

        void swap(list_training_resp_body &a, list_training_resp_body &b) {
            using ::std::swap;
            swap(a.task_status_list, b.task_status_list);
        }

        list_training_resp_body::list_training_resp_body(const list_training_resp_body &other78) {
            task_status_list = other78.task_status_list;
        }

        list_training_resp_body &list_training_resp_body::operator=(const list_training_resp_body &other79) {
            task_status_list = other79.task_status_list;
            return *this;
        }

        void list_training_resp_body::printTo(std::ostream &out) const {
            using ::apache::thrift::to_string;
            out << "list_training_resp_body(";
            out << "task_status_list=" << to_string(task_status_list);
            out << ")";
        }


        list_training_resp::~list_training_resp() throw() {
        }


        void list_training_resp::__set_body(const list_training_resp_body &val) {
            this->body = val;
        }

        std::ostream &operator<<(std::ostream &out, const list_training_resp &obj) {
            obj.printTo(out);
            return out;
        }


        uint32_t list_training_resp::read(::apache::thrift::protocol::TProtocol *iprot) {

            ::apache::thrift::protocol::TInputRecursionTracker tracker(*iprot);
            uint32_t xfer = 0;
            std::string fname;
            ::apache::thrift::protocol::TType ftype;
            int16_t fid;

            xfer += iprot->readStructBegin(fname);

            using ::apache::thrift::protocol::TProtocolException;


            while (true) {
                xfer += iprot->readFieldBegin(fname, ftype, fid);
                if (ftype == ::apache::thrift::protocol::T_STOP) {
                    break;
                }
                switch (fid) {
                    case 1:
                        if (ftype == ::apache::thrift::protocol::T_STRUCT) {
                            xfer += this->body.read(iprot);
                            this->__isset.body = true;
                        } else {
                            xfer += iprot->skip(ftype);
                        }
                        break;
                    default:
                        xfer += iprot->skip(ftype);
                        break;
                }
                xfer += iprot->readFieldEnd();
            }

            xfer += iprot->readStructEnd();

            return xfer;
        }

        uint32_t list_training_resp::write(::apache::thrift::protocol::TProtocol *oprot) const {
            uint32_t xfer = 0;
            ::apache::thrift::protocol::TOutputRecursionTracker tracker(*oprot);
            xfer += oprot->writeStructBegin("list_training_resp");

            xfer += oprot->writeFieldBegin("body", ::apache::thrift::protocol::T_STRUCT, 1);
            xfer += this->body.write(oprot);
            xfer += oprot->writeFieldEnd();

            xfer += oprot->writeFieldStop();
            xfer += oprot->writeStructEnd();
            return xfer;
        }

        void swap(list_training_resp &a, list_training_resp &b) {
            using ::std::swap;
            swap(a.body, b.body);
            swap(a.__isset, b.__isset);
        }

        list_training_resp::list_training_resp(const list_training_resp &other80) {
            body = other80.body;
            __isset = other80.__isset;
        }

        list_training_resp &list_training_resp::operator=(const list_training_resp &other81) {
            body = other81.body;
            __isset = other81.__isset;
            return *this;
        }

        void list_training_resp::printTo(std::ostream &out) const {
            using ::apache::thrift::to_string;
            out << "list_training_resp(";
            out << "body=" << to_string(body);
            out << ")";
        }


        logs_req_body::~logs_req_body() throw() {
        }


        void logs_req_body::__set_task_id(const std::string &val) {
            this->task_id = val;
        }

        void logs_req_body::__set_peer_nodes_list(const std::vector<std::string> &val) {
            this->peer_nodes_list = val;
        }

        void logs_req_body::__set_head_or_tail(const int8_t val) {
            this->head_or_tail = val;
        }

        void logs_req_body::__set_number_of_lines(const int16_t val) {
            this->number_of_lines = val;
        }

        std::ostream &operator<<(std::ostream &out, const logs_req_body &obj) {
            obj.printTo(out);
            return out;
        }


        uint32_t logs_req_body::read(::apache::thrift::protocol::TProtocol *iprot) {

            ::apache::thrift::protocol::TInputRecursionTracker tracker(*iprot);
            uint32_t xfer = 0;
            std::string fname;
            ::apache::thrift::protocol::TType ftype;
            int16_t fid;

            xfer += iprot->readStructBegin(fname);

            using ::apache::thrift::protocol::TProtocolException;

            bool isset_task_id = false;
            bool isset_peer_nodes_list = false;
            bool isset_head_or_tail = false;
            bool isset_number_of_lines = false;

            while (true) {
                xfer += iprot->readFieldBegin(fname, ftype, fid);
                if (ftype == ::apache::thrift::protocol::T_STOP) {
                    break;
                }
                switch (fid) {
                    case 1:
                        if (ftype == ::apache::thrift::protocol::T_STRING) {
                            xfer += iprot->readString(this->task_id);
                            isset_task_id = true;
                        } else {
                            xfer += iprot->skip(ftype);
                        }
                        break;
                    case 2:
                        if (ftype == ::apache::thrift::protocol::T_LIST) {
                            {
                                this->peer_nodes_list.clear();
                                uint32_t _size82;
                                ::apache::thrift::protocol::TType _etype85;
                                xfer += iprot->readListBegin(_etype85, _size82);
                                this->peer_nodes_list.resize(_size82);
                                uint32_t _i86;
                                for (_i86 = 0; _i86 < _size82; ++_i86) {
                                    xfer += iprot->readString(this->peer_nodes_list[_i86]);
                                }
                                xfer += iprot->readListEnd();
                            }
                            isset_peer_nodes_list = true;
                        } else {
                            xfer += iprot->skip(ftype);
                        }
                        break;
                    case 3:
                        if (ftype == ::apache::thrift::protocol::T_BYTE) {
                            xfer += iprot->readByte(this->head_or_tail);
                            isset_head_or_tail = true;
                        } else {
                            xfer += iprot->skip(ftype);
                        }
                        break;
                    case 4:
                        if (ftype == ::apache::thrift::protocol::T_I16) {
                            xfer += iprot->readI16(this->number_of_lines);
                            isset_number_of_lines = true;
                        } else {
                            xfer += iprot->skip(ftype);
                        }
                        break;
                    default:
                        xfer += iprot->skip(ftype);
                        break;
                }
                xfer += iprot->readFieldEnd();
            }

            xfer += iprot->readStructEnd();

            if (!isset_task_id)
                throw TProtocolException(TProtocolException::INVALID_DATA);
            if (!isset_peer_nodes_list)
                throw TProtocolException(TProtocolException::INVALID_DATA);
            if (!isset_head_or_tail)
                throw TProtocolException(TProtocolException::INVALID_DATA);
            if (!isset_number_of_lines)
                throw TProtocolException(TProtocolException::INVALID_DATA);
            return xfer;
        }

        uint32_t logs_req_body::write(::apache::thrift::protocol::TProtocol *oprot) const {
            uint32_t xfer = 0;
            ::apache::thrift::protocol::TOutputRecursionTracker tracker(*oprot);
            xfer += oprot->writeStructBegin("logs_req_body");

            xfer += oprot->writeFieldBegin("task_id", ::apache::thrift::protocol::T_STRING, 1);
            xfer += oprot->writeString(this->task_id);
            xfer += oprot->writeFieldEnd();

            xfer += oprot->writeFieldBegin("peer_nodes_list", ::apache::thrift::protocol::T_LIST, 2);
            {
                xfer += oprot->writeListBegin(::apache::thrift::protocol::T_STRING,
                                              static_cast<uint32_t>(this->peer_nodes_list.size()));
                std::vector<std::string>::const_iterator _iter87;
                for (_iter87 = this->peer_nodes_list.begin(); _iter87 != this->peer_nodes_list.end(); ++_iter87) {
                    xfer += oprot->writeString((*_iter87));
                }
                xfer += oprot->writeListEnd();
            }
            xfer += oprot->writeFieldEnd();

            xfer += oprot->writeFieldBegin("head_or_tail", ::apache::thrift::protocol::T_BYTE, 3);
            xfer += oprot->writeByte(this->head_or_tail);
            xfer += oprot->writeFieldEnd();

            xfer += oprot->writeFieldBegin("number_of_lines", ::apache::thrift::protocol::T_I16, 4);
            xfer += oprot->writeI16(this->number_of_lines);
            xfer += oprot->writeFieldEnd();

            xfer += oprot->writeFieldStop();
            xfer += oprot->writeStructEnd();
            return xfer;
        }

        void swap(logs_req_body &a, logs_req_body &b) {
            using ::std::swap;
            swap(a.task_id, b.task_id);
            swap(a.peer_nodes_list, b.peer_nodes_list);
            swap(a.head_or_tail, b.head_or_tail);
            swap(a.number_of_lines, b.number_of_lines);
        }

        logs_req_body::logs_req_body(const logs_req_body &other88) {
            task_id = other88.task_id;
            peer_nodes_list = other88.peer_nodes_list;
            head_or_tail = other88.head_or_tail;
            number_of_lines = other88.number_of_lines;
        }

        logs_req_body &logs_req_body::operator=(const logs_req_body &other89) {
            task_id = other89.task_id;
            peer_nodes_list = other89.peer_nodes_list;
            head_or_tail = other89.head_or_tail;
            number_of_lines = other89.number_of_lines;
            return *this;
        }

        void logs_req_body::printTo(std::ostream &out) const {
            using ::apache::thrift::to_string;
            out << "logs_req_body(";
            out << "task_id=" << to_string(task_id);
            out << ", " << "peer_nodes_list=" << to_string(peer_nodes_list);
            out << ", " << "head_or_tail=" << to_string(head_or_tail);
            out << ", " << "number_of_lines=" << to_string(number_of_lines);
            out << ")";
        }


        logs_req::~logs_req() throw() {
        }


        void logs_req::__set_body(const logs_req_body &val) {
            this->body = val;
        }

        std::ostream &operator<<(std::ostream &out, const logs_req &obj) {
            obj.printTo(out);
            return out;
        }


        uint32_t logs_req::read(::apache::thrift::protocol::TProtocol *iprot) {

            ::apache::thrift::protocol::TInputRecursionTracker tracker(*iprot);
            uint32_t xfer = 0;
            std::string fname;
            ::apache::thrift::protocol::TType ftype;
            int16_t fid;

            xfer += iprot->readStructBegin(fname);

            using ::apache::thrift::protocol::TProtocolException;


            while (true) {
                xfer += iprot->readFieldBegin(fname, ftype, fid);
                if (ftype == ::apache::thrift::protocol::T_STOP) {
                    break;
                }
                switch (fid) {
                    case 1:
                        if (ftype == ::apache::thrift::protocol::T_STRUCT) {
                            xfer += this->body.read(iprot);
                            this->__isset.body = true;
                        } else {
                            xfer += iprot->skip(ftype);
                        }
                        break;
                    default:
                        xfer += iprot->skip(ftype);
                        break;
                }
                xfer += iprot->readFieldEnd();
            }

            xfer += iprot->readStructEnd();

            return xfer;
        }

        uint32_t logs_req::write(::apache::thrift::protocol::TProtocol *oprot) const {
            uint32_t xfer = 0;
            ::apache::thrift::protocol::TOutputRecursionTracker tracker(*oprot);
            xfer += oprot->writeStructBegin("logs_req");

            xfer += oprot->writeFieldBegin("body", ::apache::thrift::protocol::T_STRUCT, 1);
            xfer += this->body.write(oprot);
            xfer += oprot->writeFieldEnd();

            xfer += oprot->writeFieldStop();
            xfer += oprot->writeStructEnd();
            return xfer;
        }

        void swap(logs_req &a, logs_req &b) {
            using ::std::swap;
            swap(a.body, b.body);
            swap(a.__isset, b.__isset);
        }

        logs_req::logs_req(const logs_req &other90) {
            body = other90.body;
            __isset = other90.__isset;
        }

        logs_req &logs_req::operator=(const logs_req &other91) {
            body = other91.body;
            __isset = other91.__isset;
            return *this;
        }

        void logs_req::printTo(std::ostream &out) const {
            using ::apache::thrift::to_string;
            out << "logs_req(";
            out << "body=" << to_string(body);
            out << ")";
        }


        peer_node_log::~peer_node_log() throw() {
        }


        void peer_node_log::__set_peer_node_id(const std::string &val) {
            this->peer_node_id = val;
        }

        void peer_node_log::__set_log_content(const std::string &val) {
            this->log_content = val;
        }

        std::ostream &operator<<(std::ostream &out, const peer_node_log &obj) {
            obj.printTo(out);
            return out;
        }


        uint32_t peer_node_log::read(::apache::thrift::protocol::TProtocol *iprot) {

            ::apache::thrift::protocol::TInputRecursionTracker tracker(*iprot);
            uint32_t xfer = 0;
            std::string fname;
            ::apache::thrift::protocol::TType ftype;
            int16_t fid;

            xfer += iprot->readStructBegin(fname);

            using ::apache::thrift::protocol::TProtocolException;

            bool isset_peer_node_id = false;
            bool isset_log_content = false;

            while (true) {
                xfer += iprot->readFieldBegin(fname, ftype, fid);
                if (ftype == ::apache::thrift::protocol::T_STOP) {
                    break;
                }
                switch (fid) {
                    case 1:
                        if (ftype == ::apache::thrift::protocol::T_STRING) {
                            xfer += iprot->readString(this->peer_node_id);
                            isset_peer_node_id = true;
                        } else {
                            xfer += iprot->skip(ftype);
                        }
                        break;
                    case 2:
                        if (ftype == ::apache::thrift::protocol::T_STRING) {
                            xfer += iprot->readString(this->log_content);
                            isset_log_content = true;
                        } else {
                            xfer += iprot->skip(ftype);
                        }
                        break;
                    default:
                        xfer += iprot->skip(ftype);
                        break;
                }
                xfer += iprot->readFieldEnd();
            }

            xfer += iprot->readStructEnd();

            if (!isset_peer_node_id)
                throw TProtocolException(TProtocolException::INVALID_DATA);
            if (!isset_log_content)
                throw TProtocolException(TProtocolException::INVALID_DATA);
            return xfer;
        }

        uint32_t peer_node_log::write(::apache::thrift::protocol::TProtocol *oprot) const {
            uint32_t xfer = 0;
            ::apache::thrift::protocol::TOutputRecursionTracker tracker(*oprot);
            xfer += oprot->writeStructBegin("peer_node_log");

            xfer += oprot->writeFieldBegin("peer_node_id", ::apache::thrift::protocol::T_STRING, 1);
            xfer += oprot->writeString(this->peer_node_id);
            xfer += oprot->writeFieldEnd();

            xfer += oprot->writeFieldBegin("log_content", ::apache::thrift::protocol::T_STRING, 2);
            xfer += oprot->writeString(this->log_content);
            xfer += oprot->writeFieldEnd();

            xfer += oprot->writeFieldStop();
            xfer += oprot->writeStructEnd();
            return xfer;
        }

        void swap(peer_node_log &a, peer_node_log &b) {
            using ::std::swap;
            swap(a.peer_node_id, b.peer_node_id);
            swap(a.log_content, b.log_content);
        }

        peer_node_log::peer_node_log(const peer_node_log &other92) {
            peer_node_id = other92.peer_node_id;
            log_content = other92.log_content;
        }

        peer_node_log &peer_node_log::operator=(const peer_node_log &other93) {
            peer_node_id = other93.peer_node_id;
            log_content = other93.log_content;
            return *this;
        }

        void peer_node_log::printTo(std::ostream &out) const {
            using ::apache::thrift::to_string;
            out << "peer_node_log(";
            out << "peer_node_id=" << to_string(peer_node_id);
            out << ", " << "log_content=" << to_string(log_content);
            out << ")";
        }


        logs_resp_body::~logs_resp_body() throw() {
        }


        void logs_resp_body::__set_log(const peer_node_log &val) {
            this->log = val;
        }

        std::ostream &operator<<(std::ostream &out, const logs_resp_body &obj) {
            obj.printTo(out);
            return out;
        }


        uint32_t logs_resp_body::read(::apache::thrift::protocol::TProtocol *iprot) {

            ::apache::thrift::protocol::TInputRecursionTracker tracker(*iprot);
            uint32_t xfer = 0;
            std::string fname;
            ::apache::thrift::protocol::TType ftype;
            int16_t fid;

            xfer += iprot->readStructBegin(fname);

            using ::apache::thrift::protocol::TProtocolException;

            bool isset_log = false;

            while (true) {
                xfer += iprot->readFieldBegin(fname, ftype, fid);
                if (ftype == ::apache::thrift::protocol::T_STOP) {
                    break;
                }
                switch (fid) {
                    case 1:
                        if (ftype == ::apache::thrift::protocol::T_STRUCT) {
                            xfer += this->log.read(iprot);
                            isset_log = true;
                        } else {
                            xfer += iprot->skip(ftype);
                        }
                        break;
                    default:
                        xfer += iprot->skip(ftype);
                        break;
                }
                xfer += iprot->readFieldEnd();
            }

            xfer += iprot->readStructEnd();

            if (!isset_log)
                throw TProtocolException(TProtocolException::INVALID_DATA);
            return xfer;
        }

        uint32_t logs_resp_body::write(::apache::thrift::protocol::TProtocol *oprot) const {
            uint32_t xfer = 0;
            ::apache::thrift::protocol::TOutputRecursionTracker tracker(*oprot);
            xfer += oprot->writeStructBegin("logs_resp_body");

            xfer += oprot->writeFieldBegin("log", ::apache::thrift::protocol::T_STRUCT, 1);
            xfer += this->log.write(oprot);
            xfer += oprot->writeFieldEnd();

            xfer += oprot->writeFieldStop();
            xfer += oprot->writeStructEnd();
            return xfer;
        }

        void swap(logs_resp_body &a, logs_resp_body &b) {
            using ::std::swap;
            swap(a.log, b.log);
        }

        logs_resp_body::logs_resp_body(const logs_resp_body &other94) {
            log = other94.log;
        }

        logs_resp_body &logs_resp_body::operator=(const logs_resp_body &other95) {
            log = other95.log;
            return *this;
        }

        void logs_resp_body::printTo(std::ostream &out) const {
            using ::apache::thrift::to_string;
            out << "logs_resp_body(";
            out << "log=" << to_string(log);
            out << ")";
        }


        logs_resp::~logs_resp() throw() {
        }


        void logs_resp::__set_body(const logs_resp_body &val) {
            this->body = val;
        }

        std::ostream &operator<<(std::ostream &out, const logs_resp &obj) {
            obj.printTo(out);
            return out;
        }


        uint32_t logs_resp::read(::apache::thrift::protocol::TProtocol *iprot) {

            ::apache::thrift::protocol::TInputRecursionTracker tracker(*iprot);
            uint32_t xfer = 0;
            std::string fname;
            ::apache::thrift::protocol::TType ftype;
            int16_t fid;

            xfer += iprot->readStructBegin(fname);

            using ::apache::thrift::protocol::TProtocolException;


            while (true) {
                xfer += iprot->readFieldBegin(fname, ftype, fid);
                if (ftype == ::apache::thrift::protocol::T_STOP) {
                    break;
                }
                switch (fid) {
                    case 1:
                        if (ftype == ::apache::thrift::protocol::T_STRUCT) {
                            xfer += this->body.read(iprot);
                            this->__isset.body = true;
                        } else {
                            xfer += iprot->skip(ftype);
                        }
                        break;
                    default:
                        xfer += iprot->skip(ftype);
                        break;
                }
                xfer += iprot->readFieldEnd();
            }

            xfer += iprot->readStructEnd();

            return xfer;
        }

        uint32_t logs_resp::write(::apache::thrift::protocol::TProtocol *oprot) const {
            uint32_t xfer = 0;
            ::apache::thrift::protocol::TOutputRecursionTracker tracker(*oprot);
            xfer += oprot->writeStructBegin("logs_resp");

            xfer += oprot->writeFieldBegin("body", ::apache::thrift::protocol::T_STRUCT, 1);
            xfer += this->body.write(oprot);
            xfer += oprot->writeFieldEnd();

            xfer += oprot->writeFieldStop();
            xfer += oprot->writeStructEnd();
            return xfer;
        }

        void swap(logs_resp &a, logs_resp &b) {
            using ::std::swap;
            swap(a.body, b.body);
            swap(a.__isset, b.__isset);
        }

        logs_resp::logs_resp(const logs_resp &other96) {
            body = other96.body;
            __isset = other96.__isset;
        }

        logs_resp &logs_resp::operator=(const logs_resp &other97) {
            body = other97.body;
            __isset = other97.__isset;
            return *this;
        }

        void logs_resp::printTo(std::ostream &out) const {
            using ::apache::thrift::to_string;
            out << "logs_resp(";
            out << "body=" << to_string(body);
            out << ")";
        }


        show_req_body::~show_req_body() throw() {
        }


        void show_req_body::__set_o_node_id(const std::string &val) {
            this->o_node_id = val;
        }

        void show_req_body::__set_d_node_id(const std::string &val) {
            this->d_node_id = val;
        }

        void show_req_body::__set_keys(const std::vector<std::string> &val) {
            this->keys = val;
        }

        std::ostream &operator<<(std::ostream &out, const show_req_body &obj) {
            obj.printTo(out);
            return out;
        }


        uint32_t show_req_body::read(::apache::thrift::protocol::TProtocol *iprot) {

            ::apache::thrift::protocol::TInputRecursionTracker tracker(*iprot);
            uint32_t xfer = 0;
            std::string fname;
            ::apache::thrift::protocol::TType ftype;
            int16_t fid;

            xfer += iprot->readStructBegin(fname);

            using ::apache::thrift::protocol::TProtocolException;

            bool isset_o_node_id = false;
            bool isset_d_node_id = false;
            bool isset_keys = false;

            while (true) {
                xfer += iprot->readFieldBegin(fname, ftype, fid);
                if (ftype == ::apache::thrift::protocol::T_STOP) {
                    break;
                }
                switch (fid) {
                    case 1:
                        if (ftype == ::apache::thrift::protocol::T_STRING) {
                            xfer += iprot->readString(this->o_node_id);
                            isset_o_node_id = true;
                        } else {
                            xfer += iprot->skip(ftype);
                        }
                        break;
                    case 2:
                        if (ftype == ::apache::thrift::protocol::T_STRING) {
                            xfer += iprot->readString(this->d_node_id);
                            isset_d_node_id = true;
                        } else {
                            xfer += iprot->skip(ftype);
                        }
                        break;
                    case 3:
                        if (ftype == ::apache::thrift::protocol::T_LIST) {
                            {
                                this->keys.clear();
                                uint32_t _size98;
                                ::apache::thrift::protocol::TType _etype101;
                                xfer += iprot->readListBegin(_etype101, _size98);
                                this->keys.resize(_size98);
                                uint32_t _i102;
                                for (_i102 = 0; _i102 < _size98; ++_i102) {
                                    xfer += iprot->readString(this->keys[_i102]);
                                }
                                xfer += iprot->readListEnd();
                            }
                            isset_keys = true;
                        } else {
                            xfer += iprot->skip(ftype);
                        }
                        break;
                    default:
                        xfer += iprot->skip(ftype);
                        break;
                }
                xfer += iprot->readFieldEnd();
            }

            xfer += iprot->readStructEnd();

            if (!isset_o_node_id)
                throw TProtocolException(TProtocolException::INVALID_DATA);
            if (!isset_d_node_id)
                throw TProtocolException(TProtocolException::INVALID_DATA);
            if (!isset_keys)
                throw TProtocolException(TProtocolException::INVALID_DATA);
            return xfer;
        }

        uint32_t show_req_body::write(::apache::thrift::protocol::TProtocol *oprot) const {
            uint32_t xfer = 0;
            ::apache::thrift::protocol::TOutputRecursionTracker tracker(*oprot);
            xfer += oprot->writeStructBegin("show_req_body");

            xfer += oprot->writeFieldBegin("o_node_id", ::apache::thrift::protocol::T_STRING, 1);
            xfer += oprot->writeString(this->o_node_id);
            xfer += oprot->writeFieldEnd();

            xfer += oprot->writeFieldBegin("d_node_id", ::apache::thrift::protocol::T_STRING, 2);
            xfer += oprot->writeString(this->d_node_id);
            xfer += oprot->writeFieldEnd();

            xfer += oprot->writeFieldBegin("keys", ::apache::thrift::protocol::T_LIST, 3);
            {
                xfer += oprot->writeListBegin(::apache::thrift::protocol::T_STRING,
                                              static_cast<uint32_t>(this->keys.size()));
                std::vector<std::string>::const_iterator _iter103;
                for (_iter103 = this->keys.begin(); _iter103 != this->keys.end(); ++_iter103) {
                    xfer += oprot->writeString((*_iter103));
                }
                xfer += oprot->writeListEnd();
            }
            xfer += oprot->writeFieldEnd();

            xfer += oprot->writeFieldStop();
            xfer += oprot->writeStructEnd();
            return xfer;
        }

        void swap(show_req_body &a, show_req_body &b) {
            using ::std::swap;
            swap(a.o_node_id, b.o_node_id);
            swap(a.d_node_id, b.d_node_id);
            swap(a.keys, b.keys);
        }

        show_req_body::show_req_body(const show_req_body &other104) {
            o_node_id = other104.o_node_id;
            d_node_id = other104.d_node_id;
            keys = other104.keys;
        }

        show_req_body &show_req_body::operator=(const show_req_body &other105) {
            o_node_id = other105.o_node_id;
            d_node_id = other105.d_node_id;
            keys = other105.keys;
            return *this;
        }

        void show_req_body::printTo(std::ostream &out) const {
            using ::apache::thrift::to_string;
            out << "show_req_body(";
            out << "o_node_id=" << to_string(o_node_id);
            out << ", " << "d_node_id=" << to_string(d_node_id);
            out << ", " << "keys=" << to_string(keys);
            out << ")";
        }


        show_req::~show_req() throw() {
        }


        void show_req::__set_body(const show_req_body &val) {
            this->body = val;
        }

        std::ostream &operator<<(std::ostream &out, const show_req &obj) {
            obj.printTo(out);
            return out;
        }


        uint32_t show_req::read(::apache::thrift::protocol::TProtocol *iprot) {

            ::apache::thrift::protocol::TInputRecursionTracker tracker(*iprot);
            uint32_t xfer = 0;
            std::string fname;
            ::apache::thrift::protocol::TType ftype;
            int16_t fid;

            xfer += iprot->readStructBegin(fname);

            using ::apache::thrift::protocol::TProtocolException;


            while (true) {
                xfer += iprot->readFieldBegin(fname, ftype, fid);
                if (ftype == ::apache::thrift::protocol::T_STOP) {
                    break;
                }
                switch (fid) {
                    case 1:
                        if (ftype == ::apache::thrift::protocol::T_STRUCT) {
                            xfer += this->body.read(iprot);
                            this->__isset.body = true;
                        } else {
                            xfer += iprot->skip(ftype);
                        }
                        break;
                    default:
                        xfer += iprot->skip(ftype);
                        break;
                }
                xfer += iprot->readFieldEnd();
            }

            xfer += iprot->readStructEnd();

            return xfer;
        }

        uint32_t show_req::write(::apache::thrift::protocol::TProtocol *oprot) const {
            uint32_t xfer = 0;
            ::apache::thrift::protocol::TOutputRecursionTracker tracker(*oprot);
            xfer += oprot->writeStructBegin("show_req");

            xfer += oprot->writeFieldBegin("body", ::apache::thrift::protocol::T_STRUCT, 1);
            xfer += this->body.write(oprot);
            xfer += oprot->writeFieldEnd();

            xfer += oprot->writeFieldStop();
            xfer += oprot->writeStructEnd();
            return xfer;
        }

        void swap(show_req &a, show_req &b) {
            using ::std::swap;
            swap(a.body, b.body);
            swap(a.__isset, b.__isset);
        }

        show_req::show_req(const show_req &other106) {
            body = other106.body;
            __isset = other106.__isset;
        }

        show_req &show_req::operator=(const show_req &other107) {
            body = other107.body;
            __isset = other107.__isset;
            return *this;
        }

        void show_req::printTo(std::ostream &out) const {
            using ::apache::thrift::to_string;
            out << "show_req(";
            out << "body=" << to_string(body);
            out << ")";
        }


        show_resp_body::~show_resp_body() throw() {
        }


        void show_resp_body::__set_o_node_id(const std::string &val) {
            this->o_node_id = val;
        }

        void show_resp_body::__set_d_node_id(const std::string &val) {
            this->d_node_id = val;
        }

        void show_resp_body::__set_kvs(const std::map<std::string, std::string> &val) {
            this->kvs = val;
        }

        std::ostream &operator<<(std::ostream &out, const show_resp_body &obj) {
            obj.printTo(out);
            return out;
        }


        uint32_t show_resp_body::read(::apache::thrift::protocol::TProtocol *iprot) {

            ::apache::thrift::protocol::TInputRecursionTracker tracker(*iprot);
            uint32_t xfer = 0;
            std::string fname;
            ::apache::thrift::protocol::TType ftype;
            int16_t fid;

            xfer += iprot->readStructBegin(fname);

            using ::apache::thrift::protocol::TProtocolException;

            bool isset_o_node_id = false;
            bool isset_d_node_id = false;
            bool isset_kvs = false;

            while (true) {
                xfer += iprot->readFieldBegin(fname, ftype, fid);
                if (ftype == ::apache::thrift::protocol::T_STOP) {
                    break;
                }
                switch (fid) {
                    case 1:
                        if (ftype == ::apache::thrift::protocol::T_STRING) {
                            xfer += iprot->readString(this->o_node_id);
                            isset_o_node_id = true;
                        } else {
                            xfer += iprot->skip(ftype);
                        }
                        break;
                    case 2:
                        if (ftype == ::apache::thrift::protocol::T_STRING) {
                            xfer += iprot->readString(this->d_node_id);
                            isset_d_node_id = true;
                        } else {
                            xfer += iprot->skip(ftype);
                        }
                        break;
                    case 3:
                        if (ftype == ::apache::thrift::protocol::T_MAP) {
                            {
                                this->kvs.clear();
                                uint32_t _size108;
                                ::apache::thrift::protocol::TType _ktype109;
                                ::apache::thrift::protocol::TType _vtype110;
                                xfer += iprot->readMapBegin(_ktype109, _vtype110, _size108);
                                uint32_t _i112;
                                for (_i112 = 0; _i112 < _size108; ++_i112) {
                                    std::string _key113;
                                    xfer += iprot->readString(_key113);
                                    std::string &_val114 = this->kvs[_key113];
                                    xfer += iprot->readString(_val114);
                                }
                                xfer += iprot->readMapEnd();
                            }
                            isset_kvs = true;
                        } else {
                            xfer += iprot->skip(ftype);
                        }
                        break;
                    default:
                        xfer += iprot->skip(ftype);
                        break;
                }
                xfer += iprot->readFieldEnd();
            }

            xfer += iprot->readStructEnd();

            if (!isset_o_node_id)
                throw TProtocolException(TProtocolException::INVALID_DATA);
            if (!isset_d_node_id)
                throw TProtocolException(TProtocolException::INVALID_DATA);
            if (!isset_kvs)
                throw TProtocolException(TProtocolException::INVALID_DATA);
            return xfer;
        }

        uint32_t show_resp_body::write(::apache::thrift::protocol::TProtocol *oprot) const {
            uint32_t xfer = 0;
            ::apache::thrift::protocol::TOutputRecursionTracker tracker(*oprot);
            xfer += oprot->writeStructBegin("show_resp_body");

            xfer += oprot->writeFieldBegin("o_node_id", ::apache::thrift::protocol::T_STRING, 1);
            xfer += oprot->writeString(this->o_node_id);
            xfer += oprot->writeFieldEnd();

            xfer += oprot->writeFieldBegin("d_node_id", ::apache::thrift::protocol::T_STRING, 2);
            xfer += oprot->writeString(this->d_node_id);
            xfer += oprot->writeFieldEnd();

            xfer += oprot->writeFieldBegin("kvs", ::apache::thrift::protocol::T_MAP, 3);
            {
                xfer += oprot->writeMapBegin(::apache::thrift::protocol::T_STRING, ::apache::thrift::protocol::T_STRING,
                                             static_cast<uint32_t>(this->kvs.size()));
                std::map<std::string, std::string>::const_iterator _iter115;
                for (_iter115 = this->kvs.begin(); _iter115 != this->kvs.end(); ++_iter115) {
                    xfer += oprot->writeString(_iter115->first);
                    xfer += oprot->writeString(_iter115->second);
                }
                xfer += oprot->writeMapEnd();
            }
            xfer += oprot->writeFieldEnd();

            xfer += oprot->writeFieldStop();
            xfer += oprot->writeStructEnd();
            return xfer;
        }

        void swap(show_resp_body &a, show_resp_body &b) {
            using ::std::swap;
            swap(a.o_node_id, b.o_node_id);
            swap(a.d_node_id, b.d_node_id);
            swap(a.kvs, b.kvs);
        }

        show_resp_body::show_resp_body(const show_resp_body &other116) {
            o_node_id = other116.o_node_id;
            d_node_id = other116.d_node_id;
            kvs = other116.kvs;
        }

        show_resp_body &show_resp_body::operator=(const show_resp_body &other117) {
            o_node_id = other117.o_node_id;
            d_node_id = other117.d_node_id;
            kvs = other117.kvs;
            return *this;
        }

        void show_resp_body::printTo(std::ostream &out) const {
            using ::apache::thrift::to_string;
            out << "show_resp_body(";
            out << "o_node_id=" << to_string(o_node_id);
            out << ", " << "d_node_id=" << to_string(d_node_id);
            out << ", " << "kvs=" << to_string(kvs);
            out << ")";
        }


        show_resp::~show_resp() throw() {
        }


        void show_resp::__set_body(const show_resp_body &val) {
            this->body = val;
        }

        std::ostream &operator<<(std::ostream &out, const show_resp &obj) {
            obj.printTo(out);
            return out;
        }


        uint32_t show_resp::read(::apache::thrift::protocol::TProtocol *iprot) {

            ::apache::thrift::protocol::TInputRecursionTracker tracker(*iprot);
            uint32_t xfer = 0;
            std::string fname;
            ::apache::thrift::protocol::TType ftype;
            int16_t fid;

            xfer += iprot->readStructBegin(fname);

            using ::apache::thrift::protocol::TProtocolException;


            while (true) {
                xfer += iprot->readFieldBegin(fname, ftype, fid);
                if (ftype == ::apache::thrift::protocol::T_STOP) {
                    break;
                }
                switch (fid) {
                    case 1:
                        if (ftype == ::apache::thrift::protocol::T_STRUCT) {
                            xfer += this->body.read(iprot);
                            this->__isset.body = true;
                        } else {
                            xfer += iprot->skip(ftype);
                        }
                        break;
                    default:
                        xfer += iprot->skip(ftype);
                        break;
                }
                xfer += iprot->readFieldEnd();
            }

            xfer += iprot->readStructEnd();

            return xfer;
        }

        uint32_t show_resp::write(::apache::thrift::protocol::TProtocol *oprot) const {
            uint32_t xfer = 0;
            ::apache::thrift::protocol::TOutputRecursionTracker tracker(*oprot);
            xfer += oprot->writeStructBegin("show_resp");

            xfer += oprot->writeFieldBegin("body", ::apache::thrift::protocol::T_STRUCT, 1);
            xfer += this->body.write(oprot);
            xfer += oprot->writeFieldEnd();

            xfer += oprot->writeFieldStop();
            xfer += oprot->writeStructEnd();
            return xfer;
        }

        void swap(show_resp &a, show_resp &b) {
            using ::std::swap;
            swap(a.body, b.body);
            swap(a.__isset, b.__isset);
        }

        show_resp::show_resp(const show_resp &other118) {
            body = other118.body;
            __isset = other118.__isset;
        }

        show_resp &show_resp::operator=(const show_resp &other119) {
            body = other119.body;
            __isset = other119.__isset;
            return *this;
        }

        void show_resp::printTo(std::ostream &out) const {
            using ::apache::thrift::to_string;
            out << "show_resp(";
            out << "body=" << to_string(body);
            out << ")";
        }


        node_service_info::~node_service_info() throw() {
        }


        void node_service_info::__set_service_list(const std::vector<std::string> &val) {
            this->service_list = val;
        }

        void node_service_info::__set_name(const std::string &val) {
            this->name = val;
            __isset.name = true;
        }

        void node_service_info::__set_time_stamp(const int64_t val) {
            this->time_stamp = val;
            __isset.time_stamp = true;
        }

        void node_service_info::__set_kvs(const std::map<std::string, std::string> &val) {
            this->kvs = val;
            __isset.kvs = true;
        }

        std::ostream &operator<<(std::ostream &out, const node_service_info &obj) {
            obj.printTo(out);
            return out;
        }


        uint32_t node_service_info::read(::apache::thrift::protocol::TProtocol *iprot) {

            ::apache::thrift::protocol::TInputRecursionTracker tracker(*iprot);
            uint32_t xfer = 0;
            std::string fname;
            ::apache::thrift::protocol::TType ftype;
            int16_t fid;

            xfer += iprot->readStructBegin(fname);

            using ::apache::thrift::protocol::TProtocolException;

            bool isset_service_list = false;

            while (true) {
                xfer += iprot->readFieldBegin(fname, ftype, fid);
                if (ftype == ::apache::thrift::protocol::T_STOP) {
                    break;
                }
                switch (fid) {
                    case 1:
                        if (ftype == ::apache::thrift::protocol::T_LIST) {
                            {
                                this->service_list.clear();
                                uint32_t _size120;
                                ::apache::thrift::protocol::TType _etype123;
                                xfer += iprot->readListBegin(_etype123, _size120);
                                this->service_list.resize(_size120);
                                uint32_t _i124;
                                for (_i124 = 0; _i124 < _size120; ++_i124) {
                                    xfer += iprot->readString(this->service_list[_i124]);
                                }
                                xfer += iprot->readListEnd();
                            }
                            isset_service_list = true;
                        } else {
                            xfer += iprot->skip(ftype);
                        }
                        break;
                    case 2:
                        if (ftype == ::apache::thrift::protocol::T_STRING) {
                            xfer += iprot->readString(this->name);
                            this->__isset.name = true;
                        } else {
                            xfer += iprot->skip(ftype);
                        }
                        break;
                    case 3:
                        if (ftype == ::apache::thrift::protocol::T_I64) {
                            xfer += iprot->readI64(this->time_stamp);
                            this->__isset.time_stamp = true;
                        } else {
                            xfer += iprot->skip(ftype);
                        }
                        break;
                    case 4:
                        if (ftype == ::apache::thrift::protocol::T_MAP) {
                            {
                                this->kvs.clear();
                                uint32_t _size125;
                                ::apache::thrift::protocol::TType _ktype126;
                                ::apache::thrift::protocol::TType _vtype127;
                                xfer += iprot->readMapBegin(_ktype126, _vtype127, _size125);
                                uint32_t _i129;
                                for (_i129 = 0; _i129 < _size125; ++_i129) {
                                    std::string _key130;
                                    xfer += iprot->readString(_key130);
                                    std::string &_val131 = this->kvs[_key130];
                                    xfer += iprot->readString(_val131);
                                }
                                xfer += iprot->readMapEnd();
                            }
                            this->__isset.kvs = true;
                        } else {
                            xfer += iprot->skip(ftype);
                        }
                        break;
                    default:
                        xfer += iprot->skip(ftype);
                        break;
                }
                xfer += iprot->readFieldEnd();
            }

            xfer += iprot->readStructEnd();

            if (!isset_service_list)
                throw TProtocolException(TProtocolException::INVALID_DATA);
            return xfer;
        }

        uint32_t node_service_info::write(::apache::thrift::protocol::TProtocol *oprot) const {
            uint32_t xfer = 0;
            ::apache::thrift::protocol::TOutputRecursionTracker tracker(*oprot);
            xfer += oprot->writeStructBegin("node_service_info");

            xfer += oprot->writeFieldBegin("service_list", ::apache::thrift::protocol::T_LIST, 1);
            {
                xfer += oprot->writeListBegin(::apache::thrift::protocol::T_STRING,
                                              static_cast<uint32_t>(this->service_list.size()));
                std::vector<std::string>::const_iterator _iter132;
                for (_iter132 = this->service_list.begin(); _iter132 != this->service_list.end(); ++_iter132) {
                    xfer += oprot->writeString((*_iter132));
                }
                xfer += oprot->writeListEnd();
            }
            xfer += oprot->writeFieldEnd();

            if (this->__isset.name) {
                xfer += oprot->writeFieldBegin("name", ::apache::thrift::protocol::T_STRING, 2);
                xfer += oprot->writeString(this->name);
                xfer += oprot->writeFieldEnd();
            }
            if (this->__isset.time_stamp) {
                xfer += oprot->writeFieldBegin("time_stamp", ::apache::thrift::protocol::T_I64, 3);
                xfer += oprot->writeI64(this->time_stamp);
                xfer += oprot->writeFieldEnd();
            }
            if (this->__isset.kvs) {
                xfer += oprot->writeFieldBegin("kvs", ::apache::thrift::protocol::T_MAP, 4);
                {
                    xfer += oprot->writeMapBegin(::apache::thrift::protocol::T_STRING,
                                                 ::apache::thrift::protocol::T_STRING,
                                                 static_cast<uint32_t>(this->kvs.size()));
                    std::map<std::string, std::string>::const_iterator _iter133;
                    for (_iter133 = this->kvs.begin(); _iter133 != this->kvs.end(); ++_iter133) {
                        xfer += oprot->writeString(_iter133->first);
                        xfer += oprot->writeString(_iter133->second);
                    }
                    xfer += oprot->writeMapEnd();
                }
                xfer += oprot->writeFieldEnd();
            }
            xfer += oprot->writeFieldStop();
            xfer += oprot->writeStructEnd();
            return xfer;
        }

        void swap(node_service_info &a, node_service_info &b) {
            using ::std::swap;
            swap(a.service_list, b.service_list);
            swap(a.name, b.name);
            swap(a.time_stamp, b.time_stamp);
            swap(a.kvs, b.kvs);
            swap(a.__isset, b.__isset);
        }

        node_service_info::node_service_info(const node_service_info &other134) {
            service_list = other134.service_list;
            name = other134.name;
            time_stamp = other134.time_stamp;
            kvs = other134.kvs;
            __isset = other134.__isset;
        }

        node_service_info &node_service_info::operator=(const node_service_info &other135) {
            service_list = other135.service_list;
            name = other135.name;
            time_stamp = other135.time_stamp;
            kvs = other135.kvs;
            __isset = other135.__isset;
            return *this;
        }

        void node_service_info::printTo(std::ostream &out) const {
            using ::apache::thrift::to_string;
            out << "node_service_info(";
            out << "service_list=" << to_string(service_list);
            out << ", " << "name=";
            (__isset.name ? (out << to_string(name)) : (out << "<null>"));
            out << ", " << "time_stamp=";
            (__isset.time_stamp ? (out << to_string(time_stamp)) : (out << "<null>"));
            out << ", " << "kvs=";
            (__isset.kvs ? (out << to_string(kvs)) : (out << "<null>"));
            out << ")";
        }


        service_broadcast_req_body::~service_broadcast_req_body() throw() {
        }


        void
        service_broadcast_req_body::__set_node_service_info_map(const std::map<std::string, node_service_info> &val) {
            this->node_service_info_map = val;
        }

        std::ostream &operator<<(std::ostream &out, const service_broadcast_req_body &obj) {
            obj.printTo(out);
            return out;
        }


        uint32_t service_broadcast_req_body::read(::apache::thrift::protocol::TProtocol *iprot) {

            ::apache::thrift::protocol::TInputRecursionTracker tracker(*iprot);
            uint32_t xfer = 0;
            std::string fname;
            ::apache::thrift::protocol::TType ftype;
            int16_t fid;

            xfer += iprot->readStructBegin(fname);

            using ::apache::thrift::protocol::TProtocolException;

            bool isset_node_service_info_map = false;

            while (true) {
                xfer += iprot->readFieldBegin(fname, ftype, fid);
                if (ftype == ::apache::thrift::protocol::T_STOP) {
                    break;
                }
                switch (fid) {
                    case 1:
                        if (ftype == ::apache::thrift::protocol::T_MAP) {
                            {
                                this->node_service_info_map.clear();
                                uint32_t _size136;
                                ::apache::thrift::protocol::TType _ktype137;
                                ::apache::thrift::protocol::TType _vtype138;
                                xfer += iprot->readMapBegin(_ktype137, _vtype138, _size136);
                                uint32_t _i140;
                                for (_i140 = 0; _i140 < _size136; ++_i140) {
                                    std::string _key141;
                                    xfer += iprot->readString(_key141);
                                    node_service_info &_val142 = this->node_service_info_map[_key141];
                                    xfer += _val142.read(iprot);
                                }
                                xfer += iprot->readMapEnd();
                            }
                            isset_node_service_info_map = true;
                        } else {
                            xfer += iprot->skip(ftype);
                        }
                        break;
                    default:
                        xfer += iprot->skip(ftype);
                        break;
                }
                xfer += iprot->readFieldEnd();
            }

            xfer += iprot->readStructEnd();

            if (!isset_node_service_info_map)
                throw TProtocolException(TProtocolException::INVALID_DATA);
            return xfer;
        }

        uint32_t service_broadcast_req_body::write(::apache::thrift::protocol::TProtocol *oprot) const {
            uint32_t xfer = 0;
            ::apache::thrift::protocol::TOutputRecursionTracker tracker(*oprot);
            xfer += oprot->writeStructBegin("service_broadcast_req_body");

            xfer += oprot->writeFieldBegin("node_service_info_map", ::apache::thrift::protocol::T_MAP, 1);
            {
                xfer += oprot->writeMapBegin(::apache::thrift::protocol::T_STRING, ::apache::thrift::protocol::T_STRUCT,
                                             static_cast<uint32_t>(this->node_service_info_map.size()));
                std::map<std::string, node_service_info>::const_iterator _iter143;
                for (_iter143 = this->node_service_info_map.begin();
                     _iter143 != this->node_service_info_map.end(); ++_iter143) {
                    xfer += oprot->writeString(_iter143->first);
                    xfer += _iter143->second.write(oprot);
                }
                xfer += oprot->writeMapEnd();
            }
            xfer += oprot->writeFieldEnd();

            xfer += oprot->writeFieldStop();
            xfer += oprot->writeStructEnd();
            return xfer;
        }

        void swap(service_broadcast_req_body &a, service_broadcast_req_body &b) {
            using ::std::swap;
            swap(a.node_service_info_map, b.node_service_info_map);
        }

        service_broadcast_req_body::service_broadcast_req_body(const service_broadcast_req_body &other144) {
            node_service_info_map = other144.node_service_info_map;
        }

        service_broadcast_req_body &service_broadcast_req_body::operator=(const service_broadcast_req_body &other145) {
            node_service_info_map = other145.node_service_info_map;
            return *this;
        }

        void service_broadcast_req_body::printTo(std::ostream &out) const {
            using ::apache::thrift::to_string;
            out << "service_broadcast_req_body(";
            out << "node_service_info_map=" << to_string(node_service_info_map);
            out << ")";
        }


        service_broadcast_req::~service_broadcast_req() throw() {
        }


        void service_broadcast_req::__set_body(const service_broadcast_req_body &val) {
            this->body = val;
        }

        std::ostream &operator<<(std::ostream &out, const service_broadcast_req &obj) {
            obj.printTo(out);
            return out;
        }


        uint32_t service_broadcast_req::read(::apache::thrift::protocol::TProtocol *iprot) {

            ::apache::thrift::protocol::TInputRecursionTracker tracker(*iprot);
            uint32_t xfer = 0;
            std::string fname;
            ::apache::thrift::protocol::TType ftype;
            int16_t fid;

            xfer += iprot->readStructBegin(fname);

            using ::apache::thrift::protocol::TProtocolException;


            while (true) {
                xfer += iprot->readFieldBegin(fname, ftype, fid);
                if (ftype == ::apache::thrift::protocol::T_STOP) {
                    break;
                }
                switch (fid) {
                    case 1:
                        if (ftype == ::apache::thrift::protocol::T_STRUCT) {
                            xfer += this->body.read(iprot);
                            this->__isset.body = true;
                        } else {
                            xfer += iprot->skip(ftype);
                        }
                        break;
                    default:
                        xfer += iprot->skip(ftype);
                        break;
                }
                xfer += iprot->readFieldEnd();
            }

            xfer += iprot->readStructEnd();

            return xfer;
        }

        uint32_t service_broadcast_req::write(::apache::thrift::protocol::TProtocol *oprot) const {
            uint32_t xfer = 0;
            ::apache::thrift::protocol::TOutputRecursionTracker tracker(*oprot);
            xfer += oprot->writeStructBegin("service_broadcast_req");

            xfer += oprot->writeFieldBegin("body", ::apache::thrift::protocol::T_STRUCT, 1);
            xfer += this->body.write(oprot);
            xfer += oprot->writeFieldEnd();

            xfer += oprot->writeFieldStop();
            xfer += oprot->writeStructEnd();
            return xfer;
        }

        void swap(service_broadcast_req &a, service_broadcast_req &b) {
            using ::std::swap;
            swap(a.body, b.body);
            swap(a.__isset, b.__isset);
        }

        service_broadcast_req::service_broadcast_req(const service_broadcast_req &other146) {
            body = other146.body;
            __isset = other146.__isset;
        }

        service_broadcast_req &service_broadcast_req::operator=(const service_broadcast_req &other147) {
            body = other147.body;
            __isset = other147.__isset;
            return *this;
        }

        void service_broadcast_req::printTo(std::ostream &out) const {
            using ::apache::thrift::to_string;
            out << "service_broadcast_req(";
            out << "body=" << to_string(body);
            out << ")";
        }

    }
} // namespace
