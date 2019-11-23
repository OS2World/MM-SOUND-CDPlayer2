#*************************************************************************
#* cdplayer.mak (c) Axel Salomon fÅr Inside OS/2 Ausgabe 11'93           *
#************************************************************************* 

CC	    = icc
LINK	    = link386

BASELIBS    = DDE4MBS.LIB OS2386.LIB MMPM2.LIB

DEBUG	   = /Ti+ /O-

#
# Compilation Switches
#
#     /G3s	     : Generate 386 code with no stack checking.
#     /C+	     : Compile only one module.
#     /W3	     : Warning level.
#     /Gd-	     : Link to static C libraries.
#     /Gm+	     : Use multithreaded libraries.
#     /DINCL_32      : Use IBM code.
#     /Ti+	     : Generate debugging code.
#     /Sm	     : Generate debugging code.
#     /O-	     : Turn optimization off.
#
COMPILE = /G3s /C+ /W3 /Ki- /Kb- /Ss+ /Gd- /Ms /Gm+ /DINCL_32

CFLAGS = $(COMPILE)

#
# Link Switches
#
#    /map     : Creates a listing file containing all pulbic symbols.
#    /nod     : Causes all default libraries to be ignored.
#    /noe     : The linker will not search the extended dictionary.
#

LFLAGS	= /map /nod /noe /co

all: cdplayer.exe

cdplayer.exe: cdplayer.obj cdplayer.res cdplayer.def
  $(LINK) cdplayer.obj, cdplayer.exe, $(LFLAGS) /BASE:0x10000, $(BASELIBS), cdplayer.def
  rc cdplayer.res

cdplayer.obj: cdplayer.c cdplayer.h
  $(CC) $(CFLAGS) $(DEBUG) cdplayer.c

cdplayer.res: cdplayer.rc cdplayer.dlg cdplayer.h cdplayer.ico
  rc -r cdplayer.rc

