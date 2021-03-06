#ifndef UV_H
#define UV_H

#include <stddef.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/time.h>
#include <string.h>
#include "ff.h"
#include "fsm.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "lwip/sockets.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "lwip/opt.h"

#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "esp_vfs.h"
#include "esp_system.h"
#include "esp_sleep.h"
#include "esp_heap_caps.h"

/* everything_main */
#define SERVERIP "192.168.0.200"
#define SERVERPORT 50000
#define CLIENTIP "192.168.0.201"

/* tcp main */

#define ID 1

/// Some global constants

#define SIGNAL_TASK_PRIORITY 4
#define LOOP_RATE_MS 10

// fs flags

#define UV_FS_O_APPEND FA_OPEN_APPEND // No se usa
#define UV_FS_O_CREAT   FA_OPEN_ALWAYS
#define UV_FS_O_DIRECT  0
#define UV_FS_O_DIRECTORY   0x50
#define UV_FS_O_DSYNC   0
#define UV_FS_O_EXCL    FA_CREATE_NEW // it does not exactly mean this
#define UV_FS_O_EXLOCK  0
#define UV_FS_O_FILEMAP 0
#define UV_FS_O_NOATIME 0
#define UV_FS_O_NOCTYY  0
#define UV_FS_O_NOFOLLOW    0
#define UV_FS_O_NONBLOCK    0
#define UV_FS_O_RANDOM  0
#define UV_FS_O_RDONLY  FA_READ
#define UV_FS_O_RDWR    FA_WRITE | FA_READ
#define UV_FS_O_SEQUENTIAL  0
#define UV_FS_O_SHORT_LIVED 0
#define UV_FS_O_SYMLINK 0
#define UV_FS_O_SYNC    0
#define UV_FS_O_TEMPORARY   0
#define UV_FS_O_TRUNC   FA_CREATE_ALWAYS
#define UV_FS_O_WRONLY  FA_WRITE

// Enums

typedef enum{
    UV_ENOENT = FR_NO_FILE,
    UV_EACCES = FR_DENIED,
    UV_ENOTDIR = FR_NO_PATH
} uv_errno_t;

typedef enum{
    CONNECT,
    LISTEN,
    ACCEPT,
    READ_START,
    READ_STOP,
    WRITE
}tcp_type;

typedef enum {
    UV_DIRENT_UNKNOWN,
    UV_DIRENT_FILE
} uv_dirent_type_t;

typedef enum {
    UV_UNKNOWN_REQ = 0,
    UV_REQ,
    UV_CONNECT,
    UV_WRITE,
    UV_SHUTDOWN,
    UV_UDP_SEND,
    UV_FS,
    UV_WORK,
    UV_GETADDRINFO,
    UV_GETNAMEINFO,
    UV_RANDOM,
    UV_ACCEPT,
    UV_FS_EVENT_REQ,
    UV_POLL_REQ,
    UV_PROCESS_EXIT,
    UV_READ,
    UV_UDP_RECV,
    UV_WAKEUP,
    UV_SIGNAL_REQ,
    UV_REQ_TYPE_MAX
} uv_req_type;

typedef enum {
    UV_UNKNOWN_HANDLE = 0,
    UV_ASYNC,
    UV_CHECK,
    UV_FS_EVENT,
    UV_FS_POLL,
    UV_HANDLE,
    UV_IDLE,
    UV_NAMED_PIPE,
    UV_POLL,
    UV_PREPARE,
    UV_PROCESS,
    UV_STREAM,
    UV_TCP,
    UV_TIMER,
    UV_TTY,
    UV_UDP,
    UV_SIGNAL,
    UV_FILE,
    UV_HANDLE_TYPE_MAX,
    UV_READ_START
} uv_handle_type;

/* Only UV_RUN_DEFAULT is used anyway */
typedef enum{
    UV_RUN_DEFAULT = 0,
    UV_RUN_ONCE,
    UV_RUN_NOWAIT
}   uv_run_mode;

/// Types declaration

// Basic objects and vtbl

typedef struct handle_vtbl_s handle_vtbl_t;
typedef struct uv_handle_s uv_handle_t;

// Handles

typedef struct uv_loop_s uv_loop_t;
typedef struct uv_signal_s uv_signal_t;
typedef struct uv_timer_s uv_timer_t;
typedef struct uv_tcp_s uv_tcp_t;
typedef struct uv_stream_s uv_stream_t;
typedef struct uv_check_s uv_check_t;
typedef struct uv_poll_s uv_poll_t;

typedef struct uv_write_s uv_write_t;
typedef struct uv_connect_s uv_connect_t;
typedef struct uv_bind_s uv_bind_t;
typedef struct uv_listen_s uv_listen_t;
typedef struct uv_accept_s uv_accept_t;
typedef struct uv_read_start_s uv_read_start_t;
typedef struct uv_read_stop_s uv_read_stop_t;
typedef struct uv_fs_s uv_fs_t;
typedef struct uv_work_s uv_work_t;

// Others

typedef struct loopFSM_s loopFSM_t;
typedef FIL uv_file;
typedef struct uv_dirent_s uv_dirent_t;
typedef struct uv_buf_s uv_buf_t;
typedef struct uv_stat_s uv_stat_t;

// For signal purposes
typedef void (*uv_signal_cb)(uv_signal_t* handle, int signum);

// For timer purposes
typedef void (*uv_timer_cb)(uv_timer_t* handle);

// For stream/tcp purposes
typedef void (*uv_alloc_cb)(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf);
typedef void (*uv_read_cb)(uv_stream_t* stream, ssize_t nread, uv_buf_t* buf);
typedef void (*uv_connection_cb)(uv_stream_t* server, int status);
typedef void (*uv_close_cb)(uv_handle_t* handle);
typedef void (*uv_write_cb)(uv_write_t* req, int status);
typedef void (*uv_connect_cb)(uv_connect_t* req, int status);

// For check purposes
typedef void (*uv_check_cb)(uv_check_t* handle);

// For poll purposes
typedef void (*uv_poll_cb)(uv_poll_t* handle, int status, int events);

// For fs puposes
typedef void (*uv_fs_cb)(uv_fs_t* req);

// For work purposes
typedef void (*uv_work_cb)(uv_work_t* req);
typedef void (*uv_after_work_cb)(uv_work_t* req, int status);

// handle "class"
struct uv_handle_s {
    /* public */
    void* data;
    /* read-only */
    uv_loop_t* loop;
    uv_handle_type type;
    /* private */
    handle_vtbl_t* vtbl;
    int active;
    int remove;
};

// virtual table for every handle
struct handle_vtbl_s {
    void (*run)(uv_handle_t* handle);
};

// polymorphic method
void handle_run(uv_handle_t* handle);

/// Types definition
/* Various */
struct uv_dirent_s {
    char name[256];
    uv_dirent_type_t type;
};

struct uv_stat_s {
    uint32_t st_size;
    int st_mode;
};

/* Requests */

struct uv_write_s {
    uv_handle_t req;
    /* public */
    void* data;
    /* read-only */
    uv_req_type type;
    /* private */
    uv_loop_t* loop;
    uv_write_cb cb;
    uv_buf_t* bufs;
    int nbufs;
    uv_stream_t* stream;
};

struct uv_connect_s {
    uv_handle_t req;
    /* public */
    void* data;
    /* read-only */
    uv_req_type type;
    /* private */
    uv_loop_t* loop;
    const struct sockaddr dest_sockaddr;
    uv_connect_cb cb;
    int status;
    uv_tcp_t* tcp;
};

struct uv_accept_s {
    uv_handle_t req;
    uv_loop_t* loop;
    uv_stream_t* server;
    uv_stream_t* client;
};

struct uv_read_start_s {
    uv_handle_t req;
    uv_loop_t* loop;
    uv_stream_t* stream;
    uv_alloc_cb alloc_cb;
    uv_read_cb read_cb;
    uv_buf_t* buf;
};

struct uv_fs_s {
    uv_handle_t req;
    /* public */
    void* data;
    /* read-only */
    uv_req_type type;
    /* private */
    uv_loop_t* loop;
    uv_fs_cb cb;
    uv_stat_t statbuf;
    char path[255];
    FF_DIR dp;
};

struct uv_work_s {
    uv_handle_t req;
    /* public */
    void* data;
    /* read-only */
    uv_req_type type;
    /* private */
    uv_loop_t* loop;
    uv_work_cb work_cb;
    uv_after_work_cb after_work_cb;
};

/* Handles */

struct uv_poll_s {
    uv_handle_t self;
    /* public */
    void* data;
    /* read-only */
    uv_loop_t* loop;
    uv_handle_type type;
    /* private */
    uv_poll_cb cb;
    int events;
    int fd;
    fd_set readset;
    fd_set writeset;
};


struct uv_check_s {
    uv_handle_t self;
    /* public */
    void* data;
    /* read-only */
    uv_loop_t* loop;
    uv_handle_type type;
    /* private */
    uv_check_cb cb;
};

struct uv_stream_s {
    uv_handle_t self;
    /* public */
    void* data;
    /* read-only */
    uv_loop_t* loop;
    uv_handle_type type;
    /* private */
    int socket;
    uv_handle_t* reqs[40];
    int nreqs;
};


struct uv_tcp_s {
    uv_handle_t self;
    /* public */
    void* data;
    /* read-only */
    uv_loop_t* loop;
    uv_handle_type type;
    /* private */
    int socket;
    uv_handle_t* reqs[40];
    int nreqs;
};

struct uv_buf_s {
    char* base;
    size_t len;
};

struct uv_timer_s {
    uv_handle_t self;
     /* public */
    void* data;
    /* read-only */
    uv_loop_t* loop;
    uv_handle_type type;
    /* private */
    uv_timer_cb timer_cb;
    uint64_t timeout;
    uint64_t repeat;
};

struct uv_signal_s {
    uv_handle_t self;
     /* public */
    void* data;
    /* read-only */
    uv_loop_t* loop;
    uv_handle_type type;
    /* private */
    uv_signal_cb signal_cb;
    int signum; // indicates pin
    uint64_t intr_bit : 1;
};

struct uv_loop_s {
    /* User data */
    void* data;
    /* Private */
    loopFSM_t* loop;
    fsm_t* fsm;
};

//Fsm needed data
struct loopFSM_s
{
    uint32_t time;

    int loop_is_closing : 1;
    int loop_is_starting : 1;
    int signal_isr_activated : 1;

    uv_handle_t* active_handlers[40]; 
    int n_active_handlers; // number of signal handlers
    int last_n_active_handlers;
    int n_handlers_run; // number of signal handlers that have been run
};

// Some function prototypes
void uv_update_time (loopFSM_t* loop);
void uv_close(uv_handle_t* handle, uv_close_cb close_cb);

// Timer function protypes
int uv_timer_init(uv_loop_t* loop, uv_timer_t* handle);
int uv_timer_start(uv_timer_t* handle, uv_timer_cb cb, uint64_t timeout, uint64_t repeat);
int uv_timer_stop(uv_timer_t* handle);
int uv_timer_again(uv_timer_t* handle);

// Signal function prototypes
int uv_signal_init(uv_loop_t* loop, uv_signal_t* handle);
int uv_signal_start(uv_signal_t* handle, uv_signal_cb signal_cb, int signum);
int uv_signal_stop(uv_signal_t* handle);

// Check function prototypes
int uv_check_init(uv_loop_t* loop, uv_check_t* check);
int uv_check_start(uv_check_t* handle, uv_check_cb cb);
int uv_check_stop(uv_check_t* handle);

// Loop function prototypes
int uv_loop_init (uv_loop_t* loop);
int uv_loop_close (uv_loop_t* loop);
int uv_run (uv_loop_t* loop, uv_run_mode mode);
uint32_t uv_now(const uv_loop_t* loop);
int uv_is_active(uv_handle_t* handle);

// TCP function prototypes
int uv_tcp_init(uv_loop_t* loop, uv_tcp_t* tcp);
int uv_tcp_bind(uv_tcp_t* handle, const struct sockaddr* addr, unsigned int flags);
int uv_tcp_connect(uv_connect_t* req, uv_tcp_t* handle, const struct  sockaddr* addr, uv_connect_cb cb);

// Stream function prototypes
int uv_listen(uv_stream_t* stream, int backlog, uv_connection_cb cb);
int uv_accept(uv_stream_t* server, uv_stream_t* client);
int uv_read_start(uv_stream_t* stream, uv_alloc_cb alloc_cb, uv_read_cb read_cb);
int uv_read_stop(uv_stream_t* stream);
int uv_write(uv_write_t* req, uv_stream_t* handle, const uv_buf_t bufs[], unsigned int nbufs, uv_write_cb cb);

// FS function protypes
int uv_fs_close(uv_loop_t* loop, uv_fs_t* req, uv_file file, uv_fs_cb cb);
FIL uv_fs_open(uv_loop_t* loop, uv_fs_t* req, const char* path, int flags, int mode, uv_fs_cb cb);
int uv_fs_write(uv_loop_t* loop, uv_fs_t* req, uv_file* file, const uv_buf_t bufs[], unsigned int nbufs, int64_t offset, uv_fs_cb cb);
int uv_fs_scandir(uv_loop_t* loop, uv_fs_t* req, const char* path, int flags, uv_fs_cb cb);
int uv_fs_scandir_next(uv_fs_t* req, uv_dirent_t* ent);
int uv_fs_stat(uv_loop_t* loop, uv_fs_t* req, const char* path, uv_fs_cb cb);
int uv_fs_rename(uv_loop_t* loop, uv_fs_t* req, const char* path, const char* new_path, uv_fs_cb cb);
int uv_fs_fsync(uv_loop_t* loop, uv_fs_t* req, uv_file file, uv_fs_cb cb);
int uv_fs_ftruncate(uv_loop_t* loop, uv_fs_t* req, uv_file file, int64_t offset, uv_fs_cb cb);
int uv_fs_unlink(uv_loop_t* loop, uv_fs_t* req, const char* path, uv_fs_cb cb);
int uv_fs_read(uv_loop_t* loop, uv_fs_t* req, uv_file file, const uv_buf_t bufs[], unsigned int nbufs, int64_t offset, uv_fs_cb cb);

// work function prototypes
int uv_queue_work(uv_loop_t* loop, uv_work_t* req, uv_work_cb work_cb, uv_after_work_cb after_work_cb);

// Core function prototypes
int uv_insert_handle(loopFSM_t* loop, uv_handle_t* handle);
int uv_remove_handle(loopFSM_t* loop, uv_handle_t* handle);
void add_req_to_stream(uv_stream_t* stream, uv_handle_t* req);
void remove_req_from_stream(uv_stream_t* stream, uv_handle_t* req);

// Miscelaneous function prototypes

int uv_ip4_addr(const char* ip, int port, struct sockaddr_in* addr);

// Main raft
void main_raft(void* ignore);

#endif /* UV_H */
