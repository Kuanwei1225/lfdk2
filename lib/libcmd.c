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

#define CMD_STRING_SIZE 256

MemPanel CMDScreen;

extern int x, y;
extern int input;
extern unsigned int counter;
extern int ibuf;
extern char wbuf;
extern char enter_mem;
struct cmd_data_t *cmd_data;
static unsigned int in_counter = 0;
static unsigned char cmd_string[CMD_STRING_SIZE];

static struct cmd_data_t* set_command(unsigned char *cmd_string) {
	unsigned char *c, *delim = " ";
	struct cmd_data_t *data;
	
	data = (struct cmd_data_t*)malloc(sizeof(struct cmd_data_t));
	if(data == NULL)
		return NULL;
	memset(data, 0, sizeof(struct cmd_data_t));
	c = strtok(cmd_string, delim);
	for (int i = 0; i < 3; i++) {
		if(c == NULL)
			goto error;
		switch (i)
		{
			case 0:
				// set command
				if(*c == 'o' || *c == 'O') // only support out command
					data->cmd = 'o';			
				else
					goto error;
				break;
			case 1:
				// set address
				for(; *c != '\0'; c++) {
					data->addr <<= 4;
					if(*c >= '0' && *c <= '9') {
						data->addr |= *c - '0';
					} else if (*c >= 'a' && *c <= 'f') {
						data->addr |= *c - 'a';
					} else if (*c >= 'A' && *c <= 'F') {
						data->addr |= *c - 'A';
					}
				}
				break;
			case 2:
				// set address
				for( ; *c != '\0'; c++) {
					data->val <<= 4;
					if(*c >= '0' && *c <= '9') {
						data->val |= *c - '0';
					} else if (*c >= 'a' && *c <= 'f') {
						data->val |= *c - 'a';
					} else if (*c >= 'A' && *c <= 'F') {
						data->val |= *c - 'A';
					}
				}
				break;
		}	
		c = strtok(NULL, delim);
	}			
	return data;
error:
	free(data);
	return NULL;
}
struct cmd_data_t* command_parser(unsigned char *cmd) {
    unsigned char *pch, *delim = "\n";
	struct cmd_data_t *ptr, *head;

	pch = strtok(cmd, delim);
	while (pch != NULL) {
		if(head == NULL) {
			head = set_command(pch);
			if(head == NULL) {
				return NULL;
			}
			ptr = head->next;
		} else {
			ptr = set_command(pch);
			if(ptr == NULL) {
				return head;
			}
			ptr = ptr->next;
		}
		pch = strtok(NULL, delim);
	}
	return head;
}

static void free_command(struct cmd_data_t *cmd) { 
	struct cmd_data_t *ptr, *head;
	
	if(cmd == NULL)
		return;
	for(head = ptr = cmd; ptr != NULL; ) {
		ptr = ptr->next;
		free(head);
		head = NULL;
		head = ptr;
	}
}
void ClearCMDScreen(void) {
    DestroyWin(CMDScreen, offset);
    DestroyWin(CMDScreen, info);
    DestroyWin(CMDScreen, value);
    DestroyWin(CMDScreen, ascii);
//	free_command(cmd_data);
    cmd_data = command_parser(cmd_string);
}
void PrintCMDScreen(void) {
	if (enter_mem) {
		if (ibuf == KEY_BACKSPACE) {
			if (in_counter) {
				in_counter--;
				cmd_string[in_counter] = ' ';
			} else {
				cmd_string[0] = '\0';
			}
		} else if ((ibuf >= 'A' && ibuf <= 'Z') ||
				(ibuf >= 'a' && ibuf <= 'z') ||
				(ibuf >= '0' && ibuf <= '9') || (ibuf == 0x0a) || (ibuf == ' ') ) {
			cmd_string[in_counter] = ibuf;
			in_counter++;
			cmd_string[in_counter] = '\n';
			cmd_string[in_counter + 1] = '\0';
			in_counter %= CMD_STRING_SIZE - 1;
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
    mvwprintw(CMDScreen.info, 0, 0, "Type: Command");
    wattrset(CMDScreen.value, A_NORMAL);
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
    for (int i = 0; i < LFDK_BYTE_PER_LINE; i++) {
	for (int j = 0; j < LFDK_BYTE_PER_LINE; j++) {
	    wprintw(CMDScreen.ascii, ".");
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
    wprintw(CMDScreen.value, "%s", cmd_string);
}
