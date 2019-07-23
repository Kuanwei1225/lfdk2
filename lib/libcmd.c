/*
 * LFDK - Linux Firmware Debug Kit
 * File: libio.c
 *
 * Copyright (C) 2006 - 2010 Merck Hung <merckhung@gmail.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/io.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <ncurses.h>
#include <panel.h>

#include "lfdk.h"

MemPanel CMDScreen;
struct lfdk_io_t lfdk_io_data;

extern int x, y;
extern int input;
extern unsigned int counter;
extern int ibuf;
extern char wbuf;
extern char enter_mem;

static unsigned int ioaddr = 0;

static void WriteIOByteValue(void) {
	lfdk_io_data.addr = ioaddr + x * LFDK_BYTE_PER_LINE + y;
	lfdk_io_data.buf = wbuf;
	// write IO here
	outb(lfdk_io_data.buf, lfdk_io_data.addr);
}

void ClearCMDScreen(void) {
	DestroyWin(CMDScreen, offset);
	DestroyWin(CMDScreen, info);
	DestroyWin(CMDScreen, value);
	DestroyWin(CMDScreen, ascii);
}

void PrintCMDScreen(void) {
	int i, j;
	char tmp;

	if (enter_mem) {
		if (ibuf == 0x0a) {
			if (!ioperm(ioaddr, LFDK_MASSBUF_SIZE, 1)) {
				enter_mem = 0;
			}
			return;
		} else if (((ibuf >= '0') && (ibuf <= '9')) ||
				((ibuf >= 'a') && (ibuf <= 'f')) ||
				((ibuf >= 'A') && (ibuf <= 'F'))) {
			ioaddr <<= 4;
			ioaddr &= 0xffff;

			if (ibuf <= '9') {
				ioaddr |= (unsigned int)(ibuf - 0x30);
			} else if (ibuf > 'F') {
				ioaddr |= (unsigned int)(ibuf - 0x60 + 9);
			} else {
				ioaddr |= (unsigned int)(ibuf - 0x40 + 9);
			}
		}
	} else {
		if (ibuf == KEY_UP) {
			if (x > 0) {
				x--;
			}

			input = 0;
		} else if (ibuf == KEY_DOWN) {
			if (x < 15) {
				x++;
			}
			input = 0;
		} else if (ibuf == KEY_LEFT) {
			if (y > 0) {
				y--;
			}
			input = 0;
		} else if (ibuf == KEY_RIGHT) {
			if (y < 15) {
				y++;
			}
			input = 0;
		} else if (ibuf == 0x0a) {
			if (input) {
				input = 0;
				WriteIOByteValue();
			}
		} else if (((ibuf >= '0') && (ibuf <= '9')) ||
				((ibuf >= 'a') && (ibuf <= 'f')) ||
				((ibuf >= 'A') && (ibuf <= 'F'))) {
			if (!input) {
				wbuf = 0x00;
				input = 1;
			}

			wbuf <<= 4;
			wbuf &= 0xf0;

			if (ibuf <= '9') {
				wbuf |= ibuf - 0x30;
			} else if (ibuf > 'F') {
				wbuf |= ibuf - 0x60 + 9;
			} else {
				wbuf |= ibuf - 0x40 + 9;
			}
		}
	}

	//
	// Print Offset Text
	//
	PrintFixedWin(CMDScreen, offset, 17, 52, 4, 1, RED_BLUE,
			"0000 00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E "
			"0F0000\n0010\n0020\n0030\n0040\n0050\n0060\n0070\n0080\n0090"
			"\n00A0\n00B0\n00C0\n00D0\n00E0\n00F0");

	//
	// Print memory address
	//
	if (!CMDScreen.info) {
		CMDScreen.info = newwin(1, 47, 22, 0);
		CMDScreen.p_info = new_panel(CMDScreen.info);
	}
	wbkgd(CMDScreen.info, COLOR_PAIR(WHITE_BLUE));
	wattrset(CMDScreen.info, COLOR_PAIR(WHITE_BLUE) | A_BOLD);
	mvwprintw(CMDScreen.info, 0, 0, "Type: I/O Space Address:     ");

	if (enter_mem) {
		if (counter % 2) {
			wattrset(CMDScreen.info, COLOR_PAIR(YELLOW_RED) | A_BOLD);
		} else {
			wattrset(CMDScreen.info, COLOR_PAIR(YELLOW_BLACK) | A_BOLD);
		}

		wprintw(CMDScreen.info, "%4.4X", ioaddr);

		counter++;
	} else {
		wattrset(CMDScreen.info, COLOR_PAIR(WHITE_BLUE) | A_BOLD);
		wprintw(CMDScreen.info, "%4.4X", ioaddr);
	}

	wattrset(CMDScreen.info, COLOR_PAIR(WHITE_BLUE) | A_BOLD);
	wprintw(CMDScreen.info, "h");
	wattrset(CMDScreen.info, A_NORMAL);

	//
	// Read memory space 256 bytes
	//
	if (enter_mem) {
		memset(lfdk_io_data.mass_buf, 0xff, LFDK_MASSBUF_SIZE);
	} else {
		lfdk_io_data.addr = ioaddr;
		// read 256 bytes IO here
		for (int i = 0; i < LFDK_MASSBUF_SIZE; i++) {
			lfdk_io_data.mass_buf[i] = inb(ioaddr + i);
		}
	}

	//
	// Print ASCII content
	//
	if (!CMDScreen.ascii) {
		CMDScreen.ascii = newwin(17, 16, 4, 58);
		CMDScreen.p_ascii = new_panel(CMDScreen.ascii);
	}

	wbkgd(CMDScreen.ascii, COLOR_PAIR(CYAN_BLUE));
	wattrset(CMDScreen.ascii, COLOR_PAIR(CYAN_BLUE) | A_BOLD);
	mvwprintw(CMDScreen.ascii, 0, 0, "");

	wprintw(CMDScreen.ascii, "0123456789ABCDEF");
	for (i = 0; i < LFDK_BYTE_PER_LINE; i++) {
		for (j = 0; j < LFDK_BYTE_PER_LINE; j++) {
			tmp = ((unsigned char)
					lfdk_io_data.mass_buf[(i * LFDK_BYTE_PER_LINE) + j]);
			if ((tmp >= '!') && (tmp <= '~')) {
				wprintw(CMDScreen.ascii, "%c", tmp);
			} else {
				wprintw(CMDScreen.ascii, ".");
			}
		}
	}

	wattrset(CMDScreen.ascii, A_NORMAL);

	//
	// Print 256bytes content
	//
	if (!CMDScreen.value) {
		CMDScreen.value = newwin(17, 47, 5, 6);
		CMDScreen.p_value = new_panel(CMDScreen.value);
	}

	wbkgd(CMDScreen.value, COLOR_PAIR(WHITE_BLUE));
	mvwprintw(CMDScreen.value, 0, 0, "");

	for (i = 0; i < LFDK_BYTE_PER_LINE; i++) {
		for (j = 0; j < LFDK_BYTE_PER_LINE; j++) {
			//
			// Change Color Pair
			//
			if (y == j && x == i) {
				if (input) {
					if (counter % 2) {
						wattrset(CMDScreen.value,
								COLOR_PAIR(YELLOW_RED) | A_BOLD);
					} else {
						wattrset(CMDScreen.value,
								COLOR_PAIR(YELLOW_BLACK) | A_BOLD);
					}

					counter++;
				} else {
					wattrset(CMDScreen.value,
							COLOR_PAIR(BLACK_YELLOW) | A_BOLD);
				}
			} else if (((unsigned char)lfdk_io_data
						.mass_buf[(i * LFDK_BYTE_PER_LINE) + j])) {
				wattrset(CMDScreen.value, COLOR_PAIR(YELLOW_BLUE) | A_BOLD);
			} else {
				wattrset(CMDScreen.value, COLOR_PAIR(WHITE_BLUE) | A_BOLD);
			}

			//
			// Handle input display
			//
			if (y == j && x == i) {
				if (input) {
					wprintw(CMDScreen.value, "%2.2X", (unsigned char)wbuf);
				} else {
					wprintw(CMDScreen.value, "%2.2X",
							(unsigned char)lfdk_io_data
							.mass_buf[(i * LFDK_BYTE_PER_LINE) + j]);
				}
			} else {
				wprintw(CMDScreen.value, "%2.2X",
						(unsigned char)lfdk_io_data
						.mass_buf[(i * LFDK_BYTE_PER_LINE) + j]);
			}

			//
			// End of color pair
			//
			wattrset(CMDScreen.value, A_NORMAL);

			//
			// Move to next byte
			//
			if (j != 15) {
				wprintw(CMDScreen.value, " ");
			}
		}
	}
}
