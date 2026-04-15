#include "app_motor.h"

#include "app_config.h"

uint16_t App_Motor_MapThrottleUsToDshot(uint16_t throttle_us)
{
    uint32_t scaled;

    if (throttle_us <= APP_RC_LOW_THROTTLE_US)
    {
        return 0U;
    }

    if (throttle_us >= APP_RC_MAX_COMMAND_US)
    {
        return APP_DSHOT_MAX_DEMO;
    }

    scaled = (uint32_t)(throttle_us - APP_RC_LOW_THROTTLE_US) *
        (uint32_t)(APP_DSHOT_MAX_DEMO - APP_DSHOT_MIN_THROTTLE);
    scaled /= (uint32_t)(APP_RC_MAX_COMMAND_US - APP_RC_LOW_THROTTLE_US);

    return (uint16_t)(APP_DSHOT_MIN_THROTTLE + scaled);
}

uint16_t App_Motor_GetOutputCommand(uint8_t armed, uint16_t throttle_us)
{
    uint16_t command;

    if (armed == 0U)
    {
        return 0U;
    }

    command = App_Motor_MapThrottleUsToDshot(throttle_us);
    if (command > APP_DSHOT_MAX_THROTTLE)
    {
        command = APP_DSHOT_MAX_THROTTLE;
    }

    if ((command != 0U) && (command < APP_DSHOT_MIN_THROTTLE))
    {
        command = APP_DSHOT_MIN_THROTTLE;
    }

    return command;
}
