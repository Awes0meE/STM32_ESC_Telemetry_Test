#include "bsp_hc05.h"

#include <string.h>

#include "app_config.h"
#include "usart.h"

typedef struct
{
    char command_buffer[24];
    uint8_t command_index;
    uint8_t status_request_pending;
    uint8_t stop_request_pending;
} BspHc05Context_t;

static BspHc05Context_t g_hc05;

static void Bsp_Hc05_ProcessCommand(const char *command_text);

void Bsp_Hc05_Init(void)
{
    memset(&g_hc05, 0, sizeof(g_hc05));
}

void Bsp_Hc05_HandleRxByte(uint8_t byte)
{
    char c = (char)byte;

    if ((c == '\r') || (c == '\n'))
    {
        if (g_hc05.command_index > 0U)
        {
            g_hc05.command_buffer[g_hc05.command_index] = '\0';
            Bsp_Hc05_ProcessCommand(g_hc05.command_buffer);
            g_hc05.command_index = 0U;
        }
        return;
    }

    if ((c >= 'a') && (c <= 'z'))
    {
        c = (char)(c - 'a' + 'A');
    }

    if (((c >= 'A') && (c <= 'Z')) || ((c >= '0') && (c <= '9')) || (c == '_'))
    {
        if (g_hc05.command_index < (uint8_t)(sizeof(g_hc05.command_buffer) - 1U))
        {
            g_hc05.command_buffer[g_hc05.command_index++] = c;
            g_hc05.command_buffer[g_hc05.command_index] = '\0';
        }
        else
        {
            g_hc05.command_index = 0U;
        }
    }
    else
    {
        g_hc05.command_index = 0U;
    }
}

void Bsp_Hc05_Write(const char *text)
{
    size_t length;

    if (text == NULL)
    {
        return;
    }

    length = strlen(text);
    if (length == 0U)
    {
        return;
    }

    (void)HAL_UART_Transmit(&huart1, (uint8_t *)text, (uint16_t)length, APP_HC05_UART_TIMEOUT_MS);
}

uint8_t Bsp_Hc05_IsConnected(void)
{
    return (HAL_GPIO_ReadPin(HC05_STATE_GPIO_Port, HC05_STATE_Pin) == GPIO_PIN_SET) ?
        APP_HC05_CONNECTED_LEVEL : 0U;
}

uint8_t Bsp_Hc05_ConsumeStatusRequest(void)
{
    uint8_t pending = g_hc05.status_request_pending;
    g_hc05.status_request_pending = 0U;
    return pending;
}

uint8_t Bsp_Hc05_ConsumeStopRequest(void)
{
    uint8_t pending = g_hc05.stop_request_pending;
    g_hc05.stop_request_pending = 0U;
    return pending;
}

static void Bsp_Hc05_ProcessCommand(const char *command_text)
{
    if (command_text == NULL)
    {
        return;
    }

    if (strcmp(command_text, "STATUS") == 0)
    {
        g_hc05.status_request_pending = 1U;
    }
    else if (strcmp(command_text, "STOP") == 0)
    {
        g_hc05.stop_request_pending = 1U;
    }
}
