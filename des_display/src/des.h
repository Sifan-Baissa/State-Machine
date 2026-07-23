/*
 * des.h
 * CST8244 - Real-Time Programming - Assignment 1
 * Building Entry Controller (Discrete Event System)
 *
 * Single, shared header used by des_inputs, des_controller and des_display.
 * It defines the input events, their text tokens, and the message object
 * that is passed between the programs.
 */
#ifndef DES_H_
#define DES_H_

/*
 * All three programs use the QNX "first channel" convention from Lab 5:
 * a server's ChannelCreate(0) returns channel id 1, so a client only needs
 * the server's process id (given on the command line) to ConnectAttach().
 */
#define DES_CHANNEL 1

/* ---------------------------------------------------------------------------
 * Input events (the DES "alphabet").
 * ls/rs carry a person_id, ws carries a weight; the rest carry no data.
 * ------------------------------------------------------------------------- */
#define NUM_EVENTS 12
typedef enum
{
	LS_EVENT = 0, /* left scan   (carries person_id)  */
	RS_EVENT,     /* right scan  (carries person_id)  */
	WS_EVENT,     /* weigh scale (carries weight)     */
	LO_EVENT,     /* left door opened                 */
	RO_EVENT,     /* right door opened                */
	LC_EVENT,     /* left door closed                 */
	RC_EVENT,     /* right door closed                */
	GLU_EVENT,    /* guard unlocks left door          */
	GLL_EVENT,    /* guard locks   left door          */
	GRU_EVENT,    /* guard unlocks right door         */
	GRL_EVENT,    /* guard locks   right door         */
	EXIT_EVENT    /* shut the system down             */
} Event;

/* Text tokens the user types, index-aligned with the Event enum above.
 * (Marked "unused" so the header stays warning-free in files that don't
 *  reference it, e.g. des_display.c.) */
static const char *eventToken[NUM_EVENTS] __attribute__((unused)) = {
	"ls", "rs", "ws", "lo", "ro", "lc", "rc",
	"glu", "gll", "gru", "grl", "exit"};

/* ---------------------------------------------------------------------------
 * Message object.
 *   des_inputs   -> des_controller : event + data
 *   des_controller -> des_display  : event + data + text (line to print)
 * ------------------------------------------------------------------------- */
typedef struct
{
	int event;     /* one of Event                       */
	int data;      /* person_id, weight, or 0            */
	char text[80]; /* status line composed by controller */
} DisplayMsg;

#endif /* DES_H_ */
