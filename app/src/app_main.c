#include "app_main.h"

#include <stdio.h>
#include <string.h>

#include "adc.h"
#include "app_arm.h"
#include "app_config.h"
#include "app_display.h"
#include "app_motor.h"
#include "app_rc_crsf.h"
#include "bsp_adc_mon.h"
#include "bsp_dshot.h"
#include "bsp_hc05.h"
#include "bsp_key.h"
#include "usart.h"

typedef struct
{
    uint32_t last_status_ms;
} AppMainContext_t;

static AppMainContext_t g_app;
static uint8_t g_uart1_rx_byte;
static uint8_t g_uart2_rx_byte;

static uint16_t App_Main_GetChannelUs(const AppRcCrsfData_t *rc_data, uint8_t channel_1based);
static void App_Main_FormatFixed2(char *dst, size_t len, float value);
static void App_Main_SendStatusLine(uint8_t rx_active,
                                    const AppArmStatus_t *arm_status,
                                    uint16_t throttle_us,
                                    uint16_t dshot_command,
                                    const BspAdcMonData_t *adc_data);
static void App_Main_StartUartReception(void);

void App_Main_Init(void)
{
    memset(&g_app, 0, sizeof(g_app));

    Bsp_Hc05_Init();
    Bsp_Key_Init();
    Bsp_Dshot_Init();
    Bsp_AdcMon_Init();
    App_RcCrsf_Init();
    App_Arm_Init();
    App_Display_Init();
    App_Main_StartUartReception();

    Bsp_Hc05_Write("# boot,fw=ELRS_DSHOT_BENCH,rc=CRSF420000,dshot=PA6,uart1=HC05\r\n");
}

void App_Main_Task(void)
{
    AppRcCrsfData_t rc_data;
    AppArmInputs_t arm_inputs;
    AppArmStatus_t arm_status;
    AppDisplayModel_t display_model;
    const BspAdcMonData_t *adc_data;
    uint32_t now_ms;
    uint16_t throttle_us;
    uint16_t arm_channel_us;
    uint16_t dshot_command;
    uint8_t rx_active;
    uint8_t hc05_stop_request;
    uint8_t hc05_status_request;

    now_ms = HAL_GetTick();

    Bsp_Key_Task(now_ms);
    App_RcCrsf_GetSnapshot(&rc_data);

    rx_active = App_RcCrsf_HasFreshFrame(now_ms, APP_RX_FAILSAFE_TIMEOUT_MS);
    throttle_us = rx_active != 0U ?
        App_Main_GetChannelUs(&rc_data, APP_RC_THROTTLE_CH) : APP_RC_MIN_COMMAND_US;
    arm_channel_us = rx_active != 0U ?
        App_Main_GetChannelUs(&rc_data, APP_RC_ARM_CH) : APP_RC_MIN_COMMAND_US;

    hc05_stop_request = Bsp_Hc05_ConsumeStopRequest();
    hc05_status_request = Bsp_Hc05_ConsumeStatusRequest();

    arm_inputs.rx_active = rx_active;
    arm_inputs.throttle_low = (throttle_us <= APP_RC_LOW_THROTTLE_US) ? 1U : 0U;
    arm_inputs.arm_switch_on = (arm_channel_us >= APP_RC_ARM_ON_US) ? 1U : 0U;
    arm_inputs.toggle_estop_request = Bsp_Key_ConsumeShortPress();
    arm_inputs.force_estop_request = hc05_stop_request;

    App_Arm_Update(&arm_inputs, now_ms);
    App_Arm_GetStatus(&arm_status);

    dshot_command = App_Motor_GetOutputCommand(arm_status.armed, throttle_us);
    Bsp_Dshot_SetThrottle(dshot_command);
    Bsp_Dshot_Task(now_ms);

    Bsp_AdcMon_Task((dshot_command == 0U) ? 1U : 0U, now_ms);
    adc_data = Bsp_AdcMon_GetData();

    display_model.state = arm_status.state;
    display_model.rx_active = rx_active;
    display_model.armed = arm_status.armed;
    display_model.throttle_us = throttle_us;
    display_model.dshot_command = dshot_command;
    display_model.vbat_v = adc_data->vbat_v;
    display_model.current_a = adc_data->current_a;
    App_Display_Task(&display_model, now_ms);

    if (((now_ms - g_app.last_status_ms) >= APP_HC05_STATUS_INTERVAL_MS) ||
        (hc05_status_request != 0U))
    {
        g_app.last_status_ms = now_ms;
        App_Main_SendStatusLine(rx_active, &arm_status, throttle_us, dshot_command, adc_data);
    }
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart == NULL)
    {
        return;
    }

    if (huart->Instance == USART1)
    {
        Bsp_Hc05_HandleRxByte(g_uart1_rx_byte);
        (void)HAL_UART_Receive_IT(&huart1, &g_uart1_rx_byte, 1U);
    }
    else if (huart->Instance == USART2)
    {
        App_RcCrsf_HandleRxByte(g_uart2_rx_byte);
        (void)HAL_UART_Receive_IT(&huart2, &g_uart2_rx_byte, 1U);
    }
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    if (huart == NULL)
    {
        return;
    }

    if (huart->Instance == USART1)
    {
        (void)HAL_UART_Receive_IT(&huart1, &g_uart1_rx_byte, 1U);
    }
    else if (huart->Instance == USART2)
    {
        (void)HAL_UART_Receive_IT(&huart2, &g_uart2_rx_byte, 1U);
    }
}

void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim)
{
    Bsp_Dshot_HandlePulseFinished(htim);
}

static uint16_t App_Main_GetChannelUs(const AppRcCrsfData_t *rc_data, uint8_t channel_1based)
{
    uint8_t index;

    if ((rc_data == NULL) || (channel_1based == 0U) || (channel_1based > APP_RC_CHANNEL_COUNT))
    {
        return APP_RC_MIN_COMMAND_US;
    }

    index = (uint8_t)(channel_1based - 1U);
    return rc_data->us[index];
}

static void App_Main_FormatFixed2(char *dst, size_t len, float value)
{
    int32_t scaled;
    int32_t integer_part;
    int32_t fractional_part;

    if ((dst == NULL) || (len == 0U))
    {
        return;
    }

    if (value < 0.0f)
    {
        value = 0.0f;
    }

    scaled = (int32_t)(value * 100.0f + 0.5f);
    integer_part = scaled / 100;
    fractional_part = scaled % 100;

    (void)snprintf(dst, len, "%ld.%02ld", (long)integer_part, (long)fractional_part);
}

static void App_Main_SendStatusLine(uint8_t rx_active,
                                    const AppArmStatus_t *arm_status,
                                    uint16_t throttle_us,
                                    uint16_t dshot_command,
                                    const BspAdcMonData_t *adc_data)
{
    char line[192];
    char vbat_text[16];
    char current_text[16];
    int length;

    if ((arm_status == NULL) || (adc_data == NULL))
    {
        return;
    }

    App_Main_FormatFixed2(vbat_text, sizeof(vbat_text), adc_data->vbat_v);
    App_Main_FormatFixed2(current_text, sizeof(current_text), adc_data->current_a);

    length = snprintf(line,
                      sizeof(line),
                      "# status,state=%s,rx=%u,arm=%u,estop=%u,thr_us=%u,dshot=%u,vbat=%s,current=%s,bt=%u\r\n",
                      App_Arm_GetStateName(arm_status->state),
                      (unsigned int)rx_active,
                      (unsigned int)arm_status->armed,
                      (unsigned int)arm_status->estop_latched,
                      (unsigned int)throttle_us,
                      (unsigned int)dshot_command,
                      vbat_text,
                      current_text,
                      (unsigned int)Bsp_Hc05_IsConnected());
    if (length > 0)
    {
        Bsp_Hc05_Write(line);
    }
}

static void App_Main_StartUartReception(void)
{
    if (HAL_UART_Receive_IT(&huart1, &g_uart1_rx_byte, 1U) != HAL_OK)
    {
        Error_Handler();
    }

    if (HAL_UART_Receive_IT(&huart2, &g_uart2_rx_byte, 1U) != HAL_OK)
    {
        Error_Handler();
    }
}
