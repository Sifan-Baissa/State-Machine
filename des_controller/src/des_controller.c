/*
 * des_controller.c
 * CST8244 - Real-Time Programming - Assignment 1
 *
 * Building entry controller. Runs the finite state machine, keeps the
 * persistent data (person_id, weight), and drives des_display.
 *
 * It is BOTH a message-passing SERVER (to des_inputs) and a CLIENT (to
 * des_display).
 *
 * The FSM uses one handler function per state. Each handler processes the
 * input event and RETURNS the next state (a function pointer plus the name of
 * the newly accepted state). Illegal events keep the person in the same state
 * (quarantine).
 *
 * Usage: des_controller <display_pid>
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/neutrino.h>
#include "des.h"


typedef struct
{
	int display_coid; /* connection to des_display */
	int person_id;    /* last scanned id           */
	int weight;       /* last weighed value        */
} Context;


/* A state handler returns the next state: its function pointer + its name.   */
struct StateResult;
typedef struct StateResult (*StateHandler)(Context *ctx, DisplayMsg *in);
struct StateResult
{
	StateHandler next; /* next state handler (function pointer) */
	const char *name;  /* name of the newly accepted state      */
};
typedef struct StateResult StateResult;

/* forward declarations of every state handler */
static StateResult st_idle(Context *, DisplayMsg *);
static StateResult st_left_scan(Context *, DisplayMsg *);
static StateResult st_right_scan(Context *, DisplayMsg *);
static StateResult st_left_unlocked(Context *, DisplayMsg *);
static StateResult st_right_unlocked(Context *, DisplayMsg *);
static StateResult st_left_open(Context *, DisplayMsg *);
static StateResult st_right_open(Context *, DisplayMsg *);
static StateResult st_weighed(Context *, DisplayMsg *);
static StateResult st_left_closed(Context *, DisplayMsg *);
static StateResult st_right_closed(Context *, DisplayMsg *);
static StateResult st_left_locked(Context *, DisplayMsg *);
static StateResult st_right_locked(Context *, DisplayMsg *);


/* Build a StateResult (next handler + its state name). */
static StateResult go(StateHandler fn, const char *name)
{
	StateResult r;
	r.next = fn;
	r.name = name;
	return r;
}

/* Send a status line to des_display and wait for its EOK reply. */
static void toDisplay(Context *ctx, const char *text)
{
	DisplayMsg out;
	out.event = -1; /* not the exit event */
	out.data = 0;
	strncpy(out.text, text, sizeof(out.text) - 1);
	out.text[sizeof(out.text) - 1] = '\0';
	MsgSend(ctx->display_coid, &out, sizeof(out), NULL, 0);
}


/* IDLE / START: wait for a person to scan at either door. */
static StateResult st_idle(Context *ctx, DisplayMsg *in)
{
	char buf[80];
	switch (in->event)
	{
	case LS_EVENT:
		ctx->person_id = in->data;
		sprintf(buf, "Left scan: person %d has been scanned", in->data);
		toDisplay(ctx, buf);
		return go(st_left_scan, "LEFT_SCAN_STATE");
	case RS_EVENT:
		ctx->person_id = in->data;
		sprintf(buf, "Right scan: person %d has been scanned", in->data);
		toDisplay(ctx, buf);
		return go(st_right_scan, "RIGHT_SCAN_STATE");
	default:
		return go(st_idle, "IDLE_STATE"); /* quarantine */
	}
}

/* LEFT_SCAN: wait for the guard to unlock the left door. */
static StateResult st_left_scan(Context *ctx, DisplayMsg *in)
{
	if (in->event == GLU_EVENT)
	{
		toDisplay(ctx, "Guard unlocks the left door");
		return go(st_left_unlocked, "LEFT_UNLOCKED_STATE");
	}
	return go(st_left_scan, "LEFT_SCAN_STATE");
}

/* RIGHT_SCAN: wait for the guard to unlock the right door. */
static StateResult st_right_scan(Context *ctx, DisplayMsg *in)
{
	if (in->event == GRU_EVENT)
	{
		toDisplay(ctx, "Guard unlocks the right door");
		return go(st_right_unlocked, "RIGHT_UNLOCKED_STATE");
	}
	return go(st_right_scan, "RIGHT_SCAN_STATE");
}

/* LEFT_UNLOCKED: wait for the left door to open. */
static StateResult st_left_unlocked(Context *ctx, DisplayMsg *in)
{
	if (in->event == LO_EVENT)
	{
		toDisplay(ctx, "Left door is open");
		return go(st_left_open, "LEFT_OPEN_STATE");
	}
	return go(st_left_unlocked, "LEFT_UNLOCKED_STATE");
}

/* RIGHT_UNLOCKED: wait for the right door to open. */
static StateResult st_right_unlocked(Context *ctx, DisplayMsg *in)
{
	if (in->event == RO_EVENT)
	{
		toDisplay(ctx, "Right door is open");
		return go(st_right_open, "RIGHT_OPEN_STATE");
	}
	return go(st_right_unlocked, "RIGHT_UNLOCKED_STATE");
}

/*
 * LEFT_OPEN:
 *   - If the left door is the FIRST (entry) door, the person is weighed (ws).
 *   - If the left door is the SECOND (exit) door, the person just leaves and
 *     the door closes (lc); no second weighing.
 */
static StateResult st_left_open(Context *ctx, DisplayMsg *in)
{
	char buf[80];
	switch (in->event)
	{
	case WS_EVENT:
		ctx->weight = in->data;
		sprintf(buf, "Person's weight is %d", in->data);
		toDisplay(ctx, buf);
		return go(st_weighed, "WEIGHED_STATE");
	case LC_EVENT:
		toDisplay(ctx, "Left door is closed");
		return go(st_left_closed, "LEFT_CLOSED_STATE");
	default:
		return go(st_left_open, "LEFT_OPEN_STATE");
	}
}

/*
 * RIGHT_OPEN:
 *   - If the right door is the FIRST (entry) door, the person is weighed (ws).
 *   - If the right door is the SECOND (exit) door, the person just leaves and
 *     the door closes (rc); no second weighing.
 */
static StateResult st_right_open(Context *ctx, DisplayMsg *in)
{
	char buf[80];
	switch (in->event)
	{
	case WS_EVENT:
		ctx->weight = in->data;
		sprintf(buf, "Person's weight is %d", in->data);
		toDisplay(ctx, buf);
		return go(st_weighed, "WEIGHED_STATE");
	case RC_EVENT:
		toDisplay(ctx, "Right door is closed");
		return go(st_right_closed, "RIGHT_CLOSED_STATE");
	default:
		return go(st_right_open, "RIGHT_OPEN_STATE");
	}
}

/* WEIGHED: wait for the (first) door to close. */
static StateResult st_weighed(Context *ctx, DisplayMsg *in)
{
	switch (in->event)
	{
	case LC_EVENT:
		toDisplay(ctx, "Left door is closed");
		return go(st_left_closed, "LEFT_CLOSED_STATE");
	case RC_EVENT:
		toDisplay(ctx, "Right door is closed");
		return go(st_right_closed, "RIGHT_CLOSED_STATE");
	default:
		return go(st_weighed, "WEIGHED_STATE");
	}
}

/* LEFT_CLOSED: wait for the guard to lock the left door. */
static StateResult st_left_closed(Context *ctx, DisplayMsg *in)
{
	if (in->event == GLL_EVENT)
	{
		toDisplay(ctx, "Guard locks the left door");
		return go(st_left_locked, "LEFT_LOCKED_STATE");
	}
	return go(st_left_closed, "LEFT_CLOSED_STATE");
}

/* RIGHT_CLOSED: wait for the guard to lock the right door. */
static StateResult st_right_closed(Context *ctx, DisplayMsg *in)
{
	if (in->event == GRL_EVENT)
	{
		toDisplay(ctx, "Guard locks the right door");
		return go(st_right_locked, "RIGHT_LOCKED_STATE");
	}
	return go(st_right_closed, "RIGHT_CLOSED_STATE");
}

/*
 * LEFT_LOCKED:
 *   - If left was the FIRST door (person entering from the left), the guard
 *     now unlocks the right door (gru) to let the person into the building.
 *   - If left was the SECOND door (person leaving from the right), the cycle
 *     is complete and the machine loops back to IDLE, ready for a new scan.
 */
static StateResult st_left_locked(Context *ctx, DisplayMsg *in)
{
	char buf[80];
	switch (in->event)
	{
	case GRU_EVENT:
		toDisplay(ctx, "Guard unlocks the right door");
		return go(st_right_unlocked, "RIGHT_UNLOCKED_STATE");
	case LS_EVENT: /* loop back to IDLE: new person, left scan */
		ctx->person_id = in->data;
		sprintf(buf, "Left scan: person %d has been scanned", in->data);
		toDisplay(ctx, buf);
		return go(st_left_scan, "LEFT_SCAN_STATE");
	case RS_EVENT: /* loop back to IDLE: new person, right scan */
		ctx->person_id = in->data;
		sprintf(buf, "Right scan: person %d has been scanned", in->data);
		toDisplay(ctx, buf);
		return go(st_right_scan, "RIGHT_SCAN_STATE");
	default:
		return go(st_left_locked, "LEFT_LOCKED_STATE");
	}
}

/*
 * RIGHT_LOCKED:
 *   - If right was the FIRST door (person leaving from the right), the guard
 *     now unlocks the left door (glu).
 *   - If right was the SECOND door (person entering from the left), the cycle
 *     is complete and the machine loops back to IDLE, ready for a new scan.
 */
static StateResult st_right_locked(Context *ctx, DisplayMsg *in)
{
	char buf[80];
	switch (in->event)
	{
	case GLU_EVENT:
		toDisplay(ctx, "Guard unlocks the left door");
		return go(st_left_unlocked, "LEFT_UNLOCKED_STATE");
	case LS_EVENT: /* loop back to IDLE: new person, left scan */
		ctx->person_id = in->data;
		sprintf(buf, "Left scan: person %d has been scanned", in->data);
		toDisplay(ctx, buf);
		return go(st_left_scan, "LEFT_SCAN_STATE");
	case RS_EVENT: /* loop back to IDLE: new person, right scan */
		ctx->person_id = in->data;
		sprintf(buf, "Right scan: person %d has been scanned", in->data);
		toDisplay(ctx, buf);
		return go(st_right_scan, "RIGHT_SCAN_STATE");
	default:
		return go(st_right_locked, "RIGHT_LOCKED_STATE");
	}
}


int main(int argc, char *argv[])
{
	Context ctx;
	int chid;
	int rcvid;
	DisplayMsg in;
	DisplayMsg bye;
	StateHandler current;
	const char *currentName;
	StateResult r;

	/* Command-line argument: display's process id. */
	if (argc != 2)
	{
		fprintf(stderr, "usage: %s <display_pid>\n", argv[0]);
		return EXIT_FAILURE;
	}

	ctx.person_id = 0;
	ctx.weight = 0;

	/* SERVER: create the channel that des_inputs will connect to. */
	chid = ChannelCreate(0);
	if (chid == -1)
	{
		perror("ChannelCreate");
		return EXIT_FAILURE;
	}

	/* CLIENT: attach to des_display. */
	ctx.display_coid = ConnectAttach(0, atoi(argv[1]), DES_CHANNEL, 0, 0);
	if (ctx.display_coid == -1)
	{
		perror("ConnectAttach (could not attach to display)");
		ChannelDestroy(chid);
		return EXIT_FAILURE;
	}

	printf("The controller is running as process_id %d\n", getpid());
	fflush(stdout);

	current = st_idle;
	currentName = "IDLE_STATE";

	while (1)
	{
		rcvid = MsgReceive(chid, &in, sizeof(in), NULL);
		if (rcvid == -1)
		{
			continue;
		}

		/* Reply immediately so des_inputs unblocks (EOK). */
		MsgReply(rcvid, EOK, NULL, 0);

		/* exit terminates gracefully from any state. */
		if (in.event == EXIT_EVENT)
		{
			printf("Controller: %s -> EXIT_STATE\n", currentName);
			fflush(stdout);
			break;
		}

		/* Run the current state handler; it returns the next state. */
		r = current(&ctx, &in);

		if (r.next == current)
		{
			/* No transition: illegal event for this state -> quarantine. */
			printf("Controller: illegal event '%s' in %s; person quarantined "
				   "(staying in same state).\n",
				   eventToken[in.event], currentName);
		}
		else
		{
			printf("Controller: %s -> %s\n", currentName, r.name);
		}
		fflush(stdout);

		current = r.next;
		currentName = r.name;
	}

	/* Tell the display to shut down, then clean up. */
	bye.event = EXIT_EVENT;
	bye.data = 0;
	strcpy(bye.text, "exit");
	MsgSend(ctx.display_coid, &bye, sizeof(bye), NULL, 0);

	ConnectDetach(ctx.display_coid);
	ChannelDestroy(chid);
	printf("Controller terminating.\n");
	return EXIT_SUCCESS;
}
