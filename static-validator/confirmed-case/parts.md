# 15 confirmed cases of the kernel binary built by PARTS

This document describes potentially vulnerable cases that static validator reported and are confirmed by hand.
We give an extra explanation of how each case can be exploited.
(Note that we have not checked if the cases are exploitable in reality. That is, we do not claim these are exploitable.)

## P1: Complete protection (2 cases)

#### P1_1

```
ffff0000107c66ec:   aa0003f3    mov x19, x0  // (1) x19 and x0 contain the first function argument
ffff0000107c66f0:   52947508    mov w8, #0xa3a8 
ffff0000107c66f4:   f8686a68    ldr x8, [x19, x8]  // (2) load a function pointer from memory
ffff0000107c66f8:   d63f0100    blr x8  // (3) use the function pointer without authentication
```

#### P1_2

```
ffff00001114826c:   f9404768    ldr x8, [x27, #136]  // (1) load a function pointer from memory
ffff000011148270:   b4000068    cbz x8, ffff00001114827c <cgroup_init+0x348>
ffff000011148274:   f8757b20    ldr x0, [x25, x21, lsl #3]
ffff000011148278:   d63f0100    blr x8  // (2) use the function pointer without authentication
```

## P2: No time-of-check-time-of-use

None

## P3: No signing oracle (12 cases)

The cases below share the same pattern to that of Figure8_line22 in the paper.

- func: __cancel_work_timer / line : ffff0000100fa0f8:  dac102fa    pacia   x26, x23
- func: __setup_irq / line : ffff00001013f458:  dac102e9    pacia   x9, x23
- func: perf_event_release_kernel / line : ffff0000101a8f48:    dac102e8    pacia   x8, x23
- func: xhci_reset / line : ffff0000108b5810:   dac102e1    pacia   x1, x23
- func: xhci_mem_cleanup / line : ffff0000108be898: dac102e1    pacia   x1, x23
- func: xhci_mem_init / line : ffff0000108bf680:    dac102e1    pacia   x1, x23
- func: xhci_mem_init / line : ffff0000108bfb94:    dac102e8    pacia   x8, x23
- func: xhci_mem_init / line : ffff0000108bffc4:    dac102e1    pacia   x1, x23
- func: usb_stor_CB_transport / line : ffff0000108d0508:    dac102eb    pacia   x11, x23
- func: usb_stor_CB_transport / line : ffff0000108d0600:    dac102eb    pacia   x11, x23
- func: usb_stor_reset_common / line : ffff0000108d0ca4:    dac102e8    pacia   x8, x23
- func: usb_stor_reset_common / line : ffff0000108d0d2c:    dac102eb    pacia   x11, x23

## P4: No unchecked control-flow change (1 case)

#### P4_1

```
ENTRY(cpu_switch_to)
    ldp x19, x20, [x8], #16
    ldp x21, x22, [x8], #16
    ldp x23, x24, [x8], #16
    ldp x25, x26, [x8], #16
    ldp x27, x28, [x8], #16
    ldp x29, x9, [x8], #16
    ldr lr, [x8]  // (1) restore lr register from read-write memory, which is controllable by attackers 
    ...
    mov sp, x9
    msr sp_el0, x1
    ret  // (2) jump to the lr register
```