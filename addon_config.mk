meta:
    ADDON_NAME = ofxOceanodeOnlineRadio
    ADDON_DESCRIPTION = Internet radio streaming node for Oceanode with multi-channel output routing
    ADDON_AUTHOR = Your Name
    ADDON_TAGS = "radio" "streaming" "vlc" "audio" "oceanode"
    ADDON_URL = http://github.com/yourusername/ofxOceanodeOnlineRadio

common:
    ADDON_DEPENDENCIES = ofxOceanode
    ADDON_SOURCES = src/radioStationVLC.cpp
    ADDON_SOURCES += src/radioStationVLC.h
    ADDON_SOURCES += src/ofxOceanodeOnlineRadio.h
    ADDON_INCLUDES = src

osx:
    # Include paths
    ADDON_INCLUDES += /Applications/VLC.app/Contents/MacOS/include
    
    # Library paths with proper rpath settings
    ADDON_LDFLAGS = -L/Applications/VLC.app/Contents/MacOS/lib
    ADDON_LDFLAGS += -lvlc -lvlccore
    ADDON_LDFLAGS += -Wl,-rpath,/Applications/VLC.app/Contents/MacOS/lib