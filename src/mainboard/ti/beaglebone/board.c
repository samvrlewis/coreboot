#include <symbols.h>
#include <bootmem.h>

void bootmem_platform_add_ranges(void)
{
    // todo: Work out the size properly
    // It shouldn't overwrite the cbmem tables
    bootmem_add_range((uintptr_t)_eprogram, 0x19000000, BM_MEM_RAM);
	//bootmem_add_range((uintptr_t)_eprogram, (CONFIG_DRAM_SIZE_MB - 10) * MiB , BM_MEM_RAM);
}