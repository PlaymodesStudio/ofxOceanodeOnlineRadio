meta:
    ADDON_NAME = ofxOceanodeOnlineRadio
    ADDON_DESCRIPTION = Internet radio streaming node for Oceanode with multi-channel output routing
    ADDON_AUTHOR = Your Name
    ADDON_TAGS = "radio" "streaming" "vlc" "audio" "oceanode"
    ADDON_URL = http://github.com/yourusername/ofxOceanodeOnlineRadio

common:
    # dependencies with other addons
    ADDON_DEPENDENCIES = ofxOceanode

    # source files - include both .cpp and .h files
    ADDON_SOURCES = src/radioStationVLC.cpp
    ADDON_SOURCES += src/radioStationVLC.h
    ADDON_SOURCES += src/ofxOceanodeOnlineRadio.h
    
    # include search paths
    ADDON_INCLUDES = src

osx:
    # VLC paths
    ADDON_INCLUDES += "/Applications/VLC.app/Contents/MacOS/include"
    
    # Compiler flags
    ADDON_CFLAGS = -DVLC_PLUGIN_PATH='\"/Applications/VLC.app/Contents/MacOS/plugins\"'
    ADDON_CPPFLAGS = -DVLC_PLUGIN_PATH='\"/Applications/VLC.app/Contents/MacOS/plugins\"'
    
    # Linker flags
    ADDON_LDFLAGS = -L"/Applications/VLC.app/Contents/MacOS/lib"
    ADDON_LDFLAGS += -lvlc -lvlccore
    ADDON_LDFLAGS += -Wl,-rpath,"/Applications/VLC.app/Contents/MacOS/lib"
    
    # Libraries
    ADDON_LIBS = "/Applications/VLC.app/Contents/MacOS/lib/libvlc.dylib"
    ADDON_LIBS += "/Applications/VLC.app/Contents/MacOS/lib/libvlccore.dylib"