#include "uv.h"
#include "freertos/task.h"

// Me gustaria que fuese una funcion que aceptase punteros y dependiendo de un segundo
// argumento supiese que son (para usar la misma funcion para todos los tipos de handlers)
// cuidado con el stack_size -> cual poner?
void
uv_create_task_signal (uv_signal_t* handle){
    signal_cb_param parameters = {handle, handle->signum};
    xTaskCreate(handle->signal_cb, "signal", 2048, (void*) &parameters, SIGNAL_TASK_PRIORITY, NULL);
}