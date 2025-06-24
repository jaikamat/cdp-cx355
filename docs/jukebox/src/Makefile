# This really should be a recursive makefile but since I don't exect
# to invest any more real time in this project I'm not going to bother.

CC=g++
DFLAGS=-g -O2 -Wall -Wstrict-prototypes
CFLAGS=-I$(PWD)/../include/
CLIBS =

JUKEBOX_H=
OBJS=jukebox.o ds/bstree.o ds/treenode.o ds/linkedlist.o ds/listnode.o \
	ds/string.o protocol/entry.o protocol/ddt.o protocol/title.o \
	protocol/artist.o protocol/cdplayer.o protocol/library.o \
	iface/cddb/cddb.o iface/cddb/identify.o iface/editor/editor.o \
	iface/jukebox/autoplay.o iface/jukebox/cut.o iface/jukebox/fade.o \
	iface/jukebox/jukebox.o iface/jukebox/next.o iface/jukebox/pause.o \
	iface/jukebox/play.o iface/jukebox/playorder.o iface/jukebox/pps.o \
	iface/jukebox/queue.o iface/jukebox/stop.o iface/jukebox/tracklist.o \
	iface/remote/remote.o iface/remote/pause.o iface/remote/play.o \
	iface/remote/stop.o iface/startup/startup.o gal/abutton.o \
	gal/acheck.o gal/adellist.o gal/aedit.o gal/aeditlist.o \
	gal/alist.o gal/astatic.o gal/astaticlist.o gal/awindow.o \
	gal/ctrlbase.o gal/dibase.o

all: $(OBJS)
	$(CC) $(DFLAGS) -o jukebox $(OBJS) $(CFLAGS) $(CLIBS)

jukebox.o: $(JUKEBOX_H) jukebox.C
	$(CC) $(DFLAGS) -c jukebox.C $(CFLAGS)

ds/bstree.o: $(JUKEBOX_H) ds/bstree.C
	$(CC) $(DFLAGS) -o ds/bstree.o -c ds/bstree.C $(CFLAGS)

ds/treenode.o: $(JUKEBOX_H) ds/treenode.C
	$(CC) $(DFLAGS) -o ds/treenode.o -c ds/treenode.C $(CFLAGS)

ds/linkedlist.o: $(JUKEBOX_H) ds/linkedlist.C
	$(CC) $(DFLAGS) -o ds/linkedlist.o -c ds/linkedlist.C $(CFLAGS)

ds/listnode.o: $(JUKEBOX_H) ds/listnode.C
	$(CC) $(DFLAGS) -o ds/listnode.o -c ds/listnode.C $(CFLAGS)

ds/string.o: $(JUKEBOX_H) ds/string.C
	$(CC) $(DFLAGS) -o ds/string.o -c ds/string.C $(CFLAGS)

# Removed until/if we decide to use it
#protocol/cddb.o: $(JUKEBOX_H) protocol/cddb.C
#	$(CC) $(DFLAGS) -o protocol/cddb.o -c protocol/cddb.C $(CFLAGS)

protocol/entry.o: $(JUKEBOX_H) protocol/entry.C
	$(CC) $(DFLAGS) -o protocol/entry.o -c protocol/entry.C $(CFLAGS)

protocol/ddt.o: $(JUKEBOX_H) protocol/ddt.C
	$(CC) $(DFLAGS) -o protocol/ddt.o -c protocol/ddt.C $(CFLAGS)

protocol/title.o: $(JUKEBOX_H) protocol/title.C
	$(CC) $(DFLAGS) -o protocol/title.o -c protocol/title.C $(CFLAGS)

protocol/artist.o: $(JUKEBOX_H) protocol/artist.C
	$(CC) $(DFLAGS) -o protocol/artist.o -c protocol/artist.C $(CFLAGS)

protocol/cdplayer.o: $(JUKEBOX_H) protocol/cdplayer.C
	$(CC) $(DFLAGS) -o protocol/cdplayer.o -c protocol/cdplayer.C $(CFLAGS)

protocol/library.o: $(JUKEBOX_H) protocol/library.C
	$(CC) $(DFLAGS) -o protocol/library.o -c protocol/library.C $(CFLAGS)

iface/cddb/cddb.o: $(JUKEBOX_H) iface/cddb/cddb.C
	$(CC) $(DFLAGS) -o iface/cddb/cddb.o -c iface/cddb/cddb.C $(CFLAGS)

iface/cddb/identify.o: $(JUKEBOX_H) iface/cddb/identify.C
	$(CC) $(DFLAGS) -o iface/cddb/identify.o -c iface/cddb/identify.C $(CFLAGS)

iface/editor/editor.o: $(JUKEBOX_H) iface/editor/editor.C
	$(CC) $(DFLAGS) -o iface/editor/editor.o -c iface/editor/editor.C $(CFLAGS)

iface/jukebox/autoplay.o: $(JUKEBOX_H) iface/jukebox/autoplay.C
	$(CC) $(DFLAGS) -o iface/jukebox/autoplay.o -c iface/jukebox/autoplay.C $(CFLAGS)

iface/jukebox/cut.o: $(JUKEBOX_H) iface/jukebox/cut.C
	$(CC) $(DFLAGS) -o iface/jukebox/cut.o -c iface/jukebox/cut.C $(CFLAGS)

iface/jukebox/fade.o: $(JUKEBOX_H) iface/jukebox/fade.C
	$(CC) $(DFLAGS) -o iface/jukebox/fade.o -c iface/jukebox/fade.C $(CFLAGS)

iface/jukebox/jukebox.o: $(JUKEBOX_H) iface/jukebox/jukebox.C
	$(CC) $(DFLAGS) -o iface/jukebox/jukebox.o -c iface/jukebox/jukebox.C $(CFLAGS)

iface/jukebox/next.o: $(JUKEBOX_H) iface/jukebox/next.C
	$(CC) $(DFLAGS) -o iface/jukebox/next.o -c iface/jukebox/next.C $(CFLAGS)

iface/jukebox/pause.o: $(JUKEBOX_H) iface/jukebox/pause.C
	$(CC) $(DFLAGS) -o iface/jukebox/pause.o -c iface/jukebox/pause.C $(CFLAGS)

iface/jukebox/play.o: $(JUKEBOX_H) iface/jukebox/play.C
	$(CC) $(DFLAGS) -o iface/jukebox/play.o -c iface/jukebox/play.C $(CFLAGS)

iface/jukebox/playorder.o: $(JUKEBOX_H) iface/jukebox/playorder.C
	$(CC) $(DFLAGS) -o iface/jukebox/playorder.o -c iface/jukebox/playorder.C $(CFLAGS)

iface/jukebox/pps.o: $(JUKEBOX_H) iface/jukebox/pps.C
	$(CC) $(DFLAGS) -o iface/jukebox/pps.o -c iface/jukebox/pps.C $(CFLAGS)

iface/jukebox/queue.o: $(JUKEBOX_H) iface/jukebox/queue.C
	$(CC) $(DFLAGS) -o iface/jukebox/queue.o -c iface/jukebox/queue.C $(CFLAGS)

iface/jukebox/stop.o: $(JUKEBOX_H) iface/jukebox/stop.C
	$(CC) $(DFLAGS) -o iface/jukebox/stop.o -c iface/jukebox/stop.C $(CFLAGS)

iface/jukebox/tracklist.o: $(JUKEBOX_H) iface/jukebox/tracklist.C
	$(CC) $(DFLAGS) -o iface/jukebox/tracklist.o -c iface/jukebox/tracklist.C $(CFLAGS)

iface/remote/remote.o: $(JUKEBOX_H) iface/remote/remote.C
	$(CC) $(DFLAGS) -o iface/remote/remote.o -c iface/remote/remote.C $(CFLAGS)

iface/remote/pause.o: $(JUKEBOX_H) iface/remote/pause.C
	$(CC) $(DFLAGS) -o iface/remote/pause.o -c iface/remote/pause.C $(CFLAGS)

iface/remote/play.o: $(JUKEBOX_H) iface/remote/play.C
	$(CC) $(DFLAGS) -o iface/remote/play.o -c iface/remote/play.C $(CFLAGS)

iface/remote/stop.o: $(JUKEBOX_H) iface/remote/stop.C
	$(CC) $(DFLAGS) -o iface/remote/stop.o -c iface/remote/stop.C $(CFLAGS)

iface/startup/startup.o: $(JUKEBOX_H) iface/startup/startup.C
	$(CC) $(DFLAGS) -o iface/startup/startup.o -c iface/startup/startup.C $(CFLAGS)

gal/abutton.o: $(JUKEBOX_H) gal/abutton.C
	$(CC) $(DFLAGS) -o gal/abutton.o -c gal/abutton.C $(CFLAGS)

gal/acheck.o: $(JUKEBOX_H) gal/acheck.C 
	$(CC) $(DFLAGS) -o gal/acheck.o -c gal/acheck.C $(CFLAGS)

gal/adellist.o: $(JUKEBOX_H) gal/adellist.C
	$(CC) $(DFLAGS) -o gal/adellist.o -c gal/adellist.C $(CFLAGS)

gal/aedit.o: $(JUKEBOX_H) gal/aedit.C
	$(CC) $(DFLAGS) -o gal/aedit.o -c gal/aedit.C $(CFLAGS)

gal/aeditlist.o: $(JUKEBOX_H) gal/aeditlist.C
	$(CC) $(DFLAGS) -o gal/aeditlist.o -c gal/aeditlist.C $(CFLAGS)

gal/alist.o: $(JUKEBOX_H) gal/alist.C
	$(CC) $(DFLAGS) -o gal/alist.o -c gal/alist.C $(CFLAGS)

gal/astatic.o: $(JUKEBOX_H) gal/astatic.C
	$(CC) $(DFLAGS) -o gal/astatic.o -c gal/astatic.C $(CFLAGS)

gal/astaticlist.o: $(JUKEBOX_H) gal/astaticlist.C
	$(CC) $(DFLAGS) -o gal/astaticlist.o -c gal/astaticlist.C $(CFLAGS)

gal/awindow.o: $(JUKEBOX_H) gal/awindow.C
	$(CC) $(DFLAGS) -o gal/awindow.o -c gal/awindow.C $(CFLAGS)

gal/ctrlbase.o: $(JUKEBOX_H) gal/ctrlbase.C
	$(CC) $(DFLAGS) -o gal/ctrlbase.o -c gal/ctrlbase.C $(CFLAGS)

gal/dibase.o: $(JUKEBOX_H) gal/dibase.C
	$(CC) $(DFLAGS) -o gal/dibase.o -c gal/dibase.C $(CFLAGS)

install:
	mv -f jukebox ../jukebox

clean:
	rm -f $(OBJS) jukebox
