/* cmsis_baremetal.c */
#include "stm32f103xb.h"

// Interrupt Service Routine
void EXTI0_IRQHandler(void) {
    // Clear the EXTI0 pending flag
    EXTI->PR |= EXTI_PR_PR0;
    // Toggle the PC13 LED
    GPIOC->ODR ^= (1 << 13);
}

int main(void) {
    // 1. Enable Clocks
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_IOPCEN | RCC_APB2ENR_AFIOEN;
    RCC->APB1ENR |= RCC_APB1ENR_PWREN; 

    // 2. Configure GPIOs
    // PC13: Output, Push-Pull, 2MHz
    GPIOC->CRH &= ~(GPIO_CRH_MODE13 | GPIO_CRH_CNF13); 
    GPIOC->CRH |= GPIO_CRH_MODE13_1; 
    GPIOC->ODR |= (1 << 13); // Turn off LED initially

    // PA0: Input, Pull-up
    GPIOA->CRL &= ~(GPIO_CRL_MODE0 | GPIO_CRL_CNF0); 
    GPIOA->CRL |= GPIO_CRL_CNF0_1; 
    GPIOA->ODR |= (1 << 0); 

    // 3. Configure EXTI & NVIC
    AFIO->EXTICR[0] &= ~AFIO_EXTICR1_EXTI0; // Map EXTI0 to PA0
    EXTI->FTSR |= EXTI_FTSR_TR0;            // Falling edge
    EXTI->IMR |= EXTI_IMR_MR0;              // Unmask EXTI0

    NVIC_EnableIRQ(EXTI0_IRQn);             // Enable EXTI0 in NVIC
    NVIC_SetPriority(EXTI0_IRQn, 1);        // Set Priority

    // 4. Configure Power & Sleep Mode
    PWR->CR &= ~PWR_CR_PDDS;                // Stop Mode
    PWR->CR |= PWR_CR_LPDS;                 // Low-power regulator
    SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;      // Set SLEEPDEEP

    // 5. Main Loop
    while (1) {
        __WFI(); // Wait For Interrupt macro provided by CMSIS
    }
}