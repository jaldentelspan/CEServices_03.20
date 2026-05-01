/*
 * system.h - SOPC Builder system and BSP software package information
 *
 * Machine generated for CPU 'nios2_gen2_0' in SOPC Builder design 'VNS_FPGA_qsys_top'
 * SOPC Builder design path: /home/eric/TelspanDataProjectsInternal/iES_12/Development/FPGA/IES_COMMON_IP/VNS_FPGA_qsys_top.sopcinfo
 *
 * Generated: Fri Dec 01 12:08:38 MST 2023
 */

/*
 * DO NOT MODIFY THIS FILE
 *
 * Changing this file will have subtle consequences
 * which will almost certainly lead to a nonfunctioning
 * system. If you do modify this file, be aware that your
 * changes will be overwritten and lost when this file
 * is generated again.
 *
 * DO NOT MODIFY THIS FILE
 */

/*
 * License Agreement
 *
 * Copyright (c) 2008
 * Altera Corporation, San Jose, California, USA.
 * All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * This agreement shall be governed in all respects by the laws of the State
 * of California and by the laws of the United States of America.
 */

#ifndef __SYSTEM_H_
#define __SYSTEM_H_

/* Include definitions from linker script generator */
/* #include "linker.h" */


/*
 * CPU configuration
 *
 */

#define ALT_CPU_ARCHITECTURE "altera_nios2_gen2"
#define ALT_CPU_BIG_ENDIAN 0
#define ALT_CPU_BREAK_ADDR 0x00043820
#define ALT_CPU_CPU_ARCH_NIOS2_R1
#define ALT_CPU_CPU_FREQ 100000000u
#define ALT_CPU_CPU_ID_SIZE 1
#define ALT_CPU_CPU_ID_VALUE 0x00000000
#define ALT_CPU_CPU_IMPLEMENTATION "tiny"
#define ALT_CPU_DATA_ADDR_WIDTH 0x1d
#define ALT_CPU_DCACHE_LINE_SIZE 0
#define ALT_CPU_DCACHE_LINE_SIZE_LOG2 0
#define ALT_CPU_DCACHE_SIZE 0
#define ALT_CPU_EXCEPTION_ADDR 0x00000020
#define ALT_CPU_FLASH_ACCELERATOR_LINES 0
#define ALT_CPU_FLASH_ACCELERATOR_LINE_SIZE 0
#define ALT_CPU_FLUSHDA_SUPPORTED
#define ALT_CPU_FREQ 100000000
#define ALT_CPU_HARDWARE_DIVIDE_PRESENT 0
#define ALT_CPU_HARDWARE_MULTIPLY_PRESENT 0
#define ALT_CPU_HARDWARE_MULX_PRESENT 0
#define ALT_CPU_HAS_DEBUG_CORE 1
#define ALT_CPU_HAS_DEBUG_STUB
#define ALT_CPU_HAS_ILLEGAL_INSTRUCTION_EXCEPTION
#define ALT_CPU_HAS_JMPI_INSTRUCTION
#define ALT_CPU_ICACHE_LINE_SIZE 0
#define ALT_CPU_ICACHE_LINE_SIZE_LOG2 0
#define ALT_CPU_ICACHE_SIZE 0
#define ALT_CPU_INST_ADDR_WIDTH 0x13
#define ALT_CPU_NAME "nios2_gen2_0"
#define ALT_CPU_OCI_VERSION 1
#define ALT_CPU_RESET_ADDR 0x00000000


/*
 * CPU configuration (with legacy prefix - don't use these anymore)
 *
 */

#define NIOS2_BIG_ENDIAN 0
#define NIOS2_BREAK_ADDR 0x00043820
#define NIOS2_CPU_ARCH_NIOS2_R1
#define NIOS2_CPU_FREQ 100000000u
#define NIOS2_CPU_ID_SIZE 1
#define NIOS2_CPU_ID_VALUE 0x00000000
#define NIOS2_CPU_IMPLEMENTATION "tiny"
#define NIOS2_DATA_ADDR_WIDTH 0x1d
#define NIOS2_DCACHE_LINE_SIZE 0
#define NIOS2_DCACHE_LINE_SIZE_LOG2 0
#define NIOS2_DCACHE_SIZE 0
#define NIOS2_EXCEPTION_ADDR 0x00000020
#define NIOS2_FLASH_ACCELERATOR_LINES 0
#define NIOS2_FLASH_ACCELERATOR_LINE_SIZE 0
#define NIOS2_FLUSHDA_SUPPORTED
#define NIOS2_HARDWARE_DIVIDE_PRESENT 0
#define NIOS2_HARDWARE_MULTIPLY_PRESENT 0
#define NIOS2_HARDWARE_MULX_PRESENT 0
#define NIOS2_HAS_DEBUG_CORE 1
#define NIOS2_HAS_DEBUG_STUB
#define NIOS2_HAS_ILLEGAL_INSTRUCTION_EXCEPTION
#define NIOS2_HAS_JMPI_INSTRUCTION
#define NIOS2_ICACHE_LINE_SIZE 0
#define NIOS2_ICACHE_LINE_SIZE_LOG2 0
#define NIOS2_ICACHE_SIZE 0
#define NIOS2_INST_ADDR_WIDTH 0x13
#define NIOS2_OCI_VERSION 1
#define NIOS2_RESET_ADDR 0x00000000


/*
 * Define for each module class mastered by the CPU
 *
 */

#define __ALTERA_AVALON_EPCS_FLASH_CONTROLLER
#define __ALTERA_AVALON_I2C
#define __ALTERA_AVALON_JTAG_UART
#define __ALTERA_AVALON_MM_BRIDGE
#define __ALTERA_AVALON_ONCHIP_MEMORY2
#define __ALTERA_AVALON_PIO
#define __ALTERA_AVALON_SYSID_QSYS
#define __ALTERA_AVALON_TIMER
#define __ALTERA_AVALON_UART
#define __ALTERA_ETH_TSE
#define __ALTERA_MEM_IF_DDR2_EMIF
#define __ALTERA_MSGDMA
#define __ALTERA_NIOS2_GEN2
#define __ALTERA_REMOTE_UPDATE
#define __CH10_FILE_READER
#define __ENET_CH10_CH7_DECODER_2P0
#define __ENET_CH10_CH7_ENCODER_2P0
#define __ENET_HDLC_DECODER_2P0
#define __ENET_HDLC_ENCODER_2P0
#define __GPS_TOP
#define __LED_CONTROLLER
#define __OPPS_SLAVE_TOP
#define __RTC_TIME_CAL
#define __RX_IRIG_TOP
#define __TIME_MANUAL_INC_TOP
#define __TIME_SELECTOR_TOP
#define __TX_IRIG
#define __TX_IRIG_CH10_TOP


/*
 * System configuration
 *
 */

#define ALT_DEVICE_FAMILY "Cyclone V"
#define ALT_ENHANCED_INTERRUPT_API_PRESENT
#define ALT_IRQ_BASE NULL
#define ALT_LOG_PORT "/dev/null"
#define ALT_LOG_PORT_BASE 0x0
#define ALT_LOG_PORT_DEV null
#define ALT_LOG_PORT_TYPE ""
#define ALT_NUM_EXTERNAL_INTERRUPT_CONTROLLERS 0
#define ALT_NUM_INTERNAL_INTERRUPT_CONTROLLERS 1
#define ALT_NUM_INTERRUPT_CONTROLLERS 1
#define ALT_STDERR "/dev/jtag_uart_0"
#define ALT_STDERR_BASE 0x4f000
#define ALT_STDERR_DEV jtag_uart_0
#define ALT_STDERR_IS_JTAG_UART
#define ALT_STDERR_PRESENT
#define ALT_STDERR_TYPE "altera_avalon_jtag_uart"
#define ALT_STDIN "/dev/jtag_uart_0"
#define ALT_STDIN_BASE 0x4f000
#define ALT_STDIN_DEV jtag_uart_0
#define ALT_STDIN_IS_JTAG_UART
#define ALT_STDIN_PRESENT
#define ALT_STDIN_TYPE "altera_avalon_jtag_uart"
#define ALT_STDOUT "/dev/jtag_uart_0"
#define ALT_STDOUT_BASE 0x4f000
#define ALT_STDOUT_DEV jtag_uart_0
#define ALT_STDOUT_IS_JTAG_UART
#define ALT_STDOUT_PRESENT
#define ALT_STDOUT_TYPE "altera_avalon_jtag_uart"
#define ALT_SYSTEM_NAME "VNS_FPGA_qsys_top"


/*
 * digital_in configuration
 *
 */

#define ALT_MODULE_CLASS_digital_in altera_avalon_pio
#define DIGITAL_IN_BASE 0x45600
#define DIGITAL_IN_BIT_CLEARING_EDGE_REGISTER 0
#define DIGITAL_IN_BIT_MODIFYING_OUTPUT_REGISTER 0
#define DIGITAL_IN_CAPTURE 0
#define DIGITAL_IN_DATA_WIDTH 8
#define DIGITAL_IN_DO_TEST_BENCH_WIRING 0
#define DIGITAL_IN_DRIVEN_SIM_VALUE 0
#define DIGITAL_IN_EDGE_TYPE "NONE"
#define DIGITAL_IN_FREQ 100000000
#define DIGITAL_IN_HAS_IN 1
#define DIGITAL_IN_HAS_OUT 0
#define DIGITAL_IN_HAS_TRI 0
#define DIGITAL_IN_IRQ -1
#define DIGITAL_IN_IRQ_INTERRUPT_CONTROLLER_ID -1
#define DIGITAL_IN_IRQ_TYPE "NONE"
#define DIGITAL_IN_NAME "/dev/digital_in"
#define DIGITAL_IN_RESET_VALUE 0
#define DIGITAL_IN_SPAN 16
#define DIGITAL_IN_TYPE "altera_avalon_pio"


/*
 * digital_out configuration
 *
 */

#define ALT_MODULE_CLASS_digital_out altera_avalon_pio
#define DIGITAL_OUT_BASE 0x44500
#define DIGITAL_OUT_BIT_CLEARING_EDGE_REGISTER 0
#define DIGITAL_OUT_BIT_MODIFYING_OUTPUT_REGISTER 0
#define DIGITAL_OUT_CAPTURE 0
#define DIGITAL_OUT_DATA_WIDTH 15
#define DIGITAL_OUT_DO_TEST_BENCH_WIRING 0
#define DIGITAL_OUT_DRIVEN_SIM_VALUE 0
#define DIGITAL_OUT_EDGE_TYPE "NONE"
#define DIGITAL_OUT_FREQ 100000000
#define DIGITAL_OUT_HAS_IN 0
#define DIGITAL_OUT_HAS_OUT 1
#define DIGITAL_OUT_HAS_TRI 0
#define DIGITAL_OUT_IRQ -1
#define DIGITAL_OUT_IRQ_INTERRUPT_CONTROLLER_ID -1
#define DIGITAL_OUT_IRQ_TYPE "NONE"
#define DIGITAL_OUT_NAME "/dev/digital_out"
#define DIGITAL_OUT_RESET_VALUE 0
#define DIGITAL_OUT_SPAN 16
#define DIGITAL_OUT_TYPE "altera_avalon_pio"


/*
 * enet_ch10_ch7_decoder_2p0_0 configuration
 *
 */

#define ALT_MODULE_CLASS_enet_ch10_ch7_decoder_2p0_0 enet_ch10_ch7_decoder_2p0
#define ENET_CH10_CH7_DECODER_2P0_0_BASE 0x53000
#define ENET_CH10_CH7_DECODER_2P0_0_IRQ -1
#define ENET_CH10_CH7_DECODER_2P0_0_IRQ_INTERRUPT_CONTROLLER_ID -1
#define ENET_CH10_CH7_DECODER_2P0_0_NAME "/dev/enet_ch10_ch7_decoder_2p0_0"
#define ENET_CH10_CH7_DECODER_2P0_0_SPAN 64
#define ENET_CH10_CH7_DECODER_2P0_0_TYPE "enet_ch10_ch7_decoder_2p0"


/*
 * enet_ch10_ch7_decoder_2p0_driver configuration
 *
 */

#define TSD_CH7_DECODER_2_0_DRIVER


/*
 * enet_ch10_ch7_encoder_2p0_0 configuration
 *
 */

#define ALT_MODULE_CLASS_enet_ch10_ch7_encoder_2p0_0 enet_ch10_ch7_encoder_2p0
#define ENET_CH10_CH7_ENCODER_2P0_0_BASE 0x56000
#define ENET_CH10_CH7_ENCODER_2P0_0_IRQ -1
#define ENET_CH10_CH7_ENCODER_2P0_0_IRQ_INTERRUPT_CONTROLLER_ID -1
#define ENET_CH10_CH7_ENCODER_2P0_0_NAME "/dev/enet_ch10_ch7_encoder_2p0_0"
#define ENET_CH10_CH7_ENCODER_2P0_0_SPAN 64
#define ENET_CH10_CH7_ENCODER_2P0_0_TYPE "enet_ch10_ch7_encoder_2p0"


/*
 * enet_ch10_ch7_encoder_2p0_driver configuration
 *
 */

#define TSD_CH7_ENCODER_2_0_DRIVER


/*
 * enet_hdlc_decoder_2p0_0 configuration
 *
 */

#define ALT_MODULE_CLASS_enet_hdlc_decoder_2p0_0 enet_hdlc_decoder_2p0
#define ENET_HDLC_DECODER_2P0_0_BASE 0x54000
#define ENET_HDLC_DECODER_2P0_0_IRQ -1
#define ENET_HDLC_DECODER_2P0_0_IRQ_INTERRUPT_CONTROLLER_ID -1
#define ENET_HDLC_DECODER_2P0_0_NAME "/dev/enet_hdlc_decoder_2p0_0"
#define ENET_HDLC_DECODER_2P0_0_SPAN 64
#define ENET_HDLC_DECODER_2P0_0_TYPE "enet_hdlc_decoder_2p0"


/*
 * enet_hdlc_decoder_2p0_driver configuration
 *
 */

#define TSD_HDLC_DECODER_2_0_DRIVER


/*
 * enet_hdlc_encoder_2p0_0 configuration
 *
 */

#define ALT_MODULE_CLASS_enet_hdlc_encoder_2p0_0 enet_hdlc_encoder_2p0
#define ENET_HDLC_ENCODER_2P0_0_BASE 0x55000
#define ENET_HDLC_ENCODER_2P0_0_IRQ -1
#define ENET_HDLC_ENCODER_2P0_0_IRQ_INTERRUPT_CONTROLLER_ID -1
#define ENET_HDLC_ENCODER_2P0_0_NAME "/dev/enet_hdlc_encoder_2p0_0"
#define ENET_HDLC_ENCODER_2P0_0_SPAN 64
#define ENET_HDLC_ENCODER_2P0_0_TYPE "enet_hdlc_encoder_2p0"


/*
 * enet_hdlc_encoder_2p0_driver configuration
 *
 */

#define TSD_HDLC_ENCODER_2_0_DRIVER


/*
 * epcs_flash_controller_0 configuration
 *
 */

#define ALT_MODULE_CLASS_epcs_flash_controller_0 altera_avalon_epcs_flash_controller
#define EPCS_FLASH_CONTROLLER_0_BASE 0x47000
#define EPCS_FLASH_CONTROLLER_0_IRQ 5
#define EPCS_FLASH_CONTROLLER_0_IRQ_INTERRUPT_CONTROLLER_ID 0
#define EPCS_FLASH_CONTROLLER_0_NAME "/dev/epcs_flash_controller_0"
#define EPCS_FLASH_CONTROLLER_0_REGISTER_OFFSET 1024
#define EPCS_FLASH_CONTROLLER_0_SPAN 2048
#define EPCS_FLASH_CONTROLLER_0_TYPE "altera_avalon_epcs_flash_controller"


/*
 * gps_top_0 configuration
 *
 */

#define ALT_MODULE_CLASS_gps_top_0 gps_top
#define GPS_TOP_0_BASE 0x42c00
#define GPS_TOP_0_IRQ -1
#define GPS_TOP_0_IRQ_INTERRUPT_CONTROLLER_ID -1
#define GPS_TOP_0_NAME "/dev/gps_top_0"
#define GPS_TOP_0_SPAN 64
#define GPS_TOP_0_TYPE "gps_top"


/*
 * gps_uart configuration
 *
 */

#define ALT_MODULE_CLASS_gps_uart altera_avalon_uart
#define GPS_UART_BASE 0x44400
#define GPS_UART_BAUD 115200
#define GPS_UART_DATA_BITS 8
#define GPS_UART_FIXED_BAUD 0
#define GPS_UART_FREQ 100000000
#define GPS_UART_IRQ 0
#define GPS_UART_IRQ_INTERRUPT_CONTROLLER_ID 0
#define GPS_UART_NAME "/dev/gps_uart"
#define GPS_UART_PARITY 'N'
#define GPS_UART_SIM_CHAR_STREAM ""
#define GPS_UART_SIM_TRUE_BAUD 0
#define GPS_UART_SPAN 32
#define GPS_UART_STOP_BITS 1
#define GPS_UART_SYNC_REG_DEPTH 2
#define GPS_UART_TYPE "altera_avalon_uart"
#define GPS_UART_USE_CTS_RTS 0
#define GPS_UART_USE_EOP_REGISTER 0


/*
 * hal configuration
 *
 */

#define ALT_INCLUDE_INSTRUCTION_RELATED_EXCEPTION_API
#define ALT_MAX_FD 32
#define ALT_SYS_CLK TIMER_0
#define ALT_TIMESTAMP_CLK none


/*
 * jtag_uart_0 configuration
 *
 */

#define ALT_MODULE_CLASS_jtag_uart_0 altera_avalon_jtag_uart
#define JTAG_UART_0_BASE 0x4f000
#define JTAG_UART_0_IRQ 2
#define JTAG_UART_0_IRQ_INTERRUPT_CONTROLLER_ID 0
#define JTAG_UART_0_NAME "/dev/jtag_uart_0"
#define JTAG_UART_0_READ_DEPTH 64
#define JTAG_UART_0_READ_THRESHOLD 8
#define JTAG_UART_0_SPAN 8
#define JTAG_UART_0_TYPE "altera_avalon_jtag_uart"
#define JTAG_UART_0_WRITE_DEPTH 64
#define JTAG_UART_0_WRITE_THRESHOLD 8


/*
 * led_controller configuration
 *
 */

#define ALT_MODULE_CLASS_led_controller led_controller
#define LED_CONTROLLER_BASE 0x42800
#define LED_CONTROLLER_IRQ -1
#define LED_CONTROLLER_IRQ_INTERRUPT_CONTROLLER_ID -1
#define LED_CONTROLLER_NAME "/dev/led_controller"
#define LED_CONTROLLER_SPAN 1024
#define LED_CONTROLLER_TYPE "led_controller"


/*
 * mem_if_ddr2_emif_0 configuration
 *
 */

#define ALT_MODULE_CLASS_mem_if_ddr2_emif_0 altera_mem_if_ddr2_emif
#define MEM_IF_DDR2_EMIF_0_BASE 0x10000000
#define MEM_IF_DDR2_EMIF_0_IRQ -1
#define MEM_IF_DDR2_EMIF_0_IRQ_INTERRUPT_CONTROLLER_ID -1
#define MEM_IF_DDR2_EMIF_0_NAME "/dev/mem_if_ddr2_emif_0"
#define MEM_IF_DDR2_EMIF_0_SPAN 268435456
#define MEM_IF_DDR2_EMIF_0_TYPE "altera_mem_if_ddr2_emif"


/*
 * mmdrv_printf configuration
 *
 */

#define _MMDRV_PRINTF 1


/*
 * onchip_memory2_0 configuration
 *
 */

#define ALT_MODULE_CLASS_onchip_memory2_0 altera_avalon_onchip_memory2
#define ONCHIP_MEMORY2_0_ALLOW_IN_SYSTEM_MEMORY_CONTENT_EDITOR 0
#define ONCHIP_MEMORY2_0_ALLOW_MRAM_SIM_CONTENTS_ONLY_FILE 0
#define ONCHIP_MEMORY2_0_BASE 0x0
#define ONCHIP_MEMORY2_0_CONTENTS_INFO ""
#define ONCHIP_MEMORY2_0_DUAL_PORT 0
#define ONCHIP_MEMORY2_0_GUI_RAM_BLOCK_TYPE "AUTO"
#define ONCHIP_MEMORY2_0_INIT_CONTENTS_FILE "VNS_FPGA_qsys_top_onchip_memory2_0"
#define ONCHIP_MEMORY2_0_INIT_MEM_CONTENT 1
#define ONCHIP_MEMORY2_0_INSTANCE_ID "NONE"
#define ONCHIP_MEMORY2_0_IRQ -1
#define ONCHIP_MEMORY2_0_IRQ_INTERRUPT_CONTROLLER_ID -1
#define ONCHIP_MEMORY2_0_NAME "/dev/onchip_memory2_0"
#define ONCHIP_MEMORY2_0_NON_DEFAULT_INIT_FILE_ENABLED 1
#define ONCHIP_MEMORY2_0_RAM_BLOCK_TYPE "AUTO"
#define ONCHIP_MEMORY2_0_READ_DURING_WRITE_MODE "DONT_CARE"
#define ONCHIP_MEMORY2_0_SINGLE_CLOCK_OP 0
#define ONCHIP_MEMORY2_0_SIZE_MULTIPLE 1
#define ONCHIP_MEMORY2_0_SIZE_VALUE 258048
#define ONCHIP_MEMORY2_0_SPAN 258048
#define ONCHIP_MEMORY2_0_TYPE "altera_avalon_onchip_memory2"
#define ONCHIP_MEMORY2_0_WRITABLE 1


/*
 * opps_slave_top_0 configuration
 *
 */

#define ALT_MODULE_CLASS_opps_slave_top_0 opps_slave_top
#define OPPS_SLAVE_TOP_0_BASE 0x43000
#define OPPS_SLAVE_TOP_0_IRQ -1
#define OPPS_SLAVE_TOP_0_IRQ_INTERRUPT_CONTROLLER_ID -1
#define OPPS_SLAVE_TOP_0_NAME "/dev/opps_slave_top_0"
#define OPPS_SLAVE_TOP_0_SPAN 1024
#define OPPS_SLAVE_TOP_0_TYPE "opps_slave_top"


/*
 * opps_slave_top_ext configuration
 *
 */

#define ALT_MODULE_CLASS_opps_slave_top_ext opps_slave_top
#define OPPS_SLAVE_TOP_EXT_BASE 0x45800
#define OPPS_SLAVE_TOP_EXT_IRQ -1
#define OPPS_SLAVE_TOP_EXT_IRQ_INTERRUPT_CONTROLLER_ID -1
#define OPPS_SLAVE_TOP_EXT_NAME "/dev/opps_slave_top_ext"
#define OPPS_SLAVE_TOP_EXT_SPAN 1024
#define OPPS_SLAVE_TOP_EXT_TYPE "opps_slave_top"


/*
 * remote_update configuration
 *
 */

#define ALT_MODULE_CLASS_remote_update altera_remote_update
#define REMOTE_UPDATE_BASE 0x4a000
#define REMOTE_UPDATE_IRQ -1
#define REMOTE_UPDATE_IRQ_INTERRUPT_CONTROLLER_ID -1
#define REMOTE_UPDATE_NAME "/dev/remote_update"
#define REMOTE_UPDATE_SPAN 32
#define REMOTE_UPDATE_TYPE "altera_remote_update"


/*
 * remote_update_bridge configuration
 *
 */

#define ALT_MODULE_CLASS_remote_update_bridge altera_avalon_mm_bridge
#define REMOTE_UPDATE_BRIDGE_BASE 0x45700
#define REMOTE_UPDATE_BRIDGE_IRQ -1
#define REMOTE_UPDATE_BRIDGE_IRQ_INTERRUPT_CONTROLLER_ID -1
#define REMOTE_UPDATE_BRIDGE_NAME "/dev/remote_update_bridge"
#define REMOTE_UPDATE_BRIDGE_SPAN 256
#define REMOTE_UPDATE_BRIDGE_TYPE "altera_avalon_mm_bridge"


/*
 * rtc_i2c configuration
 *
 */

#define ALT_MODULE_CLASS_rtc_i2c altera_avalon_i2c
#define RTC_I2C_BASE 0x49140
#define RTC_I2C_FIFO_DEPTH 4
#define RTC_I2C_FREQ 25000000
#define RTC_I2C_IRQ 7
#define RTC_I2C_IRQ_INTERRUPT_CONTROLLER_ID 0
#define RTC_I2C_NAME "/dev/rtc_i2c"
#define RTC_I2C_SPAN 64
#define RTC_I2C_TYPE "altera_avalon_i2c"
#define RTC_I2C_USE_AV_ST 0


/*
 * rtc_time_cal_0 configuration
 *
 */

#define ALT_MODULE_CLASS_rtc_time_cal_0 rtc_time_cal
#define RTC_TIME_CAL_0_BASE 0x41000
#define RTC_TIME_CAL_0_IRQ -1
#define RTC_TIME_CAL_0_IRQ_INTERRUPT_CONTROLLER_ID -1
#define RTC_TIME_CAL_0_NAME "/dev/rtc_time_cal_0"
#define RTC_TIME_CAL_0_SPAN 1024
#define RTC_TIME_CAL_0_TYPE "rtc_time_cal"


/*
 * rx_irig_top_0 configuration
 *
 */

#define ALT_MODULE_CLASS_rx_irig_top_0 rx_irig_top
#define RX_IRIG_TOP_0_BASE 0x41400
#define RX_IRIG_TOP_0_IRQ -1
#define RX_IRIG_TOP_0_IRQ_INTERRUPT_CONTROLLER_ID -1
#define RX_IRIG_TOP_0_NAME "/dev/rx_irig_top_0"
#define RX_IRIG_TOP_0_SPAN 1024
#define RX_IRIG_TOP_0_TYPE "rx_irig_top"


/*
 * rx_irig_top_1 configuration
 *
 */

#define ALT_MODULE_CLASS_rx_irig_top_1 rx_irig_top
#define RX_IRIG_TOP_1_BASE 0x44000
#define RX_IRIG_TOP_1_IRQ -1
#define RX_IRIG_TOP_1_IRQ_INTERRUPT_CONTROLLER_ID -1
#define RX_IRIG_TOP_1_NAME "/dev/rx_irig_top_1"
#define RX_IRIG_TOP_1_SPAN 1024
#define RX_IRIG_TOP_1_TYPE "rx_irig_top"


/*
 * siosc_i2c configuration
 *
 */

#define ALT_MODULE_CLASS_siosc_i2c altera_avalon_i2c
#define SIOSC_I2C_BASE 0x49100
#define SIOSC_I2C_FIFO_DEPTH 4
#define SIOSC_I2C_FREQ 25000000
#define SIOSC_I2C_IRQ 6
#define SIOSC_I2C_IRQ_INTERRUPT_CONTROLLER_ID 0
#define SIOSC_I2C_NAME "/dev/siosc_i2c"
#define SIOSC_I2C_SPAN 64
#define SIOSC_I2C_TYPE "altera_avalon_i2c"
#define SIOSC_I2C_USE_AV_ST 0


/*
 * sysid configuration
 *
 */

#define ALT_MODULE_CLASS_sysid altera_avalon_sysid_qsys
#define SYSID_BASE 0x4f010
#define SYSID_ID 0
#define SYSID_IRQ -1
#define SYSID_IRQ_INTERRUPT_CONTROLLER_ID -1
#define SYSID_NAME "/dev/sysid"
#define SYSID_SPAN 8
#define SYSID_TIMESTAMP 1701355948
#define SYSID_TYPE "altera_avalon_sysid_qsys"


/*
 * time_manual_inc_top_0 configuration
 *
 */

#define ALT_MODULE_CLASS_time_manual_inc_top_0 time_manual_inc_top
#define TIME_MANUAL_INC_TOP_0_BASE 0x4c000
#define TIME_MANUAL_INC_TOP_0_IRQ -1
#define TIME_MANUAL_INC_TOP_0_IRQ_INTERRUPT_CONTROLLER_ID -1
#define TIME_MANUAL_INC_TOP_0_NAME "/dev/time_manual_inc_top_0"
#define TIME_MANUAL_INC_TOP_0_SPAN 1024
#define TIME_MANUAL_INC_TOP_0_TYPE "time_manual_inc_top"


/*
 * time_selector_top_0 configuration
 *
 */

#define ALT_MODULE_CLASS_time_selector_top_0 time_selector_top
#define TIME_SELECTOR_TOP_0_BASE 0x41c00
#define TIME_SELECTOR_TOP_0_IRQ -1
#define TIME_SELECTOR_TOP_0_IRQ_INTERRUPT_CONTROLLER_ID -1
#define TIME_SELECTOR_TOP_0_NAME "/dev/time_selector_top_0"
#define TIME_SELECTOR_TOP_0_SPAN 1024
#define TIME_SELECTOR_TOP_0_TYPE "time_selector_top"


/*
 * timer_0 configuration
 *
 */

#define ALT_MODULE_CLASS_timer_0 altera_avalon_timer
#define TIMER_0_ALWAYS_RUN 0
#define TIMER_0_BASE 0x4f100
#define TIMER_0_COUNTER_SIZE 32
#define TIMER_0_FIXED_PERIOD 0
#define TIMER_0_FREQ 100000000
#define TIMER_0_IRQ 1
#define TIMER_0_IRQ_INTERRUPT_CONTROLLER_ID 0
#define TIMER_0_LOAD_VALUE 99999
#define TIMER_0_MULT 0.001
#define TIMER_0_NAME "/dev/timer_0"
#define TIMER_0_PERIOD 1
#define TIMER_0_PERIOD_UNITS "ms"
#define TIMER_0_RESET_OUTPUT 0
#define TIMER_0_SNAPSHOT 1
#define TIMER_0_SPAN 32
#define TIMER_0_TICKS_PER_SEC 1000
#define TIMER_0_TIMEOUT_PULSE_OUTPUT 0
#define TIMER_0_TYPE "altera_avalon_timer"


/*
 * tsd_gps_core_driver configuration
 *
 */

#define TSD_GPS_CORE_DRIVER


/*
 * tsd_manual_inc_time_core_driver configuration
 *
 */

#define TSD_MANUAL_INC_TIME_CORE_DRIVER


/*
 * tsd_rtc_calibrator_driver configuration
 *
 */

#define TSD_RTC_CALIBRATOR_DRIVER


/*
 * tsd_rx_1pps_core_driver configuration
 *
 */

#define TSD_RX_1PPS_CORE_DRIVER


/*
 * tsd_rx_irig_core_driver configuration
 *
 */

#define TSD_RX_IRIG_DRIVER


/*
 * tsd_shift_reg_led_driver configuration
 *
 */

#define TSD_SXE_CH10_DRIVER


/*
 * tsd_time_bcd configuration
 *
 */

#define TSD_TIME_BCD 1


/*
 * tsd_time_input_selector_driver configuration
 *
 */

#define TSD_TIME_INPUT_SELECTOR_DRIVER


/*
 * tsd_tse_sxe_core_driver configuration
 *
 */

#define TSD_TSE_SXE_DRIVER


/*
 * tsd_tx_irig_ch10_core_driver configuration
 *
 */

#define TSD_TX_IRIG_CH10_CORE_DRIVER


/*
 * tsd_tx_irig_core_driver configuration
 *
 */

#define TSD_TX_IRIG_CORE_DRIVER


/*
 * tse_subsystem_eth_tse_0 configuration
 *
 */

#define ALT_MODULE_CLASS_tse_subsystem_eth_tse_0 altera_eth_tse
#define TSE_SUBSYSTEM_ETH_TSE_0_BASE 0x41800
#define TSE_SUBSYSTEM_ETH_TSE_0_ENABLE_MACLITE 0
#define TSE_SUBSYSTEM_ETH_TSE_0_FIFO_WIDTH 32
#define TSE_SUBSYSTEM_ETH_TSE_0_IRQ -1
#define TSE_SUBSYSTEM_ETH_TSE_0_IRQ_INTERRUPT_CONTROLLER_ID -1
#define TSE_SUBSYSTEM_ETH_TSE_0_IS_MULTICHANNEL_MAC 0
#define TSE_SUBSYSTEM_ETH_TSE_0_MACLITE_GIGE 0
#define TSE_SUBSYSTEM_ETH_TSE_0_MDIO_SHARED 0
#define TSE_SUBSYSTEM_ETH_TSE_0_NAME "/dev/tse_subsystem_eth_tse_0"
#define TSE_SUBSYSTEM_ETH_TSE_0_NUMBER_OF_CHANNEL 1
#define TSE_SUBSYSTEM_ETH_TSE_0_NUMBER_OF_MAC_MDIO_SHARED 1
#define TSE_SUBSYSTEM_ETH_TSE_0_PCS 1
#define TSE_SUBSYSTEM_ETH_TSE_0_PCS_ID 0
#define TSE_SUBSYSTEM_ETH_TSE_0_PCS_SGMII 1
#define TSE_SUBSYSTEM_ETH_TSE_0_RECEIVE_FIFO_DEPTH 64
#define TSE_SUBSYSTEM_ETH_TSE_0_REGISTER_SHARED 0
#define TSE_SUBSYSTEM_ETH_TSE_0_RGMII 0
#define TSE_SUBSYSTEM_ETH_TSE_0_SPAN 1024
#define TSE_SUBSYSTEM_ETH_TSE_0_TRANSMIT_FIFO_DEPTH 2048
#define TSE_SUBSYSTEM_ETH_TSE_0_TYPE "altera_eth_tse"
#define TSE_SUBSYSTEM_ETH_TSE_0_USE_MDIO 0


/*
 * tx_rtc_time_cal configuration
 *
 */

#define ALT_MODULE_CLASS_tx_rtc_time_cal rtc_time_cal
#define TX_RTC_TIME_CAL_BASE 0x52000
#define TX_RTC_TIME_CAL_IRQ -1
#define TX_RTC_TIME_CAL_IRQ_INTERRUPT_CONTROLLER_ID -1
#define TX_RTC_TIME_CAL_NAME "/dev/tx_rtc_time_cal"
#define TX_RTC_TIME_CAL_SPAN 1024
#define TX_RTC_TIME_CAL_TYPE "rtc_time_cal"


/*
 * txirig_subsystem_ch10_file_reader_0 configuration
 *
 */

#define ALT_MODULE_CLASS_txirig_subsystem_ch10_file_reader_0 ch10_file_reader
#define TXIRIG_SUBSYSTEM_CH10_FILE_READER_0_BASE 0x51800
#define TXIRIG_SUBSYSTEM_CH10_FILE_READER_0_IRQ -1
#define TXIRIG_SUBSYSTEM_CH10_FILE_READER_0_IRQ_INTERRUPT_CONTROLLER_ID -1
#define TXIRIG_SUBSYSTEM_CH10_FILE_READER_0_NAME "/dev/txirig_subsystem_ch10_file_reader_0"
#define TXIRIG_SUBSYSTEM_CH10_FILE_READER_0_SPAN 1024
#define TXIRIG_SUBSYSTEM_CH10_FILE_READER_0_TYPE "ch10_file_reader"


/*
 * txirig_subsystem_msgdma_0_csr configuration
 *
 */

#define ALT_MODULE_CLASS_txirig_subsystem_msgdma_0_csr altera_msgdma
#define TXIRIG_SUBSYSTEM_MSGDMA_0_CSR_BASE 0x51c00
#define TXIRIG_SUBSYSTEM_MSGDMA_0_CSR_BURST_ENABLE 1
#define TXIRIG_SUBSYSTEM_MSGDMA_0_CSR_BURST_WRAPPING_SUPPORT 0
#define TXIRIG_SUBSYSTEM_MSGDMA_0_CSR_CHANNEL_ENABLE 0
#define TXIRIG_SUBSYSTEM_MSGDMA_0_CSR_CHANNEL_ENABLE_DERIVED 0
#define TXIRIG_SUBSYSTEM_MSGDMA_0_CSR_CHANNEL_WIDTH 8
#define TXIRIG_SUBSYSTEM_MSGDMA_0_CSR_DATA_FIFO_DEPTH 256
#define TXIRIG_SUBSYSTEM_MSGDMA_0_CSR_DATA_WIDTH 32
#define TXIRIG_SUBSYSTEM_MSGDMA_0_CSR_DESCRIPTOR_FIFO_DEPTH 8
#define TXIRIG_SUBSYSTEM_MSGDMA_0_CSR_DMA_MODE 1
#define TXIRIG_SUBSYSTEM_MSGDMA_0_CSR_ENHANCED_FEATURES 0
#define TXIRIG_SUBSYSTEM_MSGDMA_0_CSR_ERROR_ENABLE 0
#define TXIRIG_SUBSYSTEM_MSGDMA_0_CSR_ERROR_ENABLE_DERIVED 0
#define TXIRIG_SUBSYSTEM_MSGDMA_0_CSR_ERROR_WIDTH 8
#define TXIRIG_SUBSYSTEM_MSGDMA_0_CSR_IRQ 3
#define TXIRIG_SUBSYSTEM_MSGDMA_0_CSR_IRQ_INTERRUPT_CONTROLLER_ID 0
#define TXIRIG_SUBSYSTEM_MSGDMA_0_CSR_MAX_BURST_COUNT 32
#define TXIRIG_SUBSYSTEM_MSGDMA_0_CSR_MAX_BYTE 1024
#define TXIRIG_SUBSYSTEM_MSGDMA_0_CSR_MAX_STRIDE 1
#define TXIRIG_SUBSYSTEM_MSGDMA_0_CSR_NAME "/dev/txirig_subsystem_msgdma_0_csr"
#define TXIRIG_SUBSYSTEM_MSGDMA_0_CSR_PACKET_ENABLE 1
#define TXIRIG_SUBSYSTEM_MSGDMA_0_CSR_PACKET_ENABLE_DERIVED 1
#define TXIRIG_SUBSYSTEM_MSGDMA_0_CSR_PREFETCHER_ENABLE 0
#define TXIRIG_SUBSYSTEM_MSGDMA_0_CSR_PROGRAMMABLE_BURST_ENABLE 0
#define TXIRIG_SUBSYSTEM_MSGDMA_0_CSR_RESPONSE_PORT 2
#define TXIRIG_SUBSYSTEM_MSGDMA_0_CSR_SPAN 32
#define TXIRIG_SUBSYSTEM_MSGDMA_0_CSR_STRIDE_ENABLE 0
#define TXIRIG_SUBSYSTEM_MSGDMA_0_CSR_STRIDE_ENABLE_DERIVED 0
#define TXIRIG_SUBSYSTEM_MSGDMA_0_CSR_TRANSFER_TYPE "Full Word Accesses Only"
#define TXIRIG_SUBSYSTEM_MSGDMA_0_CSR_TYPE "altera_msgdma"


/*
 * txirig_subsystem_msgdma_0_descriptor_slave configuration
 *
 */

#define ALT_MODULE_CLASS_txirig_subsystem_msgdma_0_descriptor_slave altera_msgdma
#define TXIRIG_SUBSYSTEM_MSGDMA_0_DESCRIPTOR_SLAVE_BASE 0x51c20
#define TXIRIG_SUBSYSTEM_MSGDMA_0_DESCRIPTOR_SLAVE_BURST_ENABLE 1
#define TXIRIG_SUBSYSTEM_MSGDMA_0_DESCRIPTOR_SLAVE_BURST_WRAPPING_SUPPORT 0
#define TXIRIG_SUBSYSTEM_MSGDMA_0_DESCRIPTOR_SLAVE_CHANNEL_ENABLE 0
#define TXIRIG_SUBSYSTEM_MSGDMA_0_DESCRIPTOR_SLAVE_CHANNEL_ENABLE_DERIVED 0
#define TXIRIG_SUBSYSTEM_MSGDMA_0_DESCRIPTOR_SLAVE_CHANNEL_WIDTH 8
#define TXIRIG_SUBSYSTEM_MSGDMA_0_DESCRIPTOR_SLAVE_DATA_FIFO_DEPTH 256
#define TXIRIG_SUBSYSTEM_MSGDMA_0_DESCRIPTOR_SLAVE_DATA_WIDTH 32
#define TXIRIG_SUBSYSTEM_MSGDMA_0_DESCRIPTOR_SLAVE_DESCRIPTOR_FIFO_DEPTH 8
#define TXIRIG_SUBSYSTEM_MSGDMA_0_DESCRIPTOR_SLAVE_DMA_MODE 1
#define TXIRIG_SUBSYSTEM_MSGDMA_0_DESCRIPTOR_SLAVE_ENHANCED_FEATURES 0
#define TXIRIG_SUBSYSTEM_MSGDMA_0_DESCRIPTOR_SLAVE_ERROR_ENABLE 0
#define TXIRIG_SUBSYSTEM_MSGDMA_0_DESCRIPTOR_SLAVE_ERROR_ENABLE_DERIVED 0
#define TXIRIG_SUBSYSTEM_MSGDMA_0_DESCRIPTOR_SLAVE_ERROR_WIDTH 8
#define TXIRIG_SUBSYSTEM_MSGDMA_0_DESCRIPTOR_SLAVE_IRQ -1
#define TXIRIG_SUBSYSTEM_MSGDMA_0_DESCRIPTOR_SLAVE_IRQ_INTERRUPT_CONTROLLER_ID -1
#define TXIRIG_SUBSYSTEM_MSGDMA_0_DESCRIPTOR_SLAVE_MAX_BURST_COUNT 32
#define TXIRIG_SUBSYSTEM_MSGDMA_0_DESCRIPTOR_SLAVE_MAX_BYTE 1024
#define TXIRIG_SUBSYSTEM_MSGDMA_0_DESCRIPTOR_SLAVE_MAX_STRIDE 1
#define TXIRIG_SUBSYSTEM_MSGDMA_0_DESCRIPTOR_SLAVE_NAME "/dev/txirig_subsystem_msgdma_0_descriptor_slave"
#define TXIRIG_SUBSYSTEM_MSGDMA_0_DESCRIPTOR_SLAVE_PACKET_ENABLE 1
#define TXIRIG_SUBSYSTEM_MSGDMA_0_DESCRIPTOR_SLAVE_PACKET_ENABLE_DERIVED 1
#define TXIRIG_SUBSYSTEM_MSGDMA_0_DESCRIPTOR_SLAVE_PREFETCHER_ENABLE 0
#define TXIRIG_SUBSYSTEM_MSGDMA_0_DESCRIPTOR_SLAVE_PROGRAMMABLE_BURST_ENABLE 0
#define TXIRIG_SUBSYSTEM_MSGDMA_0_DESCRIPTOR_SLAVE_RESPONSE_PORT 2
#define TXIRIG_SUBSYSTEM_MSGDMA_0_DESCRIPTOR_SLAVE_SPAN 16
#define TXIRIG_SUBSYSTEM_MSGDMA_0_DESCRIPTOR_SLAVE_STRIDE_ENABLE 0
#define TXIRIG_SUBSYSTEM_MSGDMA_0_DESCRIPTOR_SLAVE_STRIDE_ENABLE_DERIVED 0
#define TXIRIG_SUBSYSTEM_MSGDMA_0_DESCRIPTOR_SLAVE_TRANSFER_TYPE "Full Word Accesses Only"
#define TXIRIG_SUBSYSTEM_MSGDMA_0_DESCRIPTOR_SLAVE_TYPE "altera_msgdma"


/*
 * txirig_subsystem_onchip_memory2_0 configuration
 *
 */

#define ALT_MODULE_CLASS_txirig_subsystem_onchip_memory2_0 altera_avalon_onchip_memory2
#define TXIRIG_SUBSYSTEM_ONCHIP_MEMORY2_0_ALLOW_IN_SYSTEM_MEMORY_CONTENT_EDITOR 0
#define TXIRIG_SUBSYSTEM_ONCHIP_MEMORY2_0_ALLOW_MRAM_SIM_CONTENTS_ONLY_FILE 0
#define TXIRIG_SUBSYSTEM_ONCHIP_MEMORY2_0_BASE 0x50000
#define TXIRIG_SUBSYSTEM_ONCHIP_MEMORY2_0_CONTENTS_INFO ""
#define TXIRIG_SUBSYSTEM_ONCHIP_MEMORY2_0_DUAL_PORT 0
#define TXIRIG_SUBSYSTEM_ONCHIP_MEMORY2_0_GUI_RAM_BLOCK_TYPE "AUTO"
#define TXIRIG_SUBSYSTEM_ONCHIP_MEMORY2_0_INIT_CONTENTS_FILE "VNS_FPGA_qsys_top_txirig_subsystem_onchip_memory2_0"
#define TXIRIG_SUBSYSTEM_ONCHIP_MEMORY2_0_INIT_MEM_CONTENT 1
#define TXIRIG_SUBSYSTEM_ONCHIP_MEMORY2_0_INSTANCE_ID "NONE"
#define TXIRIG_SUBSYSTEM_ONCHIP_MEMORY2_0_IRQ -1
#define TXIRIG_SUBSYSTEM_ONCHIP_MEMORY2_0_IRQ_INTERRUPT_CONTROLLER_ID -1
#define TXIRIG_SUBSYSTEM_ONCHIP_MEMORY2_0_NAME "/dev/txirig_subsystem_onchip_memory2_0"
#define TXIRIG_SUBSYSTEM_ONCHIP_MEMORY2_0_NON_DEFAULT_INIT_FILE_ENABLED 0
#define TXIRIG_SUBSYSTEM_ONCHIP_MEMORY2_0_RAM_BLOCK_TYPE "AUTO"
#define TXIRIG_SUBSYSTEM_ONCHIP_MEMORY2_0_READ_DURING_WRITE_MODE "DONT_CARE"
#define TXIRIG_SUBSYSTEM_ONCHIP_MEMORY2_0_SINGLE_CLOCK_OP 0
#define TXIRIG_SUBSYSTEM_ONCHIP_MEMORY2_0_SIZE_MULTIPLE 1
#define TXIRIG_SUBSYSTEM_ONCHIP_MEMORY2_0_SIZE_VALUE 4096
#define TXIRIG_SUBSYSTEM_ONCHIP_MEMORY2_0_SPAN 4096
#define TXIRIG_SUBSYSTEM_ONCHIP_MEMORY2_0_TYPE "altera_avalon_onchip_memory2"
#define TXIRIG_SUBSYSTEM_ONCHIP_MEMORY2_0_WRITABLE 1


/*
 * txirig_subsystem_tx_irig_0 configuration
 *
 */

#define ALT_MODULE_CLASS_txirig_subsystem_tx_irig_0 tx_irig
#define TXIRIG_SUBSYSTEM_TX_IRIG_0_BASE 0x51000
#define TXIRIG_SUBSYSTEM_TX_IRIG_0_IRQ -1
#define TXIRIG_SUBSYSTEM_TX_IRIG_0_IRQ_INTERRUPT_CONTROLLER_ID -1
#define TXIRIG_SUBSYSTEM_TX_IRIG_0_NAME "/dev/txirig_subsystem_tx_irig_0"
#define TXIRIG_SUBSYSTEM_TX_IRIG_0_SPAN 1024
#define TXIRIG_SUBSYSTEM_TX_IRIG_0_TYPE "tx_irig"


/*
 * txirig_subsystem_tx_irig_ch10_top_0 configuration
 *
 */

#define ALT_MODULE_CLASS_txirig_subsystem_tx_irig_ch10_top_0 tx_irig_ch10_top
#define TXIRIG_SUBSYSTEM_TX_IRIG_CH10_TOP_0_BASE 0x51400
#define TXIRIG_SUBSYSTEM_TX_IRIG_CH10_TOP_0_IRQ -1
#define TXIRIG_SUBSYSTEM_TX_IRIG_CH10_TOP_0_IRQ_INTERRUPT_CONTROLLER_ID -1
#define TXIRIG_SUBSYSTEM_TX_IRIG_CH10_TOP_0_NAME "/dev/txirig_subsystem_tx_irig_ch10_top_0"
#define TXIRIG_SUBSYSTEM_TX_IRIG_CH10_TOP_0_SPAN 1024
#define TXIRIG_SUBSYSTEM_TX_IRIG_CH10_TOP_0_TYPE "tx_irig_ch10_top"

#endif /* __SYSTEM_H_ */
