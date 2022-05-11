/**
 * Autogenerated by Thrift Compiler (0.11.0)
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated
 */
#ifndef db_task_info_TYPES_H
#define db_task_info_TYPES_H

#include <iosfwd>
#include "network/protocol/protocol.h"
#include "network/protocol/net_message.h"
#include "util/TToString.h"

namespace dbc {

	class db_task_info;

	typedef struct _db_task_info__isset {
		_db_task_info__isset() : image_name(false), login_password(false), ssh_port(false), create_time(false), operation_system(false), bios_mode(false), rdp_port(false), custom_port(false), multicast(false), desc(false), vda_rootbackfile(false), network_name(false), public_ip(false), nwfilter(false), login_username(false) {}
		bool image_name : 1;
		bool login_password : 1;
		bool ssh_port : 1;
		bool create_time : 1;
		bool operation_system : 1;
		bool bios_mode : 1;
		bool rdp_port : 1;
		bool custom_port : 1;
		bool multicast : 1;
		bool desc : 1;
		bool vda_rootbackfile : 1;
		bool network_name : 1;
		bool public_ip : 1;
		bool nwfilter : 1;
		bool login_username :1;
	} _db_task_info__isset;

	class db_task_info : public virtual ::apache::thrift::TBase {
	public:

		db_task_info(const db_task_info&);
		db_task_info& operator=(const db_task_info&);
		db_task_info() : task_id(), image_name(), login_password(), ssh_port(), create_time(0), operation_system(), bios_mode(), rdp_port(), desc(), vda_rootbackfile(), network_name(), public_ip(), login_username() {
		}

		virtual ~db_task_info() throw();
		std::string task_id;
		std::string image_name;
		std::string login_password;
		std::string ssh_port;
		int64_t create_time;
		std::string operation_system;
		std::string bios_mode;
		std::string rdp_port;
		std::vector<std::string>  custom_port;
		std::vector<std::string>  multicast;
		std::string desc;
		std::string vda_rootbackfile;
		std::string network_name;
		std::string public_ip;
		std::vector<std::string>  nwfilter;
		std::string login_username;

		_db_task_info__isset __isset;

		void __set_task_id(const std::string& val);

		void __set_image_name(const std::string& val);

		void __set_login_password(const std::string& val);

		void __set_ssh_port(const std::string& val);

		void __set_create_time(const int64_t val);

		void __set_operation_system(const std::string& val);

		void __set_bios_mode(const std::string& val);

		void __set_rdp_port(const std::string& val);

		void __set_custom_port(const std::vector<std::string>& val);

		void __set_multicast(const std::vector<std::string>& val);

		void __set_desc(const std::string& val);

		void __set_vda_rootbackfile(const std::string& val);

		void __set_network_name(const std::string& val);

		void __set_public_ip(const std::string& val);

		void __set_nwfilter(const std::vector<std::string>& val);

		void __set_login_username(const std::string& val);

		bool operator == (const db_task_info& rhs) const
		{
			if (!(task_id == rhs.task_id))
				return false;
			if (__isset.image_name != rhs.__isset.image_name)
				return false;
			else if (__isset.image_name && !(image_name == rhs.image_name))
				return false;
			if (__isset.login_password != rhs.__isset.login_password)
				return false;
			else if (__isset.login_password && !(login_password == rhs.login_password))
				return false;
			if (__isset.ssh_port != rhs.__isset.ssh_port)
				return false;
			else if (__isset.ssh_port && !(ssh_port == rhs.ssh_port))
				return false;
			if (__isset.create_time != rhs.__isset.create_time)
				return false;
			else if (__isset.create_time && !(create_time == rhs.create_time))
				return false;
			if (__isset.operation_system != rhs.__isset.operation_system)
				return false;
			else if (__isset.operation_system && !(operation_system == rhs.operation_system))
				return false;
			if (__isset.bios_mode != rhs.__isset.bios_mode)
				return false;
			else if (__isset.bios_mode && !(bios_mode == rhs.bios_mode))
				return false;
			if (__isset.rdp_port != rhs.__isset.rdp_port)
				return false;
			else if (__isset.rdp_port && !(rdp_port == rhs.rdp_port))
				return false;
			if (__isset.custom_port != rhs.__isset.custom_port)
				return false;
			else if (__isset.custom_port && !(custom_port == rhs.custom_port))
				return false;
			if (__isset.multicast != rhs.__isset.multicast)
				return false;
			else if (__isset.multicast && !(multicast == rhs.multicast))
				return false;
			if (__isset.desc != rhs.__isset.desc)
				return false;
			else if (__isset.desc && !(desc == rhs.desc))
				return false;
			if (__isset.vda_rootbackfile != rhs.__isset.vda_rootbackfile)
				return false;
			else if (__isset.vda_rootbackfile && !(vda_rootbackfile == rhs.vda_rootbackfile))
				return false;
			if (__isset.network_name != rhs.__isset.network_name)
				return false;
			else if (__isset.network_name && !(network_name == rhs.network_name))
				return false;
			if (__isset.public_ip != rhs.__isset.public_ip)
				return false;
			else if (__isset.public_ip && !(public_ip == rhs.public_ip))
				return false;
			if (__isset.nwfilter != rhs.__isset.nwfilter)
				return false;
			else if (__isset.nwfilter && !(nwfilter == rhs.nwfilter))
				return false;
			if (__isset.login_username != rhs.__isset.login_username)
				return false;
			else if (__isset.login_username && !(login_username == rhs.login_username))
				return false;
			return true;
		}
		bool operator != (const db_task_info& rhs) const {
			return !(*this == rhs);
		}

		bool operator < (const db_task_info&) const;

		uint32_t read(::apache::thrift::protocol::TProtocol* iprot);
		uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

		virtual void printTo(std::ostream& out) const;
	};

	void swap(db_task_info& a, db_task_info& b);

	std::ostream& operator<<(std::ostream& out, const db_task_info& obj);

} // namespace

#endif
