/* generated vector source file - do not edit */
#include "bsp_api.h"
/* Do not build these data structures if no interrupts are currently allocated because IAR will have build errors. */
#if VECTOR_DATA_IRQ_COUNT > 0
        BSP_DONT_REMOVE const fsp_vector_t g_vector_table[BSP_ICU_VECTOR_NUM_ENTRIES] BSP_PLACE_IN_SECTION(BSP_SECTION_APPLICATION_VECTORS) =
        {
                        [0] = r_icu_isr, /* ICU IRQ10 (External pin interrupt 10) */
            [1] = sci_uart_rxi_isr, /* SCI9 RXI (Receive data full) */
            [2] = sci_uart_txi_isr, /* SCI9 TXI (Transmit data empty) */
            [3] = sci_uart_tei_isr, /* SCI9 TEI (Transmit end) */
            [4] = sci_uart_eri_isr, /* SCI9 ERI (Receive error) */
            [5] = spi_rxi_isr, /* SPI0 RXI (Receive buffer full) */
            [6] = spi_txi_isr, /* SPI0 TXI (Transmit buffer empty) */
            [7] = spi_tei_isr, /* SPI0 TEI (Transmission complete event) */
            [8] = spi_eri_isr, /* SPI0 ERI (Error) */
        };
        #if BSP_FEATURE_ICU_HAS_IELSR
        const bsp_interrupt_event_t g_interrupt_event_link_select[BSP_ICU_VECTOR_NUM_ENTRIES] =
        {
            [0] = BSP_PRV_VECT_ENUM(EVENT_ICU_IRQ10,GROUP0), /* ICU IRQ10 (External pin interrupt 10) */
            [1] = BSP_PRV_VECT_ENUM(EVENT_SCI9_RXI,GROUP1), /* SCI9 RXI (Receive data full) */
            [2] = BSP_PRV_VECT_ENUM(EVENT_SCI9_TXI,GROUP2), /* SCI9 TXI (Transmit data empty) */
            [3] = BSP_PRV_VECT_ENUM(EVENT_SCI9_TEI,GROUP3), /* SCI9 TEI (Transmit end) */
            [4] = BSP_PRV_VECT_ENUM(EVENT_SCI9_ERI,GROUP4), /* SCI9 ERI (Receive error) */
            [5] = BSP_PRV_VECT_ENUM(EVENT_SPI0_RXI,GROUP5), /* SPI0 RXI (Receive buffer full) */
            [6] = BSP_PRV_VECT_ENUM(EVENT_SPI0_TXI,GROUP6), /* SPI0 TXI (Transmit buffer empty) */
            [7] = BSP_PRV_VECT_ENUM(EVENT_SPI0_TEI,GROUP7), /* SPI0 TEI (Transmission complete event) */
            [8] = BSP_PRV_VECT_ENUM(EVENT_SPI0_ERI,GROUP0), /* SPI0 ERI (Error) */
        };
        #endif
        #endif
