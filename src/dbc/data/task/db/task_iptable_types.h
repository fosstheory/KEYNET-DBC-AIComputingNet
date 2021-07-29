/**
 * Autogenerated by Thrift Compiler (0.12.0)
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated
 */
#ifndef task_iptable_TYPES_H
#define task_iptable_TYPES_H

#include <iosfwd>
#include "network/protocol/protocol.h"
#include "network/protocol/service_message.h"
#include "util/TToString.h"

namespace dbc {

    class TaskIpTable;


    class TaskIpTable : public virtual ::apache::thrift::TBase {
    public:

        TaskIpTable(const TaskIpTable&);
        TaskIpTable& operator=(const TaskIpTable&);
        TaskIpTable() : task_id(), host_ip(), vm_local_ip(), ssh_port() {
        }

        virtual ~TaskIpTable() throw();
        std::string task_id;
        std::string host_ip;
        std::string vm_local_ip;
        std::string ssh_port;

        void __set_task_id(const std::string& val);

        void __set_host_ip(const std::string& val);

        void __set_vm_local_ip(const std::string& val);

        void __set_ssh_port(const std::string& val);

        bool operator == (const TaskIpTable & rhs) const
        {
            if (!(task_id == rhs.task_id))
                return false;
            if (!(host_ip == rhs.host_ip))
                return false;
            if (!(vm_local_ip == rhs.vm_local_ip))
                return false;
            if (!(ssh_port == rhs.ssh_port))
                return false;
            return true;
        }
        bool operator != (const TaskIpTable &rhs) const {
            return !(*this == rhs);
        }

        bool operator < (const TaskIpTable & ) const;

        uint32_t read(::apache::thrift::protocol::TProtocol* iprot);
        uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

        virtual void printTo(std::ostream& out) const;
    };

    void swap(TaskIpTable &a, TaskIpTable &b);

    std::ostream& operator<<(std::ostream& out, const TaskIpTable& obj);

} // namespace

#endif
