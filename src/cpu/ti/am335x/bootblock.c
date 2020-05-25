/* SPDX-License-Identifier: GPL-2.0-only */
/* This file is part of the coreboot project. */

#include <types.h>

#include <arch/cache.h>
#include <bootblock_common.h>
#include <symbols.h>
enum {
	// device type (asynchronous / "posted" writes)
	type_device	= 1 << 2 | 0 << 12 | 0 << 14,

	// device type with synchronous ("non-posted") writes
	type_sync	= 0 << 2 | 0 << 12 | 0 << 14,
	// ARM calls this "strongly ordered", although in ARMv8 they changed
	// terminology to "Device-nGnRnE" ... not sure that's an improvement.
};

// normal memory type with specified cache policy
enum CachePolicy {
	nc = 0,  // non-cacheable
	wt = 2,  // write-through, read-allocate, no write-allocate
	wb = 3,  // write-back,    read-allocate, no write-allocate
	wa = 1,  // write-back,    read-allocate, write-allocate
};
static inline u32 type_normal( enum CachePolicy l1, enum CachePolicy l2 )
{
	bool shared = false;
	return l1 << 2 | l2 << 12 | 1 << 14 | shared << 16;
}

enum Perms {
	sect_access	= 1 << 10,
	sect_user	= 1 << 11,
	sect_readonly	= 1 << 15,
	sect_noexec	= 1 <<  4,

	// section permissions:
	______	= 0,
	rwx___	= sect_access,
	rwxrwx	= sect_access | sect_user,

	r_x___	= rwx___ | sect_readonly,
	r_xr_x	= rwxrwx | sect_readonly,

	r_____	= r_x___ | sect_noexec,
	rw____	= rwx___ | sect_noexec,
	r__r__	= r_xr_x | sect_noexec,
	rw_rw_	= rwxrwx | sect_noexec,

	// these are only valid if not using simplified permissions model:
	rwxr_x	= sect_user,
	rw_r__	= rwxr_x | sect_noexec,
};

enum {
	sect_fault	= 0,  // translation fault
	sect_pgtbl	= 1,  // translation via page table
	sect_map	= 2,  // section translation

	// section translation descriptor flags
	sect_super	= 1 << 18,  // supersection (16 MB)
	sect_proc	= 1 << 17,  // process-specific (non-global)
};

static uint32_t *section_table = (uint32_t *)_ttb;

// create 1 MB flat mapping
static void map_section( uint32_t desc ) {
	desc |= sect_map;
	section_table[ desc >> 20 ] = desc;
}

// create 16 MB flat mapping
static void map_supersection( uint32_t desc ) {
	desc |= sect_map | sect_super;
	for( uint32_t i = 0; i < 16; i++ )
		section_table[ ( desc >> 20 ) + i ] = desc;
}

// get L2 cache policy (assumes the address is mapped as normal memory)
static enum CachePolicy get_L2_policy( uint32_t vaddr ) {
	return (enum CachePolicy)( section_table[ vaddr >> 20 ] >> 12 & 3 );
}

static void init_mmu(void)
{
	// peripherals
	for( u32 addr = 0x44000000; addr != 0x57000000; addr += 0x01000000 )
		map_supersection( addr | rw_rw_ | type_device );

	// external RAM
	for( u32 addr = 0x80000000; addr != 0xC0000000; addr += 0x01000000 )
		map_supersection( addr | rwxrwx | type_normal( wb, wa ) );

	// local memories
	map_section( 0x40000000 | r_xr_x | type_normal( wt, nc ) ); // mpuss rom
	map_section( 0x40200000 | rwxrwx | type_normal( wt, wa ) ); // mpuss ram
	map_section( 0x40300000 | rwxrwx | type_normal( wt, wa ) ); // ocmc ram

	asm( "dsb" ::: "memory" );

	// configure the section table address and cache policy into the MMU
	u32 tr_base = (u32) section_table;
	tr_base |= get_L2_policy( tr_base ) << 3;
	asm( "mcr\tp15, 0, %0, c2, c0, 0" :: "r" (tr_base) );

	// configure domain access
	asm( "mcr\tp15, 0, %0, c3, c0, 0" :: "r" (1) );

	uint32_t reg = (1<<0) | (1<<2) || (1<<12);
	asm( "mcr\tp15, 0, %0, c1, c0, 0" :: "r"(reg) );

}



void bootblock_soc_init(void)
{
	// uint32_t sctlr;

	init_mmu();
	// mmu_init();

	// /* start with mapping everything as strongly ordered. */
	// mmu_config_range(0, 4096, DCACHE_OFF);
	// mmu_config_range_kb(0x402F0400/1024,
	//  		    (0x402F1400-0x402F0400)/1024, DCACHE_WRITETHROUGH);
	// dcache_mmu_enable();

	// /* enable dcache */
	// sctlr = read_sctlr();
	// sctlr |= SCTLR_C;
	// write_sctlr(sctlr);
}
