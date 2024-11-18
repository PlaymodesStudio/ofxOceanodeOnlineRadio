# VLC Dependencies for ofxOceanodeOnlineRadio

This addon requires VLC (VideoLAN Client) 3.0+ for audio streaming capabilities.

## Requirements

- VLC 3.0 or higher installed at `/Applications/VLC.app` (macOS)
- VLC libraries:
  - `libvlc.dylib`
  - `libvlccore.dylib`
- VLC plugins directory with streaming components

## Automatic Installation

Run the provided installation script:
```bash
cd scripts
./install_dependencies.sh
```

This will:
1. Check if VLC is installed
2. Install VLC via Homebrew if needed
3. Verify required libraries are present

## Manual Installation

If you prefer manual installation:

1. Download and install VLC 3.0+ from https://www.videolan.org/
2. Make sure VLC is installed in `/Applications/VLC.app`
3. Verify these paths exist:
   - `/Applications/VLC.app/Contents/MacOS/lib/libvlc.dylib`
   - `/Applications/VLC.app/Contents/MacOS/lib/libvlccore.dylib`
   - `/Applications/VLC.app/Contents/MacOS/plugins/` (with streaming plugins)

## Verification

The addon requires these components:
```
/Applications/VLC.app/
├── Contents/
    ├── MacOS/
        ├── lib/
        │   ├── libvlc.dylib
        │   └── libvlccore.dylib
        └── plugins/
            ├── access/
            ├── audio_output/
            ├── codec/
            └── demux/
```

## Troubleshooting

If you encounter issues:

1. Check VLC version: `/Applications/VLC.app/Contents/MacOS/VLC --version`
2. Verify library permissions: `ls -l /Applications/VLC.app/Contents/MacOS/lib/`
3. Check plugin availability: `ls /Applications/VLC.app/Contents/MacOS/plugins/`
4. Try reinstalling VLC: `brew reinstall --cask vlc`

## Note

Don't place VLC libraries directly in this folder. The addon will use the libraries from your VLC installation. This README is for reference only.