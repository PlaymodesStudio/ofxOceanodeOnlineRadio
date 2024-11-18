#pragma once

#include "radioStationVLC.h"

class ofxOceanodeOnlineRadio {
public:
    // Register the radio node with Oceanode
    static void registerModels(ofxOceanode &oceanode) {
        oceanode.registerModel<RadioStationVLC>("Radio");
    }
};