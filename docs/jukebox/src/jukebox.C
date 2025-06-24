// CD Jukebox Control Program

#include <jukebox.h>

int JBControl::Setup(void) {

  fprintf(stdout, RESET_ASC LOC FILL "Loading CD Jukebox\n\n",
          1, 1, BLACK);

  cd_player = new CDPlayer();
  cd_library = new Library();

  // Setup CD player support
  if (!cd_player->Setup(JIFFIES_PER_SEC)) {
    fprintf(stderr, "Make sure you have properly installed the "
            "S-Link device driver.\n"); 
    return 0;
  }

  // Setup the CD Library
  cd_library->Setup(QueryCDPlayer());

  SetInterface(new JBInterfaceStartup(this));
  ControlBase::Setup();

  return 1;
} /* Setup() */

int JBControl::Shutdown(void) {

  // Shutdown interface
  ControlBase::Shutdown();

  // Shutdown CD player support
  cd_player->Shutdown();
  cd_library->Close();

  delete cd_player;
  delete cd_library;

  return 1;
} /* Shutdown() */

int JBControl::ProcessEvents(unsigned long jiffie) {

  cd_player->ProcessEvents(jiffie);
//  cd_database.ProcessEvents(jiffie);
  ControlBase::ProcessEvents(jiffie);

  return 1;
} /* ProcessEvents() */

// Some error logging
int PostError(const char *msg) {
  FILE *errorlog;
  static int lineno=0;
  char buf[81];
  
  bzero(buf, 81);

  if (!lineno) {
    if ((errorlog=fopen("errorlog", "w")) != NULL) { 
      fwrite("--- Start of error log. ---\n", 28, 1, errorlog);
      fclose(errorlog);
      lineno++;
      return 0;
    }
  } 

  if ((errorlog=fopen("errorlog", "a")) != NULL) {
    sprintf(buf, "%8d: ", lineno);
    strncpy(buf + 9, msg, 70);
    buf[strlen(buf)] = '\n';
    fwrite(buf, strlen(buf), 1, errorlog);
    fclose(errorlog);
    lineno++;
    return 0;
  }

  return 1;
}

// Launch Control Object
int main(int /* argc */, char * /* argv */ ) {
  JBControl ctrl;

  ctrl.Setup();

  return 1;
} /* main() */
