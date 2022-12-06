# 7 confirmed cases found during PAL development

## P1: Complete protection (1 case)

#### P1_1

validator report: `[nv] func: packet_setsockopt / line : ffff000010c22744:  d61f0020    br  x1`

manual analysis:
```
// This was caused by a switch-case optimization with a jump table.
// NOTE: This is not a real vulnerability since none of values involved in jump table calculation is controllable, at least for this case.
//       But, if some value leaks to the stack, it would become a vulnerable gadget.
//       So we treated this case, during our development, as potentially vulnerable.
 
ffff000010c22730:   b00014c2    adrp    x2, ffff000010ebb000 <snmp4_net_list+0x748>
ffff000010c22734:   911c8042    add x2, x2, #0x720
ffff000010c22738:   78615841    ldrh    w1, [x2, w1, uxtw #1]
ffff000010c2273c:   10000062    adr x2, ffff000010c22748 <packet_setsockopt+0x68>
ffff000010c22740:   8b21a841    add x1, x2, w1, sxth #2
ffff000010c22744:   d61f0020    br  x1  // (1) use a branch instruction without authentication
```

## P2: No time-of-check-time-of-use (1 case)

#### P2_1

validator report: `[nv] func: sort / line : ffff0000104549d8:    dac11402    autib   x2, x0`

manual analysis:
```
ffff0000104549d8:   dac11402    autib   x2, x0     // (1) authenticate x2 and x1
ffff0000104549dc:   dac11461    autib   x1, x3
ffff0000104549e0:   a9090ba1    stp x1, x2, [x29, #144] // (2) spill authenticated pointer onto stack
                                            // attackers can corrupt the pointer in stack
....
ffff000010454a18:   f9404fa3    ldr x3, [x29, #152]   // (3) load the corrupted pointer
ffff000010454a1c:   531f7a73    lsl w19, w19, #1
ffff000010454a20:   aa1703e0    mov x0, x23
ffff000010454a24:   2a1503e2    mov w2, w21
ffff000010454a28:   aa1c03e1    mov x1, x28
ffff000010454a2c:   d63f0060    blr x3   // (4) jump to the attacker-chosen pointer
```

## P3: No signing oracle (5 cases)

#### P3_1

validator report:  `[nv] func: audit_log_start / line : ffff0000101963e4: dac10403    pacib   x3, x0`

manual analysis:
```
ffff000010196398:   90fffba0    adrp    x0, ffff00001010a000 <move_queued_task.isra.13+0x150>
ffff00001019639c:   91008281    add x1, x20, #0x20
ffff0000101963a0:   912a8000    add x0, x0, #0xaa0
ffff0000101963a4:   a904efba    stp x26, x27, [x29, #72]
ffff0000101963a8:   f9002fbc    str x28, [x29, #88]
ffff0000101963ac:   f90037a0    str x0, [x29, #104]
.....
ffff0000101963dc:   f94037a3    ldr x3, [x29, #104] // x3 is from constant --> ffff00001010a000
ffff0000101963e0:   aa1c03e1    mov x1, x28
ffff0000101963e4:   dac10403    pacib   x3, x0
```

#### P3_2

validator report: `[nv] func: nobh_write_begin / line : ffff0000102a1b78:    dac10420    pacib   x0, x1`

manual report:
```
ffff0000102a1a48:   f0ffffc0    adrp    x0, ffff00001029c000 <fs_parse+0xe0>
ffff0000102a1a4c:   2a1403fa    mov w26, w20
ffff0000102a1a50:   91248000    add x0, x0, #0x920
ffff0000102a1a54:   291717a4    stp w4, w5, [x29, #184]
ffff0000102a1a58:   52800015    mov w21, #0x0                       // #0
ffff0000102a1a5c:   2a0403f7    mov w23, w4
ffff0000102a1a60:   2a0303f4    mov w20, w3
ffff0000102a1a64:   f90037a0    str x0, [x29, #104]
....
ffff0000102a1b70:   f94037a0    ldr x0, [x29, #104]
ffff0000102a1b74:   52800003    mov w3, #0x0                    // #0
ffff0000102a1b78:   dac10420    pacib   x0, x1
```

#### P3_3 ~ P3_5

P3_3 ~ P3_5 share the same binary pattern to that of P3_1 and P3_2.

- P3_3's validator report: `[nv] func: __arm64_sys_io_uring_register / line : ffff0000102c08c4: dac10440    pacib   x0, x2`
- P3_4's validator report: `[nv] func: ext4_bio_write_page / line : ffff00001033a2a8:   dac10420    pacib   x0, x1`
- P3_5's validator report: `[nv] func: audit_log_start / line : ffff0000101963e4: dac10403    pacib   x3, x0`

## P4: No unchecked control-flow change

None
