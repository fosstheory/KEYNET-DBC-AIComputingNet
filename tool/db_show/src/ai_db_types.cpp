/**
 * Autogenerated by Thrift Compiler (0.11.0)
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated
 */
#include "ai_db_types.h"

#include <algorithm>
#include <ostream>
#include <TToString.h>

namespace ai { namespace dbc {


ai_training_task::~ai_training_task() throw() {
}


void ai_training_task::__set_task_id(const std::string& val) {
  this->task_id = val;
}

void ai_training_task::__set_select_mode(const int8_t val) {
  this->select_mode = val;
}

void ai_training_task::__set_master(const std::string& val) {
  this->master = val;
__isset.master = true;
}

void ai_training_task::__set_peer_nodes_list(const std::vector<std::string> & val) {
  this->peer_nodes_list = val;
}

void ai_training_task::__set_server_specification(const std::string& val) {
  this->server_specification = val;
__isset.server_specification = true;
}

void ai_training_task::__set_server_count(const int32_t val) {
  this->server_count = val;
__isset.server_count = true;
}

void ai_training_task::__set_training_engine(const std::string& val) {
  this->training_engine = val;
}

void ai_training_task::__set_code_dir(const std::string& val) {
  this->code_dir = val;
}

void ai_training_task::__set_entry_file(const std::string& val) {
  this->entry_file = val;
}

void ai_training_task::__set_data_dir(const std::string& val) {
  this->data_dir = val;
}

void ai_training_task::__set_checkpoint_dir(const std::string& val) {
  this->checkpoint_dir = val;
}

void ai_training_task::__set_hyper_parameters(const std::string& val) {
  this->hyper_parameters = val;
__isset.hyper_parameters = true;
}

void ai_training_task::__set_ai_user_node_id(const std::string& val) {
  this->ai_user_node_id = val;
}

void ai_training_task::__set_start_time(const int64_t val) {
  this->start_time = val;
}

void ai_training_task::__set_end_time(const int64_t val) {
  this->end_time = val;
}

void ai_training_task::__set_error_times(const int8_t val) {
  this->error_times = val;
}

void ai_training_task::__set_container_id(const std::string& val) {
  this->container_id = val;
}

void ai_training_task::__set_received_time_stamp(const int64_t val) {
  this->received_time_stamp = val;
}

void ai_training_task::__set_status(const int8_t val) {
  this->status = val;
}
std::ostream& operator<<(std::ostream& out, const ai_training_task& obj)
{
  obj.printTo(out);
  return out;
}


uint32_t ai_training_task::read(::apache::thrift::protocol::TProtocol* iprot) {

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
  bool isset_ai_user_node_id = false;
  bool isset_start_time = false;
  bool isset_end_time = false;
  bool isset_error_times = false;
  bool isset_container_id = false;
  bool isset_received_time_stamp = false;
  bool isset_status = false;

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
            uint32_t _size0;
            ::apache::thrift::protocol::TType _etype3;
            xfer += iprot->readListBegin(_etype3, _size0);
            this->peer_nodes_list.resize(_size0);
            uint32_t _i4;
            for (_i4 = 0; _i4 < _size0; ++_i4)
            {
              xfer += iprot->readString(this->peer_nodes_list[_i4]);
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
          xfer += iprot->readString(this->ai_user_node_id);
          isset_ai_user_node_id = true;
        } else {
          xfer += iprot->skip(ftype);
        }
        break;
      case 14:
        if (ftype == ::apache::thrift::protocol::T_I64) {
          xfer += iprot->readI64(this->start_time);
          isset_start_time = true;
        } else {
          xfer += iprot->skip(ftype);
        }
        break;
      case 15:
        if (ftype == ::apache::thrift::protocol::T_I64) {
          xfer += iprot->readI64(this->end_time);
          isset_end_time = true;
        } else {
          xfer += iprot->skip(ftype);
        }
        break;
      case 252:
        if (ftype == ::apache::thrift::protocol::T_BYTE) {
          xfer += iprot->readByte(this->error_times);
          isset_error_times = true;
        } else {
          xfer += iprot->skip(ftype);
        }
        break;
      case 253:
        if (ftype == ::apache::thrift::protocol::T_STRING) {
          xfer += iprot->readString(this->container_id);
          isset_container_id = true;
        } else {
          xfer += iprot->skip(ftype);
        }
        break;
      case 254:
        if (ftype == ::apache::thrift::protocol::T_I64) {
          xfer += iprot->readI64(this->received_time_stamp);
          isset_received_time_stamp = true;
        } else {
          xfer += iprot->skip(ftype);
        }
        break;
      case 255:
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
  if (!isset_ai_user_node_id)
    throw TProtocolException(TProtocolException::INVALID_DATA);
  if (!isset_start_time)
    throw TProtocolException(TProtocolException::INVALID_DATA);
  if (!isset_end_time)
    throw TProtocolException(TProtocolException::INVALID_DATA);
  if (!isset_error_times)
    throw TProtocolException(TProtocolException::INVALID_DATA);
  if (!isset_container_id)
    throw TProtocolException(TProtocolException::INVALID_DATA);
  if (!isset_received_time_stamp)
    throw TProtocolException(TProtocolException::INVALID_DATA);
  if (!isset_status)
    throw TProtocolException(TProtocolException::INVALID_DATA);
  return xfer;
}

uint32_t ai_training_task::write(::apache::thrift::protocol::TProtocol* oprot) const {
  uint32_t xfer = 0;
  ::apache::thrift::protocol::TOutputRecursionTracker tracker(*oprot);
  xfer += oprot->writeStructBegin("ai_training_task");

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
    std::vector<std::string> ::const_iterator _iter5;
    for (_iter5 = this->peer_nodes_list.begin(); _iter5 != this->peer_nodes_list.end(); ++_iter5)
    {
      xfer += oprot->writeString((*_iter5));
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
  xfer += oprot->writeFieldBegin("ai_user_node_id", ::apache::thrift::protocol::T_STRING, 13);
  xfer += oprot->writeString(this->ai_user_node_id);
  xfer += oprot->writeFieldEnd();

  xfer += oprot->writeFieldBegin("start_time", ::apache::thrift::protocol::T_I64, 14);
  xfer += oprot->writeI64(this->start_time);
  xfer += oprot->writeFieldEnd();

  xfer += oprot->writeFieldBegin("end_time", ::apache::thrift::protocol::T_I64, 15);
  xfer += oprot->writeI64(this->end_time);
  xfer += oprot->writeFieldEnd();

  xfer += oprot->writeFieldBegin("error_times", ::apache::thrift::protocol::T_BYTE, 252);
  xfer += oprot->writeByte(this->error_times);
  xfer += oprot->writeFieldEnd();

  xfer += oprot->writeFieldBegin("container_id", ::apache::thrift::protocol::T_STRING, 253);
  xfer += oprot->writeString(this->container_id);
  xfer += oprot->writeFieldEnd();

  xfer += oprot->writeFieldBegin("received_time_stamp", ::apache::thrift::protocol::T_I64, 254);
  xfer += oprot->writeI64(this->received_time_stamp);
  xfer += oprot->writeFieldEnd();

  xfer += oprot->writeFieldBegin("status", ::apache::thrift::protocol::T_BYTE, 255);
  xfer += oprot->writeByte(this->status);
  xfer += oprot->writeFieldEnd();

  xfer += oprot->writeFieldStop();
  xfer += oprot->writeStructEnd();
  return xfer;
}

void swap(ai_training_task &a, ai_training_task &b) {
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
  swap(a.ai_user_node_id, b.ai_user_node_id);
  swap(a.start_time, b.start_time);
  swap(a.end_time, b.end_time);
  swap(a.error_times, b.error_times);
  swap(a.container_id, b.container_id);
  swap(a.received_time_stamp, b.received_time_stamp);
  swap(a.status, b.status);
  swap(a.__isset, b.__isset);
}

ai_training_task::ai_training_task(const ai_training_task& other6) {
  task_id = other6.task_id;
  select_mode = other6.select_mode;
  master = other6.master;
  peer_nodes_list = other6.peer_nodes_list;
  server_specification = other6.server_specification;
  server_count = other6.server_count;
  training_engine = other6.training_engine;
  code_dir = other6.code_dir;
  entry_file = other6.entry_file;
  data_dir = other6.data_dir;
  checkpoint_dir = other6.checkpoint_dir;
  hyper_parameters = other6.hyper_parameters;
  ai_user_node_id = other6.ai_user_node_id;
  start_time = other6.start_time;
  end_time = other6.end_time;
  error_times = other6.error_times;
  container_id = other6.container_id;
  received_time_stamp = other6.received_time_stamp;
  status = other6.status;
  __isset = other6.__isset;
}
ai_training_task& ai_training_task::operator=(const ai_training_task& other7) {
  task_id = other7.task_id;
  select_mode = other7.select_mode;
  master = other7.master;
  peer_nodes_list = other7.peer_nodes_list;
  server_specification = other7.server_specification;
  server_count = other7.server_count;
  training_engine = other7.training_engine;
  code_dir = other7.code_dir;
  entry_file = other7.entry_file;
  data_dir = other7.data_dir;
  checkpoint_dir = other7.checkpoint_dir;
  hyper_parameters = other7.hyper_parameters;
  ai_user_node_id = other7.ai_user_node_id;
  start_time = other7.start_time;
  end_time = other7.end_time;
  error_times = other7.error_times;
  container_id = other7.container_id;
  received_time_stamp = other7.received_time_stamp;
  status = other7.status;
  __isset = other7.__isset;
  return *this;
}
void ai_training_task::printTo(std::ostream& out) const {
  using ::apache::thrift::to_string;
  out << "ai_training_task(";
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
  out << ", " << "ai_user_node_id=" << to_string(ai_user_node_id);
  out << ", " << "start_time=" << to_string(start_time);
  out << ", " << "end_time=" << to_string(end_time);
  out << ", " << "error_times=" << to_string(error_times);
  out << ", " << "container_id=" << to_string(container_id);
  out << ", " << "received_time_stamp=" << to_string(received_time_stamp);
  out << ", " << "status=" << to_string(status);
  out << ")";
}


cmd_task_info::~cmd_task_info() throw() {
}


void cmd_task_info::__set_task_id(const std::string& val) {
  this->task_id = val;
}

void cmd_task_info::__set_create_time(const int64_t val) {
  this->create_time = val;
}

void cmd_task_info::__set_result(const std::string& val) {
  this->result = val;
}

void cmd_task_info::__set_status(const int8_t val) {
  this->status = val;
}
std::ostream& operator<<(std::ostream& out, const cmd_task_info& obj)
{
  obj.printTo(out);
  return out;
}


uint32_t cmd_task_info::read(::apache::thrift::protocol::TProtocol* iprot) {

  ::apache::thrift::protocol::TInputRecursionTracker tracker(*iprot);
  uint32_t xfer = 0;
  std::string fname;
  ::apache::thrift::protocol::TType ftype;
  int16_t fid;

  xfer += iprot->readStructBegin(fname);

  using ::apache::thrift::protocol::TProtocolException;

  bool isset_task_id = false;
  bool isset_create_time = false;
  bool isset_result = false;
  bool isset_status = false;

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
        if (ftype == ::apache::thrift::protocol::T_I64) {
          xfer += iprot->readI64(this->create_time);
          isset_create_time = true;
        } else {
          xfer += iprot->skip(ftype);
        }
        break;
      case 3:
        if (ftype == ::apache::thrift::protocol::T_STRING) {
          xfer += iprot->readString(this->result);
          isset_result = true;
        } else {
          xfer += iprot->skip(ftype);
        }
        break;
      case 4:
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
  if (!isset_create_time)
    throw TProtocolException(TProtocolException::INVALID_DATA);
  if (!isset_result)
    throw TProtocolException(TProtocolException::INVALID_DATA);
  if (!isset_status)
    throw TProtocolException(TProtocolException::INVALID_DATA);
  return xfer;
}

uint32_t cmd_task_info::write(::apache::thrift::protocol::TProtocol* oprot) const {
  uint32_t xfer = 0;
  ::apache::thrift::protocol::TOutputRecursionTracker tracker(*oprot);
  xfer += oprot->writeStructBegin("cmd_task_info");

  xfer += oprot->writeFieldBegin("task_id", ::apache::thrift::protocol::T_STRING, 1);
  xfer += oprot->writeString(this->task_id);
  xfer += oprot->writeFieldEnd();

  xfer += oprot->writeFieldBegin("create_time", ::apache::thrift::protocol::T_I64, 2);
  xfer += oprot->writeI64(this->create_time);
  xfer += oprot->writeFieldEnd();

  xfer += oprot->writeFieldBegin("result", ::apache::thrift::protocol::T_STRING, 3);
  xfer += oprot->writeString(this->result);
  xfer += oprot->writeFieldEnd();

  xfer += oprot->writeFieldBegin("status", ::apache::thrift::protocol::T_BYTE, 4);
  xfer += oprot->writeByte(this->status);
  xfer += oprot->writeFieldEnd();

  xfer += oprot->writeFieldStop();
  xfer += oprot->writeStructEnd();
  return xfer;
}

void swap(cmd_task_info &a, cmd_task_info &b) {
  using ::std::swap;
  swap(a.task_id, b.task_id);
  swap(a.create_time, b.create_time);
  swap(a.result, b.result);
  swap(a.status, b.status);
}

cmd_task_info::cmd_task_info(const cmd_task_info& other8) {
  task_id = other8.task_id;
  create_time = other8.create_time;
  result = other8.result;
  status = other8.status;
}
cmd_task_info& cmd_task_info::operator=(const cmd_task_info& other9) {
  task_id = other9.task_id;
  create_time = other9.create_time;
  result = other9.result;
  status = other9.status;
  return *this;
}
void cmd_task_info::printTo(std::ostream& out) const {
  using ::apache::thrift::to_string;
  out << "cmd_task_info(";
  out << "task_id=" << to_string(task_id);
  out << ", " << "create_time=" << to_string(create_time);
  out << ", " << "result=" << to_string(result);
  out << ", " << "status=" << to_string(status);
  out << ")";
}

}} // namespace
