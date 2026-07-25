Title
    Building Entry Controller - A Two-Door DES Simulation (CST8244 Assignment 1)

Author
    @author bais0011@algonquinlive.com
    Sifan Baissa - Wrote des_controller.c. Implemted the state machine logic
    @author gumi0002@algonqiunlive.com
    Mateusz Gumienny - Wrote des_inputs.c and des_display.c for implementing prompting and display logic.

Status
    Good news first: all three programs - des_display, des_controller, and
    des_inputs - build cleanly with no errors and no warnings, and they run the
    way the spec describes. I based the whole thing on Lab 5, so the message
    passing follows the same client/server pattern: des_inputs is a client that
    sends events, des_controller is the server that runs the state machine, and
    des_display is a second server that just prints status updates.

    The part I spent the most time on was the controller's state machine. Each
    state is its own handler function, and instead of a big switch statement in
    main, every handler figures out the next state and returns a function
    pointer to it (along with the state's name for printing). Main just keeps
    calling "the current state" and swapping in whatever it returns. Once that
    clicked it actually made the code a lot cleaner to follow.

    A couple of design decisions worth mentioning: the person only gets weighed
    at the entry door, not the exit door, so the door-open states check which
    event comes next (a weigh vs. a close) to tell whether they're the first or
    second door. That let me reuse the same states for people entering AND
    exiting instead of doubling everything. Illegal events don't crash anything
    or jump to some error state - the person just stays put in the current state
    (quarantine), exactly like the spec asks. The exit event shuts all three
    programs down cleanly.

    I tested it against all three required scenarios (LeftRightScan, ExitScan,
    and ErrorScan) and the behaviour matches what I expected - no segfaults, no
    hangs, and the LeftRightScan output lines up with the reference memo.

Known Issues
    Nothing broken that I've found. The one thing to flag is that, like Lab 5,
    the clients connect using channel id 1 (the first channel a server creates),
    so only the server PID gets passed on the command line. That's the
    convention we used in the lab, but I'm mentioning it in case you started the
    programs in a different order than expected.

Expected Grade
    A+