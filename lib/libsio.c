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

MemPanel SIOScreen;
struct lfdk_io_t lfdk_sio_data;

extern int x, y;
extern int input;
extern unsigned int counter;
extern int ibuf;
extern char wbuf;
extern char enter_mem;
extern struct cmd_data_t *cmd_data;

static unsigned int sioaddr_index = 0;
static unsigned int sioaddr_data = 0;
// SIO function
static inline unsigned char sio_inb(int addr_index, int addr_data, int index)
{
	outb(index, addr_index);
	return inb(addr_data);
}
static inline void sio_outb(int addr_index, int addr_data,int index, int val)
{
	outb(index, addr_index);
	outb(val, addr_data);
}
void sio_command(void) 
{
	// do command
	if(cmd_data == NULL) {
		return;
	}
	for(struct cmd_data_t *ptr = cmd_data; ptr != NULL; ptr = ptr->next) {
		switch (ptr->cmd)
		{
			case 'o':
				outb(ptr->val, ptr->addr);
				break;
		}
	}
}
static void WriteSIOByteValue(void) 
{
	lfdk_sio_data.addr = sioaddr_index + x * LFDK_BYTE_PER_LINE + y;
	lfdk_sio_data.buf = wbuf;
	// write SIO here
	sio_command();
	outb(lfdk_sio_data.buf, lfdk_sio_data.addr);
}
static void ReadAllSioValue(void)
{
	for(int i = 0; i < LFDK_MASSBUF_SIZE; i++) {
		sio_command();
		lfdk_sio_data.mass_buf[i] = sio_inb(sioaddr_index, sioaddr_data, i);
	}
}
void ClearSIOScreen(void) {
	DestroyWin(SIOScreen, offset);
	DestroyWin(SIOScreen, info);
	DestroyWin(SIOScreen, value);
	DestroyWin(SIOScreen, ascii);
	ioperm(sioaddr_index, 1, 0);
	ioperm(sioaddr_data, 1, 0);
}

void PrintSIOScreen(void) {
	if (enter_mem != SHOW_DATA) {
		if (ibuf == 0x0a) {
			switch (enter_mem) {
				case PARAM_1ST:
					enter_mem = PARAM_2ND;
					return;
				case PARAM_2ND:
					if(ioperm(sioaddr_index, 1, 1)) {
						printf("IO permission failed.\n");
						return;
					}
					if(ioperm(sioaddr_data, 1, 1)) {
						printf("IO permission failed.\n");
						return;
					}
					enter_mem = SHOW_DATA;
					return;
			}
		} else if (((ibuf >= '0') && (ibuf <= '9')) ||
				((ibuf >= 'a') && (ibuf <= 'f')) ||
				((ibuf >= 'A') && (ibuf <= 'F'))) {
			unsigned int *tmp = NULL;

			if(enter_mem == PARAM_1ST) {
				tmp = &sioaddr_index;
			} else if (enter_mem == PARAM_2ND) {
				tmp = &sioaddr_data;
			}
			*tmp <<= 4;
			*tmp &= 0xffff;
			if (ibuf <= '9') {
				*tmp |= (unsigned int)(ibuf - 0x30);
			} else if (ibuf > 'F') {
				*tmp |= (unsigned int)(ibuf - 0x60 + 9);
			} else {
				*tmp |= (unsigned int)(ibuf - 0x40 + 9);
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
				WriteSIOByteValue();
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
	PrintFixedWin(SIOScreen, offset, 17, 52, 4, 1, RED_BLUE,
			"0000 00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E "
			"0F0000\n0010\n0020\n0030\n0040\n0050\n0060\n0070\n0080\n0090"
			"\n00A0\n00B0\n00C0\n00D0\n00E0\n00F0");
	//
	// Print memory address
	//
	if (!SIOScreen.info) {
		SIOScreen.info = newwin(1, 47, 22, 0);
		SIOScreen.p_info = new_panel(SIOScreen.info);
	}
	wbkgd(SIOScreen.info, COLOR_PAIR(WHITE_BLUE));
	wattrset(SIOScreen.info, COLOR_PAIR(WHITE_BLUE) | A_BOLD);
	mvwprintw(SIOScreen.info, 0, 0, "Type: SIO Space Address: ");

	if (enter_mem == PARAM_1ST) {
		if (counter % 2) {
			wattrset(SIOScreen.info, COLOR_PAIR(YELLOW_RED) | A_BOLD);
		} else {
			wattrset(SIOScreen.info, COLOR_PAIR(YELLOW_BLACK) | A_BOLD);
		}
		wprintw(SIOScreen.info, "%4.4X", sioaddr_index);
		counter++;
	} else {
		wattrset(SIOScreen.info, COLOR_PAIR(WHITE_BLUE) | A_BOLD);
		wprintw(SIOScreen.info, "%4.4X", sioaddr_index);
	}
	wattrset(SIOScreen.info, COLOR_PAIR(WHITE_BLUE) | A_BOLD);
	wprintw(SIOScreen.info, "h, ");
	wattrset(SIOScreen.info, A_NORMAL);

	if (enter_mem == PARAM_2ND) {
		if (counter % 2) {
			wattrset(SIOScreen.info, COLOR_PAIR(YELLOW_RED) | A_BOLD);
		} else {
			wattrset(SIOScreen.info, COLOR_PAIR(YELLOW_BLACK) | A_BOLD);
		}
		wprintw(SIOScreen.info, "%4.4X", sioaddr_data);
		counter++;
	} else {
		wattrset(SIOScreen.info, COLOR_PAIR(WHITE_BLUE) | A_BOLD);
		wprintw(SIOScreen.info, "%4.4X", sioaddr_data);
	}
	wattrset(SIOScreen.info, COLOR_PAIR(WHITE_BLUE) | A_BOLD);
	wprintw(SIOScreen.info, "h");
	wattrset(SIOScreen.info, A_NORMAL);
	//
	// Read memory space 256 bytes
	//
	if (enter_mem != SHOW_DATA) {
		memset(lfdk_sio_data.mass_buf, 0xff, LFDK_MASSBUF_SIZE);
	} else {
		// read 256 bytes SIO here
		ReadAllSioValue();
	}

	//
	// Print ASCII content
	//
	if (!SIOScreen.ascii) {
		SIOScreen.ascii = newwin(17, 16, 4, 58);
		SIOScreen.p_ascii = new_panel(SIOScreen.ascii);
	}

	wbkgd(SIOScreen.ascii, COLOR_PAIR(CYAN_BLUE));
	wattrset(SIOScreen.ascii, COLOR_PAIR(CYAN_BLUE) | A_BOLD);
	mvwprintw(SIOScreen.ascii, 0, 0, "");

	wprintw(SIOScreen.ascii, "0123456789ABCDEF");
	int i, j;
	for (unsigned char tmp, i = 0; i < LFDK_BYTE_PER_LINE; i++) {
		for (j = 0; j < LFDK_BYTE_PER_LINE; j++) {
			tmp = ((unsigned char)
					lfdk_sio_data.mass_buf[(i * LFDK_BYTE_PER_LINE) + j]);
			if ((tmp >= '!') && (tmp <= '~')) {
				wprintw(SIOScreen.ascii, "%c", tmp);
			} else {
				wprintw(SIOScreen.ascii, ".");
			}
		}
	}

	wattrset(SIOScreen.ascii, A_NORMAL);

	//
	// Print 256bytes content
	//
	if (!SIOScreen.value) {
		SIOScreen.value = newwin(17, 47, 5, 6);
		SIOScreen.p_value = new_panel(SIOScreen.value);
	}

	wbkgd(SIOScreen.value, COLOR_PAIR(WHITE_BLUE));
	mvwprintw(SIOScreen.value, 0, 0, "");

	for (i = 0; i < LFDK_BYTE_PER_LINE; i++) {
		for (j = 0; j < LFDK_BYTE_PER_LINE; j++) {
			//
			// Change Color Pair
			//
			if (y == j && x == i) {
				if (input) {
					if (counter % 2) {
						wattrset(SIOScreen.value,
								COLOR_PAIR(YELLOW_RED) | A_BOLD);
					} else {
						wattrset(SIOScreen.value,
								COLOR_PAIR(YELLOW_BLACK) | A_BOLD);
					}

					counter++;
				} else {
					wattrset(SIOScreen.value,
							COLOR_PAIR(BLACK_YELLOW) | A_BOLD);
				}
			} else if (((unsigned char)lfdk_sio_data
						.mass_buf[(i * LFDK_BYTE_PER_LINE) + j])) {
				wattrset(SIOScreen.value, COLOR_PAIR(YELLOW_BLUE) | A_BOLD);
			} else {
				wattrset(SIOScreen.value, COLOR_PAIR(WHITE_BLUE) | A_BOLD);
			}

			//
			// Handle input display
			//
			if (y == j && x == i) {
				if (input) {
					wprintw(SIOScreen.value, "%2.2X", (unsigned char)wbuf);
				} else {
					wprintw(SIOScreen.value, "%2.2X",
							(unsigned char)lfdk_sio_data
							.mass_buf[(i * LFDK_BYTE_PER_LINE) + j]);
				}
			} else {
				wprintw(SIOScreen.value, "%2.2X",
						(unsigned char)lfdk_sio_data
						.mass_buf[(i * LFDK_BYTE_PER_LINE) + j]);
			}
			//
			// End of color pair
			//
			wattrset(SIOScreen.value, A_NORMAL);
			//
			// Move to next byte
			//
			if (j != 15) {
				wprintw(SIOScreen.value, " ");
			}
		}
	}
}
