#ifndef DBC_TIMER_MATRIX_MANAGER_H
#define DBC_TIMER_MATRIX_MANAGER_H

#include "util/utils.h"
#include "network/nio_loop_group.h"
#include "network/protocol/service_message.h"
#include "service_module/service_module.h"

#define DEFAULT_TIMER_MATRIX_THREAD_COUNT           1
#define DEFAULT_TIMER_INTERVAL                      100 //unit:ms for one tick

class timer_matrix_manager : public std::enable_shared_from_this<timer_matrix_manager>
{
    typedef void (timer_handler_type)(const boost::system::error_code &);

public:
    timer_matrix_manager() = default;

    virtual ~timer_matrix_manager() = default;

    ERRCODE init();

    void exit();

    static uint64_t get_cur_tick() { return m_cur_tick; }

protected:
    void start_timer();

    void stop_timer();

    std::shared_ptr<dbc::network::message> make_time_tick_notification();

    void on_timer_expired(const boost::system::error_code& error);

private:
    std::shared_ptr<dbc::network::nio_loop_group> m_timer_group;

    std::shared_ptr<steady_timer> m_timer;

    static std::atomic<std::uint64_t> m_cur_tick;
};

#endif
