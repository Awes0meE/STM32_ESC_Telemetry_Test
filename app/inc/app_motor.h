#ifndef APP_MOTOR_H
#define APP_MOTOR_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

uint16_t App_Motor_MapThrottleUsToDshot(uint16_t throttle_us);
uint16_t App_Motor_GetOutputCommand(uint8_t armed, uint16_t throttle_us);

#ifdef __cplusplus
}
#endif

#endif /* APP_MOTOR_H */
