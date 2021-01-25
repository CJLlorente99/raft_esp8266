#include "uv.h"

// EL UNICO USO QUE SE HACE DEL SIGNAL EN RAFT ES PARA CAPTAR SEÑALES DE INTERRUPCION
// GENERADAS POR EL USER

int 
uv_signal_init (loopFSM_t* loop, uv_signal_t* handle){
  handle->loop = loop;
  return 0;
}

int
uv_signal_start(uv_signal_t* handle, uv_signal_cb signal_cb, int signum) {
  // If handler array exists, do some check
  if(handle->loop->active_signal_handlers){
    // Check if handler with same signum
    uv_signal_t** handlers = handle->loop->active_signal_handlers;
    for(int i = 0; i < handle->loop->n_active_signal_handlers; i++){
      if(handlers[i]->signum == signum){
        handle->signal_cb = signal_cb;
        return 0;
      }
    }
  }
  // "Fill" handle

  handle->signal_cb = signal_cb;
  handle->signum = signum;

  // Set signum pin as input
  GPIO_AS_INPUT(signum);

  // If we have achieved to get here, create new handle and add it to signal_handlers 
  uv_signal_t** handlers = handle->loop->active_signal_handlers;
  int i = handle->loop->n_active_signal_handlers; // array index

  if(handle->loop->n_active_signal_handlers == 0){
    *handlers = malloc(sizeof(uv_signal_t));
    memcpy((uv_signal_t*)handlers[0], handle, sizeof(uv_signal_t));
  } else {
    *handlers = realloc(*handlers, sizeof(uv_signal_t));
    memcpy((uv_signal_t*)handlers[i], handle, sizeof(uv_signal_t));
  }

  handle->loop->n_active_signal_handlers++;

  return 0;

}