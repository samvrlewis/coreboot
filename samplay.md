


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

I reckon there's something dodgy going on with bss