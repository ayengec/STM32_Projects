/* hal_framework.c */
#include "stm32f1xx_hal.h"

void SystemClock_Config(void);

// HAL EXTI Callback - Automatically called by HAL's hidden ISR
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    if (GPIO_Pin == GPIO_PIN_0) {
        // Toggle the PC13 LED
        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
    }
}

int main(void) {
    // 1. Initialize the HAL Library
    HAL_Init();

    // 2. Enable Clocks
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_PWR_CLK_ENABLE();

    // 3. Configure GPIOs using structs
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // PC13 LED Configuration
    GPIO_InitStruct.Pin = GPIO_PIN_13;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP; 
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET); // Turn off LED

    // PA0 Button & EXTI Configuration
    GPIO_InitStruct.Pin = GPIO_PIN_0;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING; 
    GPIO_InitStruct.Pull = GPIO_PULLUP;          
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    // 4. Configure NVIC
    HAL_NVIC_SetPriority(EXTI0_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(EXTI0_IRQn);

    // 5. Main Loop
    while (1) {
        // Enter Stop Mode
        HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI);
        
        // System Clock must be reconfigured after waking up from Stop Mode
        // SystemClock_Config(); 
    }
}

// Dummy clock config for completeness (requires actual oscillator setup in real usage)
void SystemClock_Config(void) {
    // Reconfigure clocks to HSI/HSE after waking up from STOP mode
}