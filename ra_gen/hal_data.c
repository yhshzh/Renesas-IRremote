/* generated HAL source file - do not edit */
#include "hal_data.h"

#define RA_NOT_DEFINED (UINT32_MAX)
#if (RA_NOT_DEFINED) != (RA_NOT_DEFINED)

/* If the transfer module is DMAC, define a DMAC transfer callback. */
#include "r_dmac.h"
extern void spi_tx_dmac_callback(spi_instance_ctrl_t const * const p_ctrl);

void g_oled_spi_tx_transfer_callback (dmac_callback_args_t * p_args)
{
    FSP_PARAMETER_NOT_USED(p_args);
    spi_tx_dmac_callback(&g_oled_spi_ctrl);
}
#endif

#if (RA_NOT_DEFINED) != (RA_NOT_DEFINED)

/* If the transfer module is DMAC, define a DMAC transfer callback. */
#include "r_dmac.h"
extern void spi_rx_dmac_callback(spi_instance_ctrl_t const * const p_ctrl);

void g_oled_spi_rx_transfer_callback (dmac_callback_args_t * p_args)
{
    FSP_PARAMETER_NOT_USED(p_args);
    spi_rx_dmac_callback(&g_oled_spi_ctrl);
}
#endif
#undef RA_NOT_DEFINED

spi_instance_ctrl_t g_oled_spi_ctrl;

/** SPI extended configuration for SPI HAL driver */
const spi_extended_cfg_t g_oled_spi_ext_cfg = { .spi_clksyn =
		SPI_SSL_MODE_CLK_SYN, .spi_comm = SPI_COMMUNICATION_FULL_DUPLEX,
		.ssl_polarity = SPI_SSLP_LOW, .ssl_select = SPI_SSL_SELECT_SSL0,
		.mosi_idle = SPI_MOSI_IDLE_VALUE_FIXING_DISABLE, .parity =
				SPI_PARITY_MODE_DISABLE, .byte_swap = SPI_BYTE_SWAP_DISABLE,
		.spck_div = {
		/* Actual calculated bitrate: 1000000. */.spbr = 24, .brdv = 0 },
		.spck_delay = SPI_DELAY_COUNT_1,
		.ssl_negation_delay = SPI_DELAY_COUNT_1, .next_access_delay =
				SPI_DELAY_COUNT_1, .burst_interframe_delay =
				SPI_BURST_TRANSFER_WITH_DELAY };

/** SPI configuration for SPI HAL driver */
const spi_cfg_t g_oled_spi_cfg = { .channel = 0,

#if defined(VECTOR_NUMBER_SPI0_RXI)
    .rxi_irq             = VECTOR_NUMBER_SPI0_RXI,
#else
		.rxi_irq = FSP_INVALID_VECTOR,
#endif
#if defined(VECTOR_NUMBER_SPI0_TXI)
    .txi_irq             = VECTOR_NUMBER_SPI0_TXI,
#else
		.txi_irq = FSP_INVALID_VECTOR,
#endif
#if defined(VECTOR_NUMBER_SPI0_TEI)
    .tei_irq             = VECTOR_NUMBER_SPI0_TEI,
#else
		.tei_irq = FSP_INVALID_VECTOR,
#endif
#if defined(VECTOR_NUMBER_SPI0_ERI)
    .eri_irq             = VECTOR_NUMBER_SPI0_ERI,
#else
		.eri_irq = FSP_INVALID_VECTOR,
#endif

		.rxi_ipl = (12), .txi_ipl = (12), .tei_ipl = (12), .eri_ipl = (12),

		.operating_mode = SPI_MODE_MASTER,

		.clk_phase = SPI_CLK_PHASE_EDGE_ODD, .clk_polarity =
				SPI_CLK_POLARITY_LOW,

		.mode_fault = SPI_MODE_FAULT_ERROR_DISABLE, .bit_order =
				SPI_BIT_ORDER_MSB_FIRST, .p_transfer_tx =
				g_oled_spi_P_TRANSFER_TX, .p_transfer_rx =
				g_oled_spi_P_TRANSFER_RX, .p_callback = spi_callback,

		.p_context = NULL, .p_extend = (void*) &g_oled_spi_ext_cfg, };

/* Instance structure to use this module. */
const spi_instance_t g_oled_spi = { .p_ctrl = &g_oled_spi_ctrl, .p_cfg =
		&g_oled_spi_cfg, .p_api = &g_spi_on_spi };
sci_uart_instance_ctrl_t g_esp_uart_ctrl;

baud_setting_t g_esp_uart_baud_setting = {
/* Baud rate calculated with 0.469% error. */.semr_baudrate_bits_b.abcse = 0,
		.semr_baudrate_bits_b.abcs = 0, .semr_baudrate_bits_b.bgdm = 1,
		.cks = 0, .brr = 26, .mddr = (uint8_t) 256, .semr_baudrate_bits_b.brme =
				false };

/** UART extended configuration for UARTonSCI HAL driver */
const sci_uart_extended_cfg_t g_esp_uart_cfg_extend = { .clock =
		SCI_UART_CLOCK_INT, .rx_edge_start = SCI_UART_START_BIT_FALLING_EDGE,
		.noise_cancel = SCI_UART_NOISE_CANCELLATION_DISABLE, .rx_fifo_trigger =
				SCI_UART_RX_FIFO_TRIGGER_MAX, .p_baud_setting =
				&g_esp_uart_baud_setting, .flow_control =
				SCI_UART_FLOW_CONTROL_RTS,
#if 0xFF != 0xFF
                .flow_control_pin       = BSP_IO_PORT_FF_PIN_0xFF,
                #else
		.flow_control_pin = (bsp_io_port_pin_t) UINT16_MAX,
#endif
		.rs485_setting = { .enable = SCI_UART_RS485_DISABLE, .polarity =
				SCI_UART_RS485_DE_POLARITY_HIGH,
#if 0xFF != 0xFF
                    .de_control_pin = BSP_IO_PORT_FF_PIN_0xFF,
                #else
				.de_control_pin = (bsp_io_port_pin_t) UINT16_MAX,
#endif
				}, .irda_setting = { .ircr_bits_b.ire = 0,
				.ircr_bits_b.irrxinv = 0, .ircr_bits_b.irtxinv = 0, }, };

/** UART interface configuration */
const uart_cfg_t g_esp_uart_cfg = { .channel = 9, .data_bits = UART_DATA_BITS_8,
		.parity = UART_PARITY_OFF, .stop_bits = UART_STOP_BITS_1, .p_callback =
				esp_uart_callback, .p_context = NULL, .p_extend =
				&g_esp_uart_cfg_extend,
#define RA_NOT_DEFINED (1)
#if (RA_NOT_DEFINED == RA_NOT_DEFINED)
		.p_transfer_tx = NULL,
#else
                .p_transfer_tx       = &RA_NOT_DEFINED,
#endif
#if (RA_NOT_DEFINED == RA_NOT_DEFINED)
		.p_transfer_rx = NULL,
#else
                .p_transfer_rx       = &RA_NOT_DEFINED,
#endif
#undef RA_NOT_DEFINED
		.rxi_ipl = (3), .txi_ipl = (3), .tei_ipl = (3), .eri_ipl = (3),
#if defined(VECTOR_NUMBER_SCI9_RXI)
                .rxi_irq             = VECTOR_NUMBER_SCI9_RXI,
#else
		.rxi_irq = FSP_INVALID_VECTOR,
#endif
#if defined(VECTOR_NUMBER_SCI9_TXI)
                .txi_irq             = VECTOR_NUMBER_SCI9_TXI,
#else
		.txi_irq = FSP_INVALID_VECTOR,
#endif
#if defined(VECTOR_NUMBER_SCI9_TEI)
                .tei_irq             = VECTOR_NUMBER_SCI9_TEI,
#else
		.tei_irq = FSP_INVALID_VECTOR,
#endif
#if defined(VECTOR_NUMBER_SCI9_ERI)
                .eri_irq             = VECTOR_NUMBER_SCI9_ERI,
#else
		.eri_irq = FSP_INVALID_VECTOR,
#endif
		};

/* Instance structure to use this module. */
const uart_instance_t g_esp_uart = { .p_ctrl = &g_esp_uart_ctrl, .p_cfg =
		&g_esp_uart_cfg, .p_api = &g_uart_on_sci };
gpt_instance_ctrl_t g_ir_tx_gpt_ctrl;
#if 0
const gpt_extended_pwm_cfg_t g_ir_tx_gpt_pwm_extend =
{
    .trough_ipl             = (BSP_IRQ_DISABLED),
#if defined(VECTOR_NUMBER_GPT4_COUNTER_UNDERFLOW)
    .trough_irq             = VECTOR_NUMBER_GPT4_COUNTER_UNDERFLOW,
#else
    .trough_irq             = FSP_INVALID_VECTOR,
#endif
    .poeg_link              = GPT_POEG_LINK_POEG0,
    .output_disable         = (gpt_output_disable_t) ( GPT_OUTPUT_DISABLE_NONE),
    .adc_trigger            = (gpt_adc_trigger_t) ( GPT_ADC_TRIGGER_NONE),
    .dead_time_count_up     = 0,
    .dead_time_count_down   = 0,
    .adc_a_compare_match    = 0,
    .adc_b_compare_match    = 0,
    .interrupt_skip_source  = GPT_INTERRUPT_SKIP_SOURCE_NONE,
    .interrupt_skip_count   = GPT_INTERRUPT_SKIP_COUNT_0,
    .interrupt_skip_adc     = GPT_INTERRUPT_SKIP_ADC_NONE,
    .gtioca_disable_setting = GPT_GTIOC_DISABLE_PROHIBITED,
    .gtiocb_disable_setting = GPT_GTIOC_DISABLE_PROHIBITED,
};
#endif
const gpt_extended_cfg_t g_ir_tx_gpt_extend =
		{ .gtioca = { .output_enabled = true, .stop_level = GPT_PIN_LEVEL_LOW },
				.gtiocb = { .output_enabled = false, .stop_level =
						GPT_PIN_LEVEL_LOW }, .start_source = (gpt_source_t)(
						GPT_SOURCE_NONE), .stop_source = (gpt_source_t)(
						GPT_SOURCE_NONE), .clear_source = (gpt_source_t)(
						GPT_SOURCE_NONE), .count_up_source = (gpt_source_t)(
						GPT_SOURCE_NONE), .count_down_source = (gpt_source_t)(
						GPT_SOURCE_NONE), .capture_a_source = (gpt_source_t)(
						GPT_SOURCE_NONE), .capture_b_source = (gpt_source_t)(
						GPT_SOURCE_NONE), .capture_a_ipl = (BSP_IRQ_DISABLED),
				.capture_b_ipl = (BSP_IRQ_DISABLED), .compare_match_c_ipl =
						(BSP_IRQ_DISABLED), .compare_match_d_ipl =
						(BSP_IRQ_DISABLED), .compare_match_e_ipl =
						(BSP_IRQ_DISABLED), .compare_match_f_ipl =
						(BSP_IRQ_DISABLED),
#if defined(VECTOR_NUMBER_GPT4_CAPTURE_COMPARE_A)
    .capture_a_irq         = VECTOR_NUMBER_GPT4_CAPTURE_COMPARE_A,
#else
				.capture_a_irq = FSP_INVALID_VECTOR,
#endif
#if defined(VECTOR_NUMBER_GPT4_CAPTURE_COMPARE_B)
    .capture_b_irq         = VECTOR_NUMBER_GPT4_CAPTURE_COMPARE_B,
#else
				.capture_b_irq = FSP_INVALID_VECTOR,
#endif
#if defined(VECTOR_NUMBER_GPT4_COMPARE_C)
    .compare_match_c_irq   = VECTOR_NUMBER_GPT4_COMPARE_C,
#else
				.compare_match_c_irq = FSP_INVALID_VECTOR,
#endif
#if defined(VECTOR_NUMBER_GPT4_COMPARE_D)
    .compare_match_d_irq   = VECTOR_NUMBER_GPT4_COMPARE_D,
#else
				.compare_match_d_irq = FSP_INVALID_VECTOR,
#endif
#if defined(VECTOR_NUMBER_GPT4_COMPARE_E)
    .compare_match_e_irq   = VECTOR_NUMBER_GPT4_COMPARE_E,
#else
				.compare_match_e_irq = FSP_INVALID_VECTOR,
#endif
#if defined(VECTOR_NUMBER_GPT4_COMPARE_F)
    .compare_match_f_irq   = VECTOR_NUMBER_GPT4_COMPARE_F,
#else
				.compare_match_f_irq = FSP_INVALID_VECTOR,
#endif
				.compare_match_value = { (uint32_t) 0x0, /* CMP_A */
						(uint32_t) 0x0, /* CMP_B */(uint32_t) 0x0, /* CMP_C */
						(uint32_t) 0x0, /* CMP_D */(uint32_t) 0x0, /* CMP_E */
						(uint32_t) 0x0, /* CMP_F */}, .compare_match_status =
						((0U << 5U) | (0U << 4U) | (0U << 3U) | (0U << 2U)
								| (0U << 1U) | 0U), .capture_filter_gtioca =
						GPT_CAPTURE_FILTER_NONE, .capture_filter_gtiocb =
						GPT_CAPTURE_FILTER_NONE,
#if 0
    .p_pwm_cfg             = &g_ir_tx_gpt_pwm_extend,
#else
				.p_pwm_cfg = NULL,
#endif
#if 0
    .gtior_setting.gtior_b.gtioa  = (0U << 4U) | (0U << 2U) | (0U << 0U),
    .gtior_setting.gtior_b.oadflt = (uint32_t) GPT_PIN_LEVEL_LOW,
    .gtior_setting.gtior_b.oahld  = 0U,
    .gtior_setting.gtior_b.oae    = (uint32_t) true,
    .gtior_setting.gtior_b.oadf   = (uint32_t) GPT_GTIOC_DISABLE_PROHIBITED,
    .gtior_setting.gtior_b.nfaen  = ((uint32_t) GPT_CAPTURE_FILTER_NONE & 1U),
    .gtior_setting.gtior_b.nfcsa  = ((uint32_t) GPT_CAPTURE_FILTER_NONE >> 1U),
    .gtior_setting.gtior_b.gtiob  = (0U << 4U) | (0U << 2U) | (0U << 0U),
    .gtior_setting.gtior_b.obdflt = (uint32_t) GPT_PIN_LEVEL_LOW,
    .gtior_setting.gtior_b.obhld  = 0U,
    .gtior_setting.gtior_b.obe    = (uint32_t) false,
    .gtior_setting.gtior_b.obdf   = (uint32_t) GPT_GTIOC_DISABLE_PROHIBITED,
    .gtior_setting.gtior_b.nfben  = ((uint32_t) GPT_CAPTURE_FILTER_NONE & 1U),
    .gtior_setting.gtior_b.nfcsb  = ((uint32_t) GPT_CAPTURE_FILTER_NONE >> 1U),
#else
				.gtior_setting.gtior = 0U,
#endif

				.gtioca_polarity = GPT_GTIOC_POLARITY_NORMAL, .gtiocb_polarity =
						GPT_GTIOC_POLARITY_NORMAL, };

const timer_cfg_t g_ir_tx_gpt_cfg =
		{ .mode = TIMER_MODE_PWM,
				/* Actual period: 0.0000263 seconds. Actual duty: 49.961977186311785%. */.period_counts =
						(uint32_t) 0x523, .duty_cycle_counts = 0x291,
				.source_div = (timer_source_div_t) 0, .channel = 4,
				.p_callback = NULL,
				/** If NULL then do not add & */
#if defined(NULL)
    .p_context           = NULL,
#else
				.p_context = (void*) &NULL,
#endif
				.p_extend = &g_ir_tx_gpt_extend, .cycle_end_ipl =
						(BSP_IRQ_DISABLED),
#if defined(VECTOR_NUMBER_GPT4_COUNTER_OVERFLOW)
    .cycle_end_irq       = VECTOR_NUMBER_GPT4_COUNTER_OVERFLOW,
#else
				.cycle_end_irq = FSP_INVALID_VECTOR,
#endif
		};
/* Instance structure to use this module. */
const timer_instance_t g_ir_tx_gpt = { .p_ctrl = &g_ir_tx_gpt_ctrl, .p_cfg =
		&g_ir_tx_gpt_cfg, .p_api = &g_timer_on_gpt };
void g_hal_init(void) {
	g_common_init();
}
