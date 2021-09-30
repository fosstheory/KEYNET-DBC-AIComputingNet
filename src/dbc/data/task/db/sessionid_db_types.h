/**
 * Autogenerated by Thrift Compiler (0.12.0)
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated
 */
#ifndef sessionid_db_TYPES_H
#define sessionid_db_TYPES_H

#include <iosfwd>
#include "network/protocol/protocol.h"
#include "network/protocol/service_message.h"
#include "util/TToString.h"

namespace dbc {

    class rent_sessionid;


    class rent_sessionid : public virtual ::apache::thrift::TBase {
    public:

        rent_sessionid(const rent_sessionid&);
        rent_sessionid& operator=(const rent_sessionid&);
        rent_sessionid() : rent_wallet(), session_id() {
        }

        virtual ~rent_sessionid() throw();
        std::string rent_wallet;
        std::string session_id;

        void __set_rent_wallet(const std::string& val);

        void __set_session_id(const std::string& val);

        bool operator == (const rent_sessionid & rhs) const
        {
            if (!(rent_wallet == rhs.rent_wallet))
                return false;
            if (!(session_id == rhs.session_id))
                return false;
            return true;
        }
        bool operator != (const rent_sessionid &rhs) const {
            return !(*this == rhs);
        }

        bool operator < (const rent_sessionid & ) const;

        uint32_t read(::apache::thrift::protocol::TProtocol* iprot);
        uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

        virtual void printTo(std::ostream& out) const;
    };

    void swap(rent_sessionid &a, rent_sessionid &b);

    std::ostream& operator<<(std::ostream& out, const rent_sessionid& obj);

} // namespace

#endif
