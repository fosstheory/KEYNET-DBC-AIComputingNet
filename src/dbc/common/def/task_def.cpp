#include "task_def.h"

std::string task_operation_string(int32_t op) {
    std::string operation_string = "none";
    switch(op) {
        case T_OP_Create:
            operation_string = "create";
            break;
        case T_OP_Start:
            operation_string = "start";
            break;
        case T_OP_Stop:
            operation_string = "stop";
            break;
        case T_OP_ReStart:
            operation_string = "restart";
            break;
        case T_OP_Reset:
            operation_string = "reset";
            break;
        case T_OP_Delete:
            operation_string = "delete";
            break;
        default:
            break;
    }

    return operation_string;
}

std::string task_status_string(int32_t status) {
    std::string status_string = "closed";
    switch (status) {
        case TS_ShutOff:
            status_string = "closed";
            break;
        case TS_Creating:
            status_string = "creating";
            break;
        case TS_Running:
            status_string = "running";
            break;
        case TS_Starting:
            status_string = "starting";
            break;
        case TS_Stopping:
            status_string = "stopping";
            break;
        case TS_Restarting:
            status_string = "restarting";
            break;
        case TS_Resetting:
            status_string = "resetting";
            break;
        case TS_Deleting:
            status_string = "deleting";
            break;
        case TS_Error:
            status_string = "error";
            break;
        case TS_CreateError:
            status_string = "create error";
            break;
        case TS_StartError:
            status_string = "start error";
            break;
        case TS_RestartError:
            status_string = "restart error";
            break;
        case TS_ResetError:
            status_string = "reset error";
            break;
        case TS_DeleteError:
            status_string = "delete error";
            break;
        default:
            break;
    }

    return status_string;
}

std::string vm_status_string(int32_t status) {
    std::string status_string = "none";
    switch (status) {
        case VS_None:
            status_string = "none";
            break;
        case VS_PAUSED:
            status_string = "paused";
            break;
        case VS_RUNNING:
            status_string = "running";
            break;
        case VS_SHUT_OFF:
            status_string = "shut_off";
            break;
        default:
            break;
    }

    return status_string;
}

/*
std::string to_training_task_status_string(int8_t status) {
    switch (status) {
        case task_status_unknown:
            return "task_status_unknown";
        case task_status_queueing:
            return "task_status_queueing";
        case task_status_running: {
            //std::string task_state = { 0x1b,0x5b,0x33,0x32,0x6d};  //green
            //task_state += "task_status_running";
            //task_state += {0x1b,0x5b,0x30,0x6d};
            return "task_status_running";
        }
        case task_status_stopped:
            return "task_status_stopped";
        case task_successfully_closed:
            return "task_successfully_closed";
        case task_abnormally_closed: {
            //std::string task_state = { 0x1b,0x5b,0x33,0x31,0x6d }; //red
            //task_state += "task_abnormally_closed";
            //task_state += {0x1b, 0x5b, 0x30, 0x6d};
            return "task_abnormally_closed";
        }
        case task_overdue_closed:
            return "task_overdue_closed";
        case task_status_pulling_image:
            return "task_status_pulling_image";
        case task_noimage_closed:
            return "task_noimage_closed";
        case task_nospace_closed:
            return "task_nospace_closed";
        case task_status_out_of_gpu_resource:
            return "task_status_out_of_gpu_resource";
        case task_status_update_error:
            return "task_status_update_error";
        case task_status_creating_image:
            return "task_status_creating_image";
        default:
            return DEFAULT_STRING;
    }
}

std::string engine_reg;

bool check_task_engine(const std::string &engine) {
    if (engine_reg.empty()) {
        return engine.length() != 0;
    }
    try {
        cregex reg = cregex::compile(engine_reg);
        return regex_match(engine.c_str(), reg);
    }
    catch (...) {
        return false;
    }

    return false;
}

void set_task_engine(std::string engine) {
    engine_reg = engine;
}
*/
