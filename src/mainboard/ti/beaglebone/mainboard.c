#include <symbols.h>
#include <bootmem.h>
#include <console/console.h>
#include <device/device.h>

static void mainboard_enable(struct device *dev)
{
	printk(BIOS_DEBUG, "MAINBOARD ENABLE\n");
	ram_resource(dev, 0, (uintptr_t)_dram / KiB, CONFIG_DRAM_SIZE_MB * MiB/KiB);
}

struct chip_operations mainboard_ops = {
	.enable_dev = mainboard_enable,
};
