/**
 * Autogenerated by Thrift Compiler (0.12.0)
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated
 */
#include "task_iptable_types.h"
#include <algorithm>
#include <ostream>

namespace dbc {


    task_iptable::~task_iptable() throw() {
    }


    void task_iptable::__set_task_id(const std::string& val) {
        this->task_id = val;
    }

    void task_iptable::__set_host_ip(const std::string& val) {
        this->host_ip = val;
        __isset.host_ip = true;
    }

    void task_iptable::__set_vm_local_ip(const std::string& val) {
        this->vm_local_ip = val;
        __isset.vm_local_ip = true;
    }

    void task_iptable::__set_ssh_port(const std::string& val) {
        this->ssh_port = val;
        __isset.ssh_port = true;
    }

    void task_iptable::__set_rdp_port(const std::string& val) {
        this->rdp_port = val;
        __isset.rdp_port = true;
    }

    void task_iptable::__set_custom_port(const std::vector<std::string> & val) {
        this->custom_port = val;
        __isset.custom_port = true;
    }

    void task_iptable::__set_public_ip(const std::string& val) {
        this->public_ip = val;
        __isset.public_ip = true;
    }
    std::ostream& operator<<(std::ostream& out, const task_iptable& obj)
    {
        obj.printTo(out);
        return out;
    }


    uint32_t task_iptable::read(::apache::thrift::protocol::TProtocol* iprot) {

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
                case 2:
                    if (ftype == ::apache::thrift::protocol::T_STRING) {
                        xfer += iprot->readString(this->host_ip);
                        this->__isset.host_ip = true;
                    } else {
                        xfer += iprot->skip(ftype);
                    }
                    break;
                case 3:
                    if (ftype == ::apache::thrift::protocol::T_STRING) {
                        xfer += iprot->readString(this->vm_local_ip);
                        this->__isset.vm_local_ip = true;
                    } else {
                        xfer += iprot->skip(ftype);
                    }
                    break;
                case 4:
                    if (ftype == ::apache::thrift::protocol::T_STRING) {
                        xfer += iprot->readString(this->ssh_port);
                        this->__isset.ssh_port = true;
                    } else {
                        xfer += iprot->skip(ftype);
                    }
                    break;
                case 5:
                    if (ftype == ::apache::thrift::protocol::T_STRING) {
                        xfer += iprot->readString(this->rdp_port);
                        this->__isset.rdp_port = true;
                    } else {
                        xfer += iprot->skip(ftype);
                    }
                    break;
                case 6:
                    if (ftype == ::apache::thrift::protocol::T_LIST) {
                        {
                            this->custom_port.clear();
                            uint32_t _size0;
                            ::apache::thrift::protocol::TType _etype3;
                            xfer += iprot->readListBegin(_etype3, _size0);
                            this->custom_port.resize(_size0);
                            uint32_t _i4;
                            for (_i4 = 0; _i4 < _size0; ++_i4)
                            {
                                xfer += iprot->readString(this->custom_port[_i4]);
                            }
                            xfer += iprot->readListEnd();
                        }
                        this->__isset.custom_port = true;
                    } else {
                        xfer += iprot->skip(ftype);
                    }
                    break;
                case 7:
                    if (ftype == ::apache::thrift::protocol::T_STRING) {
                        xfer += iprot->readString(this->public_ip);
                        this->__isset.public_ip = true;
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

    uint32_t task_iptable::write(::apache::thrift::protocol::TProtocol* oprot) const {
        uint32_t xfer = 0;
        ::apache::thrift::protocol::TOutputRecursionTracker tracker(*oprot);
        xfer += oprot->writeStructBegin("task_iptable");

        xfer += oprot->writeFieldBegin("task_id", ::apache::thrift::protocol::T_STRING, 1);
        xfer += oprot->writeString(this->task_id);
        xfer += oprot->writeFieldEnd();

        if (this->__isset.host_ip) {
            xfer += oprot->writeFieldBegin("host_ip", ::apache::thrift::protocol::T_STRING, 2);
            xfer += oprot->writeString(this->host_ip);
            xfer += oprot->writeFieldEnd();
        }
        if (this->__isset.vm_local_ip) {
            xfer += oprot->writeFieldBegin("vm_local_ip", ::apache::thrift::protocol::T_STRING, 3);
            xfer += oprot->writeString(this->vm_local_ip);
            xfer += oprot->writeFieldEnd();
        }
        if (this->__isset.ssh_port) {
            xfer += oprot->writeFieldBegin("ssh_port", ::apache::thrift::protocol::T_STRING, 4);
            xfer += oprot->writeString(this->ssh_port);
            xfer += oprot->writeFieldEnd();
        }
        if (this->__isset.rdp_port) {
            xfer += oprot->writeFieldBegin("rdp_port", ::apache::thrift::protocol::T_STRING, 5);
            xfer += oprot->writeString(this->rdp_port);
            xfer += oprot->writeFieldEnd();
        }
        if (this->__isset.custom_port) {
            xfer += oprot->writeFieldBegin("custom_port", ::apache::thrift::protocol::T_LIST, 6);
            {
                xfer += oprot->writeListBegin(::apache::thrift::protocol::T_STRING, static_cast<uint32_t>(this->custom_port.size()));
                std::vector<std::string> ::const_iterator _iter5;
                for (_iter5 = this->custom_port.begin(); _iter5 != this->custom_port.end(); ++_iter5)
                {
                    xfer += oprot->writeString((*_iter5));
                }
                xfer += oprot->writeListEnd();
            }
            xfer += oprot->writeFieldEnd();
        }
        if (this->__isset.public_ip) {
            xfer += oprot->writeFieldBegin("public_ip", ::apache::thrift::protocol::T_STRING, 7);
            xfer += oprot->writeString(this->public_ip);
            xfer += oprot->writeFieldEnd();
        }
        xfer += oprot->writeFieldStop();
        xfer += oprot->writeStructEnd();
        return xfer;
    }

    void swap(task_iptable &a, task_iptable &b) {
        using ::std::swap;
        swap(a.task_id, b.task_id);
        swap(a.host_ip, b.host_ip);
        swap(a.vm_local_ip, b.vm_local_ip);
        swap(a.ssh_port, b.ssh_port);
        swap(a.rdp_port, b.rdp_port);
        swap(a.custom_port, b.custom_port);
        swap(a.public_ip, b.public_ip);
        swap(a.__isset, b.__isset);
    }

    task_iptable::task_iptable(const task_iptable& other6) {
        task_id = other6.task_id;
        host_ip = other6.host_ip;
        vm_local_ip = other6.vm_local_ip;
        ssh_port = other6.ssh_port;
        rdp_port = other6.rdp_port;
        custom_port = other6.custom_port;
        public_ip = other6.public_ip;
        __isset = other6.__isset;
    }
    task_iptable& task_iptable::operator=(const task_iptable& other7) {
        task_id = other7.task_id;
        host_ip = other7.host_ip;
        vm_local_ip = other7.vm_local_ip;
        ssh_port = other7.ssh_port;
        rdp_port = other7.rdp_port;
        custom_port = other7.custom_port;
        public_ip = other7.public_ip;
        __isset = other7.__isset;
        return *this;
    }
    void task_iptable::printTo(std::ostream& out) const {
        using ::apache::thrift::to_string;
        out << "task_iptable(";
        out << "task_id=" << to_string(task_id);
        out << ", " << "host_ip="; (__isset.host_ip ? (out << to_string(host_ip)) : (out << "<null>"));
        out << ", " << "vm_local_ip="; (__isset.vm_local_ip ? (out << to_string(vm_local_ip)) : (out << "<null>"));
        out << ", " << "ssh_port="; (__isset.ssh_port ? (out << to_string(ssh_port)) : (out << "<null>"));
        out << ", " << "rdp_port="; (__isset.rdp_port ? (out << to_string(rdp_port)) : (out << "<null>"));
        out << ", " << "custom_port="; (__isset.custom_port ? (out << to_string(custom_port)) : (out << "<null>"));
        out << ", " << "public_ip="; (__isset.public_ip ? (out << to_string(public_ip)) : (out << "<null>"));
        out << ")";
    }

} // namespace
