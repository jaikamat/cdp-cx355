// CD-DB (DI)

#ifndef DISPLAY_INTERFACE_CDDB_H
#define DISPLAY_INTERFACE_CDDB_H

#define THE ", The"
#define _A_ ", A"

class Identify;

class JBInterfaceCDDB : public InterfaceBase {
  private:
    Library *library;
//    CDDatabase *cddb;
    CDPlayer *cd_player;
    Identify *buttons[2];
    LinkedList<struct disc *> discs[2];
    char status[2][35];

    int ColorizeTable(char *, int, int, long);
    void RefreshTable(int, int);
    struct disc *Import(char *, int, int);

  public:
    JBInterfaceCDDB(void) : InterfaceBase(NULL) { }
    JBInterfaceCDDB(ControlBase *ptr) : InterfaceBase(ptr) { }
    virtual ~JBInterfaceCDDB(void) { }

    int Setup(void);
    int ProcessInput(const char *);
    int ProcessEvents(unsigned long);
    int Refresh(int);

}; /* class JBInterfaceCDDB */

#endif // DISPLAY_INTERFACE_CDDB_H
