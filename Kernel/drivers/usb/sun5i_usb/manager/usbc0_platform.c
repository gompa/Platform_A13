/*
*************************************************************************************
*                         			      Linux
*					           USB Host Controller Driver
*
*				        (c) Copyright 2006-2012, SoftWinners Co,Ld.
*							       All Rights Reserved
*
* File Name 	: usbc0_platform.c
*
* Author 		: javen
*
* Description 	: USB������0�豸��Ϣ
*
* History 		:
*      <author>    		<time>       	<version >    		<desc>
*       javen     	  2011-4-14            1.0          create this file
*
*************************************************************************************
*/
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/ioport.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/timer.h>
#include <linux/list.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/clk.h>

#include <linux/debugfs.h>
#include <linux/seq_file.h>

#include <asm/byteorder.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/system.h>
#include <asm/unaligned.h>
#include <mach/irqs.h>

#include  "../include/sw_usb_config.h"

//---------------------------------------------------------------
//  device ��Ϣ����
//---------------------------------------------------------------
static struct sw_udc_mach_info sw_udc_cfg;

//static u64 sw_udc_dmamask = 0xffffffffUL;

static struct platform_device sw_udc_device = {
	.name				= "sw_usb_udc",
	.id					= -1,

	.dev = {
//		.dma_mask			= &sw_udc_dmamask,
//		.coherent_dma_mask	= DMA_BIT_MASK(32),
		.platform_data		= &sw_udc_cfg,
	},
};

//---------------------------------------------------------------
//  host ��Ϣ����
//---------------------------------------------------------------
static struct sw_hcd_eps_bits sw_hcd_eps[] = {
	{ "ep1_tx", 8, },
	{ "ep1_rx", 8, },
	{ "ep2_tx", 8, },
	{ "ep2_rx", 8, },
	{ "ep3_tx", 8, },
	{ "ep3_rx", 8, },
	{ "ep4_tx", 8, },
	{ "ep4_rx", 8, },
	{ "ep5_tx", 8, },
	{ "ep5_rx", 8, },
};

static struct sw_hcd_config sw_hcd_config = {
	.multipoint		= 1,
	.dyn_fifo		= 1,
	.soft_con		= 1,
	.dma			= 0,

	.num_eps		= USBC_MAX_EP_NUM,
	.dma_channels	= 0,
	.ram_size		= USBC0_MAX_FIFO_SIZE,
	.eps_bits	    = sw_hcd_eps,
};

static struct sw_hcd_platform_data sw_hcd_plat = {
	.mode		= SW_HCD_HOST,
	.config		= &sw_hcd_config,
};

//static u64 sw_hcd_dmamask = DMA_BIT_MASK(32);

static struct platform_device sw_hcd_device = {
	.name				= "sw_hcd_host0",
	.id					= -1,

	.dev = {
//		.dma_mask			= &sw_hcd_dmamask,
//		.coherent_dma_mask	= DMA_BIT_MASK(32),
		.platform_data		= &sw_hcd_plat,
	},
};


static int (*get_usb_connect_proc)(void) = NULL;
 
extern void register_usb_connect_proc(int (*proc)(void))
{
    get_usb_connect_proc = proc;
}
EXPORT_SYMBOL(register_usb_connect_proc);
 
static ssize_t show_is_connected(struct device *dev, struct device_attribute *attr, char *buf)
{
    int r = (get_usb_connect_proc == NULL) ? 0 : (get_usb_connect_proc)();
    //printk("Showing connected status %d\n", r);
    return scnprintf(buf, PAGE_SIZE, "%d\n", r);
}

static DEVICE_ATTR(is_connected, S_IROTH, show_is_connected, NULL);

/*
*******************************************************************************
*                     usbc0_platform_device_init
*
* Description:
*    void
*
* Parameters:
*    void
*
* Return value:
*    void
*
* note:
*    void
*
*******************************************************************************
*/
__s32 usbc0_platform_device_init(struct usb_port_info *port_info)
{
    /* device */
    sw_udc_cfg.port_info = port_info;
    sw_udc_cfg.usbc_base = SW_VA_USB0_IO_BASE;

    /* host */
    sw_hcd_config.port_info = port_info;

    switch(port_info->port_type){
        case USB_PORT_TYPE_DEVICE:
            platform_device_register(&sw_udc_device);
            device_create_file(&sw_udc_device.dev, &dev_attr_is_connected);
        break;

        case USB_PORT_TYPE_HOST:
            platform_device_register(&sw_hcd_device);
        break;

        case USB_PORT_TYPE_OTG:
            platform_device_register(&sw_udc_device);
            platform_device_register(&sw_hcd_device);
            device_create_file(&sw_udc_device.dev, &dev_attr_is_connected);
        break;

        default:
            DMSG_PANIC("ERR: unkown port_type(%d)\n", port_info->port_type);
    }

    return 0;
}

/*
*******************************************************************************
*                     usbc0_platform_device_exit
*
* Description:
*    void
*
* Parameters:
*    void
*
* Return value:
*    void
*
* note:
*    void
*
*******************************************************************************
*/
__s32 usbc0_platform_device_exit(struct usb_port_info *info)
{
    switch(info->port_type){
        case USB_PORT_TYPE_DEVICE:
            device_remove_file(&sw_udc_device.dev, &dev_attr_is_connected);
            platform_device_unregister(&sw_udc_device);
        break;

        case USB_PORT_TYPE_HOST:
            device_remove_file(&sw_udc_device.dev, &dev_attr_is_connected);
            platform_device_unregister(&sw_hcd_device);
        break;

        case USB_PORT_TYPE_OTG:
            device_remove_file(&sw_udc_device.dev, &dev_attr_is_connected);
            platform_device_unregister(&sw_udc_device);
            platform_device_unregister(&sw_hcd_device);
        break;

        default:
            DMSG_PANIC("ERR: unkown port_type(%d)\n", info->port_type);
    }

    return 0;
}

