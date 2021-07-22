#include "env_manager.h"
#include <event2/thread.h>
#include <event2/event.h>
#include <event2/http.h>
#include <cstdlib>
#include <csignal>
#include "server/server.h"

void signal_usr1_handler(int)
{
    g_server->set_exited();
    close(STDIN_FILENO); // let the cli exit
}

boost::filesystem::path env_manager::m_conf_path;

boost::filesystem::path env_manager::m_dat_path;

boost::filesystem::path env_manager::m_db_path;

boost::filesystem::path env_manager::m_peer_path;

boost::filesystem::path env_manager::m_container_path;

boost::filesystem::path env_manager::m_vm_path;

boost::filesystem::path env_manager::m_home_path;
boost::filesystem::path env_manager::m_tool_path;

endian_type env_manager::m_endian_type = unknown_endian;

int32_t env_manager::init(bpo::variables_map &options)
{
    //init signal
    init_signal();
    LOG_DEBUG << "init env: os signal handler";

    //init locale
    init_locale();
    LOG_DEBUG << "init env: core locale set";

    //init endian type
    init_endian_type();
    LOG_DEBUG << "init env: core endian type";

    //init path env
    init_core_path();
    LOG_DEBUG << "init env: core path environment";

    // init global libevent config
    init_libevent_config();

    return E_SUCCESS;
}

void env_manager::init_endian_type()
{
    uint32_t a = 0x12345678;
    uint8_t  *p = (uint8_t *)(&a);
    m_endian_type = (*p == 0x78) ? little_endian : big_endian;
}

void env_manager::init_core_path()
{
    //home path
    m_home_path = util::get_exe_dir();

    //conf file full path
    m_conf_path = m_home_path;
    m_conf_path /= boost::filesystem::path(CONF_DIR_NAME);
    m_conf_path /= boost::filesystem::path(CONF_FILE_NAME);

    //dat file full path
    m_dat_path = m_home_path;
    m_dat_path /= boost::filesystem::path(DAT_DIR_NAME);

    //db file full path
    m_db_path = m_home_path;
    m_db_path /= boost::filesystem::path(DB_DIR_NAME);

    //addr file full path
    m_peer_path = m_home_path;
    m_peer_path /= boost::filesystem::path(CONF_DIR_NAME);
    m_peer_path /= boost::filesystem::path(PEER_FILE_NAME);

    m_container_path = m_home_path;
    m_container_path /= boost::filesystem::path(CONF_DIR_NAME);
    m_container_path /= boost::filesystem::path(CONTAINER_FILE_NAME);


    m_vm_path = m_home_path;
    m_vm_path /= boost::filesystem::path(CONF_DIR_NAME);
    m_vm_path /= boost::filesystem::path(VM_FILE_NAME);

    m_tool_path  = m_home_path;
    m_tool_path /= boost::filesystem::path(TOOL_DIR_NAME);
}

void env_manager::init_signal()
{
    signal(SIGPIPE, SIG_IGN);
    signal(SIGTERM, SIG_IGN);
    signal(SIGINT, SIG_IGN);
    //signal(SIGHUP, SIG_IGN);  // dbc in daemon process alwasy ignore SIGHUP; and dbc client in normal process should terminate with SIGHUP.
    //signal(SIGTTIN, SIG_IGN);
    //signal(SIGTTOU, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);  // ignore job control signal, e.g. Ctrl-z

    register_signal_function(SIGUSR1, signal_usr1_handler);
}

void env_manager::init_locale()
{
    try
    {
        std::locale("");
    }
    catch (const std::runtime_error&) {
        setenv("LC_ALL", "C", 1);
    }

    std::locale loc = boost::filesystem::path::imbue(std::locale::classic());
    boost::filesystem::path::imbue(loc);
}

void env_manager::register_signal_function(int signal, void(*handler)(int))
{
    struct sigaction sa;
    sa.sa_handler = handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(signal, &sa, nullptr);
}

void env_manager::init_libevent_config()
{
    // Redirect libevent's logging to our own log
    event_set_log_callback(&env_manager::libevent_log_cb);
    // Update libevent's log handling. Returns false if our version of
    // libevent doesn't support debug logging, in which case we should
    // clear the BCLog::LIBEVENT flag.
    // default libevent log close,libevent log open if debug mode
    update_http_server_logging(0);

    evthread_use_pthreads();
}

void env_manager::libevent_log_cb(int severity, const char *msg)
{
#ifndef EVENT_LOG_WARN
// EVENT_LOG_WARN was added in 2.0.19; but before then _EVENT_LOG_WARN existed.
# define EVENT_LOG_WARN _EVENT_LOG_WARN
#endif
    if (severity >= EVENT_LOG_WARN) {  // Log warn messages and higher without debug category
        std::string  tmp_msg = "libevent: " + std::string(msg);
        LOG_DEBUG << tmp_msg;
    }
}

void env_manager::update_http_server_logging(bool enable)
{
#if LIBEVENT_VERSION_NUMBER >= 0x02010100
    if (enable) {
        event_enable_debug_logging(EVENT_DBG_ALL);
    } else {
        event_enable_debug_logging(EVENT_DBG_NONE);
    }
    return;
#else
    // Can't update libevent logging if version < 02010100
    return;
#endif
}
