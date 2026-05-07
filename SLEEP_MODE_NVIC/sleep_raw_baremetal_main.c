/* sleep_raw_baremetal_main.c */
// Memory map definitions
#define PERIPH_BASE       0x40000000
#define APB1PERIPH_BASE   PERIPH_BASE
#define APB2PERIPH_BASE   (PERIPH_BASE + 0x10000)
#define AHBPERIPH_BASE    (PERIPH_BASE + 0x20000)

// Peripheral base addresses
#define RCC_BASE          (AHBPERIPH_BASE + 0x1000)
#define GPIOA_BASE        (APB2PERIPH_BASE + 0x0800)
#define GPIOC_BASE        (APB2PERIPH_BASE + 0x1000)
#define AFIO_BASE         (APB2PERIPH_BASE + 0x0000)
#define EXTI_BASE         (APB2PERIPH_BASE + 0x0400)
#define PWR_BASE          (APB1PERIPH_BASE + 0x7000)

// RCC Registers
#define RCC_APB2ENR       (*(volatile unsigned int *)(RCC_BASE + 0x18))
#define RCC_APB1ENR       (*(volatile unsigned int *)(RCC_BASE + 0x1C))

// GPIO Registers
#define GPIOA_CRL         (*(volatile unsigned int *)(GPIOA_BASE + 0x00))
#define GPIOA_ODR         (*(volatile unsigned int *)(GPIOA_BASE + 0x0C))
#define GPIOC_CRH         (*(volatile unsigned int *)(GPIOC_BASE + 0x04))
#define GPIOC_ODR         (*(volatile unsigned int *)(GPIOC_BASE + 0x0C))

// AFIO & EXTI Registers
#define AFIO_EXTICR1      (*(volatile unsigned int *)(AFIO_BASE + 0x08))
#define EXTI_IMR          (*(volatile unsigned int *)(EXTI_BASE + 0x00))
#define EXTI_FTSR         (*(volatile unsigned int *)(EXTI_BASE + 0x0C))
#define EXTI_PR           (*(volatile unsigned int *)(EXTI_BASE + 0x14))

// NVIC & SCB Registers (Cortex-M3 Core internal addresses)
#define NVIC_ISER0        (*(volatile unsigned int *)(0xE000E100))
#define NVIC_IPR1         (*(volatile unsigned int *)(0xE000E404)) 
#define SCB_SCR           (*(volatile unsigned int *)(0xE000ED10))

// PWR Registers
#define PWR_CR            (*(volatile unsigned int *)(PWR_BASE + 0x00))

// Interrupt Service Routine (Must match the linker script definition)
void EXTI0_IRQHandler(void) {
    EXTI_PR |= (1 << 0);       // Clear EXTI0 pending flag
    GPIOC_ODR ^= (1 << 13);    // Toggle PC13 LED
}

int main(void) {
    // 1. Enable Clocks
    RCC_APB2ENR |= (1 << 2) | (1 << 4) | (1 << 0); // GPIOA, GPIOC, AFIO
    RCC_APB1ENR |= (1 << 28);                      // PWR

    // 2. Configure GPIOs
    GPIOC_CRH &= ~(0xF << 20); // Clear PC13 configuration bits
    GPIOC_CRH |=  (0x2 << 20); // Set PC13 as Output 2MHz Push-Pull
    GPIOC_ODR |=  (1 << 13);   // Turn off LED initially (Active low)

    GPIOA_CRL &= ~(0xF << 0);  // Clear PA0 configuration bits
    GPIOA_CRL |=  (0x8 << 0);  // Set PA0 as Input with pull-up/down
    GPIOA_ODR |=  (1 << 0);    // Activate Pull-up for PA0

    // 3. Configure EXTI & NVIC
    AFIO_EXTICR1 &= ~(0xF << 0); // Map EXTI0 line to PA0
    EXTI_FTSR |= (1 << 0);       // Set falling edge trigger for EXTI0
    EXTI_IMR |= (1 << 0);        // Unmask (enable) EXTI0 line
    
    // EXTI0 is IRQ6. Priority is set in top 4 bits of the 3rd byte of IPR1 (bits 20-23)
    NVIC_IPR1 |= (1 << 20);      // Set Priority to 1
    NVIC_ISER0 |= (1 << 6);      // Enable EXTI0 (IRQ 6) in NVIC ISER0

    // 4. Configure Power & Sleep Mode
    PWR_CR &= ~(1 << 1);         // Clear PDDS bit (Choose Stop mode over Standby)
    PWR_CR |= (1 << 0);          // Set LPDS bit (Low power voltage regulator in Stop mode)
    SCB_SCR |= (1 << 2);         // Set SLEEPDEEP bit in Cortex-M3 core

    // 5. Main Loop
    while (1) {
        // CPU cannot understand C-level sleep, must use inline assembly
        __asm volatile("wfi");   
    }
}
