#include "esp32.h"
#include "../BSP/board.h"
#include "../BSP/board_config.h"

typedef enum
{
    ESP32_SM_IDLE = 0,
    ESP32_SM_RESET_LOW,
    ESP32_SM_RESET_RELEASE,
    ESP32_SM_BOOT_EN_LOW,
    ESP32_SM_BOOT_EN_HIGH
} esp32_sm_t;

static esp32_sm_t s_state = ESP32_SM_IDLE;
static uint32_t s_deadline_ms;

static void ESP32_Schedule(esp32_sm_t state, uint32_t delay_ms)
{
    s_state = state;
    s_deadline_ms = Board_Millis() + delay_ms;
}

void ESP32_Init(void)
{
    ESP32_SetIO0(true);
    ESP32_SetEN(true);
}

void ESP32_SetEN(bool level)
{
    Board_WritePin(ESP32_EN_GPIO, ESP32_EN_PIN, level ? ESP32_EN_ACTIVE_LEVEL : ESP32_EN_INACTIVE_LEVEL);
}

void ESP32_SetIO0(bool level)
{
    Board_WritePin(ESP32_IO0_GPIO, ESP32_IO0_PIN, level ? ESP32_IO0_RUN_LEVEL : ESP32_IO0_BOOT_LEVEL);
}

void ESP32_Reset(void)
{
    ESP32_SetEN(false);
    ESP32_Schedule(ESP32_SM_RESET_LOW, ESP32_RESET_DELAY_MS);
}

void ESP32_EnterBootloader(void)
{
    ESP32_SetIO0(false);
    ESP32_SetEN(false);
    ESP32_Schedule(ESP32_SM_BOOT_EN_LOW, ESP32_RESET_DELAY_MS);
}

void ESP32_ExitBootloader(void)
{
    ESP32_SetIO0(true);
    ESP32_Reset();
}

void ESP32_Task(void)
{
    if (s_state == ESP32_SM_IDLE) {
        return;
    }

    if ((int32_t)(Board_Millis() - s_deadline_ms) < 0) {
        return;
    }

    switch (s_state) {
    case ESP32_SM_RESET_LOW:
        ESP32_SetEN(true);
        ESP32_Schedule(ESP32_SM_RESET_RELEASE, ESP32_BOOT_DELAY_MS);
        break;
    case ESP32_SM_RESET_RELEASE:
        s_state = ESP32_SM_IDLE;
        break;
    case ESP32_SM_BOOT_EN_LOW:
        ESP32_SetEN(true);
        ESP32_Schedule(ESP32_SM_BOOT_EN_HIGH, ESP32_BOOT_DELAY_MS);
        break;
    case ESP32_SM_BOOT_EN_HIGH:
        s_state = ESP32_SM_IDLE;
        break;
    case ESP32_SM_IDLE:
    default:
        s_state = ESP32_SM_IDLE;
        break;
    }
}
