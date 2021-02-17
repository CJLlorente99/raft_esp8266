#include "uv.h"

// FSM states
enum states {
    IDLE,
    RUN
};

// Checking functions (static int that return either 1 or 0)
// Hace falta locks??
static int
check_all_handlers_run (fsm_t* this){
    loopFSM_t* p_this = this->user_data;
    if(p_this->n_active_handlers == p_this->n_handlers_run){
        p_this->n_handlers_run = 0;
        return 1;
    }
    return 0;
}

static int
check_is_closing (fsm_t* this){
    loopFSM_t* p_this = this->user_data;
    return p_this->loop_is_closing;
}

// FSM functions (static void)

// call every signal handler
static void
run_handlers (fsm_t* this){
    loopFSM_t* p_this = this->user_data;
    uv_update_time(p_this);
    if(p_this->n_active_handlers > 0){
        for(int i = 0; i < p_this->n_active_handlers; i++){
            uv_update_time(p_this);
            main_handler(p_this->active_handlers[i]);
            p_this->n_handlers_run++;
        }
    }
}

// FSM init

fsm_t* fsm_new_loopFSM (loopFSM_t* loop)
{
	static fsm_trans_t loopFSM_tt[] = {
        { RUN, check_all_handlers_run, RUN, run_handlers },
        { RUN, check_is_closing, IDLE, NULL},
		{ -1, NULL, -1, NULL},
	};

	return fsm_new (SIGNAL, loopFSM_tt, loop);
}

int
uv_loop_init (uv_loop_t* loop){
    loopFSM_t* newLoopFSM;
    memset(loop,0,sizeof(uv_loop_t));
    loop->loopFSM = fsm_new_loopFSM (newLoopFSM);
    return 0;
}

int
uv_loop_close (uv_loop_t* loop){
    loopFSM_t* this = loop->loopFSM->user_data; // es necesario poner esto?
    this->loop_is_closing = 1;
    return 0;
}

uint32_t
uv_now(const uv_loop_t* loop){
    loopFSM_t* this = loop->loopFSM->user_data;
    return this->time;
}

// TODO, MEJOR CREAR UN TIMER QUE LLAME A fsm_fire periodicamente?
int
uv_run (uv_loop_t* loop){ // uv_run_mode is not neccesary as only one mode is used in raft
    while(true){
        vTaskDelay(LOOP_RATE_MS/portTICK_RATE_MS);
        fsm_fire(loop->loopFSM);
    }
    return 1;
}

void
uv_update_time(loopFSM_t* loop){
    loop->time = (uint64)system_get_time()/1000;
}

void
main_handler(uv_handle_t* handle){
    handle_type type = handle->type;
    
    switch (type){
    case SIGNAL: {
        uv_signal_t* signal = handle->handle_signal;
        run_signal(signal);
        break; }

    case CHECK: {
        uv_check_t* check = handle->handle_check;
        check->cb(check);
        break; }
    
    case TIMER: {
        uv_timer_t* timer = handle->handle_timer;
        run_timer(timer);
        break; }

    default:
        break;
    }
}

void
run_signal(uv_signal_t* signal){
    if(signal->intr_bit){
        signal->signal_cb(signal, signal->signum);
        signal->intr_bit = 0;
    }
    else{
        signal->intr_bit = 0;
    }
}

void
run_timer(uv_timer_t* timer){
    loopFSM_t* loop = timer->loop->loopFSM->user_data;

    if(timer->timeout >= loop->time){
        uv_timer_stop(timer);
        uv_timer_again(timer);
        timer->timer_cb(timer);
    }
}