/*
 * LFDK - Linux Firmware Debug Kit
 * File: libmem.c
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
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <ctype.h>

#include <ncurses.h>
#include <panel.h>

#include "lfdk.h"
 
#define MAP_SIZE 4096UL
#define MAP_MASK (MAP_SIZE - 1)

MemPanel MemScreen;
struct lfdk_mem_t lfdk_mem_data;

extern int x, y;
extern int input;
extern unsigned int counter;
extern int ibuf;
extern char wbuf;
extern char enter_mem;

static unsigned long phyaddr = 0;
static unsigned char *map_base = NULL;

static unsigned char* map_memory(const unsigned long mem_addr)
{
	unsigned char *map;
	int fd;
	fd = open(LFDK_MEM_DEV, O_RDWR | O_SYNC);
	if(fd == -1)
		return NULL;
    map = mmap(NULL, MAP_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, fd, mem_addr & ~MAP_MASK);
	close(fd);
	return map;
}

static void WriteMemByteValue(void)
{
	unsigned char *virt_map = map_base + (phyaddr & MAP_MASK);
    lfdk_mem_data.addr = phyaddr + x * LFDK_BYTE_PER_LINE + y;
    lfdk_mem_data.buf = wbuf;
    // write mem here
	virt_map[x*LFDK_BYTE_PER_LINE + y] = wbuf;
}

void ClearMemScreen()
{
    DestroyWin(MemScreen, offset);
    DestroyWin(MemScreen, info);
    DestroyWin(MemScreen, value);
    DestroyWin(MemScreen, ascii);
	if(map_base != NULL) {
		munmap(map_base, LFDK_MASSBUF_SIZE);
		map_base = NULL;
	}
}

void PrintMemScreen(void)
{
	unsigned char *virt_map = NULL;
	int i, j;
	char tmp;

	if (enter_mem) {
		if (ibuf == 0x0a) {
			if(map_base != NULL) {
				munmap(map_base, LFDK_MASSBUF_SIZE);
			}
			map_base = map_memory(phyaddr);
			if(map_base != NULL) {
				enter_mem = 0;
			}
			return;
		} else if (((ibuf >= '0') && (ibuf <= '9'))
				|| ((ibuf >= 'a') && (ibuf <= 'f'))
            || ((ibuf >= 'A') && (ibuf <= 'F'))) {

            phyaddr <<= 4;
            phyaddr &= ~0x0f;

            if (ibuf <= '9') {
                phyaddr |= (unsigned int)(ibuf - 0x30);
            } else if (ibuf > 'F') {
                phyaddr |= (unsigned int)(ibuf - 0x60 + 9);
            } else {
                phyaddr |= (unsigned int)(ibuf - 0x40 + 9);
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
				WriteMemByteValue();
            }
        } else if (((ibuf >= '0') && (ibuf <= '9'))
            || ((ibuf >= 'a') && (ibuf <= 'f'))
            || ((ibuf >= 'A') && (ibuf <= 'F'))) {
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
    PrintFixedWin(MemScreen, offset, 17, 52, 4, 1, RED_BLUE, "0000 00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F0000\n0010\n0020\n0030\n0040\n0050\n0060\n0070\n0080\n0090\n00A0\n00B0\n00C0\n00D0\n00E0\n00F0");
    //
    // Print memory address
    //
    if (!MemScreen.info) {
        MemScreen.info = newwin(1, 47, 22, 0);
        MemScreen.p_info = new_panel(MemScreen.info);
    }
    wbkgd(MemScreen.info, COLOR_PAIR(WHITE_BLUE));
    wattrset(MemScreen.info, COLOR_PAIR(WHITE_BLUE) | A_BOLD);
    mvwprintw(MemScreen.info, 0, 0, "Type: Memory    Address: ");

    if (enter_mem) {

        if (counter % 2) {

            wattrset(MemScreen.info, COLOR_PAIR(YELLOW_RED) | A_BOLD);
        } else {

            wattrset(MemScreen.info, COLOR_PAIR(YELLOW_BLACK) | A_BOLD);
        }
        wprintw(MemScreen.info, "%8.8X", phyaddr);

        counter++;
    } else {

        wattrset(MemScreen.info, COLOR_PAIR(WHITE_BLUE) | A_BOLD);
        wprintw(MemScreen.info, "%8.8X", phyaddr);
    }
    wattrset(MemScreen.info, COLOR_PAIR(WHITE_BLUE) | A_BOLD);
    wprintw(MemScreen.info, "h");
    wattrset(MemScreen.info, A_NORMAL);
    //
    // Read memory space 256 bytes
    //
    if (enter_mem) {

        memset(lfdk_mem_data.mass_buf, 0xff, LFDK_MASSBUF_SIZE);
    } else {
        lfdk_mem_data.addr = phyaddr;
		// read mem here
		virt_map = map_base + (phyaddr & MAP_MASK);
		for(i = 0; i < LFDK_MASSBUF_SIZE; i++) {
			lfdk_mem_data.mass_buf[i] = virt_map[i];
		}
    }

    //
    // Print ASCII content
    //
    if (!MemScreen.ascii) {

        MemScreen.ascii = newwin(17, 16, 4, 58);
        MemScreen.p_ascii = new_panel(MemScreen.ascii);
    }

    wbkgd(MemScreen.ascii, COLOR_PAIR(CYAN_BLUE));
    wattrset(MemScreen.ascii, COLOR_PAIR(CYAN_BLUE) | A_BOLD);
    mvwprintw(MemScreen.ascii, 0, 0, "");

    wprintw(MemScreen.ascii, "0123456789ABCDEF");
    for (i = 0; i < LFDK_BYTE_PER_LINE; i++) {

        for (j = 0; j < LFDK_BYTE_PER_LINE; j++) {

            tmp = ((unsigned char)lfdk_mem_data.mass_buf[(i * LFDK_BYTE_PER_LINE) + j]);
            if ((tmp >= '!') && (tmp <= '~')) {

                wprintw(MemScreen.ascii, "%c", tmp);
            } else {

                wprintw(MemScreen.ascii, ".");
            }
        }
    }

    wattrset(MemScreen.ascii, A_NORMAL);

    //
    // Print 256bytes content
    //
    if (!MemScreen.value) {

        MemScreen.value = newwin(17, 47, 5, 6);
        MemScreen.p_value = new_panel(MemScreen.value);
    }

    wbkgd(MemScreen.value, COLOR_PAIR(WHITE_BLUE));
    mvwprintw(MemScreen.value, 0, 0, "");

    for (i = 0; i < LFDK_BYTE_PER_LINE; i++) {

        for (j = 0; j < LFDK_BYTE_PER_LINE; j++) {

            //
            // Change Color Pair
            //
            if (y == j && x == i) {

                if (input) {

                    if (counter % 2) {

                        wattrset(MemScreen.value, COLOR_PAIR(YELLOW_RED) | A_BOLD);
                    } else {

                        wattrset(MemScreen.value, COLOR_PAIR(YELLOW_BLACK) | A_BOLD);
                    }

                    counter++;
                } else {

                    wattrset(MemScreen.value, COLOR_PAIR(BLACK_YELLOW) | A_BOLD);
                }
            } else if (((unsigned char)lfdk_mem_data.mass_buf[(i * LFDK_BYTE_PER_LINE) + j])) {

                wattrset(MemScreen.value, COLOR_PAIR(YELLOW_BLUE) | A_BOLD);
            } else {

                wattrset(MemScreen.value, COLOR_PAIR(WHITE_BLUE) | A_BOLD);
            }

            //
            // Handle input display
            //
            if (y == j && x == i) {

                if (input) {

                    wprintw(MemScreen.value, "%2.2X", (unsigned char)wbuf);
                } else {

                    wprintw(MemScreen.value, "%2.2X", (unsigned char)lfdk_mem_data.mass_buf[(i * LFDK_BYTE_PER_LINE) + j]);
                }
            } else {

                wprintw(MemScreen.value, "%2.2X", (unsigned char)lfdk_mem_data.mass_buf[(i * LFDK_BYTE_PER_LINE) + j]);
            }

            //
            // End of color pair
            //
            wattrset(MemScreen.value, A_NORMAL);

            //
            // Move to next byte
            //
            if (j != 15) {

                wprintw(MemScreen.value, " ");
            }
        }
    }
}
