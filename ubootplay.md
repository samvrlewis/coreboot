# Build Uboot
make ARCH=arm CROSS_COMPILE=arm-none-eabi- am335x_evm_config
make ARCH=arm CROSS_COMPILE=arm-none-eabi-

Inspect the SPL:

arm-none-eabi-objdump -D spl/u-boot-spl | less

Seems to start in 

/home/sam/work/third_party/u-boot/arch/arm/cpu/armv7/start.S

reset
save_boot_params
    - seems to just save r0 to OMAP_SRAM_SCRATCH_BOOT_PARAMS? meh
save_boot_params_ret
    - disable fast interrupt and nmormal interrupt modes
    - go into svc32 mode. this is 'privledged mode'. set by setting CSPR "Processor mode bits".
    - sets the vector address table 
        - http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.ddi0464d/BABHBIDJ.html
cpu_init_cp15
    - invalidates a bunch of caches/branch prediction/tlb stuff
    - disables MMU and caches
    - reads the main id register ( i think this is only for the purpose of doing errata stuff)
    - loads CONFIG_SYS_INIT_SP_ADDR into the stack pointer
        - this is defined to be 
            #define CONFIG_SYS_INIT_SP_ADDR (NON_SECURE_SRAM_END - GENERATED_GBL_DATA_SIZE)
            - 0x40310000-224 = 0x4030FF20
                - Why 0x40310000? I think this is because the "public RAM" as listed in the datasheet is up to 0x4030FFFF
            - generated gbl_data_size probs changes though
cpu_init_crit
    lowlevel_init
_main
board_init_f_alloc_reserve
board_init_f_init_reserve
board_init_f
SPL_CLEAR_BSS
spl_relocate_stack_gd
board_init_r

/home/sam/work/third_party/u-boot/arch/arm/cpu/armv7/lowlevel_init.S


Then jumps to _main:

/home/sam/work/third_party/u-boot/arch/arm/lib/crt0.S

https://wiki.tizen.org/images/b/bd/Lecture-1.pdf UBoot code sequence is useful

Also a good description of bootflow in the normal readme

https://iitd-plos.github.io/col718/ref/arm-instructionset.pdf
https://stackoverflow.com/questions/19544694/understanding-mrc-on-arm7

https://github.com/ARM-software/u-boot/blob/master/include/configs/bur_am335x_common.h
https://github.com/ARM-software/u-boot/blob/master/include/configs/ti_armv7_common.h
