/*
 * Copyright (c) 2021 Xing Tang. All Rights Reserved.
 */
#ifndef __HTIF_UART_H__
#define __HTIF_UART_H__

#if __riscv_xlen == 64
# define TOHOST_CMD(dev, cmd, payload) \
  (((uint64_t)(dev) << 56) | ((uint64_t)(cmd) << 48) | (uint64_t)(payload))
#else
# define TOHOST_CMD(dev, cmd, payload) ({ \
  if ((dev) || (cmd)) __builtin_trap(); \
  (payload); })
#endif
#define FROMHOST_DEV(fromhost_value) ((uint64_t)(fromhost_value) >> 56)
#define FROMHOST_CMD(fromhost_value) ((uint64_t)(fromhost_value) << 8 >> 56)
#define FROMHOST_DATA(fromhost_value) ((uint64_t)(fromhost_value) << 16 >> 16)

int htif_console_getchar();
void htif_console_putchar(uint8_t ch);
void htif_init(void);
void print(const char *s);
void rv_printf(const char* s, ...);
 
#endif
