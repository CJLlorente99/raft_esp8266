#include "uv.h"

#define LED_DEBUG_PORT 5

/* FSM states */
enum states {
    IDLE,
    RUN,
};

/* Guards */
static int
check_all_handlers_run (fsm_t* this){
    loopFSM_t* p_this = this->user_data;
    if(p_this->last_n_active_handlers == p_this->n_handlers_run){
        return 1;
    }
    return 0;
}

static int
check_is_closing (fsm_t* this){
    loopFSM_t* p_this = this->user_data;
    return p_this->loop_is_closing;
}

static int
check_is_starting (fsm_t* this){
    loopFSM_t* p_this = this->user_data;
    return p_this->loop_is_starting;
}

/* FSM transition functions */

/* Call every handler */
static void
run_handlers (fsm_t* this){
    loopFSM_t* p_this = this->user_data;

    /* First reorder  */
    int j = 0;
    for(int i = 0; i < p_this->n_active_handlers; i++){
        if(p_this->active_handlers[i]->remove){
            free(p_this->active_handlers[i]);
        } else{
            p_this->active_handlers[j++] = p_this->active_handlers[i];
        }
    }

    p_this->n_active_handlers = j;

    /* Second run every handler */
    p_this->loop_is_starting = 0;
    p_this->n_handlers_run = 0;
    uv_update_time(p_this);
    p_this->last_n_active_handlers = p_this->n_active_handlers;
    for(int i = 0; i < p_this->last_n_active_handlers; i++){
        if(p_this->active_handlers[i]->active){
            handle_run(p_this->active_handlers[i]);
        }
        p_this->n_handlers_run++;
    }
}

static void
close_loop (fsm_t* this){
    loopFSM_t* p_this = this->user_data;
    p_this->loop_is_closing = 0;
    p_this->last_n_active_handlers = p_this->n_active_handlers;
    for(int i = 0; i < p_this->last_n_active_handlers; i++){
        free(p_this->active_handlers[i]);
    }
    free(p_this);
    free(this);
}

/* FSM init */
fsm_t* fsm_new_loopFSM (loopFSM_t* loop)
{
	static fsm_trans_t loopFSM_tt[] = {
        { RUN, check_all_handlers_run, RUN, run_handlers },
        { RUN, check_is_closing, IDLE, close_loop},
        { IDLE, check_is_starting, RUN, run_handlers},
		{ -1, NULL, -1, NULL},
	};

	return fsm_new (IDLE, loopFSM_tt, loop);
}

int
uv_loop_init (uv_loop_t* loop){
    loopFSM_t* newLoopFSM = malloc(sizeof(loopFSM_t));

    if(!newLoopFSM){
        ESP_LOGE("LOOP_INIT", "Error during malloc in loop init");
        return 1;
    }

    uint32_t begin = pdTICKS_TO_MS(xTaskGetTickCount());
    if(!begin){
        ESP_LOGE("LOOP_INIT", "Error during gettimeofday in loop init");
        return 1;
    }

    newLoopFSM->last_n_active_handlers = 0;
    newLoopFSM->loop_is_closing = 0;
    newLoopFSM->loop_is_starting = 0;
    newLoopFSM->n_active_handlers = 0;
    newLoopFSM->n_handlers_run = 0;
    newLoopFSM->signal_isr_activated = 0;
    newLoopFSM->time = begin;

    loop->fsm = fsm_new_loopFSM (newLoopFSM);
    loop->loop = loop->fsm->user_data;

    if(!loop->fsm){
        ESP_LOGE("LOOP_INIT", "Error during fsm creation");
        return 1;
    }
    return 0;
}

int
uv_loop_close (uv_loop_t* loop){
    loopFSM_t* this = loop->loop;
    this->loop_is_closing = 1;
    
    free(loop);

    return 0;
}

uint32_t
uv_now(const uv_loop_t* loop){
    loopFSM_t* this = loop->loop;
    return this->time;
}

int
uv_run (uv_loop_t* loop, uv_run_mode mode){
    portTickType xLastTime = xTaskGetTickCount();
    const portTickType xFrequency = LOOP_RATE_MS/portTICK_RATE_MS;
    ESP_LOGI("uv_run", "Entering uv_run");
    loop->loop->loop_is_starting = 1;
    while(true){
       
        fsm_fire(loop->fsm);

        /* Configure timer wakeup for light sleep (does not reset anything) */
        esp_sleep_enable_timer_wakeup(1000*pdTICKS_TO_MS(xLastTime + pdMS_TO_TICKS(LOOP_RATE_MS) - xTaskGetTickCount()));
        esp_light_sleep_start();
        vTaskDelayUntil(&xLastTime, xFrequency);
    }
    return 1;
}

void
uv_update_time(loopFSM_t* loop){
    uint32_t act_time = pdTICKS_TO_MS(xTaskGetTickCount());
    if(!act_time){
        ESP_LOGE("UV_UPDATE_TIME", "Error during gettimeofday in uv_update_time");
        return;
    }
    loop->time = act_time;
}

void
handle_run(uv_handle_t* handle){
    if (!(handle->vtbl->run)){
        ESP_LOGE("HANDLE_RUN", "Error when calling run method in handle_run");
        return;
    }
    handle->vtbl->run(handle);
}

// Como función de la vtbl?
void
uv_close(uv_handle_t* handle, uv_close_cb close_cb){
    int rv = 0;

    if(handle->type == UV_TIMER){
        handle->data = ((uv_timer_t*)handle)->data;
    } else if(handle->type == UV_TCP){
        handle->data = ((uv_tcp_t*)handle)->data;
    } else if(handle->type == UV_STREAM){
        handle->data = ((uv_stream_t*)handle)->data;
    } else if(handle->type == UV_SIGNAL){
        handle->data = ((uv_signal_t*)handle)->data;
    }

    if(close_cb)
        close_cb(handle);

    rv = uv_remove_handle(handle->loop->loop, handle);
    if(rv != 0){
        ESP_LOGE("UV_CLOSE", "Error when calling uv_remove_handle in uv_close");
    }
}

int
uv_is_active(uv_handle_t* handle){
    return handle->active;
}