[addon]
    name = ofxOceanodeOnlineRadio
    description = Internet radio streaming node for Oceanode with multi-channel output routing
    author = Your Name
    version = 1.0.0
    
    # Dependencies
    dependencies = ofxOceanode
    
    # Include search paths, relative to the addon root
    includepaths = src
    
    # Source files relative to the addon root
    srcs = src/ofxOceanodeOnlineRadio.cpp
           src/radioStationVLC.cpp
    
    # Header files relative to the addon root
    headers = src/ofxOceanodeOnlineRadio.h
              src/radioStationVLC.h
              
    # Platform specific settings - these will be automatically applied
    [osx]
        # VLC paths
        includepaths += /Applications/VLC.app/Contents/MacOS/include
        libraryPaths = /Applications/VLC.app/Contents/MacOS/lib
        
        # Link against VLC libraries
        ADDON_LDFLAGS = -lvlc -lvlccore
        
        # Runtime search paths for dynamic libraries
        ADDON_LDFLAGS += -Wl,-rpath,/Applications/VLC.app/Contents/MacOS/lib
        
        # Framework search paths
        frameworkpaths = /Applications/VLC.app/Contents/MacOS
        
        # Additional compiler flags
        ADDON_CFLAGS = -DVLC_PLUGIN_PATH='\"/Applications/VLC.app/Contents/MacOS/plugins\"'
        ADDON_CPPFLAGS = -DVLC_PLUGIN_PATH='\"/Applications/VLC.app/Contents/MacOS/plugins\"'