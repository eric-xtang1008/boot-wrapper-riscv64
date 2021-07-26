/*
 * Copyright (c) 2021. Xing Tang. All Rights Reserved.
 */

#include <stdint.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include "htif_uart.h"

/*
 * HTIF is the Host/Target Interface.
 *
 *        63       56|55     48|47            0
 *        -------------------------------------
 *        |  device  | command |     data     |
 *        -------------------------------------
 *
 *  device=0 --- syscall
 *           --- command=0, data = pointer to syscall
 *           --- command=1, data = exit code
 *
 *  device=1 --- a blocking character device
 *           --- command=0, read a character
 *           --- command=1, write a character
 */

volatile uint64_t tohost __attribute__ ((section(".htif")));
volatile uint64_t fromhost __attribute__ ((section(".htif")));
volatile int htif_console_buf;

static void __check_fromhost()
{
	uint64_t fh = fromhost;
	if (!fh)
		return;
	fromhost = 0;

	// this should be from the console
	switch (FROMHOST_CMD(fh))
	{
	case 0:
		htif_console_buf = 1 + (uint8_t) FROMHOST_DATA(fh);
		break;
	case 1:
		break;
	default:
		return;
	}
}

static void send_tohost(uintptr_t dev, uintptr_t cmd, uintptr_t data)
{
	while (tohost) ;			/* wait the data be fetched by host */

	tohost = TOHOST_CMD(dev, cmd, data);
}

void htif_console_putchar(uint8_t ch)
{
	send_tohost(1, 1, ch);
}

int htif_console_getchar()
{
	__check_fromhost();
	int ch = htif_console_buf;
	if (ch >= 0)
	{
		htif_console_buf = -1;
		while (tohost)
			__check_fromhost();
		tohost = TOHOST_CMD(1, 0, 0);
	}
	return ch - 1;
}

void print(const char *s)
{
	while (*s != '\0')
		htif_console_putchar(*s++);
}

static int vsnprintf(char *out, size_t n, const char *s, va_list vl)
{
	bool longarg = false;
	size_t pos = 0;
	long num;
	for (; *s; s++)
	{
		if (*s == '%')
		{
			s++;
			if (*s == 'l')
			{
				longarg = true;
				s++;
			}

			switch (*s)
			{
			case 'x':
				num = longarg ? va_arg(vl, long) : va_arg(vl, int);
				for (int i = 2 * (longarg ? sizeof(long) : sizeof(int)) - 1;
					 i >= 0; i--)
				{
					int d = (num >> (4 * i)) & 0xF;
					if (++pos < n)
						out[pos - 1] = (d < 10 ? '0' + d : 'a' + d - 10);
				}
				longarg = false;
				break;
			case 'd':
				num = longarg ? va_arg(vl, long) : va_arg(vl, int);
				int digits = 1;
				for (long long nn = num; nn /= 10; digits++)
					;
				for (int i = digits - 1; i >= 0; i--)
				{
					if (pos + i + 1 < n)
						out[pos + i] = '0' + (num % 10);
					num /= 10;
				}
				pos += digits;
				longarg = false;
				break;
			case 's':
				{
					const char *s2 = va_arg(vl, const char *);
					while (*s2)
					{
						if (++pos < n)
							out[pos - 1] = *s2;
						s2++;
					}
				}
				break;
			default:
				break;
			}
		} else
		{
			if (++pos < n)
				out[pos - 1] = *s;
		}
	}
	if (pos < n)
		out[pos] = 0;
	else if (n)
		out[n - 1] = 0;
	return pos;
}

static void vprintm(const char *s, va_list vl)
{
	char buf[256];
	vsnprintf(buf, sizeof buf, s, vl);
	print(buf);
}

void rv_printf(const char *s, ...)
{
	va_list vl;

	va_start(vl, s);
	vprintm(s, vl);
	va_end(vl);
}

void htif_init(void)
{
	tohost = 0;
	fromhost = 0;
}
