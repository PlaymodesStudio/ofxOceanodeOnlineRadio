# Add custom variables to this file

# OF_ROOT allows to move projects outside apps/* just set this variable to the
# absolute path to the OF root folder

OF_ROOT = $(realpath ../../..)

# USER_CFLAGS allows to pass custom flags to the compiler
# for example search paths like:
USER_CFLAGS = -I/Applications/VLC.app/Contents/MacOS/include
USER_CFLAGS += -I/Applications/VLC.app/Contents/MacOS/include/vlc

# USER_LDFLAGS allows to pass custom flags to the linker
# for example libraries like:
USER_LDFLAGS = -L/Applications/VLC.app/Contents/MacOS/lib
USER_LDFLAGS += -lvlc -lvlccore
USER_LDFLAGS += -Wl,-rpath,/Applications/VLC.app/Contents/MacOS/lib

# USER_LIBS allows to pass custom libraries to the linker
USER_LIBS = /Applications/VLC.app/Contents/MacOS/lib/libvlc.dylib
USER_LIBS += /Applications/VLC.app/Contents/MacOS/lib/libvlccore.dylib