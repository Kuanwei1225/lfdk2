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

extern int ibuf;
extern char enter_mem;
struct cmd_data_t *cmd_data = NULL;
static unsigned int in_counter = 0;
static unsigned char cmd_string[CMD_STRING_SIZE];

static struct cmd_data_t* set_command(char *cmd_str)
{
    char *c, *delim = " ", *saveptr;
    struct cmd_data_t* data;

    data = (struct cmd_data_t*)malloc(sizeof(struct cmd_data_t));
    if (data == NULL)
        return NULL;
    memset(data, 0, sizeof(struct cmd_data_t));
    c = strtok_r(cmd_str, delim, &saveptr);
    for (int i = 0; i < 3; i++) {
        if (c == NULL)
            goto error;
        switch (i) {
        case 0:
            // set command
            if (*c == 'o' || *c == 'O') // only support out command
                data->cmd = 'o';
            else
                goto error;
            break;
        case 1:
            // set address
			set_cmd_data(data->addr, c);
            break;
        case 2:
			// set value
			set_cmd_data(data->val, c);
			break;
		}
        c = strtok_r(NULL, delim, &saveptr);
    }
    return data;
error:
    free(data);
    return NULL;
}
struct cmd_data_t* command_parser(char cmd[])
{
	char cmd_cpy[CMD_STRING_SIZE];
	char *pch, *delim = ";", *saveptr = NULL;
	struct cmd_data_t *ptr = NULL, *head = NULL;

	memcpy(cmd_cpy, cmd, CMD_STRING_SIZE);
	pch = strtok_r(cmd_cpy, delim, &saveptr);
	while (pch != NULL) {
		if (head == NULL) {
			head = set_command(pch);
			if (head == NULL) {
				return NULL;
			}
			if(ioperm(head->addr, 1, 1)){
				printf("IO permission failed.\n");
				return NULL;
			}
			ptr = head;
		} else {
			ptr->next = set_command(pch);
			if (ptr->next == NULL) {
				return head;
			}
			if(ioperm(head->addr, 1, 1)){
				printf("IO permission failed.\n");
			}
			ptr = ptr->next;
		}
		pch = strtok_r(NULL, delim, &saveptr);
    }
    return head;
}

void free_command(struct cmd_data_t *head)
{
    struct cmd_data_t *ptr = head;

	if (head == NULL)
		return;
	do { 
		ptr = head->next;
		ioperm(head->addr, 1, 0);
		free(head);
		head = ptr;
	} while(ptr != NULL);
}
void ClearCMDScreen(void)
{
    DestroyWin(CMDScreen, offset);
    DestroyWin(CMDScreen, info);
    DestroyWin(CMDScreen, value);
    DestroyWin(CMDScreen, ascii);
    free_command(cmd_data);
	cmd_data = NULL;
    cmd_data = command_parser(cmd_string);
}
void PrintCMDScreen(void)
{
    if (enter_mem) {
        if (ibuf == KEY_BACKSPACE) {
            if (in_counter) {
                in_counter--;
                cmd_string[in_counter] = ' ';
            } else {
                cmd_string[0] = '\0';
            }
        } else if ((ibuf >= 'A' && ibuf <= 'Z') || (ibuf >= 'a' && ibuf <= 'z') || (ibuf >= '0' && ibuf <= '9') || (ibuf == 0x0a) || (ibuf == ' ') || (ibuf == ';')) {
            cmd_string[in_counter] = ibuf;
            in_counter++;
            cmd_string[in_counter] = '\0';
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
