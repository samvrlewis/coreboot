#include <console/console.h>
#include <device/device.h>
#include <device/pci.h>
#include <device/pci_ids.h>
#include "i82801er.h"

void i82801er_enable(device_t dev)
{
	unsigned int index = 0;
	uint8_t bHasDisableBit = 0;
	uint16_t cur_disable_mask, new_disable_mask;

//	all 82801er devices are in bus 0
	unsigned int devfn = PCI_DEVFN(0x1f, 0); // lpc
	device_t lpc_dev = dev_find_slot(0, devfn); // 0
	if (!lpc_dev)
		return;

	// Calculate disable bit position for specified device:function
	// NOTE: For ICH-5, only the following devices can be disabled:
	//		 D31: F0, F1, F2, F3, F5, F6, 
	//		 D29: F0, F1, F2, F3, F7

    if (PCI_SLOT(dev->path.u.pci.devfn) == 31) {
    	index = PCI_FUNC(dev->path.u.pci.devfn);

		switch (index) {
			case 0:
			case 1:
			case 2:
			case 3:
			case 5:
			case 6:
				bHasDisableBit = 1;
				break;
			
			default:
				break;
		};
		
		if (index == 0)
			index = 14;		// D31:F0 bit is an exception

    } else if (PCI_SLOT(dev->path.u.pci.devfn) == 29) {
    	index = 8 + PCI_FUNC(dev->path.u.pci.devfn);

		if ((PCI_FUNC(dev->path.u.pci.devfn) < 4) || (PCI_FUNC(dev->path.u.pci.devfn) == 7))
			bHasDisableBit = 1;
    }

	if (bHasDisableBit) {
		cur_disable_mask = pci_read_config16(lpc_dev, FUNC_DIS);
		new_disable_mask = cur_disable_mask & ~(1<<index); 		// enable it
		if (!dev->enabled) {
			new_disable_mask |= (1<<index);  // disable it
		}
		if (new_disable_mask != cur_disable_mask) {
			pci_write_config16(lpc_dev, FUNC_DIS, new_disable_mask);
		}
	}
}

struct chip_operations southbridge_intel_i82801er_ops = {
	CHIP_NAME("Intel 82801er Southbridge")
	.enable_dev = i82801er_enable,
};
