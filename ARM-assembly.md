First 4 arguments are stored in r0-r3

LR allows a function to 'return'

PC is always 2 ahead of current instruction

Current Program Status Register (CPSR)

instructions in ARM state are always 32-bit, and  instructions in Thumb state are 16-bit (but can be 32-bit). 

ARM introduced an enhanced Thumb instruction set (pseudo name: Thumbv2) which allows 32-bit Thumb instructions and even conditional execution, which was not possible in the versions prior to that

https://azeria-labs.com/arm-instruction-set-part-3/