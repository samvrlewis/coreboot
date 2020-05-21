


ucontext_t in qemu-linaro



fatload mmc 0 0x80800000 demo.img
go 0x80800000


# What do I know so far?

When the BBB boots, there's an initial bootloader in ROM. This has a few different ways of executing the new bootloader which is typically the SPL. I would like to replace this with coreboot.

The method of going from the initial bootloader to coreboot is to have the coreboot image in a FAT partition with the file name `MLO`.

AM335X technical data reference manual: http://www.ti.com/lit/ug/spruh73q/spruh73q.pdf?&ts=1589011010227

The image needs a table of contents header which is in 26.1.11 of the TRM.

## CHSETTINGS

These have to be in the image format but the datasheet only recommends setting these a certain way. I'm unclear what it's actually for.

```
sam@sambuntu:~$ hexdump -C ~/Downloads/MLO-am335x_evm-v2019.04-r11 | head
00000000  40 00 00 00 0c 00 00 00  00 00 00 00 00 00 00 00  |@...............|
00000010  00 00 00 00 43 48 53 45  54 54 49 4e 47 53 00 00  |....CHSETTINGS..|
00000020  ff ff ff ff ff ff ff ff  ff ff ff ff ff ff ff ff  |................|
*
00000040  c1 c0 c0 c0 00 01 00 00  00 00 00 00 00 00 00 00  |................|
00000050  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  |................|
*
00000200  94 67 01 00 00 04 2f 40  0f 00 00 ea 14 f0 9f e5  |.g..../@........|
00000210  14 f0 9f e5 14 f0 9f e5  14 f0 9f e5 14 f0 9f e5  |................|
00000220  14 f0 9f e5 14 f0 9f e5  40 04 2f 40 40 04 2f 40  |........@./@@./@|
```

Downloaded image can be from 0x402F0400 to 0x4030B800 which I think allows for 0x1B400 bytes = 109KB (matches datasheet).

The above MLO says that it takes up 0x016794 = 92052 bytes ~90 KB. Which roughly matches the size of the MLO:

```
sam@sambuntu:~$ ll -h ~/Downloads/MLO-am335x_evm-v2019.04-r11 
-rw-rw-r-- 1 sam sam 91K May  6 18:13 /home/sam/Downloads/MLO-am335x_evm-v2019.04-r11
```

(bit bigger cause of header overhead)

The coreboot MLO:

```
00000200  3b 15 00 00 00 04 2f 40  df f0 2b e3 4c 00 00 fa  |;...../@..+.L...|
```

Which would indicate that it takes up 5435 bytes.. so there should really only be data in the MLO up to 0x173B (0x200+0x153b). In reality itgoes much much past that though! I'm not 100% sure of this but I think it may be because coreboot puts both the "ROM stage" and "RAM stage" into the same file.

# Using Objdump on the binary files

```
# Need to first get rid of the header
dd if=MLO-am335x_evm-v2019.04-r11 of=test2.MLO skip=520 bs=1
arm-none-eabi-objdump -b binary -D ~/Downloads/test2.MLO -m arm | less
```

A nicer way to get the same sort of information might be to just look at the .map file in `uboot/spl/u-boot-spl.map` tho.


Replacing bootblock main with

```
void main(void)
{
	bootblock_soc_init();
	bootblock_mainboard_init();
	run_romstage();
}
```

Gets the LEDs to turn on! It must be failing somewhere prior to this. Turning off CONFIG_BOOTBLOCK_CONSOLE gets it to work.

Seems to boot only ~half the time? It was working all along, just needed the DRAM_START modified. Getting this exception: 

Actually - just realised. I moved the soc_init earlier so that may have helped the UARTs come up properly? TODO

```
coreboot-4.11-2593-g65cc80f740-dirty Mon May  4 22:46:21 UTC 2020 bootblock starting (log level: 7)...
Exception handlers installed.
exception _data_abort
R0 = 0x1a421377
R1 = 0x8eb49128
R2 = 0x00000000
R3 = 0x48048e99
R4 = 0x4030cd30
R5 = 0x402f2c8c
R6 = 0x00000038
R7 = 0x4030cd58
R8 = 0x402f2b9c
R9 = 0x4030cdf4
R10 = 0x4030cdcc
R11 = 0x00029940
IP = 0x4030cde8
SP = 0x4030cd18
LR = 0x402f17e5
PC = 0x402f17ee
DFAR = 0x1a42137b
DFSR = 0x00000001
ADFSR = 0x00000000
Dumping stack:
0x4030cf00: 07000040 40020205 07090000 01010020 0932c001 02000004 02ffffff 02810507 
0x4030cee0: 04010001 05000224 01000624 03830507 09010008 02000104 0200000a 02810507 
0x4030cec0: 00020002 00460209 c0010102 03090332 00000409 ff020201 00240502 24050110 
0x4030cea0: 02020100 050702ff 00080383 01040901 000a0200 05070200 02000281 02050700 
0x4030ce80: 01120001 00000200 00004000 00000000 01000000 00300209 c0010102 00040932 
0x4030ce60: ae4d7514 2e6a36f9 1c851f9a 4e520e3d 45534944 52454854 0054454e 000032c0 
0x4030ce40: 0010009f 0001f000 00001000 00000000 00000000 00000000 00000001 00000000 
0x4030ce20: 00020090 00020080 00020084 00020960 0002096c 00020090 00020094 00020098 
0x4030ce00: e59ff018 e59ff018 e59ff018 e59ff018 e59ff018 e59ff018 e59ff018 e59ff018 
0x4030cde0: 00000000 00000000 00000004 402f0573 deadbeef deadbeef deadbeef deadbeef 
0x4030cdc0: 00000001 00000003 00000000 402f2c8c 00000000 00000000 00000000 00000000 
0x4030cda0: 4030cde0 00000002 4030cb7c 4030cdcc 4030cdf4 00c51878 402f0543 402f2703 
0x4030cd80: 00c51878 4030cdc4 00000002 402f26c5 4030cdd0 402f296d 402f296d 402f2e5c 
0x4030cd60: 4030cd70 402f2429 4030cd90 402f2443 4030cd90 402f2c8c 00000000 402f2c8c 
0x4030cd40: 4030cd70 402f2c8c 00000000 4030cb7c 4030cdcc 402f266f 402f2b9c 4030cd70 
0x4030cd20: 4030cd70 402f261f deadbeef deadbeef 1a421377 00000000 48048e37 8eb49128 
0x4030cd00: deadbeef deadbeef 4030cd30 402f1861 4030cd30 402f17e5 48048e6f 0000002a 
```

The CCCCC printed in the serial console sometimes is the BBB trying to boot over UART. It will do this if it can't find a MLO to boot from!

Latest update:


```
region device
weak vboot locate cbfs
exception _data_abort
R0 = 0x084a0b47
R1 = 0x4204449e
R2 = 0x00000000
R3 = 0x1cb5cc0b
R4 = 0x4030cd30
R5 = 0x402f2e6c
R6 = 0x00000038
R7 = 0x4030cd58
R8 = 0x402f2d03
R9 = 0x4030cdf4
R10 = 0x4030cdcc
R11 = 0x00029940
IP = 0x4030cde8
SP = 0x4030cd18
LR = 0x402f17f1
PC = 0x402f17fa
DFAR = 0x084a0b4b
DFSR = 0x00000001
ADFSR = 0x00000000
Dumping stack:
0x4030cf00: 07000040 40020205 07090000 01010020 0932c001 02000004 02ffffff 02810507 
0x4030cee0: 04010001 05000224 01000624 03830507 09010008 02000104 0200000a 02810507 
0x4030cec0: 00020002 00460209 c0010102 03090332 00000409 ff020201 00240502 24050110 
0x4030cea0: 02020100 050702ff 00080383 01040901 000a0200 05070200 02000281 02050700 
0x4030ce80: 01120001 00000200 00004000 00000000 01000000 00300209 c0010102 00040932 
0x4030ce60: ae4f7554 2e68367b 1c851f9a 4e520e3d 45534944 52454854 0054454e 000032c0 
0x4030ce40: 0010009f 0001f000 00001000 00000000 00000000 00000000 00000001 00000000 
0x4030ce20: 00020090 00020080 00020084 00020960 0002096c 00020090 00020094 00020098 
0x4030ce00: e59ff018 e59ff018 e59ff018 e59ff018 e59ff018 e59ff018 e59ff018 e59ff018 
0x4030cde0: 00000000 00000000 00000004 402f0573 deadbeef deadbeef deadbeef deadbeef 
0x4030cdc0: 00c51878 00000003 00000000 402f2e6c 00000000 00000000 00000000 00000000 
0x4030cda0: 00000002 402f1a1d 00000003 4030cdb8 402f2791 00000003 402f0543 402f27ab 
0x4030cd80: 00000000 4030cdc4 00000002 402f2755 000000a1 402f19e9 00c51878 402f0543 
0x4030cd60: 4030cd70 402f2451 4030cd90 402f247b 4030cd90 402f2e6c 00000000 8c339848 
0x4030cd40: 4030cd70 402f2e6c 00000000 4030cb7c 4030cdcc 402f26d3 402f2d03 4030cd70 
0x4030cd20: 4030cd70 402f2683 4030cb7c 4030cdcc 084a0b47 00000000 1cb5cba9 4204449e 
0x4030cd00: 481a8000 481aa000 4030cd30 402f186d 4030cd30 402f17f1 1cb5cbe1 0000002a 
exception


```

I don't think cbfs_boot_region_device actually runs (but worth checking).

Looks like something is broken at the moment - it's not printing anything. no it does sometimes

should look in fmap_locate_area_as_rdev!

also why did LEDs stop working??? :\

I think there's some sort of memory corruption or something going on.

Clues:

- Code works when slight values are changed (I wrote a loop that flashes a LED. It works when it infinitely flashes but doesn't if I make it flash a few times.)
- I get errors about bad GPIO  writes sometimes. Looks like the memory is uninitiliazed? 


More investigation:

Looks like disabling the exception_init makes things work a bit more reliably. This might just be because its not catching the errors though!!!!
Dcache (soc_init) doesnt seem to matter.
Also need to mainboard_init before console_init

Increasing the stack to 16K seems to improve results.. maybe?

In any case it looks like cbfs_boot_locate(&file, prog_name(prog), NULL) is failing.

Digging deeper it seems `if (rdev->ops->mmap == NULL)` in rdev_mmap is causing the data abort

Call stack is (I think)

run_romstage
prog_locate
cbfs_boot_locate
cbfs_boot_region_device
fmap_locate_area_as_rdev
fmap_locate_area
rdev_mmap

Current thought is that maybe find_fmap_directory is putting NULL data into the region device. This bit of code is triggered:

	if (region_device_sz(&fmap_cache.rdev))
	{
		printk(BIOS_DEBUG, "d\n");
		return rdev_chain_full(fmrd, &fmap_cache.rdev);
	}

It gets there from the above call stack

fmap_locate_area
find_fmap_directory
rdev_chain_full
rdev_chain


Looks like the fmap_cache is not properly getting initialized. But it is static memory so should be zeroised!!!

I reckon there's something dodgy going on with bss.

Looks like the stack has some issues?

From armv7/bootblock.s:

	ldr	r0, =_stack
	ldr	r1, =_estack
	ldr	r2, =0xdeadbeef

And the resulting assembly

	402f054e:	480a      	ldr	r0, [pc, #40]	; (402f0578 <wait_for_interrupt+0x6>)
	402f0550:	490a      	ldr	r1, [pc, #40]	; (402f057c <wait_for_interrupt+0xa>)
	402f0552:	4a0b      	ldr	r2, [pc, #44]	; (402f0580 <wait_for_interrupt+0xe>)

	402f0572 <wait_for_interrupt>:
	402f0572:	bf30      	wfi
	402f0574:	46f7      	mov	pc, lr
	402f0576:	be000000 	cdplt	0, 0, cr0, cr0, cr0, {0}
	402f057a:	fe004030 	mcr2	0, 0, r4, cr0, cr0, {1}
	402f057e:	beef4030 	mcrlt	0, 7, r4, cr15, cr0, {1}
	402f0582:	2e08dead 	cdpcs	14, 0, cr13, cr8, cr13, {5}
	402f0586:	 	strlt	r4, [r8, #-47]	; 0xffffffd1

Some issues (maybe):

	- stack/estack are not aligned (do they need to be?)
	- I can't figure out what stack/estack are. Deadbeef is a bit messed up too
		- Why is half of it at 402f0584 and the other half at 402f057e?
		- The stack is defined to be `0x4030be00` in the ld script. Can at least see this at 402f057c/402f0576
		- this would mean that estack = 4030fe00 which is indeed 16K as specified


This I think is the finally memory layout:


	OUTPUT_FORMAT("elf32-littlearm", "elf32-littlearm", "elf32-littlearm")
	OUTPUT_ARCH(arm)
	PHDRS
	{
	to_load PT_LOAD;
	}
	ENTRY(stage_entry)
	SECTIONS
	{
		_ = ASSERT(. <= 0x402f0400, "dram overlaps the previous region!"); 
		. = 0x402f0400; 
		_dram = .;

		_ = ASSERT(. <= 0x402f0400, "bootblock overlaps the previous region!"); 
		. = 0x402f0400; _
		bootblock = .; 
		_ = ASSERT(. == ALIGN(1), "bootblock must be aligned to 1!"); 
	
		_ = ASSERT(. <= 0x402f0400 + 20K, "ebootblock overlaps the previous region!"); 
		. = 0x402f0400 + 20K; 
		_ebootblock = .;
	
		_ = ASSERT(. <= 0x402f5400, "romstage overlaps the previous region!"); 
		. = 0x402f5400; 
		_romstage = .; 
		_eromstage = _romstage + 88K; 
		_ = ASSERT(_eprogram - _program <= 88K, "Romstage exceeded its allotted size! (88K)"); 
		INCLUDE "romstage/lib/program.ld"

		_ = ASSERT(. <= 0x4030b400, "fmap_cache overlaps the previous region!"); 
		. = 0x4030b400; 
		_fmap_cache = .; 
		_ = ASSERT(. == ALIGN(4), "fmap_cache must be aligned to 4!"); 
		_ = ASSERT(. <= 0x4030b400 + 2K, "efmap_cache overlaps the previous region!"); 
		. = 0x4030b400 + 2K; 
		_efmap_cache = .;

		_ = ASSERT(2K >= 0xe0, "FMAP does not fit in FMAP_CACHE! (2K < 0xe0)");
		_ = ASSERT(. <= 0x4030be00, "stack overlaps the previous region!"); 
		. = 0x4030be00; 
		_stack = .; 
		_ = ASSERT(. == ALIGN(8), "stack must be aligned to 8!"); 
	
		_ = ASSERT(. <= 0x4030be00 + 16K, "estack overlaps the previous region!"); 
		. = 0x4030be00 + 16K; 
		_estack = .;
		_ = ASSERT(16K >= 2K, "stack should be >= 2K, see toolchain.inc");

		_ = ASSERT(. <= 0x80200000, "ramstage overlaps the previous region!"); 
		. = 0x80200000; 
		_ramstage = .; 
		_ = ASSERT(. == ALIGN(1), "ramstage must be aligned to 1!");
		_ = ASSERT(. <= 0x80200000 + 192K, "eramstage overlaps the previous region!"); 
		. = 0x80200000 + 192K; 
		_eramstage = .;
	
		_ = ASSERT(. <= 0x81000000, "ttb overlaps the previous region!");
		. = 0x81000000; 
		_ttb = .; 
		_ = ASSERT(. == ALIGN(16K), "ttb must be aligned to 16K!"); 
		_ = ASSERT(. <= 0x81000000 + 16K, "ettb overlaps the previous region!"); 
		. = 0x81000000 + 16K; 
		_ettb = .; 
		_ = ASSERT(16K >= 16K + 0 * 32, "TTB must be 16K (+ 32 for LPAE)!");
	}


And this is the structure within the bootblock:


	.text . : {
		_program = .;
		_text = .;
		*(.rom.text);
		*(.rom.data);
		*(.text._start);
		*(.text.stage_entry);
		KEEP(*(.id));
		*(.text);
		*(.text.*);
		. = ALIGN(8);
		_rsbe_init_begin = .;
		KEEP(*(.rsbe_init));
		_ersbe_init_begin = .;
		. = ALIGN(8);
		*(.rodata);
		*(.rodata.*);
		. = ALIGN(8);
		_etext = .;
	} : to_load

	.data . : {
		. = ALIGN(64);
		_data = .;
		*(.data);
		*(.data.*);
		*(.sdata);
		*(.sdata.*);
		PROVIDE(_preram_cbmem_console = .);
		PROVIDE(_epreram_cbmem_console = _preram_cbmem_console);
		. = ALIGN(8);
		_edata = .;
	}
	.bss . : {
		. = ALIGN(8);
		_bss = .;
		*(.bss)
		*(.bss.*)
		*(.sbss)
		*(.sbss.*)
		. = ALIGN(8);
		_ebss = .;
	}
	_eprogram = .;
	zeroptr = 0;
	/DISCARD/ : {
		*(.comment)
		*(.comment.*)
		*(.note)
		*(.note.*)
		*(.eh_frame);
	}



am335x_gpio_banks is something worth looking at! Seems like it sometimes has bad data in it?

With a binary that constantly complains about bad data accesses:

	402f2c58 <am335x_gpio_banks>:

In this case bss was from 402f2e48 to 402f2f88 and fmap cache was from 4030b400 to 4030bc00

No rodata section..? All strings are in text. I wonder if this blows out the size of text?


More interesting stuff.

If I boot only the bootblock.bin either over tftp or by using ./bbb-asm-demo/bin/mk-gpimage to make a MLO file only containing the bootblock, I don't get all the corrupted strings in my output.

Hypthesises:

1) Something sketchy happens when coreboot combines everything into the MLO
2) The bootROM doesn't like copying bigger files into memory
3) The MLO headers are wrong and somehow not all memory is getting copied across

Something that's not clear to me is how the 4MB MLO size works. Is this all copied into memory? If so, does the BBB cope (does it have enough SDRAM) and if not, how is the extra memory supposed to be accessed?

I hex edited a MLO file to increase the size in the MLO header and it looks a lot better afterwards!! 

`header_load_size` is how this is set. It currently uses cbfstool to get the offset + size of the romstage. For my current build this comes out to be 10514 bytes but bootblock.bin is 11208 bytes, so it's definetely wrong.. I'm not really sure why tho.

The file size of the romstage.bin file is larger than what the cbfstool reports. Bootblock.bin too.

The actual size that should be copied, looking at the linker dump above should be:

0x4030B400-0x402f0400=0x1B000=110592=108KB.. Which is from the bootblock to the end of the romstage. Which is actually ok and fits within the constraints for the public image location.

Having the fmapcache at 0x4030b400 and a 4K stack at 0x4030be00 means that all the coreboot stuff stops at 0x4030CE00, which marries up with the datasheet - 0x4030CE00 is the end of user avaliable memory (although there's some stuff beyond this that might be able to be repurposed? like ram memory vectors)

Questions:

- Why does cbfstool have the romstage at an offset of 0x80? Shouldn't it be later because the bootblock is first. It was at 0x4c00 in the old coreboot.
	- I think this has something to do with using FMAPs now.
- Why was the decision to make the coreboot.rom 4MB when this was too big to be loaded by the inbuilt ROM code?
	- Assuming I can somehow get the CB ROM stage loading, how should I load the CB RAM stage?
- Why is the CB ROM stage fixed at 88KB when it seems to be much smaller.




# Hex editing in vim
:%!xxd to enter
:%!xxd -r to exit

Need to exit before saving otherwise it saves it as a (human readable) hexdump!

# TFTP booting

```
cat /etc/NetworkManager/dnsmasq-shared.d/spl-uboot-fit.conf 
bootp-dynamic
enable-tftp
tftp-root=/tmp/bbimages
dhcp-vendorclass=set:want-spl,AM335x ROM
dhcp-vendorclass=set:want-uboot,AM335x U-Boot SPL
dhcp-boot=tag:want-spl,/spl
dhcp-boot=tag:want-uboot,/uboot
```

Use nm-connection-editor to choose "shared with other devices" on the USB0 ethernet interface that appears when you plug the BBB into your host computer using USB. You may need to hold the S2 button when booting.

Might need to restart NetworkManager afterwards. 

The above config will be picked up by any dnsmasq adapter that NM starts.

Important - you should load the bootblock.bin file directly over TFTP, it doesn't need the MLO header in it. For some reason though I wasn't able to get the coreboot.pre/coreboot.rom files to work. Unsure why - maybe too large?


# Beaglebone commits

3c7e939c3e18b3d286c084ff95266611a0150ca1 - intial work
3aa58162e1be25ad77800879e73a087ddbdc660c - cleanup am335x


b7d81e05bb3e4058cbb28adcc5af0bdccfe88337 - ARM rom stage
ddbfc645c2fb9c2aab55c9d5f7c55fa80fd8da64 - omap header
437a1e67a3e4c530292d947ef5e1adbf3cc7650a - expand rom size to 4MB
1ef5ff277085512a2d24bef0b5f6f185a952b3b7 - make internal rom code load only bootblock/ROM 