/*
 * Copyright (c) 2021. Xing Tang. All Rights Reserved.
 */

  .section ".kernel","a",@progbits

  .align 3

  .globl _kernel_start, _kernel_end
_kernel_start:
  .incbin BBL_PAYLOAD
_kernel_end:
