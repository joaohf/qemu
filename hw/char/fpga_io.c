/*
 * fpga_io.c
 *
 *  Created on: 12/10/2014
 *      Author: joaohf
 */

#include "hw/hw.h"
#include "hw/pci/pci.h"
#include "qemu/event_notifier.h"
#include "qemu/osdep.h"

typedef struct PCIFpgaIODevState {
    PCIDevice parent_obj;

    int pos;
    char *buf;
    int buflen;

    MemoryRegion mmio;
    MemoryRegion portio;
} PCIFpgaIODevState;

#define TYPE_PCI_FPGAIO_DEV "pci-fpga-io"

#define PCI_FPGAIO_DEV(obj) \
    OBJECT_CHECK(PCIFpgaIODevState, (obj), TYPE_PCI_FPGAIO_DEV)

static uint64_t
pci_fpgaiodev_read(void *opaque, hwaddr addr, unsigned size)
{
    PCIFpgaIODevState *d = opaque;

    if (addr == 0)
        return d->buf[d->pos ++];
    else
        return d->buflen;
}

static void
pci_fpgaiodev_mmio_write(void *opaque, hwaddr addr, uint64_t val,
                       unsigned size)
{

   PCIFpgaIODevState *d = opaque;

   switch(addr) {
       case 0:
           /* write byte */
       if (!d->buf)
               break;
       if (d->pos >= d->buflen)
           break;
       d->buf[d->pos ++] = (uint8_t)val;
       break;
       case 1:
           /* reset pos */
           d->pos = 0;
       break;
       case 2:
           /* set buffer length */
       d->buflen = val + 1;
       g_free(d->buf);
       d->buf = g_malloc(d->buflen);
       break;
   }

   return;
}

static const MemoryRegionOps pci_fpgaiodev_mmio_ops = {
    .read = pci_fpgaiodev_read,
    .write = pci_fpgaiodev_mmio_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
    .impl = {
        .min_access_size = 1,
        .max_access_size = 1,
    },
};

static int pci_fpgaiodev_init(PCIDevice *pci_dev)
{
    PCIFpgaIODevState *d = PCI_FPGAIO_DEV(pci_dev);
    uint8_t *pci_conf;

    pci_conf = pci_dev->config;

    pci_conf[PCI_INTERRUPT_PIN] = 0; /* no interrupt pin */

    memory_region_init_io(&d->mmio, OBJECT(d), &pci_fpgaiodev_mmio_ops, d,
                          "pci-fpgaiodev-mmio", 128);
    pci_register_bar(pci_dev, 0, PCI_BASE_ADDRESS_SPACE_MEMORY, &d->mmio);

    d->pos = 0;
    d->buf = g_malloc(14);
    memcpy(d->buf, "Hello, world!\n", 14);
    d->buflen = 14;
    printf("Loaded fpga-io pci\n");

    return 0;
}

static void
pci_fpgaiodev_uninit(PCIDevice *dev)
{
    //PCIFpgaIODevState *d = PCI_FPGAIO_DEV(dev);
    printf("unloaded fpga-io pci\n");
}

static void qdev_pci_fpgaiodev_reset(DeviceState *dev)
{
    //PCIFpgaIODevState *d = PCI_FPGAIO_DEV(dev);
}

static void pci_fpgaiodev_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    PCIDeviceClass *k = PCI_DEVICE_CLASS(klass);

    k->init = pci_fpgaiodev_init;
    k->exit = pci_fpgaiodev_uninit;
    k->vendor_id = 0x1172;
    k->device_id = 0x0004;
    k->revision = 0x00;
    k->class_id = PCI_CLASS_OTHERS;
    dc->desc = "Padtec FPGA-IO PCI";
    set_bit(DEVICE_CATEGORY_MISC, dc->categories);
    dc->reset = qdev_pci_fpgaiodev_reset;
}

static const TypeInfo pci_fpgaio_info = {
    .name          = TYPE_PCI_FPGAIO_DEV,
    .parent        = TYPE_PCI_DEVICE,
    .instance_size = sizeof(PCIFpgaIODevState),
    .class_init    = pci_fpgaiodev_class_init,
};

static void pci_lev_register_types(void)
{
    type_register_static(&pci_fpgaio_info);
}

type_init(pci_lev_register_types)
