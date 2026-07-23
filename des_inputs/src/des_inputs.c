/*
 * des_inputs.c
 * CST8244 - Real-Time Programming - Assignment 1
 *
 * Simulates all of the input devices (card scanners, door latches, scale,
 * guard switches). It prompts the user for an event, builds a "person object"
 * (DisplayMsg), and sends it to des_controller. It is a QNX message-passing
 * CLIENT.
 *
 * Usage: des_inputs <controller_pid>
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/neutrino.h>
#include "des.h"

/* Return the Event enum value for a typed token, or -1 if unrecognized. */
static int lookupEvent(const char *token) {
	int i;
	for (i = 0; i < NUM_EVENTS; i++) {
		if (strcmp(token, eventToken[i]) == 0) {
			return i;
		}
	}
	return -1;
}

int main(int argc, char *argv[]) {
	int coid;
	int ev;
	char token[80];
	DisplayMsg msg;

	/* Command-line argument: controller's process id. */
	if (argc != 2) {
		fprintf(stderr, "usage: %s <controller_pid>\n", argv[0]);
		return EXIT_FAILURE;
	}

	/* CLIENT: attach to the controller's channel. */
	coid = ConnectAttach(0, atoi(argv[1]), DES_CHANNEL, 0, 0);
	if (coid == -1) {
		perror("ConnectAttach (could not attach to controller)");
		return EXIT_FAILURE;
	}

	printf("The des_inputs client is running as process_id %d\n", getpid());
	fflush(stdout);

	while (1) {
		printf("\nEnter the event type (ls= left scan, rs= right scan, "
				"ws= weight scale, lo= left open, ro= right open, "
				"lc= left closed, rc= right closed, gru= guard right unlock, "
				"grl= guard right lock, gll= guard left lock, "
				"glu= guard left unlock, exit): ");
		fflush(stdout);

		if (scanf("%79s", token) != 1) {
			break; /* end of input */
		}

		ev = lookupEvent(token);
		if (ev == -1) {
			printf("  '%s' is not a valid event; ignored.\n", token);
			continue; /* do not send junk to the controller */
		}

		msg.event = ev;
		msg.data = 0;
		msg.text[0] = '\0';

		/* Extra prompt for the events that carry an integer. */
		if (ev == LS_EVENT || ev == RS_EVENT) {
			printf("Enter the person_id: ");
			fflush(stdout);
			scanf("%d", &msg.data);
		} else if (ev == WS_EVENT) {
			printf("Enter the weight: ");
			fflush(stdout);
			scanf("%d", &msg.data);
		}

		/* Send the person object to the controller (empty reply = EOK). */
		if (MsgSend(coid, &msg, sizeof(msg), NULL, 0) == -1) {
			perror("MsgSend");
			break;
		}

		if (ev == EXIT_EVENT) {
			break;
		}
	}

	ConnectDetach(coid);
	printf("des_inputs terminating.\n");
	return EXIT_SUCCESS;
}
