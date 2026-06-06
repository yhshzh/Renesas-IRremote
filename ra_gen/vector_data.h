/* generated vector header file - do not edit */
#ifndef VECTOR_DATA_H
#define VECTOR_DATA_H
#ifdef __cplusplus
        extern "C" {
        #endif
/* Number of interrupts allocated */
#ifndef VECTOR_DATA_IRQ_COUNT
#define VECTOR_DATA_IRQ_COUNT    (9)
#endif
/* ISR prototypes */
void r_icu_isr(void);
void sci_uart_rxi_isr(void);
void sci_uart_txi_isr(void);
void sci_uart_tei_isr(void);
void sci_uart_eri_isr(void);
void spi_rxi_isr(void);
void spi_txi_isr(void);
void spi_tei_isr(void);
void spi_eri_isr(void);

/* Vector table allocations */
#define VECTOR_NUMBER_ICU_IRQ10 ((IRQn_Type) 0) /* ICU IRQ10 (External pin interrupt 10) */
#define ICU_IRQ10_IRQn          ((IRQn_Type) 0) /* ICU IRQ10 (External pin interrupt 10) */
#define VECTOR_NUMBER_SCI9_RXI ((IRQn_Type) 1) /* SCI9 RXI (Receive data full) */
#define SCI9_RXI_IRQn          ((IRQn_Type) 1) /* SCI9 RXI (Receive data full) */
#define VECTOR_NUMBER_SCI9_TXI ((IRQn_Type) 2) /* SCI9 TXI (Transmit data empty) */
#define SCI9_TXI_IRQn          ((IRQn_Type) 2) /* SCI9 TXI (Transmit data empty) */
#define VECTOR_NUMBER_SCI9_TEI ((IRQn_Type) 3) /* SCI9 TEI (Transmit end) */
#define SCI9_TEI_IRQn          ((IRQn_Type) 3) /* SCI9 TEI (Transmit end) */
#define VECTOR_NUMBER_SCI9_ERI ((IRQn_Type) 4) /* SCI9 ERI (Receive error) */
#define SCI9_ERI_IRQn          ((IRQn_Type) 4) /* SCI9 ERI (Receive error) */
#define VECTOR_NUMBER_SPI0_RXI ((IRQn_Type) 5) /* SPI0 RXI (Receive buffer full) */
#define SPI0_RXI_IRQn          ((IRQn_Type) 5) /* SPI0 RXI (Receive buffer full) */
#define VECTOR_NUMBER_SPI0_TXI ((IRQn_Type) 6) /* SPI0 TXI (Transmit buffer empty) */
#define SPI0_TXI_IRQn          ((IRQn_Type) 6) /* SPI0 TXI (Transmit buffer empty) */
#define VECTOR_NUMBER_SPI0_TEI ((IRQn_Type) 7) /* SPI0 TEI (Transmission complete event) */
#define SPI0_TEI_IRQn          ((IRQn_Type) 7) /* SPI0 TEI (Transmission complete event) */
#define VECTOR_NUMBER_SPI0_ERI ((IRQn_Type) 8) /* SPI0 ERI (Error) */
#define SPI0_ERI_IRQn          ((IRQn_Type) 8) /* SPI0 ERI (Error) */
/* The number of entries required for the ICU vector table. */
#define BSP_ICU_VECTOR_NUM_ENTRIES (9)

#ifdef __cplusplus
        }
        #endif
#endif /* VECTOR_DATA_H */
