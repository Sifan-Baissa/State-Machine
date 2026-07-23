/*
 * des_display.c
 * CST8244 - Real-Time Programming - Assignment 1
 *
 * Status display. A QNX message-passing SERVER: it receives a status message
 * from des_controller, replies EOK, and prints the status line to stdout.
 * It runs in the background and loops until it receives the exit event.
 *
 * Usage: des_display
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/neutrino.h>
#include "des.h"

int main(void)
{
	int chid;
	int rcvid;
	DisplayMsg msg;

	/* SERVER: create the channel the controller will connect to. */
	chid = ChannelCreate(0);
	if (chid == -1)
	{
		perror("ChannelCreate");
		return EXIT_FAILURE;
	}

	printf("The display is running as process_id %d\n", getpid());
	printf("[display] status update : initial startup\n");
	fflush(stdout);

	while (1)
	{
		rcvid = MsgReceive(chid, &msg, sizeof(msg), NULL);
		if (rcvid == -1)
		{
			continue; /* receive error - keep serving */
		}

		/* Always reply so the controller (client) unblocks. */
		MsgReply(rcvid, EOK, NULL, 0);

		if (msg.event == EXIT_EVENT)
		{
			printf("[display] Exiting.\n");
			fflush(stdout);
			break;
		}

		/* Print the status line composed by the controller. */
		printf("[display] %s\n", msg.text);
		fflush(stdout);
	}

	ChannelDestroy(chid);
	printf("des_display terminating.\n");
	return EXIT_SUCCESS;
}
