#ifndef DBC_TASK_MANAGER_H
#define DBC_TASK_MANAGER_H

#include "util/utils.h"
#include "service_module/service_module.h"
#include <boost/process.hpp>
#include "message/matrix_types.h"
#include "message/vm_task_result_types.h"
#include "vm/vm_client.h"
#include "HttpDBCChainClient.h"
#include "detail/info/TaskInfoManager.h"
#include "detail/disk/TaskDiskManager.h"
#include "detail/gpu/TaskGpuManager.h"
#include "detail/iptable/TaskIptableManager.h"
#include "detail/wallet_rent_task/WalletRentTaskManager.h"
#include "detail/wallet_session_id/WalletSessionIDManager.h"
#include "detail/image/ImageManager.h"
#include "TaskEvent.h"

namespace bp = boost::process;

struct CreateTaskParams {
    std::string task_id;
	// ����
	std::string desc;
	// ��¼����
	std::string login_password;
	// �������� (ubuntu.qcow2 ...)
	std::string image_name;
	// ssh���Ӷ˿ڣ�linux��
	uint16_t ssh_port;
	// rdp���Ӷ˿ڣ�windows��
	uint16_t rdp_port;
	// �Զ���˿�ӳ��
	std::vector<std::string> custom_port;
	// cpu��������
	int32_t cpu_sockets;
	// ÿ������cpu����������
	int32_t cpu_cores;
	// ÿ�������˵��߳���
	int32_t cpu_threads;
	// �ڴ棨KB��
	int64_t mem_size;
	// �����̴�С��KB��
	int64_t disk_size;
	// GPU�б�
	std::map<std::string, std::list<std::string>> gpus;
	// �Զ���������·��
	std::string data_file_name;
	// vnc
	uint16_t vnc_port;
	std::string vnc_password;

	// ����ϵͳ(��generic, ubuntu 18.04, windows 10)��Ĭ��ubuntu������win����Ϊ��windowsϵͳ������ȫСд��
	std::string operation_system;
	// BIOSģʽ(��legacy,uefi)��Ĭ�ϴ�ͳBIOS������ȫСд��
	std::string bios_mode;

	//�鲥��ַ(�磺"230.0.0.1:5558")
	std::vector<std::string> multicast;

	// vxlan network name
	std::string network_name;

	int64_t create_time = 0;

    // ����ip
    std::string public_ip;
    //��ȫ�飬ֻ�������˹���ip�Ż�ʹ��
    std::vector<std::string> nwfilter;
};

class TaskManager : public Singleton<TaskManager> {
public:
    TaskManager() = default;

    virtual ~TaskManager() = default;

    FResult init();
    
    void exit();

    void pushTaskEvent(const std::shared_ptr<TaskEvent>& ev);

    TaskStatus queryTaskStatus(const std::string& task_id);
    
    // task
    FResult createTask(const std::string& wallet, const std::shared_ptr<dbc::node_create_task_req_data>& data,
                       int64_t rent_end, USER_ROLE role, std::string& task_id);

    FResult startTask(const std::string& wallet, const std::string &task_id);

    FResult shutdownTask(const std::string& wallet, const std::string &task_id);

	FResult poweroffTask(const std::string& wallet, const std::string& task_id);

    FResult restartTask(const std::string& wallet, const std::string &task_id, bool force_reboot = false);

    FResult resetTask(const std::string& wallet, const std::string &task_id);

    FResult deleteTask(const std::string& wallet, const std::string &task_id);

    FResult modifyTask(const std::string& wallet, const std::shared_ptr<dbc::node_modify_task_req_data>& data);
    
    FResult passwdTask(const std::string& wallet, const std::shared_ptr<dbc::node_passwd_task_req_data>& data);

    FResult
    getTaskLog(const std::string &task_id, QUERY_LOG_DIRECTION direction, int32_t nlines, std::string &log_content);

    void listAllTask(const std::string& wallet, std::vector<std::shared_ptr<TaskInfo>> &vec);

    void listRunningTask(const std::string& wallet, std::vector<std::shared_ptr<TaskInfo>> &vec);

	std::shared_ptr<TaskInfo> findTask(const std::string& wallet, const std::string& task_id);

    int32_t getRunningTasksSize(const std::string& wallet);

    void deleteAllCheckTasks();

    void deleteOtherCheckTasks(const std::string& wallet);

    // session_id
    std::string createSessionId(const std::string& wallet, const std::vector<std::string>& multisig_signers = std::vector<std::string>());

    std::string getSessionId(const std::string& wallet);

    std::string checkSessionId(const std::string& session_id, const std::string& session_id_sign);

    // image
	FResult listImages(const std::shared_ptr<dbc::node_list_images_req_data>& data,
		const AuthoriseResult& result, std::vector<ImageFile>& images);

	FResult downloadImage(const std::string& wallet, const std::shared_ptr<dbc::node_download_image_req_data>& data);

	FResult downloadImageProgress(const std::string& wallet, const std::shared_ptr<dbc::node_download_image_progress_req_data>& data);

	FResult stopDownloadImage(const std::string& wallet, const std::shared_ptr<dbc::node_stop_download_image_req_data>& data);

	FResult uploadImage(const std::string& wallet, const std::shared_ptr<dbc::node_upload_image_req_data>& data);

	FResult uploadImageProgress(const std::string& wallet, const std::shared_ptr<dbc::node_upload_image_progress_req_data>& data);

	FResult stopUploadImage(const std::string& wallet, const std::shared_ptr<dbc::node_stop_upload_image_req_data>& data);

	FResult deleteImage(const std::string& wallet, const std::shared_ptr<dbc::node_delete_image_req_data>& data);

    // snapshot
	FResult listTaskSnapshot(const std::string& wallet, const std::string& task_id, std::vector<dbc::snapshot_info>& snapshots);

    FResult createSnapshot(const std::string& wallet, const std::string& task_id, const std::string &snapshot_name, const ImageServer& imgsvr, const std::string& desc);

	FResult terminateSnapshot(const std::string& wallet, const std::string& task_id, const std::string& snapshot_name);

    // network
	int32_t getTaskAgentInterfaceAddress(const std::string& task_id, std::vector<std::tuple<std::string, std::string>>& address);

    void broadcast_message(const std::string& msg);

protected:
    FResult init_tasks_status();

    void start_task(const std::string &task_id);

    void close_task(const std::string &task_id);

    void remove_iptable_from_system(const std::string& task_id);

    void add_iptable_to_system(const std::string& task_id);

    void shell_remove_iptable_from_system(const std::string& task_id, const std::string &host_ip,
                                          uint16_t ssh_port, const std::string &task_local_ip,
                                          const std::string &public_ip);

    void shell_add_iptable_to_system(const std::string& task_id, const std::string &host_ip,
                                     uint16_t ssh_port, uint16_t rdp_port,
                                     const std::vector<std::string>& custom_port,
                                     const std::string &task_local_ip, const std::string &public_ip);

    void shell_remove_reject_iptable_from_system();

    void delete_task(const std::string& task_id);

    FResult parse_create_params(const std::string &additional, USER_ROLE role, CreateTaskParams& params);

    bool allocate_cpu(int32_t& total_cores, int32_t& sockets, int32_t& cores_per_socket, int32_t& threads);

    bool allocate_gpu(int32_t gpu_count, std::map<std::string, std::list<std::string>>& gpus);

    bool allocate_mem(int64_t mem_size);

    bool allocate_disk(int64_t disk_size);

    bool check_iptables_port_occupied(uint16_t port);

    FResult check_image(const std::string& image_name);

    FResult check_data_image(const std::string& data_image_name);

    FResult check_cpu(int32_t sockets, int32_t cores, int32_t threads);

    FResult check_gpu(const std::map<std::string, std::list<std::string>>& gpus);

    FResult check_disk(const std::map<int32_t, uint64_t>& disks);

    FResult check_operation_system(const std::string& os);

    FResult check_bios_mode(const std::string& bios_mode);

    FResult check_multicast(const std::vector<std::string>& multicast);
    
    void process_task_thread_func();

    void process_task(const std::shared_ptr<TaskEvent>& ev);

    void process_create_task(const std::shared_ptr<TaskEvent>& ev);

    bool create_task_iptable(const std::string &domain_name, uint16_t ssh_port,
                             uint16_t rdp_port, const std::vector<std::string>& custom_port,
                             const std::string &task_local_ip, const std::string& public_ip);

    static void getNeededBackingImage(const std::string &image_name, std::vector<std::string> &backing_images);

    void process_start_task(const std::shared_ptr<TaskEvent>& ev);

    void process_shutdown_task(const std::shared_ptr<TaskEvent>& ev);

    void process_poweroff_task(const std::shared_ptr<TaskEvent>& ev);

    void process_restart_task(const std::shared_ptr<TaskEvent>& ev);

    void process_force_reboot_task(const std::shared_ptr<TaskEvent>& ev);

    void process_reset_task(const std::shared_ptr<TaskEvent>& ev);

    void process_delete_task(const std::shared_ptr<TaskEvent>& ev);

    void process_resize_disk(const std::shared_ptr<TaskEvent>& ev);

    void process_add_disk(const std::shared_ptr<TaskEvent>& ev);

    void process_delete_disk(const std::shared_ptr<TaskEvent>& ev);

    void process_create_snapshot(const std::shared_ptr<TaskEvent>& ev);

    void prune_task_thread_func();

protected:    
    std::atomic<bool> m_running {false};
    std::thread* m_process_thread = nullptr;
	std::mutex m_process_mtx;
	std::condition_variable m_process_cond;
	std::queue<std::shared_ptr<TaskEvent> > m_events;

    std::thread* m_prune_thread = nullptr;
    std::mutex m_prune_mtx;
    std::condition_variable m_prune_cond;

    int32_t m_udp_fd = -1;
};

typedef TaskManager TaskMgr;

#endif
