/*
 *
 * Serval1 UIO driver.
 *
 * Copyright (C) 2012 Vitesse Semiconductor Inc.
 * Author: Lars Povlsen (lpovlsen@vitesse.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 * VITESSE SEMICONDUCTOR INC SHALL HAVE NO LIABILITY WHATSOEVER OF ANY
 * KIND ARISING OUT OF OR RELATED TO THE PROGRAM OR THE OPEN SOURCE
 * MATERIALS UNDER ANY THEORY OF LIABILITY.
 *
 */

#include <linux/device.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/slab.h>
#include <linux/uio_driver.h>

#define PCIE_VENDOR_ID 0x101B
#define PCIE_DEVICE_ID 0xB002

static int __devinit srvl1_pci_probe(struct pci_dev *dev, const struct pci_device_id *id)
{
    struct uio_info *info;

    info = kzalloc(sizeof(struct uio_info), GFP_KERNEL);
    if (!info)
        return -ENOMEM;

    if (pci_enable_device(dev))
        goto out_free;

    if (pci_request_regions(dev, "srvl1"))
        goto out_disable;

    /* BAR0 = registers, BAR1 = CONFIG, BAR2 = DDR (unused) */
    info->mem[0].addr = pci_resource_start(dev, 0);
    if (!info->mem[0].addr)
        goto out_release;
    info->mem[0].size = pci_resource_len(dev, 0);
    info->mem[0].memtype = UIO_MEM_PHYS;

#if 1
    info->mem[1].addr = pci_resource_start(dev, 1);
    if (!info->mem[1].addr)
        goto out_release;
    info->mem[1].size = pci_resource_len(dev, 1);
    info->mem[1].memtype = UIO_MEM_PHYS;
#endif

    info->irq = -1;             /* For now */
    info->name = "Serval1 RefBoard";
    info->version = "1.0.0";

    if (uio_register_device(&dev->dev, info))
        goto out_unmap;

    pci_set_drvdata(dev, info);
    dev_info(&dev->dev, "Found %s, UIO device.\n", info->name);

    return 0;

out_unmap:
    //iounmap(info->mem[0].internal_addr);
out_release:
    pci_release_regions(dev);
out_disable:
    pci_disable_device(dev);
out_free:
    kfree(info);
    return -ENODEV;
}


static void srvl1_pci_remove(struct pci_dev *dev)
{
    struct uio_info *info = pci_get_drvdata(dev);

    uio_unregister_device(info);
    pci_release_regions(dev);
    pci_disable_device(dev);
    pci_set_drvdata(dev, NULL);

    kfree(info);
}


static struct pci_device_id srvl1_pci_ids[] = {
    {
        .vendor =	PCIE_VENDOR_ID,
        .device =	PCIE_DEVICE_ID,
        .subvendor =	PCI_ANY_ID,
        .subdevice =	PCI_ANY_ID,
    },
    { 0, }
};

static struct pci_driver srvl1_pci_driver = {
    .name = "srvl1",
    .id_table = srvl1_pci_ids,
    .probe = srvl1_pci_probe,
    .remove = srvl1_pci_remove,
};

static int __init srvl1_init_module(void)
{
    return pci_register_driver(&srvl1_pci_driver);
}

static void __exit srvl1_exit_module(void)
{
    pci_unregister_driver(&srvl1_pci_driver);
}

module_init(srvl1_init_module);
module_exit(srvl1_exit_module);

MODULE_DEVICE_TABLE(pci, srvl1_pci_ids);
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Lars Povlsen <lpovlsen@vitesse.com>");
