/* generated HAL header file - do not edit */
#ifndef HAL_DATA_H_
#define HAL_DATA_H_
#include <stdint.h>
#include "bsp_api.h"
#include "common_data.h"
#include "r_spi.h"
#include "r_sci_uart.h"
#include "r_uart_api.h"
#include "r_gpt.h"
#include "r_timer_api.h"
FSP_HEADER
/** SPI on SPI Instance. */
extern const spi_instance_t g_oled_spi;

/** Access the SPI instance using these structures when calling API functions directly (::p_api is not used). */
extern spi_instance_ctrl_t g_oled_spi_ctrl;
extern const spi_cfg_t g_oled_spi_cfg;

/** Callback used by SPI Instance. */
#ifndef spi_callback
void spi_callback(spi_callback_args_t *p_args);
#endif

#define RA_NOT_DEFINED (1)
#if (RA_NOT_DEFINED == RA_NOT_DEFINED)
#define g_oled_spi_P_TRANSFER_TX (NULL)
#else
    #define g_oled_spi_P_TRANSFER_TX (&RA_NOT_DEFINED)
#endif
#if (RA_NOT_DEFINED == RA_NOT_DEFINED)
#define g_oled_spi_P_TRANSFER_RX (NULL)
#else
    #define g_oled_spi_P_TRANSFER_RX (&RA_NOT_DEFINED)
#endif
#undef RA_NOT_DEFINED
/** UART on SCI Instance. */
extern const uart_instance_t g_esp_uart;

/** Access the UART instance using these structures when calling API functions directly (::p_api is not used). */
extern sci_uart_instance_ctrl_t g_esp_uart_ctrl;
extern const uart_cfg_t g_esp_uart_cfg;
extern const sci_uart_extended_cfg_t g_esp_uart_cfg_extend;

#ifndef esp_uart_callback
void esp_uart_callback(uart_callback_args_t *p_args);
#endif
/** Timer on GPT Instance. */
extern const timer_instance_t g_ir_tx_gpt;

/** Access the GPT instance using these structures when calling API functions directly (::p_api is not used). */
extern gpt_instance_ctrl_t g_ir_tx_gpt_ctrl;
extern const timer_cfg_t g_ir_tx_gpt_cfg;

#ifndef NULL
void NULL(timer_callback_args_t *p_args);
#endif
void hal_entry(void);
void g_hal_init(void);
FSP_FOOTER
#endif /* HAL_DATA_H_ */
