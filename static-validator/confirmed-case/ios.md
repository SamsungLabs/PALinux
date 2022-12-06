# 5 confirmed cases of ios kernel binary

This document describes potentially vulnerable cases that static validator reported and are confirmed by hand.
We give an extra explanation of how each case can be exploited.
(Note that we have not checked if the cases are exploitable in reality. That is, we do not claim these are exploitable.)

## P1: Complete protection (1 case)

#### P1_1

```
fffffff0080ccd08:   dac143e8    xpaci   x8  // (1) clear out the PAC in x8.
fffffff0080ccd0c:   dac123e8    paciza  x8  // (2) sign x8 with zero context
fffffff0080ccd10:   d63f091f    blraaz  x8  // (3) authenticate and jump to it
// if attackers can control x8, they can make an arbitrary control flow transition.
```

## P2: No time-of-check-time-of-use (1 case)

#### P2_1

```
fffffff007cc81d0:       dac11115        autia   x21, x8  // (1) authenticated pointer will be stored in x21
....
fffffff007cc81e4:       f9008275        str     x21, [x19, #256]  // (2) the pointer is going to be stored onto memory x19 points to.
// if attackers can corrupt the pointer stored in x19, they might be able to make an arbitrary control flow transition.
```

## P3: No signing oracle (3 cases)

#### P3_1

```
// (1) x2 and x19 contain a function parameter
fffffff007cc7f18:   aa0203f3    mov x19, x2
....
// (2) this will load pointer from the parameter
// If attackers can control the parameter, x22 can be an attacker-chosen pointer
fffffff007cc7fc0:   f9408276    ldr x22, [x19, #256]
....
// (3) sign the attacker-chosen pointer (x22), that is a signing oracle
fffffff007cc7fe8:   dac10116    pacia   x22, x8
```

#### P3_2

```
// (1) x2 and x19 contain a function parameter
fffffff007cc7f18:   aa0203f3    mov x19, x2
....
// (2) this will load pointer from the parameter
// If attackers can control the parameter, x21 can be an attacker-chosen pointer
fffffff007cc8000:   f9407a75    ldr x21, [x19, #240]
....
// (3) sign the attacker-chosen pointer (x21), that is a signing oracle
fffffff007cc8030:   dac10115    pacia   x21, x8
```

#### P3_3

```
// (1) x0 and x19 contain a function parameter
fffffff007cacb00:   aa0003f3    mov x19, x0
....
// (2) sign the parameter directly, it could become a signing oracle if the parameter is controllable
fffffff007cacb24:   dac10293    pacia   x19, x20
```

## P4: No unchecked control-flow change

None