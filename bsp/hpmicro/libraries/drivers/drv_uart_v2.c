/*
 * Copyright (c) 2022 hpmicro
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Change Logs:
 * Date         Author      Notes
 * 2022-03-08   hpmicro     First version
 * 2022-07-28   hpmicro     Fix compiling warning if RT_SERIAL_USING_DMA was not defined
 * 2022-08-08   hpmicro     Integrate DMA Manager and support dynamic DMA resource assignment
 *
 */
#include <rtthread.h>
#include <rtdevice.h>
#include "board.h"
#include "drv_uart_v2.h"
#include "hpm_uart_drv.h"
#include "hpm_sysctl_drv.h"
#include "hpm_l1c_drv.h"
#include "hpm_dma_drv.h"
#include "hpm_dmamux_drv.h"
#include "hpm_dma_manager.h"
#include "hpm_soc.h"

#ifdef RT_USING_SERIAL_V2

#ifdef RT_SERIAL_USING_DMA
#define BOARD_UART_DMAMUX  HPM_DMAMUX
#define UART_DMA_TRIGGER_LEVEL (1U)

typedef struct dma_channel {
    struct rt_serial_device *serial;
    hpm_dma_resource_t resource;
    void (*tranfer_done)(struct rt_serial_device *serial);
    void (*tranfer_abort)(struct rt_serial_device *serial);
    void (*tranfer_error)(struct rt_serial_device *serial);
} hpm_dma_channel_handle_t;

//static struct dma_channel dma_channels[DMA_SOC_CHANNEL_NUM];
static int hpm_uart_dma_config(struct rt_serial_device *serial, void *arg);
#endif

#define UART_ROOT_CLK_FREQ BOARD_APP_UART_SRC_FREQ

struct hpm_uart {
    UART_Type *uart_base;
    uint32_t irq_num;
    struct rt_serial_device *serial;
    char *device_name;
#ifdef RT_SERIAL_USING_DMA
    uint32_t tx_dma_mux;
    uint32_t rx_dma_mux;
    uint32_t dma_flags;
    hpm_dma_channel_handle_t tx_chn_ctx;
    hpm_dma_channel_handle_t rx_chn_ctx;
    bool tx_resource_allocated;
    bool rx_resource_allocated;
#endif
};


extern void init_uart_pins(UART_Type *ptr);
static void hpm_uart_isr(struct rt_serial_device *serial);
static rt_err_t hpm_uart_configure(struct rt_serial_device *serial, struct serial_configure *cfg);
static rt_err_t hpm_uart_control(struct rt_serial_device *serial, int cmd, void *arg);
static int hpm_uart_putc(struct rt_serial_device *serial, char ch);
static int hpm_uart_getc(struct rt_serial_device *serial);

#ifdef RT_SERIAL_USING_DMA
int hpm_uart_dma_register_channel(struct rt_serial_device *serial,
                                                 bool is_tx,
                                                     void (*done)(struct rt_serial_device *serial),
                                                     void (*abort)(struct rt_serial_device *serial),
                                                     void (*error)(struct rt_serial_device *serial))
{

    struct hpm_uart *uart = (struct hpm_uart *)serial->parent.user_data;

    if (is_tx) {
        uart->tx_chn_ctx.serial = serial;
        uart->tx_chn_ctx.tranfer_done = done;
        uart->tx_chn_ctx.tranfer_abort = abort;
        uart->tx_chn_ctx.tranfer_error = error;
    } else {
        uart->rx_chn_ctx.serial = serial;
        uart->rx_chn_ctx.tranfer_done = done;
        uart->rx_chn_ctx.tranfer_abort = abort;
        uart->rx_chn_ctx.tranfer_error = error;
    }
    return RT_EOK;
}
#endif /* RT_SERIAL_USING_DMA */

#if defined(BSP_USING_UART0)
struct rt_serial_device serial0;
void uart0_isr(void)
{
    hpm_uart_isr(&serial0);
}
SDK_DECLARE_EXT_ISR_M(IRQn_UART0,uart0_isr)
#endif


#if defined(BSP_USING_UART1)
struct rt_serial_device serial1;
void uart1_isr(void)
{
    hpm_uart_isr(&serial1);
}
SDK_DECLARE_EXT_ISR_M(IRQn_UART1,uart1_isr)
#endif


#if defined(BSP_USING_UART2)
struct rt_serial_device serial2;
void uart2_isr(void)
{
    hpm_uart_isr(&serial2);
}
SDK_DECLARE_EXT_ISR_M(IRQn_UART2,uart2_isr)
#endif


#if defined(BSP_USING_UART3)
struct rt_serial_device serial3;
void uart3_isr(void)
{
    hpm_uart_isr(&serial3);
}
SDK_DECLARE_EXT_ISR_M(IRQn_UART3,uart3_isr)
#endif


#if defined(BSP_USING_UART4)
struct rt_serial_device serial4;
void uart4_isr(void)
{
    hpm_uart_isr(&serial4);
}
SDK_DECLARE_EXT_ISR_M(IRQn_UART4,uart4_isr)
#endif


#if defined(BSP_USING_UART5)
struct rt_serial_device serial5;
void uart5_isr(void)
{
    hpm_uart_isr(&serial5);
}
SDK_DECLARE_EXT_ISR_M(IRQn_UART5,uart5_isr)
#endif


#if defined(BSP_USING_UART6)
struct rt_serial_device serial6;
void uart6_isr(void)
{
    hpm_uart_isr(&serial6);
}
SDK_DECLARE_EXT_ISR_M(IRQn_UART6,uart6_isr)
#endif


#if defined(BSP_USING_UART7)
struct rt_serial_device serial7;
void uart7_isr(void)
{
    hpm_uart_isr(&serial7);
}
SDK_DECLARE_EXT_ISR_M(IRQn_UART7,uart7_isr)
#endif


#if defined(BSP_USING_UART8)
struct rt_serial_device serial8;
void uart8_isr(void)
{
    hpm_uart_isr(&serial8);
}
SDK_DECLARE_EXT_ISR_M(IRQn_UART8,uart8_isr)
#endif


#if defined(BSP_USING_UART9)
struct rt_serial_device serial9;
void uart9_isr(void)
{
    hpm_uart_isr(&serial9);
}
SDK_DECLARE_EXT_ISR_M(IRQn_UART9,uart9_isr)
#endif


#if defined(BSP_USING_UART10)
struct rt_serial_device serial10;
void uart10_isr(void)
{
    hpm_uart_isr(&serial10);
}
SDK_DECLARE_EXT_ISR_M(IRQn_UART10,uart10_isr)
#endif

#if defined(BSP_USING_UART11)
struct rt_serial_device serial11;
void uart11_isr(void)
{
    hpm_uart_isr(&serial11);
}
SDK_DECLARE_EXT_ISR_M(IRQn_UART11,uart11_isr)
#endif

#if defined(BSP_USING_UART12)
struct rt_serial_device serial12;
void uart12_isr(void)
{
    hpm_uart_isr(&serial12);
}
SDK_DECLARE_EXT_ISR_M(IRQn_UART12,uart12_isr)
#endif

#if defined(BSP_USING_UART13)
struct rt_serial_device serial13;
void uart13_isr(void)
{
    hpm_uart_isr(&serial13);
}
SDK_DECLARE_EXT_ISR_M(IRQn_UART13,uart13_isr)
#endif

#if defined(BSP_USING_UART14)
struct rt_serial_device serial14;
void uart14_isr(void)
{
    hpm_uart_isr(&serial14);
}
SDK_DECLARE_EXT_ISR_M(IRQn_UART14,uart14_isr)
#endif

#if defined(BSP_USING_UART15)
struct rt_serial_device serial15;
void uart15_isr(void)
{
    hpm_uart_isr(&serial15);
}
SDK_DECLARE_EXT_ISR_M(IRQn_UART15,uart15_isr)
#endif

static struct hpm_uart uarts[] =
{
#if defined(BSP_USING_UART0)
{
    HPM_UART0,
    IRQn_UART0,
    &serial0,
    "uart0",
#ifdef RT_SERIAL_USING_DMA
    HPM_DMA_SRC_UART0_TX,
    HPM_DMA_SRC_UART0_RX,
    0,
#endif
},
#endif

#if defined(BSP_USING_UART1)
{
    HPM_UART1,
    IRQn_UART1,
    &serial1,
    "uart1",
#ifdef RT_SERIAL_USING_DMA
    HPM_DMA_SRC_UART1_TX,
    HPM_DMA_SRC_UART1_RX,
    0,
#endif
},
#endif

#if defined(BSP_USING_UART2)
{
    HPM_UART2,
    IRQn_UART2,
    &serial2,
    "uart2",
#ifdef RT_SERIAL_USING_DMA
    HPM_DMA_SRC_UART2_TX,
    HPM_DMA_SRC_UART2_RX,
    0,
#endif
},
#endif

#if defined(BSP_USING_UART3)
{
    HPM_UART3,
    IRQn_UART3,
    &serial3,
    "uart3",
#ifdef RT_SERIAL_USING_DMA
    HPM_DMA_SRC_UART3_TX,
    HPM_DMA_SRC_UART3_RX,
    0,
#endif
},
#endif

#if defined(BSP_USING_UART4)
{
    HPM_UART4,
    IRQn_UART4,
    &serial4,
    "uart4",
#ifdef RT_SERIAL_USING_DMA
    HPM_DMA_SRC_UART4_TX,
    HPM_DMA_SRC_UART4_RX,
    0,
#endif
},
#endif

#if defined(BSP_USING_UART5)
{
    HPM_UART5,
    IRQn_UART5,
    &serial5,
    "uart5",
#ifdef RT_SERIAL_USING_DMA
    HPM_DMA_SRC_UART5_TX,
    HPM_DMA_SRC_UART5_RX,
    0,
#endif
},
#endif

#if defined(BSP_USING_UART6)
{
    HPM_UART6,
    IRQn_UART6,
    &serial6,
    "uart6",
#ifdef RT_SERIAL_USING_DMA
    HPM_DMA_SRC_UART6_TX,
    HPM_DMA_SRC_UART6_RX,
    0,
#endif
},
#endif

#if defined(BSP_USING_UART7)
{
    HPM_UART7,
    IRQn_UART7,
    &serial7,
    "uart7",
#ifdef RT_SERIAL_USING_DMA
    HPM_DMA_SRC_UART7_TX,
    HPM_DMA_SRC_UART7_RX,
    0,
#endif
},
#endif

#if defined(BSP_USING_UART8)
{
    HPM_UART8,
    IRQn_UART8,
    &serial8,
    "uart8",
#ifdef RT_SERIAL_USING_DMA
    HPM_DMA_SRC_UART8_TX,
    HPM_DMA_SRC_UART8_RX,
    0,
#endif
},
#endif

#if defined(BSP_USING_UART9)
{
    HPM_UART9,
    IRQn_UART9,
    &serial9,
    "uart9",
#ifdef RT_SERIAL_USING_DMA
    HPM_DMA_SRC_UART9_TX,
    HPM_DMA_SRC_UART9_RX,
    0,
#endif
},
#endif

#if defined(BSP_USING_UART10)
{
    HPM_UART10,
    IRQn_UART10,
    &serial10,
    "uart10",
#ifdef RT_SERIAL_USING_DMA
    HPM_DMA_SRC_UART10_TX,
    HPM_DMA_SRC_UART10_RX,
    0,
#endif
},
#endif

#if defined(BSP_USING_UART11)
{
    HPM_UART11,
    IRQn_UART11,
    &serial11,
    "uart11",
#ifdef RT_SERIAL_USING_DMA
    HPM_DMA_SRC_UART11_TX,
    HPM_DMA_SRC_UART11_RX,
    0,
#endif
},
#endif

#if defined(BSP_USING_UART12)
{
    HPM_UART12,
    IRQn_UART12,
    &serial12,
    "uart12",
#ifdef RT_SERIAL_USING_DMA
    HPM_DMA_SRC_UART12_TX,
    HPM_DMA_SRC_UART12_RX,
    0,
#endif
},
#endif

#if defined(BSP_USING_UART13)
{
    HPM_UART13,
    IRQn_UART13,
    &serial13,
    "uart13",
#ifdef RT_SERIAL_USING_DMA
    HPM_DMA_SRC_UART13_TX,
    HPM_DMA_SRC_UART13_RX,
    0,
#endif
},
#endif

#if defined(BSP_USING_UART14)
{
    HPM_UART14,
    IRQn_UART14,
    &serial14,
    "uart14",
#ifdef RT_SERIAL_USING_DMA
    HPM_DMA_SRC_UART14_TX,
    HPM_DMA_SRC_UART14_RX,
    0,
#endif
},
#endif

#if defined(BSP_USING_UART15)
{
    HPM_UART15,
    IRQn_UART15,
    &serial15,
    "uart15",
#ifdef RT_SERIAL_USING_DMA
    HPM_DMA_SRC_UART15_TX,
    HPM_DMA_SRC_UART15_RX,
    0,
#endif
},
#endif

};

enum
{
#if defined(BSP_USING_UART0)
    HPM_UART0_INDEX,
#endif

#if defined(BSP_USING_UART1)
    HPM_UART1_INDEX,
#endif

#if defined(BSP_USING_UART2)
    HPM_UART2_INDEX,
#endif

#if defined(BSP_USING_UART3)
    HPM_UART3_INDEX,
#endif

#if defined(BSP_USING_UART4)
    HPM_UART4_INDEX,
#endif

#if defined(BSP_USING_UART5)
    HPM_UART15_INDEX,
#endif

#if defined(BSP_USING_UART6)
    HPM_UART6_INDEX,
#endif

#if defined(BSP_USING_UART7)
    HPM_UART7_INDEX,
#endif

#if defined(BSP_USING_UART8)
    HPM_UART8_INDEX,
#endif

#if defined(BSP_USING_UART9)
    HPM_UART10_INDEX,
#endif

#if defined(BSP_USING_UART10)
    HPM_UART10_INDEX,
#endif

#if defined(BSP_USING_UART11)
    HPM_UART11_INDEX,
#endif

#if defined(BSP_USING_UART12)
    HPM_UART12_INDEX,
#endif

#if defined(BSP_USING_UART13)
    HPM_UART13_INDEX,
#endif

#if defined(BSP_USING_UART14)
    HPM_UART14_INDEX,
#endif

#if defined(BSP_USING_UART15)
    HPM_UART15_INDEX,
#endif
};

#if defined(RT_SERIAL_USING_DMA)

static void uart_dma_callback(DMA_Type *base, uint32_t channel, void *user_data,  uint32_t int_stat)
{
    hpm_dma_channel_handle_t *dma_handle = (hpm_dma_channel_handle_t*)user_data;
    if ((dma_handle->resource.base != base) || (dma_handle->resource.channel != channel))
    {
        return;
    }

    if (IS_HPM_BITMASK_SET(int_stat, DMA_CHANNEL_STATUS_TC) && (dma_handle->tranfer_done != NULL))
    {
        dma_handle->tranfer_done(dma_handle->serial);
    }

    if (IS_HPM_BITMASK_SET(int_stat, DMA_CHANNEL_STATUS_ABORT) && (dma_handle->tranfer_abort != NULL))
    {
        dma_handle->tranfer_abort(dma_handle->serial);
    }

    if (IS_HPM_BITMASK_SET(int_stat, DMA_CHANNEL_STATUS_ERROR) && (dma_handle->tranfer_error != NULL))
    {
        dma_handle->tranfer_error(dma_handle->serial);
    }
}



static void uart_tx_done(struct rt_serial_device *serial)
{
    rt_hw_serial_isr(serial, RT_SERIAL_EVENT_TX_DMADONE);
}

static void uart_rx_done(struct rt_serial_device *serial)
{
    struct rt_serial_rx_fifo *rx_fifo;

    rx_fifo = (struct rt_serial_rx_fifo *)serial->serial_rx;
    if (l1c_dc_is_enabled()) {
        l1c_dc_invalidate((uint32_t)rx_fifo->buffer, serial->config.rx_bufsz);
    }
    rt_hw_serial_isr(serial, RT_SERIAL_EVENT_RX_DMADONE | (serial->config.rx_bufsz << 8));
    /* prepare for next read */
    hpm_uart_dma_config(serial, (void *)RT_DEVICE_FLAG_DMA_RX);
}
#endif /* RT_SERIAL_USING_DMA */

/**
 * @brief UART common interrupt process. This
 *
 * @param serial Serial device
 */
static void hpm_uart_isr(struct rt_serial_device *serial)
{
    struct hpm_uart *uart;
    rt_uint32_t stat, enabled_irq;

    RT_ASSERT(serial != RT_NULL);

    uart = (struct hpm_uart *)serial->parent.user_data;
    RT_ASSERT(uart != RT_NULL);

    /* enter interrupt */
    rt_interrupt_enter();
    stat = uart_get_status(uart->uart_base);
    enabled_irq = uart_get_enabled_irq(uart->uart_base);
    if ((enabled_irq & uart_intr_rx_data_avail_or_timeout) && (stat & uart_stat_data_ready)) {
        struct rt_serial_rx_fifo *rx_fifo;
        rx_fifo = (struct rt_serial_rx_fifo *) serial->serial_rx;
        rt_uint8_t put_char = 0;
        uart_receive_byte(uart->uart_base, &put_char);
        rt_ringbuffer_putchar(&(rx_fifo->rb), put_char);
        /* UART in mode Receiver */
        rt_hw_serial_isr(serial, RT_SERIAL_EVENT_RX_IND);
    }
    if ((enabled_irq & uart_intr_tx_slot_avail) && (stat & uart_stat_tx_slot_avail)) {
        /* UART in mode Transmitter */
        struct rt_serial_tx_fifo *tx_fifo;
        tx_fifo = (struct rt_serial_tx_fifo *) serial->serial_tx;
        RT_ASSERT(tx_fifo != RT_NULL);
        rt_uint8_t put_char = 0;
        for (;;) {
            if (rt_ringbuffer_getchar(&(tx_fifo->rb), &put_char)) {
                uart_send_byte(uart->uart_base, put_char);
            } else {
                uart_disable_irq(uart->uart_base, uart_intr_tx_slot_avail);
                rt_hw_serial_isr(serial, RT_SERIAL_EVENT_TX_DONE);
                break;
            }
        }
    }
    /* leave interrupt */
    rt_interrupt_leave();
}


static rt_err_t hpm_uart_configure(struct rt_serial_device *serial, struct serial_configure *cfg)
{

    RT_ASSERT(serial != RT_NULL);
    RT_ASSERT(cfg != RT_NULL);

    uart_config_t uart_config;
    struct hpm_uart *uart  = (struct hpm_uart *)serial->parent.user_data;

    init_uart_pins(uart->uart_base);
    uart_default_config(uart->uart_base, &uart_config);

    uart_config.src_freq_in_hz = board_init_uart_clock(uart->uart_base);
    uart_config.baudrate = cfg->baud_rate;
    uart_config.num_of_stop_bits = cfg->stop_bits;
    uart_config.parity = cfg->parity;
#ifdef RT_SERIAL_USING_DMA
    if (uart->dma_flags & (RT_DEVICE_FLAG_DMA_TX | RT_DEVICE_FLAG_DMA_RX)) {
        uart_config.fifo_enable = true;
        uart_config.dma_enable = true;
        if (uart->dma_flags & RT_DEVICE_FLAG_DMA_TX) {
            uart_config.tx_fifo_level = uart_tx_fifo_trg_not_full;
        }
        if (uart->dma_flags & RT_DEVICE_FLAG_DMA_RX) {
            uart_config.rx_fifo_level = uart_rx_fifo_trg_not_empty;
        }
    }
#endif

    uart_config.word_length = cfg->data_bits - DATA_BITS_5;
    hpm_stat_t status = uart_init(uart->uart_base, &uart_config);
    return (status != status_success) ? -RT_ERROR : RT_EOK;
}

#ifdef RT_SERIAL_USING_DMA

hpm_stat_t hpm_uart_dma_rx_init(struct hpm_uart *uart_ctx)
{
    hpm_stat_t status = status_fail;
    if (!uart_ctx->rx_resource_allocated)
    {
        status = dma_manager_request_resource(&uart_ctx->rx_chn_ctx.resource);
        if (status == status_success)
        {
            uart_ctx->dma_flags |= RT_DEVICE_FLAG_DMA_RX;
            uart_ctx->rx_resource_allocated = true;
            dma_manager_install_interrupt_callback(&uart_ctx->rx_chn_ctx.resource, uart_dma_callback, &uart_ctx->rx_chn_ctx);
        }
    }
    return status;
}

hpm_stat_t hpm_uart_dma_tx_init(struct hpm_uart *uart_ctx)
{
    hpm_stat_t status = status_fail;
    if (!uart_ctx->tx_resource_allocated)
    {
        status = dma_manager_request_resource(&uart_ctx->tx_chn_ctx.resource);
        if (status == status_success)
        {
            uart_ctx->dma_flags |= RT_DEVICE_FLAG_DMA_TX;
            uart_ctx->tx_resource_allocated = true;
            dma_manager_install_interrupt_callback(&uart_ctx->tx_chn_ctx.resource, uart_dma_callback, &uart_ctx->tx_chn_ctx);
        }
    }
    return status;
}

static int hpm_uart_dma_config(struct rt_serial_device *serial, void *arg)
{
    rt_ubase_t ctrl_arg = (rt_ubase_t) arg;
    struct hpm_uart *uart = (struct hpm_uart *)serial->parent.user_data;
    dma_handshake_config_t config;
    struct rt_serial_rx_fifo *rx_fifo;

    if (ctrl_arg == RT_DEVICE_FLAG_DMA_RX) {
        rx_fifo = (struct rt_serial_rx_fifo *)serial->serial_rx;
        config.ch_index = uart->rx_chn_ctx.resource.channel;
        config.dst = (uint32_t) rx_fifo->buffer;
        config.dst_fixed = false;
        config.src = (uint32_t)&(uart->uart_base->RBR);
        config.src_fixed = true;
        config.size_in_byte = serial->config.rx_bufsz;
        if (status_success != dma_setup_handshake(uart->rx_chn_ctx.resource.base, &config)) {
            return RT_ERROR;
        }
        uint32_t mux = DMA_SOC_CHN_TO_DMAMUX_CHN(uart->rx_chn_ctx.resource.base, uart->rx_dma_mux);
        dmamux_config(BOARD_UART_DMAMUX, uart->rx_chn_ctx.resource.channel, mux, true);
        hpm_uart_dma_register_channel(serial, false, uart_rx_done, RT_NULL, RT_NULL);
        intc_m_enable_irq(uart->rx_chn_ctx.resource.irq_num);
    } else if (ctrl_arg == RT_DEVICE_FLAG_DMA_TX) {
        uint32_t mux = DMA_SOC_CHN_TO_DMAMUX_CHN(uart->tx_chn_ctx.resource.base, uart->tx_dma_mux);
        dmamux_config(BOARD_UART_DMAMUX, uart->tx_chn_ctx.resource.channel, mux, true);
        intc_m_enable_irq(uart->tx_chn_ctx.resource.irq_num);
    }
    return RT_EOK;
}

static void hpm_uart_transmit_dma(DMA_Type *dma, uint32_t ch_num, UART_Type *uart, uint8_t *src, uint32_t size)
{
    rt_base_t align = 0;
    dma_handshake_config_t config;

    config.ch_index = ch_num;
    config.dst = (uint32_t)&uart->THR;
    config.dst_fixed = true;
    config.src = (uint32_t) src;
    config.src_fixed = false;
    config.size_in_byte = size;
    dma_setup_handshake(dma, &config);
}

#endif /* RT_SERIAL_USING_DMA */

static rt_err_t hpm_uart_control(struct rt_serial_device *serial, int cmd, void *arg)
{
    RT_ASSERT(serial != RT_NULL);

    rt_ubase_t ctrl_arg = (rt_ubase_t) arg;
    struct hpm_uart *uart = (struct hpm_uart *)serial->parent.user_data;

    if(ctrl_arg & (RT_DEVICE_FLAG_RX_BLOCKING | RT_DEVICE_FLAG_RX_NON_BLOCKING))
    {
#ifdef RT_SERIAL_USING_DMA
        if (uart->dma_flags & RT_DEVICE_FLAG_DMA_RX)
        {
            ctrl_arg = RT_DEVICE_FLAG_DMA_RX;
        }
        else
#endif
        {
            ctrl_arg = RT_DEVICE_FLAG_INT_RX;
        }
    }
    else if(ctrl_arg & (RT_DEVICE_FLAG_TX_BLOCKING | RT_DEVICE_FLAG_TX_NON_BLOCKING))
    {
#ifdef RT_SERIAL_USING_DMA
        if (uart->dma_flags & RT_DEVICE_FLAG_DMA_TX)
        {
            ctrl_arg = RT_DEVICE_FLAG_DMA_TX;
        }
        else
#endif
        {
            ctrl_arg = RT_DEVICE_FLAG_INT_TX;
        }
    }

    switch (cmd) {
        case RT_DEVICE_CTRL_CLR_INT:
            if (ctrl_arg == RT_DEVICE_FLAG_INT_RX) {
                /* disable rx irq */
                uart_disable_irq(uart->uart_base, uart_intr_rx_data_avail_or_timeout);
                intc_m_disable_irq(uart->irq_num);
            }
            else if (ctrl_arg == RT_DEVICE_FLAG_INT_TX) {
                /* disable tx irq */
                uart_disable_irq(uart->uart_base, uart_intr_tx_slot_avail);
                intc_m_disable_irq(uart->irq_num);
            }
#ifdef RT_SERIAL_USING_DMA
            else if (ctrl_arg == RT_DEVICE_FLAG_DMA_TX) {
                dma_manager_disable_channel_interrupt(&uart->tx_chn_ctx.resource, DMA_INTERRUPT_MASK_ALL);
                dma_abort_channel(uart->tx_chn_ctx.resource.base, uart->tx_chn_ctx.resource.channel);
            } else if (ctrl_arg == RT_DEVICE_FLAG_DMA_RX) {
                dma_manager_disable_channel_interrupt(&uart->rx_chn_ctx.resource, DMA_INTERRUPT_MASK_ALL);
                dma_abort_channel(uart->rx_chn_ctx.resource.base, uart->rx_chn_ctx.resource.channel);
            }
#endif
            break;

        case RT_DEVICE_CTRL_SET_INT:
            if (ctrl_arg == RT_DEVICE_FLAG_INT_RX) {
                /* enable rx irq */
                uart_enable_irq(uart->uart_base, uart_intr_rx_data_avail_or_timeout);
                intc_m_enable_irq_with_priority(uart->irq_num, 1);
            } else if (ctrl_arg == RT_DEVICE_FLAG_INT_TX) {
                /* enable tx irq */
                uart_enable_irq(uart->uart_base, uart_intr_tx_slot_avail);
                intc_m_enable_irq_with_priority(uart->irq_num, 1);
            }
            break;

        case RT_DEVICE_CTRL_CONFIG:
#ifdef RT_SERIAL_USING_DMA
            if (ctrl_arg & (RT_DEVICE_FLAG_DMA_RX | RT_DEVICE_FLAG_DMA_TX)) {
                hpm_uart_dma_config(serial, (void *)ctrl_arg);
            } else
#endif
            {
                hpm_uart_control(serial, RT_DEVICE_CTRL_SET_INT, (void *)ctrl_arg);
            }
            break;
        case RT_DEVICE_CHECK_OPTMODE:
#ifdef RT_SERIAL_USING_DMA
            if ((ctrl_arg & RT_DEVICE_FLAG_DMA_TX)) {
                return RT_SERIAL_TX_BLOCKING_NO_BUFFER;
            } else
#endif
            {
                return RT_SERIAL_TX_BLOCKING_BUFFER;
            }
    }

    return RT_EOK;
}


static int hpm_uart_putc(struct rt_serial_device *serial, char ch)
{
    struct hpm_uart *uart  = (struct hpm_uart *)serial->parent.user_data;
    uart_send_byte(uart->uart_base, ch);
    uart_flush(uart->uart_base);
    return ch;
}

static int hpm_uart_getc(struct rt_serial_device *serial)
{
    int result = -1;
    struct hpm_uart *uart  = (struct hpm_uart *)serial->parent.user_data;

    if (uart_check_status(uart->uart_base, uart_stat_data_ready)) {
        uart_receive_byte(uart->uart_base, (uint8_t*)&result);
    }

    return result;
}

static rt_size_t hpm_uart_transmit(struct rt_serial_device *serial,
                                    rt_uint8_t *buf,
                                    rt_size_t size,
                                    rt_uint32_t tx_flag)
{
    RT_ASSERT(serial != RT_NULL);
    RT_ASSERT(buf != RT_NULL);
    RT_ASSERT(size);

#ifdef RT_SERIAL_USING_DMA
    struct hpm_uart *uart  = (struct hpm_uart *)serial->parent.user_data;
    if (uart->dma_flags & RT_DEVICE_FLAG_DMA_TX) {
        hpm_uart_dma_register_channel(serial, true, uart_tx_done, RT_NULL, RT_NULL);
        intc_m_enable_irq(uart->tx_chn_ctx.resource.irq_num);
        hpm_uart_transmit_dma(uart->tx_chn_ctx.resource.base, uart->tx_chn_ctx.resource.channel, uart->uart_base, buf, size);
        return size;
    }
#endif
    hpm_uart_control(serial, RT_DEVICE_CTRL_CONFIG, (void *)tx_flag);
    return size;
}

static const struct rt_uart_ops hpm_uart_ops = {
    hpm_uart_configure,
    hpm_uart_control,
    hpm_uart_putc,
    hpm_uart_getc,
    hpm_uart_transmit,
};



static int hpm_uart_config(void)
{
    struct serial_configure config = RT_SERIAL_CONFIG_DEFAULT;
    hpm_stat_t status = status_success;

#ifdef BSP_USING_UART0
    uarts[HPM_UART0_INDEX].serial->config = config;
    uarts[HPM_UART0_INDEX].serial->config.rx_bufsz = BSP_UART0_RX_BUFSIZE;
    uarts[HPM_UART0_INDEX].serial->config.tx_bufsz = BSP_UART0_TX_BUFSIZE;
#ifdef RT_SERIAL_USING_DMA
    uarts[HPM_UART0_INDEX].dma_flags = 0;
#ifdef BSP_UART0_RX_USING_DMA
    status = hpm_uart_dma_rx_init(&uarts[HPM_UART0_INDEX]);
    if (status != status_success)
    {
        return -RT_ERROR;
    }
#endif //BSP_UART0_RX_USING_DMA
#ifdef BSP_UART0_TX_USING_DMA
    status = hpm_uart_dma_tx_init(&uarts[HPM_UART0_INDEX]);
    if (status != status_success)
    {
        return -RT_ERROR;
    }
#endif //BSP_UART0_TX_USING_DMA
#endif // RT_SERIAL_USING_DMA
#endif //BSP_USING_UART0

#ifdef BSP_USING_UART1
    uarts[HPM_UART1_INDEX].serial->config = config;
    uarts[HPM_UART1_INDEX].serial->config.rx_bufsz = BSP_UART1_RX_BUFSIZE;
    uarts[HPM_UART1_INDEX].serial->config.tx_bufsz = BSP_UART1_TX_BUFSIZE;
#ifdef RT_SERIAL_USING_DMA
    uarts[HPM_UART1_INDEX].dma_flags = 0;
#ifdef BSP_UART1_RX_USING_DMA
    status = hpm_uart_dma_rx_init(&uarts[HPM_UART1_INDEX]);
    if (status != status_success)
    {
        return -RT_ERROR;
    }
#endif //BSP_UART1_RX_USING_DMA
#ifdef BSP_UART1_TX_USING_DMA
    status = hpm_uart_dma_tx_init(&uarts[HPM_UART1_INDEX]);
    if (status != status_success)
    {
        return -RT_ERROR;
    }
#endif //BSP_UART1_TX_USING_DMA
#endif // RT_SERIAL_USING_DMA
#endif //BSP_USING_UART1

#ifdef BSP_USING_UART2
    uarts[HPM_UART2_INDEX].serial->config = config;
    uarts[HPM_UART2_INDEX].serial->config.rx_bufsz = BSP_UART2_RX_BUFSIZE;
    uarts[HPM_UART2_INDEX].serial->config.tx_bufsz = BSP_UART2_TX_BUFSIZE;
#ifdef RT_SERIAL_USING_DMA
    uarts[HPM_UART2_INDEX].dma_flags = 0;
#ifdef BSP_UART2_RX_USING_DMA
    status = hpm_uart_dma_rx_init(&uarts[HPM_UART2_INDEX]);
    if (status != status_success)
    {
        return -RT_ERROR;
    }
#endif //BSP_UART2_RX_USING_DMA
#ifdef BSP_UART2_TX_USING_DMA
    status = hpm_uart_dma_tx_init(&uarts[HPM_UART2_INDEX]);
    if (status != status_success)
    {
        return -RT_ERROR;
    }
#endif //BSP_UART2_TX_USING_DMA
#endif // RT_SERIAL_USING_DMA
#endif //BSP_USING_UART2

#ifdef BSP_USING_UART3
    uarts[HPM_UART3_INDEX].serial->config = config;
    uarts[HPM_UART3_INDEX].serial->config.rx_bufsz = BSP_UART3_RX_BUFSIZE;
    uarts[HPM_UART3_INDEX].serial->config.tx_bufsz = BSP_UART3_TX_BUFSIZE;
#ifdef RT_SERIAL_USING_DMA
    uarts[HPM_UART3_INDEX].dma_flags = 0;
#ifdef BSP_UART3_RX_USING_DMA
    status = hpm_uart_dma_rx_init(&uarts[HPM_UART3_INDEX]);
    if (status != status_success)
    {
        return -RT_ERROR;
    }
#endif //BSP_UART3_RX_USING_DMA
#ifdef BSP_UART3_TX_USING_DMA
    status = hpm_uart_dma_tx_init(&uarts[HPM_UART3_INDEX]);
    if (status != status_success)
    {
        return -RT_ERROR;
    }
#endif //BSP_UART3_TX_USING_DMA
#endif // RT_SERIAL_USING_DMA
#endif //BSP_USING_UART3

#ifdef BSP_USING_UART4
    uarts[HPM_UART4_INDEX].serial->config = config;
    uarts[HPM_UART4_INDEX].serial->config.rx_bufsz = BSP_UART4_RX_BUFSIZE;
    uarts[HPM_UART4_INDEX].serial->config.tx_bufsz = BSP_UART4_TX_BUFSIZE;
#ifdef RT_SERIAL_USING_DMA
    uarts[HPM_UART4_INDEX].dma_flags = 0;
#ifdef BSP_UART4_RX_USING_DMA
    status = hpm_uart_dma_rx_init(&uarts[HPM_UART4_INDEX]);
    if (status != status_success)
    {
        return -RT_ERROR;
    }
#endif //BSP_UART4_RX_USING_DMA
#ifdef BSP_UART4_TX_USING_DMA
    status = hpm_uart_dma_tx_init(&uarts[HPM_UART4_INDEX]);
    if (status != status_success)
    {
        return -RT_ERROR;
    }
#endif //BSP_UART4_TX_USING_DMA
#endif // RT_SERIAL_USING_DMA
#endif //BSP_USING_UART4

#ifdef BSP_USING_UART5
    uarts[HPM_UART5_INDEX].serial->config = config;
    uarts[HPM_UART5_INDEX].serial->config.rx_bufsz = BSP_UART5_RX_BUFSIZE;
    uarts[HPM_UART5_INDEX].serial->config.tx_bufsz = BSP_UART5_TX_BUFSIZE;
#ifdef RT_SERIAL_USING_DMA
    uarts[HPM_UART5_INDEX].dma_flags = 0;
#ifdef BSP_UART5_RX_USING_DMA
    status = hpm_uart_dma_rx_init(&uarts[HPM_UART5_INDEX]);
    if (status != status_success)
    {
        return -RT_ERROR;
    }
#endif //BSP_UART5_RX_USING_DMA
#ifdef BSP_UART5_TX_USING_DMA
    status = hpm_uart_dma_tx_init(&uarts[HPM_UART5_INDEX]);
    if (status != status_success)
    {
        return -RT_ERROR;
    }
#endif //BSP_UART5_TX_USING_DMA
#endif // RT_SERIAL_USING_DMA
#endif //BSP_USING_UART5

#ifdef BSP_USING_UART6
    uarts[HPM_UART6_INDEX].serial->config = config;
    uarts[HPM_UART6_INDEX].serial->config.rx_bufsz = BSP_UART6_RX_BUFSIZE;
    uarts[HPM_UART6_INDEX].serial->config.tx_bufsz = BSP_UART6_TX_BUFSIZE;
#ifdef RT_SERIAL_USING_DMA
    uarts[HPM_UART6_INDEX].dma_flags = 0;
#ifdef BSP_UART6_RX_USING_DMA
    status = hpm_uart_dma_rx_init(&uarts[HPM_UART6_INDEX]);
    if (status != status_success)
    {
        return -RT_ERROR;
    }
#endif //BSP_UART6_RX_USING_DMA
#ifdef BSP_UART6_TX_USING_DMA
    status = hpm_uart_dma_tx_init(&uarts[HPM_UART6_INDEX]);
    if (status != status_success)
    {
        return -RT_ERROR;
    }
#endif //BSP_UART6_TX_USING_DMA
#endif // RT_SERIAL_USING_DMA
#endif //BSP_USING_UART6

#ifdef BSP_USING_UART7
    uarts[HPM_UART7_INDEX].serial->config = config;
    uarts[HPM_UART7_INDEX].serial->config.rx_bufsz = BSP_UART7_RX_BUFSIZE;
    uarts[HPM_UART7_INDEX].serial->config.tx_bufsz = BSP_UART7_TX_BUFSIZE;
#ifdef RT_SERIAL_USING_DMA
    uarts[HPM_UART7_INDEX].dma_flags = 0;
#ifdef BSP_UART7_RX_USING_DMA
    status = hpm_uart_dma_rx_init(&uarts[HPM_UART7_INDEX]);
    if (status != status_success)
    {
        return -RT_ERROR;
    }
#endif //BSP_UART7_RX_USING_DMA
#ifdef BSP_UART0_TX_USING_DMA
    status = hpm_uart_dma_tx_init(&uarts[HPM_UART7_INDEX]);
    if (status != status_success)
    {
        return -RT_ERROR;
    }
#endif //BSP_UART7_TX_USING_DMA
#endif // RT_SERIAL_USING_DMA
#endif //BSP_USING_UART7

#ifdef BSP_USING_UART8
    uarts[HPM_UART8_INDEX].serial->config = config;
    uarts[HPM_UART8_INDEX].serial->config.rx_bufsz = BSP_UART8_RX_BUFSIZE;
    uarts[HPM_UART8_INDEX].serial->config.tx_bufsz = BSP_UART8_TX_BUFSIZE;
#ifdef RT_SERIAL_USING_DMA
    uarts[HPM_UART8_INDEX].dma_flags = 0;
#ifdef BSP_UART8_RX_USING_DMA
    status = hpm_uart_dma_rx_init(&uarts[HPM_UART8_INDEX]);
    if (status != status_success)
    {
        return -RT_ERROR;
    }
#endif //BSP_UART8_RX_USING_DMA
#ifdef BSP_UART0_TX_USING_DMA
    status = hpm_uart_dma_tx_init(&uarts[HPM_UART8_INDEX]);
    if (status != status_success)
    {
        return -RT_ERROR;
    }
#endif //BSP_UART8_TX_USING_DMA
#endif // RT_SERIAL_USING_DMA
#endif //BSP_USING_UART8

#ifdef BSP_USING_UART9
    uarts[HPM_UART9_INDEX].serial->config = config;
    uarts[HPM_UART9_INDEX].serial->config.rx_bufsz = BSP_UART9_RX_BUFSIZE;
    uarts[HPM_UART9_INDEX].serial->config.tx_bufsz = BSP_UART9_TX_BUFSIZE;
#ifdef RT_SERIAL_USING_DMA
    uarts[HPM_UART9_INDEX].dma_flags = 0;
#ifdef BSP_UART9_RX_USING_DMA
    status = hpm_uart_dma_rx_init(&uarts[HPM_UART9_INDEX]);
    if (status != status_success)
    {
        return -RT_ERROR;
    }
#endif //BSP_UART9_RX_USING_DMA
#ifdef BSP_UART9_TX_USING_DMA
    status = hpm_uart_dma_tx_init(&uarts[HPM_UART9_INDEX]);
    if (status != status_success)
    {
        return -RT_ERROR;
    }
#endif //BSP_UART9_TX_USING_DMA
#endif // RT_SERIAL_USING_DMA
#endif //BSP_USING_UART9

#ifdef BSP_USING_UART10
    uarts[HPM_UART10_INDEX].serial->config = config;
    uarts[HPM_UART10_INDEX].serial->config.rx_bufsz = BSP_UART10_RX_BUFSIZE;
    uarts[HPM_UART10_INDEX].serial->config.tx_bufsz = BSP_UART10_TX_BUFSIZE;
#ifdef RT_SERIAL_USING_DMA
    uarts[HPM_UART10_INDEX].dma_flags = 0;
#ifdef BSP_UART10_RX_USING_DMA
    status = hpm_uart_dma_rx_init(&uarts[HPM_UART10_INDEX]);
    if (status != status_success)
    {
        return -RT_ERROR;
    }
#endif //BSP_UART10_RX_USING_DMA
#ifdef BSP_UART10_TX_USING_DMA
    status = hpm_uart_dma_tx_init(&uarts[HPM_UART10_INDEX]);
    if (status != status_success)
    {
        return -RT_ERROR;
    }
#endif //BSP_UART10_TX_USING_DMA
#endif // RT_SERIAL_USING_DMA
#endif //BSP_USING_UART10

#ifdef BSP_USING_UART11
    uarts[HPM_UART11_INDEX].serial->config = config;
    uarts[HPM_UART11_INDEX].serial->config.rx_bufsz = BSP_UART11_RX_BUFSIZE;
    uarts[HPM_UART11_INDEX].serial->config.tx_bufsz = BSP_UART11_TX_BUFSIZE;
#ifdef RT_SERIAL_USING_DMA
    uarts[HPM_UART11_INDEX].dma_flags = 0;
#ifdef BSP_UART11_RX_USING_DMA
    status = hpm_uart_dma_rx_init(&uarts[HPM_UART11_INDEX]);
    if (status != status_success)
    {
        return -RT_ERROR;
    }
#endif //BSP_UART11_RX_USING_DMA
#ifdef BSP_UART11_TX_USING_DMA
    status = hpm_uart_dma_tx_init(&uarts[HPM_UART11_INDEX]);
    if (status != status_success)
    {
        return -RT_ERROR;
    }
#endif //BSP_UART11_TX_USING_DMA
#endif // RT_SERIAL_USING_DMA
#endif //BSP_USING_UART11

#ifdef BSP_USING_UART12
    uarts[HPM_UART12_INDEX].serial->config = config;
    uarts[HPM_UART12_INDEX].serial->config.rx_bufsz = BSP_UART12_RX_BUFSIZE;
    uarts[HPM_UART12_INDEX].serial->config.tx_bufsz = BSP_UART12_TX_BUFSIZE;
#ifdef RT_SERIAL_USING_DMA
    uarts[HPM_UART12_INDEX].dma_flags = 0;
#ifdef BSP_UART12_RX_USING_DMA
    status = hpm_uart_dma_rx_init(&uarts[HPM_UART12_INDEX]);
    if (status != status_success)
    {
        return -RT_ERROR;
    }
#endif //BSP_UART12_RX_USING_DMA
#ifdef BSP_UART12_TX_USING_DMA
    status = hpm_uart_dma_tx_init(&uarts[HPM_UART12_INDEX]);
    if (status != status_success)
    {
        return -RT_ERROR;
    }
#endif //BSP_UART12_TX_USING_DMA
#endif // RT_SERIAL_USING_DMA
#endif //BSP_USING_UART12

#ifdef BSP_USING_UART13
    uarts[HPM_UART13_INDEX].serial->config = config;
    uarts[HPM_UART13_INDEX].serial->config.rx_bufsz = BSP_UART13_RX_BUFSIZE;
    uarts[HPM_UART13_INDEX].serial->config.tx_bufsz = BSP_UART13_TX_BUFSIZE;
#ifdef RT_SERIAL_USING_DMA
    uarts[HPM_UART13_INDEX].dma_flags = 0;
#ifdef BSP_UART13_RX_USING_DMA
    status = hpm_uart_dma_rx_init(&uarts[HPM_UART13_INDEX]);
    if (status != status_success)
    {
        return -RT_ERROR;
    }
#endif //BSP_UART13_RX_USING_DMA
#ifdef BSP_UART13_TX_USING_DMA
    status = hpm_uart_dma_tx_init(&uarts[HPM_UART13_INDEX]);
    if (status != status_success)
    {
        return -RT_ERROR;
    }
#endif //BSP_UART13_TX_USING_DMA
#endif // RT_SERIAL_USING_DMA
#endif //BSP_USING_UART13

#ifdef BSP_USING_UART14
    uarts[HPM_UART14_INDEX].serial->config = config;
    uarts[HPM_UART14_INDEX].serial->config.rx_bufsz = BSP_UART14_RX_BUFSIZE;
    uarts[HPM_UART14_INDEX].serial->config.tx_bufsz = BSP_UART14_TX_BUFSIZE;
#ifdef RT_SERIAL_USING_DMA
    uarts[HPM_UART14_INDEX].dma_flags = 0;
#ifdef BSP_UART14_RX_USING_DMA
    status = hpm_uart_dma_rx_init(&uarts[HPM_UART14_INDEX]);
    if (status != status_success)
    {
        return -RT_ERROR;
    }
#endif //BSP_UART14_RX_USING_DMA
#ifdef BSP_UART14_TX_USING_DMA
    status = hpm_uart_dma_tx_init(&uarts[HPM_UART14_INDEX]);
    if (status != status_success)
    {
        return -RT_ERROR;
    }
#endif //BSP_UART14_TX_USING_DMA
#endif // RT_SERIAL_USING_DMA
#endif //BSP_USING_UART14

#ifdef BSP_USING_UART15
    uarts[HPM_UART15_INDEX].serial->config = config;
    uarts[HPM_UART15_INDEX].serial->config.rx_bufsz = BSP_UART15_RX_BUFSIZE;
    uarts[HPM_UART15_INDEX].serial->config.tx_bufsz = BSP_UART15_TX_BUFSIZE;
#ifdef RT_SERIAL_USING_DMA
    uarts[HPM_UART15_INDEX].dma_flags = 0;
#ifdef BSP_UART15_RX_USING_DMA
    status = hpm_uart_dma_rx_init(&uarts[HPM_UART15_INDEX]);
    if (status != status_success)
    {
        return -RT_ERROR;
    }
#endif //BSP_UART15_RX_USING_DMA
#ifdef BSP_UART15_TX_USING_DMA
    status = hpm_uart_dma_tx_init(&uarts[HPM_UART15_INDEX]);
    if (status != status_success)
    {
        return -RT_ERROR;
    }
#endif //BSP_UART15_TX_USING_DMA
#endif // RT_SERIAL_USING_DMA
#endif //BSP_USING_UART15


    return RT_EOK;
}

int rt_hw_uart_init(void)
{
    /* Added bypass logic here since the rt_hw_uart_init function will be initialized twice, the 2nd initialization should be bypassed */
    static bool initialized;
    rt_err_t err = RT_EOK;
    if (initialized)
    {
        return err;
    }
    else
    {
        initialized = true;
    }

    if (RT_EOK != hpm_uart_config()) {
        return RT_ERROR;
    }

    for (uint32_t i = 0; i < sizeof(uarts) / sizeof(uarts[0]); i++) {
        uarts[i].serial->ops = &hpm_uart_ops;

        /* register UART device */
        err = rt_hw_serial_register(uarts[i].serial,
                            uarts[i].device_name,
                            RT_DEVICE_FLAG_RDWR,
                            (void*)&uarts[i]);
    }

    return err;
}

INIT_BOARD_EXPORT(rt_hw_uart_init);

#endif /* RT_USING_SERIAL */
