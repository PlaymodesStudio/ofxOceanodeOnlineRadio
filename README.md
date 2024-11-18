# ofxOceanodeOnlineRadio

OpenFrameworks Oceanode addon for streaming internet radio with multi-channel output routing.

## Features

- Stream internet radio stations with VLC backend
- Route audio to specific output channels
- Multiple audio device support
- Easy integration with Oceanode

## Installation

1. Clone this repository to your OpenFrameworks addons folder:
```bash
cd openFrameworks/addons
git clone https://github.com/yourusername/ofxOceanodeOnlineRadio
```

2. Run the dependency installation script:
```bash
cd ofxOceanodeOnlineRadio/scripts
chmod +x install_dependencies.sh
./install_dependencies.sh
```

3. In your project's config.make, add:
```make
XCODE_LDFLAGS = "-L/Applications/VLC.app/Contents/MacOS/lib -lvlc -lvlccore"
```

4. In Xcode project settings:
   - Add to "Header Search Paths": `/Applications/VLC.app/Contents/MacOS/include`
   - Add to "Library Search Paths": `/Applications/VLC.app/Contents/MacOS/lib`

## Usage

1. Include the addon in your project:
```cpp
#include "ofxOceanodeOnlineRadio.h"
```

2. Register the radio node with Oceanode:
```cpp
ofxOceanode oceanode;
ofxOceanodeOnlineRadio::registerNodes(oceanode);
```

3. Create stations.json in your data/radio folder:
```json
{
  "station.name": "http://stream.url",
  "another.station": "http://another.url"
}
```

## Dependencies

- [ofxOceanode](https://github.com/PlaymodesStudio/ofxOceanode)
- VLC 3.0+ (installed via script)

## License

MIT License - See LICENSE.md
