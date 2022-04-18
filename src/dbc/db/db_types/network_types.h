/**
 * Autogenerated by Thrift Compiler (0.12.0)
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated
 */
#ifndef network_TYPES_H
#define network_TYPES_H

#include <iosfwd>

// #include <thrift/Thrift.h>
// #include <thrift/TApplicationException.h>
// #include <thrift/TBase.h>
// #include <thrift/protocol/TProtocol.h>
// #include <thrift/transport/TTransport.h>

// #include <thrift/stdcxx.h>

#include "network/protocol/protocol.h"
#include "network/protocol/net_message.h"
#include "util/TToString.h"

namespace dbc {

class networkInfo;

typedef struct _networkInfo__isset {
  _networkInfo__isset() : dhcpInterface(false), machineId(false), rentWallet(false), members(false), lastUseTime(false), nativeFlags(false), lastUpdateTime(false) {}
  bool dhcpInterface :1;
  bool machineId :1;
  bool rentWallet :1;
  bool members :1;
  bool lastUseTime :1;
  bool nativeFlags :1;
  bool lastUpdateTime :1;
} _networkInfo__isset;

class networkInfo : public virtual ::apache::thrift::TBase {
 public:

  networkInfo(const networkInfo&);
  networkInfo& operator=(const networkInfo&);
  networkInfo() : networkId(), bridgeName(), vxlanName(), vxlanVni(), ipCidr(), ipStart(), ipEnd(), dhcpInterface(), machineId(), rentWallet(), lastUseTime(0), nativeFlags(0), lastUpdateTime(0) {
  }

  virtual ~networkInfo() throw();
  std::string networkId;
  std::string bridgeName;
  std::string vxlanName;
  std::string vxlanVni;
  std::string ipCidr;
  std::string ipStart;
  std::string ipEnd;
  std::string dhcpInterface;
  std::string machineId;
  std::string rentWallet;
  std::vector<std::string>  members;
  int64_t lastUseTime;
  int32_t nativeFlags;
  int64_t lastUpdateTime;

  _networkInfo__isset __isset;

  void __set_networkId(const std::string& val);

  void __set_bridgeName(const std::string& val);

  void __set_vxlanName(const std::string& val);

  void __set_vxlanVni(const std::string& val);

  void __set_ipCidr(const std::string& val);

  void __set_ipStart(const std::string& val);

  void __set_ipEnd(const std::string& val);

  void __set_dhcpInterface(const std::string& val);

  void __set_machineId(const std::string& val);

  void __set_rentWallet(const std::string& val);

  void __set_members(const std::vector<std::string> & val);

  void __set_lastUseTime(const int64_t val);

  void __set_nativeFlags(const int32_t val);

  void __set_lastUpdateTime(const int64_t val);

  bool operator == (const networkInfo & rhs) const
  {
    if (!(networkId == rhs.networkId))
      return false;
    if (!(bridgeName == rhs.bridgeName))
      return false;
    if (!(vxlanName == rhs.vxlanName))
      return false;
    if (!(vxlanVni == rhs.vxlanVni))
      return false;
    if (!(ipCidr == rhs.ipCidr))
      return false;
    if (!(ipStart == rhs.ipStart))
      return false;
    if (!(ipEnd == rhs.ipEnd))
      return false;
    if (__isset.dhcpInterface != rhs.__isset.dhcpInterface)
      return false;
    else if (__isset.dhcpInterface && !(dhcpInterface == rhs.dhcpInterface))
      return false;
    if (__isset.machineId != rhs.__isset.machineId)
      return false;
    else if (__isset.machineId && !(machineId == rhs.machineId))
      return false;
    if (__isset.rentWallet != rhs.__isset.rentWallet)
      return false;
    else if (__isset.rentWallet && !(rentWallet == rhs.rentWallet))
      return false;
    if (__isset.members != rhs.__isset.members)
      return false;
    else if (__isset.members && !(members == rhs.members))
      return false;
    if (__isset.lastUseTime != rhs.__isset.lastUseTime)
      return false;
    else if (__isset.lastUseTime && !(lastUseTime == rhs.lastUseTime))
      return false;
    if (__isset.nativeFlags != rhs.__isset.nativeFlags)
      return false;
    else if (__isset.nativeFlags && !(nativeFlags == rhs.nativeFlags))
      return false;
    if (__isset.lastUpdateTime != rhs.__isset.lastUpdateTime)
      return false;
    else if (__isset.lastUpdateTime && !(lastUpdateTime == rhs.lastUpdateTime))
      return false;
    return true;
  }
  bool operator != (const networkInfo &rhs) const {
    return !(*this == rhs);
  }

  bool operator < (const networkInfo & ) const;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);
  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

  virtual void printTo(std::ostream& out) const;
};

void swap(networkInfo &a, networkInfo &b);

std::ostream& operator<<(std::ostream& out, const networkInfo& obj);

} // namespace

#endif
