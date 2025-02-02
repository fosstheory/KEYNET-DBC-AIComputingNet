namespace cpp dbc

struct db_task_info {
    1: required string task_id,             //id
    2: optional string image_name,          //镜像文件名字
    3: optional string login_password,      //登录密码
    4: optional string ssh_port,            //linux ssh端口
    7: optional i64 create_time,            //创建时间
    14: optional string operation_system,   //操作系统(如：generic, ubuntu 18.04,windows 10)
    15: optional string bios_mode,          //BIOS 模式(如：legacy, uefi)
    16: optional string rdp_port,           //windows rdp端口
    17: optional list<string> custom_port,  //自定义端口: ["xx", "xx:yy", "xx-yy", "xx-yy:ww-pp", ...]
    18: optional list<string> multicast,    //组播地址(如："230.0.0.1:5558")
    21: optional string desc,               //虚拟机描述
    22: optional string vda_rootbackfile;   //vda系统盘的根backfile

    24: optional string network_name,       //vxlan network name
    30: optional string public_ip,          //公网ip
    31: optional list<string> nwfilter,     //安全组，只有设置了公网ip才会使用
    35: optional string login_username      //登陆用户名
}
