#ifndef __DT_BINDINGS_DMA_X1600_DMA_H__
#define __DT_BINDINGS_DMA_X1600_DMA_H__

/*
 * Request type numbers for the X1600 DMA controller (written to the DRTn
 * register for the channel).
 */
#define X1600_DMA_AUTO		0x8
#define X1600_DMA_CAN0_TX	0xa
#define X1600_DMA_CAN0_RX	0xb
#define X1600_DMA_CAN1_TX	0xc
#define X1600_DMA_CAN1_RX	0xd
#define X1600_DMA_UART3_TX	0xe
#define X1600_DMA_UART3_RX	0xf
#define X1600_DMA_UART2_TX	0x10
#define X1600_DMA_UART2_RX	0x11
#define X1600_DMA_UART1_TX	0x12
#define X1600_DMA_UART1_RX	0x13
#define X1600_DMA_UART0_TX	0x14
#define X1600_DMA_UART0_RX	0x15
#define X1600_DMA_SSI0_TX	0x16
#define X1600_DMA_SSI0_RX	0x17
#define X1600_DMA_SMB0_TX	0x24
#define X1600_DMA_SMB0_RX	0x25
#define X1600_DMA_SMB1_TX	0x26
#define X1600_DMA_SMB1_RX	0x27
#define X1600_DMA_SSI_SLV_TX	0x2a
#define X1600_DMA_SSI_SLV_RX	0x2b
#define X1600_DMA_PWM0_TX	0x2c
#define X1600_DMA_PWM1_TX	0x2d
#define X1600_DMA_PWM2_TX	0x2e
#define X1600_DMA_PWM3_TX	0x2f
#define X1600_DMA_PWM4_TX	0x30
#define X1600_DMA_PWM5_TX	0x31
#define X1600_DMA_PWM6_TX	0x32
#define X1600_DMA_PWM7_TX	0x33
#define X1600_DMA_MSC0_TX	0x34
#define X1600_DMA_MSC0_RX	0x35
#define X1600_DMA_MSC1_TX	0x36
#define X1600_DMA_MSC1_RX	0x37
#define X1600_DMA_SADC_RX	0x38
#define X1600_DMA_AIC_LOOP	0x3d
#define X1600_DMA_AIC_TX	0x3e
#define X1600_DMA_AIC_RX	0x3e

#endif /* __DT_BINDINGS_DMA_X1600_DMA_H__ */
