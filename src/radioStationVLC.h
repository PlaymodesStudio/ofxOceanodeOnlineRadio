#ifndef RadioStationVLC_h
#define RadioStationVLC_h

#include "ofxOceanodeNodeModel.h"
#include <vlc/vlc.h>
#include <AudioToolbox/AudioToolbox.h>
#include <CoreAudio/CoreAudio.h>
#include <vector>
#include <mutex>
#include <atomic>

// In radioStationVLC.h, replace the current SMEMAudioBuffer class with:

class SMEMAudioBuffer {
public:
    static constexpr size_t BUFFER_SIZE = 192000 * 48;
    static constexpr size_t FRAME_SIZE = 4608;
    static constexpr size_t OPTIMAL_FILL_LEVEL = BUFFER_SIZE / 3;
    static constexpr size_t MIN_READ_THRESHOLD = FRAME_SIZE * 8;
    
    SMEMAudioBuffer() noexcept;
    ~SMEMAudioBuffer() noexcept = default;
    
    // Delete copy constructor and assignment
    SMEMAudioBuffer(const SMEMAudioBuffer&) = delete;
    SMEMAudioBuffer& operator=(const SMEMAudioBuffer&) = delete;
    
    uint8_t* getLockPointer(size_t size) noexcept;
    void unlock(size_t size) noexcept;
    size_t read(float* output, size_t frames) noexcept;
    size_t getAvailableFrames() const noexcept;
    void clear() noexcept;
    void stop() noexcept;
    void start() noexcept;
    
private:
    std::vector<uint8_t> buffer;
    size_t readPos{0};
    size_t writePos{0};
    size_t available{0};
    std::mutex mutex;
    std::atomic<bool> active{true};
};

class RadioStationVLC : public ofxOceanodeNodeModel {
public:
    struct AudioDeviceInfo {
        string name;
        AudioDeviceID deviceId;
        vector<UInt32> outputChannels;
    };
    
    RadioStationVLC();
    ~RadioStationVLC();
    
    void setup() override;
    
private:
    // VLC components
    libvlc_instance_t* vlcInstance{nullptr};
    libvlc_media_player_t* mediaPlayer{nullptr};
    libvlc_media_t* media{nullptr};
    
    // Audio components
    AudioComponent audioComponent{nullptr};
    AudioComponentInstance audioUnit{nullptr};
    std::unique_ptr<SMEMAudioBuffer> audioBuffer;
    std::mutex audioMutex;
    
    // Audio format info
    struct AudioFormat {
        unsigned int rate{44100};
        unsigned int channels{2};
        unsigned int bitsPerSample{16};
        bool isFloat{false};
        std::atomic<bool> needsUpdate{true};
    } currentFormat;
    
    // Parameters
    ofParameter<int> stationSelector;
    ofParameter<int> deviceSelector;
    ofParameter<int> channelSelector{"Output Channel", 1, 1, 64};
    ofParameter<bool> isPlaying{"Play", false};
    ofParameter<float> volume{"Volume", 1.0, 0.0, 1.0};
    ofParameter<string> status{"Status", "Initializing..."};
    ofEventListeners listeners;
    
    // Data
    vector<string> stationNames;
    vector<string> stationUrls;
    vector<string> deviceNames;
    vector<AudioDeviceInfo> devices;
    string currentUrl;
    std::atomic<bool> isStreamActive{false};
    std::atomic<bool> isChangingDevice{false};
    
    // SMEM callbacks
    static void* smemSetup(void** p_data, char* chroma, unsigned* rate, unsigned* channels);
    static void smemCleanup(void* data);
    static void smemAudioPrerender(void* data, uint8_t** pp_pcm_buffer, size_t size);
    static void smemAudioPostrender(void* data, uint8_t* p_pcm_buffer, unsigned int channels,
                                  unsigned int rate, unsigned int nb_samples, unsigned int bits_per_sample,
                                  size_t size, int64_t pts);
    
    // CoreAudio callback
    static OSStatus audioCallback(void *inRefCon,
                                AudioUnitRenderActionFlags *ioActionFlags,
                                const AudioTimeStamp *inTimeStamp,
                                UInt32 inBusNumber,
                                UInt32 inNumberFrames,
                                AudioBufferList *ioData);
    
    // Setup functions
    bool setupVLC();
    void setupParameters();
    bool setupAudioUnit();
    string getVLCOptions() const;
    
    // Utility functions
    void loadStations();
    void loadAudioDevices();
    void updateChannelCount();
    bool recreateAudioUnit();
    void cleanupAudioUnit();
    
    // Stream control
    void startStream();
    void stopStream();
    void setVolume(float vol);
    
    // Event handlers
    void onPlayStateChanged(bool& state);
    void onStationChanged(int& index);
    void onDeviceChanged(int& index);
    void onVolumeChanged(float& value);
    
    // Cleanup
    void cleanup();
};

#endif /* RadioStationVLC_h */
