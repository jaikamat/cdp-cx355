// Graphical ANSI Library (GAL)
// Input Control Base

#ifndef CONTROL_BASE
#define CONTROL_BASE

#include <signal.h>
#include <stdio.h>
#include <unistd.h>

#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/types.h>

#include "gal/ctrlbase.h"
#include "gal/dibase.h"
#include "std/ansi.h"

// Constructor; Configure Terminal
ControlBase::ControlBase(void) {

  // NULL Interface
  interface_ptr = NULL;

  // Save old term config
  ioctl(0, TCGETA, &orig);
  memcpy(&raw, &orig, sizeof(struct termio));

  // Set new term config
  raw.c_lflag &=~ (ICANON | ECHO);
  raw.c_cc[VEOL] = 1;
  raw.c_cc[VEOF] = 2;

  ioctl(0, TCSETA, &raw);

  // Set non buffered streams
  setvbuf(stdin, NULL, _IONBF, 0);
  setvbuf(stdout, NULL, _IONBF, 0);
} /* ControlBase() */

// Destructor
ControlBase::~ControlBase(void) {
  delete interface_ptr;

  // Restore original term config
  ioctl(0, TCSETA, &orig);
} /* ~ControlBase() */

int ControlBase::Setup(void) {

  // Some form of interface is required
  if (interface_ptr == NULL) {
    fprintf(stdout, "Display Interface Required.\n");
    return 0;
  }

  // Begin processing input
  ProcessInput();

  return 1;
} /* Setup() */

int ControlBase::Shutdown(void) {

  fprintf(stdout, RESET_ASC LOC FILL "Shutting down cleanly.\n\n",
          1, 1, BLACK);
  state = EXIT;

  return 1;
} /* Shutdown() */

int ControlBase::SetInterface(InterfaceBase *ptr) { 

  delete interface_ptr;
  (interface_ptr = ptr) -> Setup();

  return 1;
} /* SetInterface() */

void ControlBase::ProcessInput(void) {
  int jiffies, flag = 0, index = 0, key;
  char pending[CTRL_CODE_SIZE] = "\0";
  struct timeval idle, tv1, tv2;
  struct timezone tz;

  jiffies = 1000000 / JIFFIES_PER_SEC;
  state = IDLE;

  while (state != EXIT) {
    state = PROCESSING;
    jiffie++;

    // Aquire starting time
    gettimeofday(&tv1, &tz);

    // Keyboard check time
    idle.tv_sec = 0;
    idle.tv_usec = 10;

    // Bind stdin to input;
    FD_ZERO(&input);
    FD_SET(0 /* stdin */, &input);

    // Allow events in the interface (timers and such)

    interface_ptr -> ProcessEvents(jiffie);

    // Allow events in the parent class
    ProcessEvents(jiffie);

    if (select(1, &input, NULL, NULL, &idle)) {
      
      if ((key = getchar()) == 27) {
        if (flag) {
          pending[index] = 0;
          interface_ptr -> ProcessInput(pending);
        }

        flag = 1;
        index = 0;
        pending[index++] = key;

      } else {
        pending[index++] = key;

        if ((flag) && (index >= (CTRL_CODE_SIZE - 1)))
          flag = 0;

        if (!flag) {
          pending[index] = 0;
          interface_ptr -> ProcessInput(pending);
          index = 0;
        }
      }
    } else {
      if (index) {
        pending[index] = 0;
        interface_ptr -> ProcessInput(pending);

        flag = 0;
        index = 0;
      }
    }

    // Aquire completion time
    gettimeofday(&tv2, &tz);

    // How much of our '1 / idle.tv_usec' second remains?
    idle.tv_sec = 0;
    idle.tv_usec = jiffies - ((tv2.tv_usec + tv2.tv_sec * 1000000) -
                   (tv1.tv_usec + tv1.tv_sec * 1000000));

    if (state != EXIT)
      state = IDLE;

    // Return control to OS for remaining interval
    if (idle.tv_usec > 0)
      select(0, NULL, NULL, NULL, &idle);
  }
} /* ProcessInput() */

#endif // CONTROL_BASE
