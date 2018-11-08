/*********************************************************************************
*  Copyright (c) 2017-2018 DeepBrainChain core team
*  Distributed under the MIT software license, see the accompanying
*  file COPYING or http://www.opensource.org/licenses/mit-license.php
* file name         : cmd_line_service.cpp
* description     : cmd line api service
* date                   : 2018.03.25
* author              : Bruce Feng
**********************************************************************************/
#include <iostream>
#include <limits>
#include <cstdint>
#include "cmd_line_service.h"
#include "util.h"
#include "server.h"
#ifndef WIN32
#include "readline/readline.h"
#include "readline/history.h"
#endif

#include "log.h"
#include "filter/simple_expression.h"
#include <chrono>
#include <future>
#include <ctime>

static void cmd_line_task()
{
   static ai::dbc::cmd_line_service *service = (ai::dbc::cmd_line_service *)(g_server->get_module_manager()->get(CMD_LINE_API_MODULE).get());
    if (nullptr != service)
    {        
        service->on_usr_cmd();
    }    
}

using namespace std;
using namespace matrix::core;


// readline auto complete function
#ifndef WIN32
char **character_name_completion(const char *, int, int);
char *character_name_generator(const char *, int);

const char *character_names[] = {
        "task",
        "node",
        "start",
        "stop",
        "list",
        "clear",
        "show",
        "system",
        "quit",
        "peers",
        "result",
        "logs",
        "ps",
        "parameter",
        NULL
};

char **
character_name_completion(const char *text, int start, int end)
{
    rl_attempted_completion_over = 0;
    return rl_completion_matches(text, character_name_generator);
}

char *
character_name_generator(const char *text, int state)
{
    static int list_index, len;
    const char *name;

    if (!state)
    {
        list_index = 0;
        len = strlen(text);
    }

    while ((name = character_names[list_index++]))
    {
        if (strncmp(name, text, len) == 0)
        {
            return strdup(name);
        }
    }

    return NULL;
}
#endif

namespace ai
{
    namespace dbc
    {
        std::string get_line_from_cin() {
            std::string line;
            std::getline(std::cin, line);
            return line;
        }

        static void print_cmd_usage()
        {
            cout << "we support commands: " << endl;
            cout << "help:          print out usage information" << endl;
            cout << "peers:         get information of peers" << endl;
            cout << "task:          manipulate ai training task" << endl;
            cout << "node:          query ai node status" << endl;
            cout << "clear:         clean screen" << endl;
            cout << "system / sys:  invoke system command" << endl;
            cout << "quit / exit:   exit program" << endl;
            cout << "-----------------------------------------" << endl;
        }

        static void print_cmd_node_usage()
        {
            cout << "show:          get attributes from taget node" <<endl;
            cout << "-----------------------------------------" << endl;
        }

        static void print_cmd_task_usage()
        {
            cout << "start:         start training" << endl;
            cout << "stop:          stop training" << endl;
            cout << "start_multi:   start multi training tasks" << endl;
            cout << "result:        download training result" <<endl;
            cout << "list:          list training tasks" << endl;
            cout << "logs:          get logs of task" << endl;
            cout << "ps:            print ai request task cache info" << endl;
            cout << "clear:         remove abnormal task" << endl;
            cout << "-----------------------------------------" << endl;
        }

        cmd_line_service::cmd_line_service()
        {
            m_argvs.clear();
            memset(m_cmd_line_buf, 0x00, sizeof(m_cmd_line_buf));

            init_cmd_invoker();
        }

        int32_t cmd_line_service::init(bpo::variables_map &options)
        { 
            cout << "Welcome to DeepBrain Chain Decentralized AI world!" << std::endl;
            cout << std::endl;

            g_server->bind_idle_task(&cmd_line_task);
            return E_SUCCESS;
        }

        void cmd_line_service::init_cmd_invoker()
        {
            m_invokers["start"] = std::bind(&cmd_line_service::start_training, this, std::placeholders::_1, std::placeholders::_2);
            m_invokers["stop"] = std::bind(&cmd_line_service::stop_training, this, std::placeholders::_1, std::placeholders::_2);
            m_invokers["start_multi"] = std::bind(&cmd_line_service::start_multi_training, this, std::placeholders::_1, std::placeholders::_2);
            m_invokers["list"] = std::bind(&cmd_line_service::list_training, this, std::placeholders::_1, std::placeholders::_2);
            m_invokers["peers"] = std::bind(&cmd_line_service::get_peers, this, std::placeholders::_1, std::placeholders::_2);
            m_invokers["result"] = std::bind(&cmd_line_service::result, this, std::placeholders::_1, std::placeholders::_2);
            m_invokers["logs"] = std::bind(&cmd_line_service::logs, this, std::placeholders::_1, std::placeholders::_2);
            m_invokers["show"] = std::bind(&cmd_line_service::show, this, std::placeholders::_1, std::placeholders::_2);
            m_invokers["clear"] = std::bind(&cmd_line_service::clear, this, std::placeholders::_1, std::placeholders::_2);
            m_invokers["ps"] = std::bind(&cmd_line_service::ps, this, std::placeholders::_1, std::placeholders::_2);
            m_invokers["system"] = std::bind(&cmd_line_service::system_cmd, this, std::placeholders::_1, std::placeholders::_2);
            m_invokers["sys"] = m_invokers["system"];

            
            m_invokers["task"] = std::bind(&cmd_line_service::task, this, std::placeholders::_1, std::placeholders::_2);
            m_invokers["task_clear"] = std::bind(&cmd_line_service::task_clean, this, std::placeholders::_1, std::placeholders::_2);
            m_invokers["task_list"] = m_invokers["list"];
            m_invokers["task_start"] = m_invokers["start"];
            m_invokers["task_stop"] = m_invokers["stop"];
            m_invokers["task_result"] = m_invokers["result"];
            m_invokers["task_logs"] = m_invokers["logs"];
            m_invokers["task_start_multi"] = m_invokers["start_multi"];
            m_invokers["task_ps"] = m_invokers["ps"];


            m_invokers["node"] = std::bind(&cmd_line_service::node, this, std::placeholders::_1, std::placeholders::_2);
            m_invokers["node_show"] = m_invokers["show"];


        }
        
        void cmd_line_service::on_usr_cmd()
        {
#ifdef WIN32
            cout << "dbc>>> ";

            //read user input
            cin.get(m_cmd_line_buf, MAX_CMD_LINE_BUF_LEN);
            cin.clear();
            cin.ignore((std::numeric_limits<int>::max)(), '\n');
#else
            try
            {
                rl_attempted_completion_function = character_name_completion;

                auto line = readline("dbc>>> ");
                if (line == nullptr)
                {
                     //LOG_ERROR << "readline return nullptr";
                      return;
                }

                if (line[0]!=0)
                {
                    // save the cmd if it is not empty
                    add_history(line);
                }

                strncpy(m_cmd_line_buf,line,MAX_CMD_LINE_BUF_LEN);
                m_cmd_line_buf[MAX_CMD_LINE_BUF_LEN-1]=0;

                free(line);
            }
            catch (const std::exception &e)
            {
                return;
            }
            
#endif

#ifdef WIN32
            m_argvs = bpo::split_winmain(m_cmd_line_buf);
#else
            try
            {
                m_argvs = bpo::split_unix(m_cmd_line_buf);
            }
            catch (const boost::exception & e)
            {
                return;
            }
#endif

            if (0 == m_argvs.size())
            {
                return;
            }

            std::string cmd = m_argvs[0];
            if (cmd.compare("help") == 0)
            {
                print_cmd_usage();
                return;
            }
            else if (cmd.compare("quit") == 0 || cmd.compare("exit") == 0)
            {
                g_server->set_exited(true);
                return;
            }

            auto it = m_invokers.find(cmd);
            if (it == m_invokers.end())
            {
                cout << "unknown command, for prompt please input 'help'" << endl;
                return;
            }

            //call handler function
            auto func = it->second;
            char ** argv = new char* [m_argvs.size()];
            for (uint32_t i=0; i<m_argvs.size(); i++ )
            {
                argv[i] = (char *)m_argvs[i].c_str();
            }
            func(m_argvs.size(), argv);
            delete []argv;
        }

        void cmd_line_service::start_training(int argc, char* argv[])
        {
            bpo::variables_map vm;
            options_description opts("start task options");

            try
            {
                opts.add_options()
                    ("help,h", "start task")
                    ("config,c", bpo::value<std::string>(), "task config file path")
                    ("node,n", bpo::value< std::string >(), "the target ai training node, ignore the peer_nodes_list field in the config file if present.");

                //parse
                bpo::store(bpo::parse_command_line(argc, argv, opts), vm);
                bpo::notify(vm);

                if (vm.count("help") || vm.count("h"))
                {
                    cout << opts;
                    return;
                }


                // read other param from cmd line
                std::shared_ptr<cmd_start_training_req> req = std::make_shared<cmd_start_training_req>();
                if (vm.count("node"))
                {
                    req->parameters["node"] = vm["node"].as<std::string>();

                }

                if (vm.count("config") || vm.count("c"))
                {

                    std::string task_id;
                    {
                        req->task_file_path = vm["config"].as<std::string>();

                        auto resp = m_handler.invoke<cmd_start_training_req, cmd_start_training_resp>(req);
                        if (nullptr == resp)
                        {
                            cout << endl << "command time out" << endl;
                            return;
                        }
                        else
                        {
                            task_id = resp->task_info.task_id;

                            format_output(resp);

                            if(resp -> result != E_SUCCESS)
                            {
                                return;
                            }

                            m_last_task_id = task_id;
                        }
                    }

                    sleep(1);  // wait a second to avoid race condition between start_training_req and list_training_req in network.

                    // fetch task status from network
                    {
                        std::shared_ptr<cmd_list_training_req> req(new cmd_list_training_req);
                        req->list_type = LIST_SPECIFIC_TASKS;
                        std::vector<std::string> task_vector;
                        task_vector.push_back(task_id);
                        std::copy(std::make_move_iterator(task_vector.begin()),
                                  std::make_move_iterator(task_vector.end()), std::back_inserter(req->task_list));
                        task_vector.clear();

                        auto resp = m_handler.invoke<cmd_list_training_req, cmd_list_training_resp>(req);
                        if (nullptr == resp)
                        {
                            cout << endl << "warning: fail to fetch task status; please check if the target ai node is alive." << endl;
                        }

                    }
                }
                else
                {
                    cout << argv[0] << " invalid option" << endl;
                    cout << opts;
                }
            }
            catch (...)
            {
                cout << argv[0] << " invalid option" << endl;
                cout << opts;
            }
        }
        
        void cmd_line_service::stop_training(int argc, char* argv[])
        {
            bpo::variables_map vm;
            options_description opts("stop task options");

            try
            {
                opts.add_options()
                    ("help,h", "stop task")
                    ("task,t", bpo::value<std::string>()->default_value(m_last_task_id), "task id")
                        ("force,f", bpo::value<bool>()->default_value(false));

                //parse
                bpo::store(bpo::parse_command_line(argc, argv, opts), vm);
                bpo::notify(vm);

                if (vm.count("help") || vm.count("h"))
                {
                    cout << opts;
                    return;
                }

                bool force = false;
                if(vm.count("force"))
                {
                    force = vm["force"].as<bool>();
                }

                if (vm.count("task") || vm.count("t"))
                {
                    std::shared_ptr<cmd_stop_training_req> req = std::make_shared<cmd_stop_training_req>();
                    req->task_id = vm["task"].as<std::string>();
                    if(req->task_id.empty())
                    {
                        cout << opts;
                        return;
                    }
                    m_last_task_id = req->task_id;

                    if (!force)
                    {
                        cout << "stop task: " << req->task_id << ": [y/n] ";
                        std::string answer;
                        cin >> answer;
                        if (answer != std::string("y") && answer != std::string("Y"))
                        {
                            return;
                        }
                    }

                    std::shared_ptr<cmd_stop_training_resp> resp = m_handler.invoke<cmd_stop_training_req, cmd_stop_training_resp>(req);
                    if (nullptr == resp)
                    {
                        cout << endl << "command time out" << endl;
                    }
                    else
                    {
                        format_output(resp);
                    }
                }
                else
                {
                    cout << argv[0] << " invalid option" << endl;
                    cout << opts;
                }
            }
            catch (...)
            {
                cout << argv[0] << " invalid option" << endl;
                cout << opts;
            }
        }
        
        void cmd_line_service::start_multi_training(int argc, char* argv[])
        {
            bpo::variables_map vm;
            options_description opts("start multi task options");

            try
            {
                opts.add_options()
                    ("help,h", "start multi task")
                    ("config,c", bpo::value<std::string>(), "task config file path");

                //parse
                bpo::store(bpo::parse_command_line(argc, argv, opts), vm);
                bpo::notify(vm);

                if (vm.count("config") || vm.count("c"))
                {
                    std::shared_ptr<cmd_start_multi_training_req> req = std::make_shared<cmd_start_multi_training_req>();
                    req->mulit_task_file_path = vm["config"].as<std::string>();

                    std::shared_ptr<cmd_start_multi_training_resp> resp = m_handler.invoke<cmd_start_multi_training_req, cmd_start_multi_training_resp>(req);
                    if (nullptr == resp)
                    {
                        cout << endl << "command time out" << endl;
                    }
                    else
                    {
                        format_output(resp);
                    }
                }
                else if (vm.count("help") || vm.count("h"))
                {
                    cout << opts;
                }
                else
                {
                    cout << argv[0] << " invalid option" << endl;
                    cout << opts;
                }
            }
            catch (...)
            {
                cout << argv[0] << " invalid option" << endl;
                cout << opts;
            }
        }
        
        void cmd_line_service::list_training(int argc, char* argv[])
        {
            bpo::variables_map vm;
            options_description opts("list task options");

            try
            {
                opts.add_options()
                    ("help,h", "list task")
                    ("all,a", "list all task")
                    ("task,t", bpo::value<std::string>()->default_value(m_last_task_id), "list a task");

                //parse
                bpo::store(bpo::parse_command_line(argc, argv, opts), vm);
                bpo::notify(vm);

                if (vm.count("help") || vm.count("h"))
                {
                    cout << opts;
                    return;
                }

                if (vm.count("all") || vm.count("a"))
                {
                    std::shared_ptr<cmd_list_training_req> req= std::make_shared<cmd_list_training_req>();
                    req->list_type = LIST_ALL_TASKS;

                    std::shared_ptr<cmd_list_training_resp> resp = m_handler.invoke<cmd_list_training_req, cmd_list_training_resp>(req);
                    if (nullptr == resp)
                    {
                        cout << endl << "command time out" << endl;
                    }
                    else
                    {
                        format_output(resp);
                    }
                }
                else if (vm.count("task") || vm.count("t"))
                {
                    std::string task_id = vm["task"].as<std::string>();
                    if (task_id.empty())
                    {
                        cout << opts;
                        return;
                    }


                    std::shared_ptr<cmd_list_training_req> req(new cmd_list_training_req);
                    req->list_type = LIST_SPECIFIC_TASKS;
                    req->task_list.push_back(task_id);

                    std::shared_ptr<cmd_list_training_resp> resp = m_handler.invoke<cmd_list_training_req, cmd_list_training_resp>(req);
                    if (nullptr == resp)
                    {
                        cout << endl << "command time out" << endl;
                    }
                    else
                    {
                        format_output(resp);
                        if(resp->result == E_SUCCESS)
                        {
                            m_last_task_id = task_id;
                        }
                    }
                }
                else
                {
                    cout << argv[0] << " invalid option" << endl;
                    cout << opts;
                }
            }
            catch (...)
            {
                cout << argv[0] << " invalid option" << endl;
                cout << opts;
            }
        }
        
        void cmd_line_service::get_peers(int argc, char* argv[])
        {
            bpo::variables_map vm;
            options_description opts("list peers options");

            try
            {
                opts.add_options()
                    ("help,h", "list peers")
                    ("active,a", "list active peers")
                    ("global,g", "list global peers");

                //parse
                bpo::store(bpo::parse_command_line(argc, argv, opts), vm);
                bpo::notify(vm);

                if (vm.count("active") || vm.count("a") || vm.count("global") || vm.count("g"))
                {
                    std::shared_ptr<cmd_get_peer_nodes_req> req = std::make_shared<cmd_get_peer_nodes_req>();
                    if (vm.count("active") || vm.count("a"))
                    {
                        req->flag = matrix::service_core::flag_active;
                    }
                    else if (vm.count("global") || vm.count("g"))
                    {
                        req->flag = matrix::service_core::flag_global;
                    }

                    std::shared_ptr<cmd_get_peer_nodes_resp> resp = m_handler.invoke<cmd_get_peer_nodes_req, cmd_get_peer_nodes_resp>(req);
                    if (nullptr == resp)
                    {
                        cout << endl << "command time out" << endl;
                    }
                    else
                    {
                        cmd_get_peer_nodes_resp_formater formater(resp);
                        formater.format_output(req->flag);
                    }
                }
                else if (vm.count("help") || vm.count("h"))
                {
                    cout << opts;
                }
                else
                {
                    cout << argv[0] << " invalid option" << endl;
                    cout << opts;
                }
            }
            catch (...)
            {
                cout << argv[0] << " invalid option" << endl;
                cout << opts;
            }
        }

        void cmd_line_service::logs(int argc, char* argv[])
        {
            bpo::variables_map vm;
            options_description opts("task logs options");

            try
            {
                opts.add_options()
                    ("help,h", "get task running logs")
                    ("tail", bpo::value<std::string>(), "get log from tail")
                    ("head", bpo::value<std::string>(), "get log from head")
                    ("task,t", bpo::value<std::string>()->default_value(m_last_task_id), "task id")
                    ("flush,f", "flush log periodically, click 'q' to end");

                //parse
                bpo::store(bpo::parse_command_line(argc, argv, opts), vm);
                bpo::notify(vm);

                if (vm.count("help") || vm.count("h"))
                {
                    cout << opts;
                    return;
                }

                if (vm.count("task") || vm.count("t"))
                {
                    std::shared_ptr<cmd_logs_req> req = std::make_shared<cmd_logs_req>();
                    req->task_id = vm["task"].as<std::string>();

                    if(req->task_id.empty())
                    {
                        cout << opts;
                        return;
                    }

                    m_last_task_id = req->task_id;

                    std::string number_of_lines;

                    //tail or head
                    if (vm.count("tail"))
                    {
                        req->head_or_tail = GET_LOG_TAIL;          //get log from tail
                        number_of_lines = vm["tail"].as<std::string>();
                    }
                    else if (vm.count("head"))
                    {
                        req->head_or_tail = GET_LOG_HEAD;          //get log from head
                        number_of_lines = vm["head"].as<std::string>();
                    }
                    else
                    {
                        req->head_or_tail = GET_LOG_TAIL;          //get log from tail
                    }

                    //number of lines
                    if (number_of_lines == "all")
                    {
                        req->number_of_lines = 0;
                    }
                    else if (number_of_lines == DEFAULT_STRING)
                    {
                        req->number_of_lines = DEFAULT_NUMBER_OF_LINES;
                    }
                    else
                    {
                        uint64_t lines = (uint64_t) std::stoull(number_of_lines);
                        //req->number_of_lines = lines > MAX_NUMBER_OF_LINES ? MAX_NUMBER_OF_LINES : lines;
                        if (lines > MAX_NUMBER_OF_LINES)
                        {
                            cout << argv[0] << "number_of_lines must < " << MAX_NUMBER_OF_LINES << endl;
                            cout << opts;
                            return;
                        }

                        req->number_of_lines = (uint16_t)lines;
                    }

                    int interval = 0;
                    bool loop = false;
                    if (vm.count("flush") || vm.count("f"))
                    {
                        interval = LOG_AUTO_FLUSH_INTERVAL_IN_SECONDS;
                        loop = true;

                        req->head_or_tail = GET_LOG_TAIL;
                        req->number_of_lines = DEFAULT_NUMBER_OF_LINES;

                        cmd_logs_resp::m_series.enable = true;
                        cmd_logs_resp::m_series.last_log_date = "";
                        cmd_logs_resp::m_series.image_download_logs.clear();
                    }
                    else
                    {
                        cmd_logs_resp::m_series.enable = false;
                    }

                    // loop is true if log auto flush is enabled.
                    std::future<std::string> future;
                    if (loop)
                    {
                        future = std::async(std::launch::async, get_line_from_cin);
                    }

                    do
                    {
                        // fetch log from remote ai training node
                        std::shared_ptr<cmd_logs_resp> resp = m_handler.invoke<cmd_logs_req, cmd_logs_resp>(req);
                        if (nullptr == resp)
                        {
                            cout << endl << "command time out" << endl;
                        }
                        else
                        {
                            format_output(resp);
                        }

                        if (!loop)
                        {
                            return;
                        }

                        // repeat fetch log action after a certain time; or exit immediatelly if quit by operator.
                        if (future.wait_for(std::chrono::seconds(interval)) == std::future_status::ready)
                        {
                            if (future.get() == std::string("q")) return;

                            future = std::async(std::launch::async, get_line_from_cin);
                        }

                    }while(cmd_logs_resp::m_series.enable);

                    cout << endl << "task exec completed, press enter to continue" << endl;
                    cmd_logs_resp::m_series.last_log_date = "";
                    cmd_logs_resp::m_series.image_download_logs.clear();
                }
                else
                {
                    cout << argv[0] << " invalid option" << endl;
                    cout << opts;
                }

            }
            catch (...)
            {
                cout << argv[0] << " invalid option" << endl;
                cout << opts;
            }
        }

        template<typename resp_type>
        void cmd_line_service::format_output(std::shared_ptr<resp_type> resp)
        {
            if (nullptr == resp)
            {
                return;
            }

            //format output
            resp->format_output();
        }

        void cmd_line_service::clear(int argc, char* argv[])
        {
            bpo::variables_map vm;
            options_description opts("clear info options");
            try
            {
                opts.add_options()
                        ("help,h", "clear help")
                        //("all,a", "clear all info in this node")
                        ("screen,s", "clear screen");
                std::string cmd = "";
#if  defined(__linux__) || defined(MAC_OSX)
                cmd = "clear";
#elif defined(WIN32)
                cmd = "cls";

#endif
                //parse
                bpo::store(bpo::parse_command_line(argc, argv, opts), vm);
                bpo::notify(vm);

                if (vm.count("help") || vm.count("h"))
                {
                    cout << opts;
                    return;
                }

                if (vm.count("screen") || vm.count("s"))
                {
                    system(cmd.c_str());
                    return;
                }

               /* if (vm.count("all") || vm.count("a"))
                {
                    std::shared_ptr<cmd_clear_req> req = std::make_shared<cmd_clear_req>();
                    std::shared_ptr<cmd_clear_resp> resp = m_handler.invoke<cmd_clear_req, cmd_clear_resp>(req);
                    system("cls");
                    return;
                }*/

                system(cmd.c_str());

            }
            catch (...)
            {
                cout << argv[0] << " invalid option" << endl;
                cout << opts;
            }
        }

        void cmd_line_service::ps(int argc, char* argv[])
        {
            bpo::variables_map vm;
            options_description opts("clear info options");
            try
            {
                opts.add_options()
                    ("help,h", "clear help")
                    ("all,a","show all task cache state")
                    ("task,t",bpo::value<std::string>(), "show assign task cache state");

                //parse
                bpo::store(bpo::parse_command_line(argc, argv, opts), vm);
                bpo::notify(vm);

                if (vm.count("help") || vm.count("h"))
                {
                    cout << opts;
                    return;
                }

                if (vm.count("task") || vm.count("t"))
                {
                    std::shared_ptr<cmd_ps_req> req = std::make_shared<cmd_ps_req>();
                    req->task_id = vm["task"].as<std::string>();;
                    std::shared_ptr<cmd_ps_resp> resp = m_handler.invoke<cmd_ps_req, cmd_ps_resp>(req);
                    format_output(resp);
                    return;
                }

                if (vm.count("all") || vm.count("a"))
                {
                    std::shared_ptr<cmd_ps_req> req = std::make_shared<cmd_ps_req>();
                    req->task_id = "all";;
                    std::shared_ptr<cmd_ps_resp> resp = m_handler.invoke<cmd_ps_req, cmd_ps_resp>(req);
                    format_output(resp);
                    return;
                }
            }
            catch (...)
            {
                cout << argv[0] << " invalid option" << endl;
                cout << opts;
            }
        }


        void cmd_line_service::show(int argc, char* argv[])
        {
            bpo::variables_map vm;
            options_description opts("show node info options");

            try
            {
                opts.add_options()
                        ("help,h", "show service info and specific node info")
                        ("node,n", bpo::value<std::string>(), "print node's hardware info of indicated node id")
                        ("keys,k", bpo::value<std::vector<std::string>>(), "if -n is specified, only print the value of indicated attribute.")
                        ("service,s", "print nodes' service info in the network")
                        ("filter,f", bpo::value<std::vector<std::string>>(), "if -s is specified, only print node matchs the filter.")
                        ("order,o", bpo::value<std::string>(), "if -s is specified, print node order by specific field.")
                        ("interval,i", bpo::value<int>(), "time interval in seconds, refresh info periodically. Press 'q' to exit");

                //parse
                bpo::store(bpo::parse_command_line(argc, argv, opts), vm);
                bpo::notify(vm);

                std::shared_ptr<cmd_show_req> req= std::make_shared<cmd_show_req>();

                if (vm.count("help") || vm.count("h"))
                {
                    cout << opts;
                    return;
                }

                if (vm.count("node") || vm.count("n"))
                {
                    req->o_node_id = CONF_MANAGER->get_node_id();
                    req->d_node_id = vm["node"].as<std::string>();
                    req->op = OP_SHOW_NODE_INFO;


                    if (vm.count("keys") || vm.count("k"))
                    {
                        std::vector<std::string> tmp = vm["keys"].as<std::vector<std::string>>();
                        std::copy(std::make_move_iterator(tmp.begin()), std::make_move_iterator(tmp.end()), std::back_inserter(req->keys));
                        tmp.clear();
                    }
                    else
                    {
                        // default key: all
                        req->keys.push_back("all");
                    }
                }
                else if (vm.count("service") || vm.count("s"))
                {
                    req->op = OP_SHOW_SERVICE_LIST;


                    if (vm.count("filter") || vm.count("f"))
                    {
                        std::vector<std::string> tmp = vm["filter"].as<std::vector<std::string>>();

                        req->filter = "";
                        for (auto &s: tmp)
                        {
                            if (req->filter.length() != 0)
                            {
                                req->filter += (" and " + s);
                            }
                            else
                            {
                                req->filter += s;
                            }
                        }

                        expression e(req->filter);
                        if (!e.is_valid())
                        {
                            cout << endl << "invalid filter " << req->filter << endl;
                            return;
                        }
                    }

                    if (vm.count("order") || vm.count("o"))
                    {
                        req->set_sort(vm["order"].as<std::string>());
                    }

                }

                if(OP_SHOW_UNKNOWN == req->op)
                {
                    cout << argv[0] << " invalid option" << endl;
                    cout << opts;
                    return;
                }


                int interval = 0;
                if (vm.count("interval") || vm.count("i"))
                {
                    interval = vm["interval"].as<int>();

                    if (interval < 5)
                    {
                        interval = 5;
                    }
                }


                bool loop = (interval > 0);

                //loop ...
                std::future<std::string> future;
                if (loop)
                {
                    future = std::async(std::launch::async, get_line_from_cin);
                }

                do
                {
                    if (loop)
                    {
                        #if  defined(__linux__) || defined(MAC_OSX)
                            system("clear");
                        #elif defined(WIN32)
                            system("cls");
                        #endif

                        const time_t t = time(0);
                        std::cout << asctime(localtime(&t)) << std::endl;
                    }

                    // display service list
                    {
                        std::shared_ptr<cmd_show_resp> resp = m_handler.invoke<cmd_show_req, cmd_show_resp>(req);


                        if (nullptr == resp)
                        {
                            cout << endl << "command time out" << endl;
                        }
                        else
                        {
                            LOG_DEBUG << "cmd_show_resp received!";
                            format_output(resp);
                        }
                    }

                    if(!loop)
                    {
                        return;
                    }

                    std::cout << "Press q and then enter to exit " << std::endl;

                    if (future.wait_for(std::chrono::seconds(interval)) == std::future_status::ready)
                    {
                        auto line = future.get();
                        if ( line == std::string("q") ) return;

                        future = std::async(std::launch::async, get_line_from_cin);
                    }

                }while(loop);

            }
            catch (...)
            {
                cout << argv[0] << " invalid option" << endl;
                cout << opts;
            }
        }


        void cmd_line_service::result(int argc, char* argv[])
        {
            bpo::variables_map vm;
            options_description opts("result options");

            try
            {
                opts.add_options()
                ("help,h", "download training result")
                ("task,t", bpo::value<std::string>()->default_value(m_last_task_id), "task id")
                ("output,o", bpo::value<std::string>(), "target folder to save the training result, /tmp by default");

                //parse
                bpo::store(bpo::parse_command_line(argc, argv, opts), vm);
                bpo::notify(vm);

                if (vm.count("task") || vm.count("t"))
                {
                    std::shared_ptr<cmd_logs_req> req = std::make_shared<cmd_logs_req>();
                    req->task_id = vm["task"].as<std::string>();

                    if(req->task_id.empty())
                    {
                        cout << opts;
                        return;
                    }
                    m_last_task_id = req->task_id;

                    std::string number_of_lines;

                    req->head_or_tail = GET_LOG_TAIL;

                    req->number_of_lines = 20;

                    req->sub_op = "result";

#if  defined(__linux__) || defined(MAC_OSX)
                    req->dest_folder = "/tmp";
#elif defined(WIN32)
                    req->dest_folder = "%TMP%";
#endif

                    if (vm.count("output") || vm.count("o"))
                    {
                        req->dest_folder = vm["output"].as<std::string>();
                    }


                    {
                        std::shared_ptr<cmd_logs_resp> resp = m_handler.invoke<cmd_logs_req, cmd_logs_resp>(req);
                        if (nullptr == resp)
                        {
                            cout << endl << "command time out" << endl;
                        }
                        else
                        {
                            format_output(resp);
                        }
                    }


                }
                else if (vm.count("help") || vm.count("h"))
                {
                    cout << opts;
                }
                else
                {
                    cout << argv[0] << " invalid option" << endl;
                    cout << opts;
                }

            }
            catch (...)
            {
                cout << argv[0] << " invalid option" << endl;
                cout << opts;
            }

        }

        void cmd_line_service::system_cmd(int argc, char* argv[])
        {
            try
            {
                std::string cmd;
                for(int i=1; i<argc; i++)
                {
                    cmd += argv[i];
                    cmd += " ";
                }

                //cout << "system: " << cmd <<endl;
                if ( cmd.empty())
                {
                    cout << "system [cmd], e.g. system date" <<endl;
                }
                else
                {
                    system(cmd.c_str());
                }

            }
            catch (...)
            {
                cout << argv[0] << " invalid option" << endl;
            }
        }
        
        
        void cmd_line_service::task_clean(int argc, char* argv[])
        {
            std::shared_ptr<cmd_task_clean_req> req= std::make_shared<cmd_task_clean_req>();


            auto resp = m_handler.invoke<cmd_task_clean_req, cmd_task_clean_resp>(req);
            if (nullptr == resp)
            {
                cout << endl << "command time out" << endl;
            }
            else
            {
                format_output(resp);
            }
            
        }

        void cmd_line_service::task(int argc, char* argv[])
        {
            if(argc == 1)
            {
                print_cmd_task_usage();
                return;
            }

            try
            {
                if (argc > 1)
                {
                    std::string cmd = std::string(argv[0]) + "_" + std::string(argv[1]);
                    auto it = m_invokers.find(cmd);
                    if (it == m_invokers.end())
                    {
                        cout << "unknown command " << argv[1] << endl;
                        return;
                    }
                    
                    //call handler function
                    auto func = it->second;
                    
                    func(argc-1, argv+1);
                    
                }
            }
            catch (...)
            {

            }

        }

        void cmd_line_service::node(int argc, char* argv[])
        {
            if(argc == 1)
            {
                print_cmd_node_usage();
                return;
            }

            try
            {
                if (argc > 1)
                {
                    std::string cmd = std::string(argv[0]) + "_" + std::string(argv[1]);
                    auto it = m_invokers.find(cmd);
                    if (it == m_invokers.end())
                    {
                        cout << "unknown command " << argv[1] << endl;
                        return;
                    }

                    //call handler function
                    auto func = it->second;

                    func(argc-1, argv+1);

                }
            }
            catch (...)
            {

            }

        }

    }

}
