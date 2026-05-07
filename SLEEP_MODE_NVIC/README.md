# STM32 Deep Sleep & EXTI Wake-up — The Evolution of Bare Metal

A hands-on demonstration of low-power **Stop Mode** and **External Interrupt (EXTI) wake-up** on the STM32F103C8T6 (Blue Pill), implemented three ways — from raw memory pointers to HAL — so you can see exactly what each abstraction layer is doing underneath.

---

## The Concept

Most embedded tutorials show you *how* to make something work. This project shows you *why* it works — by implementing the exact same behavior at three different levels of abstraction:

| Level | File | Approach |
|---|---|---|
| **Raw Bare Metal** | `sleep_raw_baremetal_main.c` | Direct memory address manipulation via C pointers. Zero headers, zero libraries. |
| **CMSIS Bare Metal** | `sleep_cmsis_baremetal_main.c` | Uses `stm32f103xb.h` provided by ST, register macros, but keeps full bitwise control. |
| **HAL Framework** | `sleep_cmsis_baremetal_main.c` | STM32 HAL — the standard approach in production projects. |

All three files produce identical behavior. Reading them side by side is the point.

---

## Behavior

```
Power on → Stop Mode (CPU halted, clocks gated)
               ↓
    User presses button (PA0 → GND, falling edge)
               ↓
    EXTI0 interrupt fires → NVIC wakes CPU
               ↓
    ISR: clear pending flag → toggle PC13 LED
               ↓
    Return from ISR → WFI → Stop Mode again
```

The microcontroller spends virtually all of its time asleep. It wakes up only to service the interrupt, toggles the LED, and immediately goes back to sleep.

---

## Hardware

### Components
- STM32F103C8T6 (Blue Pill)
- Tactile push button
- 10kΩ pull-up resistor (or use internal pull-up as configured in code)
- LED + 330Ω resistor (PC13 has a built-in LED on most Blue Pill boards)
- ST-Link V2 programmer

### Wiring

```
STM32F103C8T6
┌─────────────────────────────────────┐
│                                     │
│  PA0 ──────────────── Button ── GND │  (10kΩ pull-up to 3.3V)
│                                     │
│  PC13 ─── 330Ω ─── LED ──── GND     │  (Active LOW — LED on when PC13 = 0)
│                                     │
│  3.3V ─── 10kΩ ─── PA0              │  (Pull-up)
│                                     │
└─────────────────────────────────────┘
```

### Pin Summary

| Pin | Direction | Function |
|---|---|---|
| PA0 | Input (pull-up) | Button input — EXTI0 source |
| PC13 | Output (push-pull, 2MHz) | LED — active low |

---

## How Each Level Works

### Level 1 — Raw Bare Metal

No headers. Every peripheral is accessed by its absolute address in the STM32 memory map, cast to a volatile pointer.

```c
// Everything is just an address
#define RCC_APB2ENR  (*(volatile unsigned int *)(0x40021018))
#define GPIOC_CRH    (*(volatile unsigned int *)(0x40011004))
#define NVIC_ISER0   (*(volatile unsigned int *)(0xE000E100))
#define SCB_SCR      (*(volatile unsigned int *)(0xE000ED10))
```

This is the most explicit form. You can trace every operation directly to the STM32F103 reference manual (RM0008). Nothing is hidden.

**What you learn here:** Where peripherals actually live in memory. What the RCC, EXTI, NVIC, and SCB registers look like without any abstraction. Why `volatile` is not optional.

---

### Level 2 — CMSIS Bare Metal

The same bitwise logic, but using the register structs and bit definitions from `stm32f103xb.h`. The addresses are the same — CMSIS just gives them readable names.

```c
// Same operation as raw, but readable
RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_IOPCEN | RCC_APB2ENR_AFIOEN;
EXTI->FTSR   |= EXTI_FTSR_TR0;
SCB->SCR     |= SCB_SCR_SLEEPDEEP_Msk;
```

**What you learn here:** How CMSIS maps to raw addresses. Why vendor headers exist. How to move between raw addresses and named macros without losing understanding.

---

### Level 3 — HAL Framework

CubeMX-compatible HAL code. The most common approach in production and the least transparent.

```c
// HAL handles the register details
HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI);

// In the EXTI callback:
HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
```

**What you learn here:** What HAL is doing when you call a single function — by comparing it to the raw and CMSIS versions above it.

---

## Key Concepts Explained

### Stop Mode vs Standby Mode

STM32 has several low-power modes. This project uses **Stop Mode**:

| Mode | CPU | SRAM | Peripherals | Wake-up time |
|---|---|---|---|---|
| Sleep | Off | On | On | Fast |
| **Stop** | **Off** | **On** | **Most off** | **Medium** |
| Standby | Off | Off | Off | Slow (reset) |

Stop Mode is the sweet spot for interrupt-driven systems: low power, but SRAM is retained and wake-up is fast enough for real-time response.

### The SLEEPDEEP Bit

The difference between Sleep Mode and Stop Mode is one bit: `SLEEPDEEP` in the Cortex-M3's System Control Block (SCB_SCR). When this bit is set, `WFI` enters Stop Mode instead of Sleep Mode.

```c
SCB_SCR |= (1 << 2);   // SLEEPDEEP = 1 → WFI enters Stop Mode
__asm volatile("wfi"); // CPU halts here until interrupt
```

### Why `__asm volatile("wfi")`

The compiler has no concept of "put the CPU to sleep." `WFI` (Wait For Interrupt) is a Cortex-M core instruction, not a C statement. It must be issued as inline assembly. `volatile` prevents the compiler from optimizing it away.

### The EXTI Pending Flag

After the interrupt fires and the ISR runs, the EXTI pending flag **must be cleared manually** before returning:

```c
void EXTI0_IRQHandler(void) {
    EXTI_PR |= (1 << 0);    // Write 1 to clear the pending flag
    GPIOC_ODR ^= (1 << 13); // Toggle LED
}
```

If you forget this, the interrupt fires again immediately upon wake-up — and again, and again, indefinitely. The LED still toggles, but the CPU never sleeps.

### The LPDS Bit

In Stop Mode, the voltage regulator can operate in normal or low-power mode, controlled by `LPDS` in `PWR_CR`. Low-power mode reduces current draw further but increases wake-up time slightly.

```c
PWR_CR |= (1 << 0);   // LPDS = 1 → low-power regulator in Stop Mode
```
---

## Reference Documents

- **STM32F103 Reference Manual (RM0008)** — every register address and bit definition used in the raw bare metal implementation
- **Cortex-M3 Technical Reference Manual** — SCB_SCR, NVIC_ISER, WFI instruction
- **STM32F103 Datasheet** — pin assignments, electrical characteristics
- **ARM CMSIS Documentation** — register struct definitions in `stm32f103xb.h`
