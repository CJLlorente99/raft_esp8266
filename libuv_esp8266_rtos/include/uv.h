#ifndef UV_H
#define UV_H

#include <stddef.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "fsm.h"
#include "gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "esp_common.h"
#include "espconn.h"

/// Some global constants

#define SIGNAL_TASK_PRIORITY 4
#define LOOP_RATE_MS 100

/// Declaration

typedef struct handle_vtbl_s handle_vtbl_t;

typedef struct uv_loop_s uv_loop_t;
typedef struct uv_signal_s uv_signal_t;
typedef struct uv_timer_s uv_timer_t;
typedef struct uv_tcp_s uv_tcp_t;
typedef struct uv_tcp_s uv_stream_t;
typedef struct uv_buf_s uv_buf_t;
typedef struct uv_check_s uv_check_t;
typedef struct uv_handle_s uv_handle_t;
typedef struct uv_write_s uv_write_t;
typedef struct uv_connect_s uv_connect_t;

typedef struct signal_cb_param_s signal_cb_param_t;

typedef struct loopFSM_s loopFSM_t;

// For signal purposes
typedef void (*uv_signal_cb)(uv_signal_t* handle, int signum);

// For timer purposes
typedef void (*uv_timer_cb)(uv_timer_t* handle);

// For stream/tcp purposes
typedef void (*uv_alloc_cb)(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf);
typedef void (*uv_read_cb)(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf);
typedef void (*uv_connection_cb)(uv_stream_t* server, int status);
typedef void (*uv_close_cb)(uv_handle_t* handle);
typedef void (*uv_write_cb)(uv_write_t* req, int status);
typedef void (*uv_connect_cb)(uv_connect_t* req, int status);

// For check purposes
typedef void (*uv_check_cb)(uv_check_t* handle);

// handle "class"
struct uv_handle_s {
    handle_vtbl_t *vtbl;
    uv_loop_t* loop;
};

// virtual table for every handle
struct handle_vtbl_s {
    void (*run)(uv_handle_t* handle);
};

// polymorphic method
void handle_run(uv_handle_t* handle);

/// Types definition

struct uv_write_s {

};

struct uv_connect_s {

};

struct uv_check_s {
    uv_handle_t* self;
    uv_check_cb cb;
};

struct uv_tcp_s {
    uv_handle_t* self;
    uv_loop_t* loop;
    uint64 flags;
    uv_read_cb read_cb;
    uv_alloc_cb alloc_cb;
    uv_connection_cb connection_cb;
    uv_close_cb close_cb;
    uv_handle_t* self;
    struct espconn* espconn_s;
};

struct uv_buf_s {
    char* base;
    size_t len;
};

struct uv_timer_s {
    uv_handle_t* self;
    uv_timer_cb timer_cb;
    uint64 timeout;
    uint64 repeat;
};

struct uv_signal_s {
    uv_handle_t* self;
    uv_signal_cb signal_cb;
    int signum; // indicates pin
    uint64 intr_bit : 1;
};

struct uv_loop_s {
    fsm_t* loopFSM;
};

//Fsm needed data
struct loopFSM_s
{
    uint64 time;

    int loop_is_closing : 1;

    uv_handle_t** active_handlers; // asi, al añadir nuevos handler no hace falta volver a crear el fsm_t. con este puntero y el numero de handlers itero sobre todos
    int n_active_handlers; // number of signal handlers
    int n_handlers_run; // number of signal handlers that have been run
};

// Some function prototypes

void uv_update_time (loopFSM_t* loop);

// Timer function protypes

int uv_timer_init(uv_loop_t* loop, uv_timer_t* handle);
int uv_timer_start(uv_timer_t* handle, uv_timer_cb cb, uint64_t timeout, uint64_t repeat);
int uv_timer_stop(uv_timer_t* handle);
int uv_timer_again(uv_timer_t* handle);

// Signal function prototypes

int uv_signal_init(uv_loop_t* loop, uv_signal_t* handle);
int uv_signal_start(uv_signal_t* handle, uv_signal_cb signal_cb, int signum);
int uv_signal_stop(uv_signal_t* handle);

void signal_isr(loopFSM_t* loop);

// Check function prototypes

int uv_check_init(uv_loop_t* loop, uv_check_t* check);
int uv_check_start(uv_check_t* handle, uv_check_cb cb);
int uv_check_close(uv_check_t* handle);

// Loop function prototypes

int uv_loop_init (uv_loop_t* loop);
int uv_loop_close (uv_loop_t* loop);
uint32_t uv_now(const uv_loop_t* loop);
int uv_run (uv_loop_t* loop);

<<<<<<< HEAD
void run_signal(uv_signal_t* signal);
void run_timer(uv_timer_t* timer);
void main_handler(uv_handle_t* handler);

// sockaddr

typedef struct sockaddr{
    unsigned short sa_family;
    char sa_data[14];
}sockaddr;
=======
// Core function prototypes
int remove_handle(loopFSM_t* loop, uv_handle_t* handle);
int insert_handle(loopFSM_t* loop, uv_handle_t* handle);
>>>>>>> develop_oo-handles

#endif /* UV_H */
