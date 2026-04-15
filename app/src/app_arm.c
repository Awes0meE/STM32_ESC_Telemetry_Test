#include "app_arm.h"

#include <string.h>

#include "app_config.h"

typedef struct
{
    AppArmStatus_t status;
    uint32_t rx_good_since_ms;
    uint8_t ever_rx_valid;
    uint8_t prev_arm_switch_on;
} AppArmContext_t;

static AppArmContext_t g_arm;

void App_Arm_Init(void)
{
    memset(&g_arm, 0, sizeof(g_arm));
    g_arm.status.state = APP_STATE_BOOT;
}

void App_Arm_Update(const AppArmInputs_t *inputs, uint32_t now_ms)
{
    uint8_t arm_rising_edge;
    uint8_t allow_arm;

    if (inputs == NULL)
    {
        return;
    }

    if (inputs->force_estop_request != 0U)
    {
        g_arm.status.estop_latched = 1U;
        g_arm.status.armed = 0U;
    }

    if (inputs->toggle_estop_request != 0U)
    {
        if (g_arm.status.estop_latched == 0U)
        {
            g_arm.status.estop_latched = 1U;
            g_arm.status.armed = 0U;
        }
        else if ((inputs->rx_active != 0U) &&
                 (inputs->throttle_low != 0U) &&
                 (inputs->arm_switch_on == 0U))
        {
            g_arm.status.estop_latched = 0U;
        }
    }

    if (inputs->rx_active != 0U)
    {
        g_arm.ever_rx_valid = 1U;
        if (g_arm.rx_good_since_ms == 0U)
        {
            g_arm.rx_good_since_ms = now_ms;
        }
    }
    else
    {
        g_arm.rx_good_since_ms = 0U;
        g_arm.status.armed = 0U;
    }

    if (inputs->arm_switch_on == 0U)
    {
        g_arm.status.arm_switch_seen_off = 1U;
    }

    arm_rising_edge = (g_arm.prev_arm_switch_on == 0U) && (inputs->arm_switch_on != 0U);
    g_arm.prev_arm_switch_on = inputs->arm_switch_on;

    if ((g_arm.status.armed != 0U) &&
        ((inputs->rx_active == 0U) ||
         (inputs->arm_switch_on == 0U) ||
         (g_arm.status.estop_latched != 0U)))
    {
        g_arm.status.armed = 0U;
    }

    allow_arm = 0U;
    if ((inputs->rx_active != 0U) &&
        (g_arm.rx_good_since_ms != 0U) &&
        ((now_ms - g_arm.rx_good_since_ms) >= APP_RX_STABLE_MS) &&
        (g_arm.status.estop_latched == 0U) &&
        (inputs->throttle_low != 0U) &&
        (inputs->arm_switch_on != 0U) &&
        (g_arm.status.arm_switch_seen_off != 0U) &&
        (arm_rising_edge != 0U))
    {
        allow_arm = 1U;
    }

    if (allow_arm != 0U)
    {
        g_arm.status.armed = 1U;
    }

    if (g_arm.status.estop_latched != 0U)
    {
        g_arm.status.state = APP_STATE_ESTOP;
        g_arm.status.failsafe_active = 0U;
    }
    else if (inputs->rx_active == 0U)
    {
        g_arm.status.state = (g_arm.ever_rx_valid != 0U) ? APP_STATE_FAILSAFE : APP_STATE_WAIT_RX;
        g_arm.status.failsafe_active = (g_arm.ever_rx_valid != 0U) ? 1U : 0U;
    }
    else if (g_arm.status.armed != 0U)
    {
        g_arm.status.state = APP_STATE_ARMED;
        g_arm.status.failsafe_active = 0U;
    }
    else
    {
        g_arm.status.state = APP_STATE_READY;
        g_arm.status.failsafe_active = 0U;
    }
}

void App_Arm_GetStatus(AppArmStatus_t *out_status)
{
    if (out_status == NULL)
    {
        return;
    }

    *out_status = g_arm.status;
}

const char *App_Arm_GetStateName(AppState_t state)
{
    switch (state)
    {
    case APP_STATE_BOOT:
        return "BOOT";
    case APP_STATE_WAIT_RX:
        return "WAIT_RX";
    case APP_STATE_READY:
        return "READY";
    case APP_STATE_ARMED:
        return "ARMED";
    case APP_STATE_FAILSAFE:
        return "FAILSAFE";
    case APP_STATE_ESTOP:
        return "ESTOP";
    default:
        return "UNKNOWN";
    }
}
