#include "stm32f4xx_hal.h"
#include "uart_manager.h"

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t size)
{
    UART_Manager_RxEventCallback(huart, size);
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    UART_Manager_TxCpltCallback(huart);
}
