#ifndef JUKEBOX_H
#define JUKEBOX_H

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "std/ansi.h"
#include "std/bit.h"
#include "std/cdplayer.h"
#include "std/deck.h"
#include "std/disc.h"
#include "std/global.h"
#include "std/track.h"

class CDPlayer;
class Library;

class AButton;
class ACheck;
class AColorStaticList;
class ADelList;
class AEdit;
class AEditList;
class AStatic;
class AStaticList;
class AWindow;

#include "protocol/jukebox.h"
#include "protocol/cdplayer.h"
#include "protocol/library.h"

#include "protocol/artist.h"
#include "protocol/ddt.h"
#include "protocol/title.h"

#include "ds/treenode.h"
#include "ds/bstree.h"
#include "ds/listnode.h"
#include "ds/linkedlist.h"
#include "ds/string.h"

#include "gal/awindow.h"
#include "gal/astatic.h"
#include "gal/alist.h"
#include "gal/aedit.h"
#include "gal/astaticlist.h"
#include "gal/abutton.h"
#include "gal/acheck.h"
#include "gal/adellist.h"
#include "gal/aeditlist.h"
#include "gal/ctrlbase.h"
#include "gal/dibase.h"

#include "iface/cddb.h"
#include "iface/editor.h"
#include "iface/jukebox.h"
#include "iface/remote.h"
#include "iface/startup.h"


#endif // JUKEBOX_H
