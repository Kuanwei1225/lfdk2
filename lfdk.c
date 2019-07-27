/*
 * LFDK - Linux Firmware Debug Kit
 * File: lfdk.c
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
#include <time.h>
#include <unistd.h>

#include <ncurses.h>
#include <panel.h>

#include "lib/lfdk.h"

static BasePanel BaseScreen;

int x = 0, y = 0;
int curr_index = 0, last_index;
int input = 0;
unsigned int counter = 0;
int ibuf;
char wbuf;
int func = IO_SPACE_FUNC;
int maxpcibus = 255;
char pciname[LFDK_MAX_PATH];
char enter_mem = 0;

void PrintBaseScreen(void);

static void usage(void) {
	fprintf(stderr, "\n" LFDK_VERTEXT "\n");
	fprintf(stderr,
			"Copyright (C) 2006 - 2010, Merck Hung <merckhung@gmail.com>\n");
	fprintf(stderr, "Usage: " LFDK_PROGNAME
			" [-h] [-d /dev/lfdd] [-n ./pci.ids] [-b 255]\n");
	fprintf(stderr,
			"\t-n\tFilename of PCI Name Database, default is "
			"/usr/share/misc/pci.ids\n");
	fprintf(stderr,
			"\t-d\tDevice name of Linux Firmware Debug Driver, default is "
			"/dev/lfdd\n");
	fprintf(stderr, "\t-b\tMaximum PCI Bus number to scan, default is 255\n");
	fprintf(stderr, "\t-h\tprint this message.\n");
	fprintf(stderr, "\n");
}

void InitColorPairs(void) {
	init_pair(WHITE_RED, COLOR_WHITE, COLOR_RED);
	init_pair(WHITE_BLUE, COLOR_WHITE, COLOR_BLUE);
	init_pair(BLACK_WHITE, COLOR_BLACK, COLOR_WHITE);
	init_pair(CYAN_BLUE, COLOR_CYAN, COLOR_BLUE);
	init_pair(RED_BLUE, COLOR_RED, COLOR_BLUE);
	init_pair(YELLOW_BLUE, COLOR_YELLOW, COLOR_BLUE);
	init_pair(BLACK_GREEN, COLOR_BLACK, COLOR_GREEN);
	init_pair(BLACK_YELLOW, COLOR_BLACK, COLOR_YELLOW);
	init_pair(YELLOW_RED, COLOR_YELLOW, COLOR_RED);
	init_pair(YELLOW_BLACK, COLOR_YELLOW, COLOR_BLACK);
	init_pair(WHITE_YELLOW, COLOR_WHITE, COLOR_YELLOW);
}

void PrintBaseScreen(void) {
	//
	// Background Color
	//
	PrintWin(BaseScreen, bg, 23, 80, 0, 0, WHITE_BLUE, "");

	//
	// Base Screen
	//
	PrintWin(BaseScreen, logo, 1, 80, 0, 0, WHITE_RED,
			"Linux Firmware Debug Kit " LFDK_VERSION);
	PrintWin(BaseScreen, copyright, 1, 32, 0, 48, WHITE_RED,
			"Merck Hung <merckhung@gmail.com>");
	PrintWin(BaseScreen, help, 1, 80, 23, 0, BLACK_WHITE,
			"(Q)uit (F1)IO (F2)SIO (F3)Mem ");

	update_panels();
	doupdate();
}

int main(int argc, char** argv) {
	char c, device[LFDK_MAX_PATH];
	int i, fd, orig_fl, ret;

	struct tm* nowtime;
	time_t timer;
	int last_sec;

	//
	// Ncurse start
	//
	initscr();
	start_color();
	cbreak();
	noecho();
	nodelay(stdscr, 1);
	keypad(stdscr, 1);
	curs_set(0);

	//
	// Initialize color pairs for later use
	//
	InitColorPairs();

	//
	// Prepare Base Screen
	//
	PrintBaseScreen();

	if (ioperm(0, LFDK_MASSBUF_SIZE, 1)) {
		printf("IO permission failed.\n");
		exit(EXIT_FAILURE);
	}
	for (;;) {
		ibuf = getch();
		if (ibuf == 27) {
			//
			// Exit when ESC and Q pressed
			//
			break;
		}

		//
		// Major function switch key binding
		//
		if (ibuf == KEY_F(1)) {
			enter_mem = 1;
			func = IO_SPACE_FUNC;
			ClearMemScreen();
			ClearSIOScreen();
			ClearCMDScreen();
			continue;
		} else if (ibuf == KEY_F(2)) {
			enter_mem = 1;
			func = SIO_SPACE_FUNC;
			ClearIOScreen();
			ClearMemScreen();
			ClearCMDScreen();
			continue;
		} else if (ibuf == KEY_F(3)) {
			enter_mem = 1;
			func = MEM_SPACE_FUNC;
			ClearIOScreen();
			ClearSIOScreen();
			ClearCMDScreen();
			continue;
		} else if (ibuf == KEY_F(8)) {
			enter_mem = 1;
			func = CMD_SPACE_FUNC;
			ClearIOScreen();
			ClearMemScreen();
			ClearSIOScreen();
			continue;
		}
		//
		// Update timer
		//
		time(&timer);
		nowtime = localtime(&timer);
		last_sec = nowtime->tm_sec;

		// Skip redundant update of timer
		if (last_sec == nowtime->tm_sec) {
			PrintFixedWin(BaseScreen, time, 1, 8, 23, 71, BLACK_WHITE,
					"%2.2d:%2.2d:%2.2d", nowtime->tm_hour,
					nowtime->tm_min, nowtime->tm_sec);
		}

		//
		// Major Functions
		//
		switch (func) {

			case IO_SPACE_FUNC:
				PrintIOScreen();
				break;
			case SIO_SPACE_FUNC:
				PrintSIOScreen();
				break;
			case MEM_SPACE_FUNC:
				PrintMemScreen(fd);
				break;
			case CMD_SPACE_FUNC:
				PrintCMDScreen();
				break;
			default:
				break;
		}

		//
		// Refresh Screen
		//
		update_panels();
		doupdate();

		usleep(100000); // 100 ms
	}

	endwin();
	close(fd);

	fprintf(stderr, "\n");
	usage();

	return 0;
}
