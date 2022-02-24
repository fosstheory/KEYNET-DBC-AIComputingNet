/**
 * Autogenerated by Thrift Compiler (0.12.0)
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated
 */
#ifndef monitors_db_TYPES_H
#define monitors_db_TYPES_H

#include <iosfwd>

// #include <thrift/Thrift.h>
// #include <thrift/TApplicationException.h>
// #include <thrift/TBase.h>
// #include <thrift/protocol/TProtocol.h>
// #include <thrift/transport/TTransport.h>

// #include <thrift/stdcxx.h>
#include "network/protocol/protocol.h"
#include "network/protocol/service_message.h"
#include "util/TToString.h"


namespace dbc {

class db_monitor_server;

class db_wallet_monitors;


class db_monitor_server : public virtual ::apache::thrift::TBase {
 public:

  db_monitor_server(const db_monitor_server&);
  db_monitor_server& operator=(const db_monitor_server&);
  db_monitor_server() : host(), port() {
  }

  virtual ~db_monitor_server() throw();
  std::string host;
  std::string port;

  void __set_host(const std::string& val);

  void __set_port(const std::string& val);

  bool operator == (const db_monitor_server & rhs) const
  {
    if (!(host == rhs.host))
      return false;
    if (!(port == rhs.port))
      return false;
    return true;
  }
  bool operator != (const db_monitor_server &rhs) const {
    return !(*this == rhs);
  }

  bool operator < (const db_monitor_server & ) const;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);
  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

  virtual void printTo(std::ostream& out) const;
};

void swap(db_monitor_server &a, db_monitor_server &b);

std::ostream& operator<<(std::ostream& out, const db_monitor_server& obj);


class db_wallet_monitors : public virtual ::apache::thrift::TBase {
 public:

  db_wallet_monitors(const db_wallet_monitors&);
  db_wallet_monitors& operator=(const db_wallet_monitors&);
  db_wallet_monitors() : wallet() {
  }

  virtual ~db_wallet_monitors() throw();
  std::string wallet;
  std::vector<db_monitor_server>  monitors;

  void __set_wallet(const std::string& val);

  void __set_monitors(const std::vector<db_monitor_server> & val);

  bool operator == (const db_wallet_monitors & rhs) const
  {
    if (!(wallet == rhs.wallet))
      return false;
    if (!(monitors == rhs.monitors))
      return false;
    return true;
  }
  bool operator != (const db_wallet_monitors &rhs) const {
    return !(*this == rhs);
  }

  bool operator < (const db_wallet_monitors & ) const;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);
  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

  virtual void printTo(std::ostream& out) const;
};

void swap(db_wallet_monitors &a, db_wallet_monitors &b);

std::ostream& operator<<(std::ostream& out, const db_wallet_monitors& obj);

} // namespace

#endif
