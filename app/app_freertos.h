#ifndef __APP_FREERTOS_H__
#define __APP_FREERTOS_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "FreeRTOS.h"
#include "task.h"

void app_freertos_init(void);

#ifdef __cplusplus
}
#endif

#endif /* __APP_FREERTOS_H__ */
