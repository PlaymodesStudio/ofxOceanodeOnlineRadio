#pragma once
#include "radioStationVLC.h"

class ofxOceanodeOnlineRadio {
public:
    static void registerModels(ofxOceanode &oceanode) {
        // Check and create radio directory if needed
        string radioDir = ofToDataPath("radio", true);
        if(!ofDirectory::doesDirectoryExist(radioDir)) {
            ofDirectory::createDirectory(radioDir);
            
            // Copy default stations file if it doesn't exist
            string defaultStations = ofToDataPath("radio/stations.json");
            if(!ofFile::doesFileExist(defaultStations)) {
                string addonStations = ofFilePath::join(ofFilePath::getAbsolutePath(""),
                    "../../../addons/ofxOceanodeOnlineRadio/data/radio/stations.json");
                ofFile::copyFromTo(addonStations, defaultStations);
            }
        }
        
        oceanode.registerModel<RadioStationVLC>("Radio");
    }
};
