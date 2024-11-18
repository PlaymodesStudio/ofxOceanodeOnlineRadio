#include "RadioStationVLC.h"

SMEMAudioBuffer::SMEMAudioBuffer() noexcept {
    buffer.resize(BUFFER_SIZE);
    clear();
}

void SMEMAudioBuffer::unlock(size_t size) noexcept {
    std::lock_guard<std::mutex> lock(mutex);
    if (!active) return;
    
    writePos = (writePos + size) % BUFFER_SIZE;
    available += size;
}


uint8_t* SMEMAudioBuffer::getLockPointer(size_t size) noexcept {
    std::lock_guard<std::mutex> lock(mutex);
    
    if (!active || size == 0) return nullptr;
    
    size_t availableSpace;
    if (writePos >= readPos) {
        // Space available at end of buffer
        availableSpace = BUFFER_SIZE - writePos;
        if (availableSpace >= size) {
            // Enough space at end
            return buffer.data() + writePos;
        } else if (readPos > size) {
            // Wrap around to start
            writePos = 0;
            return buffer.data() + writePos;
        } else {
            // Not enough space
            return nullptr;
        }
    } else {
        // Space between writePos and readPos
        availableSpace = readPos - writePos;
        if (availableSpace >= size) {
            return buffer.data() + writePos;
        } else {
            // Not enough space
            return nullptr;
        }
    }
}



size_t SMEMAudioBuffer::read(float* output, size_t frames) noexcept {
    std::lock_guard<std::mutex> lock(mutex);
    if (!active) return 0;
    
    size_t bytesPerFrame = 4; // 2 channels * 2 bytes per sample
    size_t availableFrames = available / bytesPerFrame;
    size_t framesToRead = std::min(frames, availableFrames);
    
    if (framesToRead == 0) {
        memset(output, 0, frames * 2 * sizeof(float));
        return frames;
    }
    
    const int16_t* input = reinterpret_cast<const int16_t*>(buffer.data() + readPos);
    
    for (size_t i = 0; i < framesToRead * 2; i++) {
        output[i] = input[i] / 32768.0f;
    }
    
    size_t bytesRead = framesToRead * bytesPerFrame;
    readPos = (readPos + bytesRead) % BUFFER_SIZE;
    available -= bytesRead;
    
    // Fill remaining frames with silence if needed
    if (framesToRead < frames) {
        memset(output + framesToRead * 2, 0, (frames - framesToRead) * 2 * sizeof(float));
    }
    
    return frames;
}

void SMEMAudioBuffer::clear() noexcept {
    std::lock_guard<std::mutex> lock(mutex);
    readPos = writePos = available = 0;
    active = true;
}

size_t SMEMAudioBuffer::getAvailableFrames() const noexcept {
    return available / 4; // 4 bytes per frame (2 channels * 2 bytes per sample)
}

void SMEMAudioBuffer::stop() noexcept {
    std::lock_guard<std::mutex> lock(mutex);
    active = false;
}

void SMEMAudioBuffer::start() noexcept {
    std::lock_guard<std::mutex> lock(mutex);
    clear();
    active = true;
}


///
///
///

RadioStationVLC::RadioStationVLC() :
    ofxOceanodeNodeModel("Radio Station VLC"),
    audioBuffer(std::make_unique<SMEMAudioBuffer>())
{
    ofLogNotice("RadioStationVLC") << "Initializing...";
    
    if(setupVLC()) {
        loadStations();
        loadAudioDevices();
        setupParameters();
        if(setupAudioUnit()) {
            status.set("Ready");
        } else {
            status.set("Audio setup failed");
        }
    } else {
        status.set("VLC initialization failed");
    }
}

RadioStationVLC::~RadioStationVLC() {
    cleanup();
}

void RadioStationVLC::setup() {
    description = "VLC-based radio station with multi-channel output routing";
}

bool RadioStationVLC::setupVLC() {
    try {
        setenv("VLC_PLUGIN_PATH", "/Applications/VLC.app/Contents/MacOS/plugins", 1);
        
        const char* vlc_args[] = {
            "--intf=dummy",
            "--vout=dummy",
            "--no-video",
            "--no-stats",
            "--verbose=0",
            "--no-media-library",
            "--no-osd",
            "--no-spu",
            "--aout=dummy",
            "--sout-mux-caching=1500",
            "--network-caching=3000",
            "--live-caching=3000",
            "--codec=any",
            "--no-drop-late-frames",
            "--stream-filter=prefetch"
        };
        
        vlcInstance = libvlc_new(sizeof(vlc_args)/sizeof(vlc_args[0]), vlc_args);
        if (!vlcInstance) {
            ofLogError("RadioStationVLC") << "Failed to create VLC instance";
            return false;
        }
        
        return true;
    } catch (const std::exception& e) {
        ofLogError("RadioStationVLC") << "VLC setup error: " << e.what();
        return false;
    }
}

void RadioStationVLC::setupParameters() {
    // Add parameters to the node
    addParameter(status);
    addParameter(volume);
    addParameterDropdown(stationSelector, "Station", 0, stationNames);
    addParameterDropdown(deviceSelector, "Audio Device", 0, deviceNames);
    addParameter(channelSelector);
    addParameter(isPlaying);
    
    // Setup listeners
    listeners.push(isPlaying.newListener([this](bool& state) {
        onPlayStateChanged(state);
    }));
    
    listeners.push(stationSelector.newListener([this](int& index) {
        onStationChanged(index);
    }));
    
    listeners.push(deviceSelector.newListener([this](int& index) {
        onDeviceChanged(index);
    }));
    
    listeners.push(volume.newListener([this](float& value) {
        onVolumeChanged(value);
    }));
}

string RadioStationVLC::getVLCOptions() const {
    char smem_options[1000];
    sprintf(smem_options,
            "#transcode{acodec=s16l,channels=2,samplerate=44100,aenc=any}:"  // Back to 44100
            "smem{audio-prerender-callback=%lld,"
            "audio-postrender-callback=%lld,"
            "audio-data=%lld,"
            "no-audio-visual}",
            (long long int)(intptr_t)(void*)&smemAudioPrerender,
            (long long int)(intptr_t)(void*)&smemAudioPostrender,
            (long long int)(intptr_t)this);

    return string(smem_options);
}

void RadioStationVLC::startStream() {
    if(!vlcInstance || stationSelector >= stationUrls.size()) return;
    
    stopStream();
    
    try {
        string url = stationUrls[stationSelector];
        string options = getVLCOptions();
        
        // Create media with SMEM options
        media = libvlc_media_new_location(vlcInstance, url.c_str());
        if(!media) {
            status.set("Failed to create media");
            return;
        }
        
        // Add sout options
        libvlc_media_add_option(media, (":sout=" + options).c_str());
        libvlc_media_add_option(media, ":no-video");
        libvlc_media_add_option(media, ":no-audio-visual");
        libvlc_media_add_option(media, ":no-sout-video");
        
        // Create player
        mediaPlayer = libvlc_media_player_new_from_media(media);
        if(!mediaPlayer) {
            libvlc_media_release(media);
            status.set("Failed to create player");
            return;
        }
        
        // Start CoreAudio
        OSStatus result = AudioOutputUnitStart(audioUnit);
        if(result != noErr) {
            ofLogError("RadioStationVLC") << "Failed to start audio unit";
            cleanup();
            return;
        }
        
        // Start playback
        if(libvlc_media_player_play(mediaPlayer) == 0) {
            isStreamActive = true;
            status.set("Playing: " + stationNames[stationSelector]);
        } else {
            status.set("Playback failed");
            cleanup();
        }
        
        libvlc_media_release(media);
        media = nullptr;
        
    } catch (const std::exception& e) {
        ofLogError("RadioStationVLC") << "Error starting stream: " << e.what();
        cleanup();
    }
}

void RadioStationVLC::stopStream() {
    if(mediaPlayer) {
        libvlc_media_player_stop(mediaPlayer);
        libvlc_media_player_release(mediaPlayer);
        mediaPlayer = nullptr;
    }
    
    // Only release media if it hasn't been released yet
    if(media) {
        libvlc_media_release(media);
        media = nullptr;
    }
    
    if(audioUnit) {
        AudioOutputUnitStop(audioUnit);
    }
    
    audioBuffer->clear();
    isStreamActive = false;
    status.set("Stopped");
}

void RadioStationVLC::cleanup() {
    stopStream();
    
    if(audioUnit) {
        cleanupAudioUnit();
    }
    
    if(vlcInstance) {
        libvlc_release(vlcInstance);
        vlcInstance = nullptr;
    }
}

// SMEM Callbacks
void* RadioStationVLC::smemSetup(void** p_data, char* chroma, unsigned* rate, unsigned* channels) {
    // Set desired audio format
    *rate = 44100;
    *channels = 2;
    strcpy(chroma, "s16l");  // Signed 16-bit little-endian
    return nullptr;
}

void RadioStationVLC::smemCleanup(void* data) {
    // Nothing needed here
}

void RadioStationVLC::smemAudioPrerender(void* data, uint8_t** pp_pcm_buffer, size_t size) {
    auto* self = static_cast<RadioStationVLC*>(data);
    if(!self || !self->audioBuffer) {
        *pp_pcm_buffer = nullptr;
        return;
    }
    
    *pp_pcm_buffer = self->audioBuffer->getLockPointer(size);
    if(!*pp_pcm_buffer) {
        ofLogError("RadioStationVLC") << "Failed to get buffer of size: " << size;
    }
}

void RadioStationVLC::smemAudioPostrender(void* data, uint8_t* p_pcm_buffer,
                                        unsigned int channels, unsigned int rate,
                                        unsigned int nb_samples, unsigned int bits_per_sample,
                                        size_t size, int64_t pts) {
    auto* self = static_cast<RadioStationVLC*>(data);
    if(!self || !self->audioBuffer) return;
    
    // Handle format changes
    if(self->currentFormat.rate != rate ||
       self->currentFormat.channels != channels ||
       self->currentFormat.bitsPerSample != bits_per_sample) {
        
        self->currentFormat.rate = rate;
        self->currentFormat.channels = channels;
        self->currentFormat.bitsPerSample = bits_per_sample;
        self->currentFormat.needsUpdate = true;
        
        ofLogNotice("RadioStationVLC") << "Audio format changed: "
                                     << rate << "Hz, "
                                     << channels << " channels, "
                                     << bits_per_sample << " bits";
        
        // Trigger audio unit recreation with new format
        self->recreateAudioUnit();
    }
    
    self->audioBuffer->unlock(size);
}

// Event Handlers
void RadioStationVLC::onPlayStateChanged(bool& state) {
    if(state) {
        startStream();
    } else {
        stopStream();
    }
}

void RadioStationVLC::onStationChanged(int& index) {
    if(isPlaying) {
        startStream();
    }
}

void RadioStationVLC::onDeviceChanged(int& index) {
    bool wasPlaying = isPlaying;
    if(wasPlaying) {
        stopStream();
    }
    
    isChangingDevice = true;
    if(recreateAudioUnit()) {
        updateChannelCount();
        if(wasPlaying) {
            startStream();
        }
    }
    isChangingDevice = false;
    
    ofLogNotice("RadioStationVLC") << "Device changed to index: " << index
                                  << " deviceId: " << devices[index].deviceId
                                  << " name: " << devices[index].name;
}

void RadioStationVLC::onVolumeChanged(float& value) {
    // Volume is applied in the audio callback
}

///
///
///
///

void RadioStationVLC::loadStations() {
string path = ofToDataPath("radio/stations.json");
if(!ofFile::doesFileExist(path)) {
    ofLogError("RadioStationVLC") << "stations.json not found at: " << path;
    status.set("No stations file found");
    return;
}

try {
    ofJson json = ofLoadJson(path);
    stationNames.clear();
    stationUrls.clear();
    
    // Sort keys for consistent order
    vector<string> keys;
    for(auto& [key, value] : json.items()) {
        keys.push_back(key);
    }
    std::sort(keys.begin(), keys.end());
    
    for(const auto& key : keys) {
        stationNames.push_back(key);
        stationUrls.push_back(json[key]);
    }
    
    ofLogNotice("RadioStationVLC") << "Loaded " << stationNames.size() << " stations";
    
    // Update station selector range if needed
    if(stationSelector.getMax() != stationNames.size() - 1) {
        stationSelector.setMax(stationNames.size() - 1);
    }
    
} catch(const std::exception& e) {
    ofLogError("RadioStationVLC") << "Error loading stations: " << e.what();
    status.set("Error loading stations");
}
}

void RadioStationVLC::loadAudioDevices() {
AudioObjectPropertyAddress propertyAddress = {
    kAudioHardwarePropertyDevices,
    kAudioObjectPropertyScopeGlobal,
    kAudioObjectPropertyElementMaster
};

UInt32 dataSize = 0;
OSStatus status = AudioObjectGetPropertyDataSize(kAudioObjectSystemObject,
                                               &propertyAddress,
                                               0,
                                               NULL,
                                               &dataSize);

if(status != noErr) {
    ofLogError("RadioStationVLC") << "Error getting audio devices size";
    return;
}

int deviceCount = dataSize / sizeof(AudioDeviceID);
vector<AudioDeviceID> deviceIds(deviceCount);

status = AudioObjectGetPropertyData(kAudioObjectSystemObject,
                                  &propertyAddress,
                                  0,
                                  NULL,
                                  &dataSize,
                                  deviceIds.data());

if(status != noErr) {
    ofLogError("RadioStationVLC") << "Error getting audio devices";
    return;
}

deviceNames.clear();
devices.clear();

// Add system default device first
deviceNames.push_back("System Default");
AudioDeviceInfo defaultDevice;
defaultDevice.name = "System Default";
defaultDevice.deviceId = kAudioObjectSystemObject;
devices.push_back(defaultDevice);

// Process each device
for(const auto& deviceId : deviceIds) {
    AudioDeviceInfo info;
    info.deviceId = deviceId;
    
    // Get device name
    CFStringRef deviceName = nullptr;
    dataSize = sizeof(CFStringRef);
    propertyAddress.mSelector = kAudioDevicePropertyDeviceNameCFString;
    
    status = AudioObjectGetPropertyData(deviceId,
                                      &propertyAddress,
                                      0,
                                      NULL,
                                      &dataSize,
                                      &deviceName);
    
    if(status == noErr && deviceName) {
        char name[256];
        if(CFStringGetCString(deviceName, name, sizeof(name), kCFStringEncodingUTF8)) {
            info.name = string(name);
            
            // Get output channels
            propertyAddress.mSelector = kAudioDevicePropertyStreamConfiguration;
            propertyAddress.mScope = kAudioDevicePropertyScopeOutput;
            
            status = AudioObjectGetPropertyDataSize(deviceId,
                                                  &propertyAddress,
                                                  0,
                                                  NULL,
                                                  &dataSize);
            
            if(status == noErr && dataSize > 0) {
                AudioBufferList* bufferList = (AudioBufferList*)malloc(dataSize);
                if(bufferList) {
                    status = AudioObjectGetPropertyData(deviceId,
                                                      &propertyAddress,
                                                      0,
                                                      NULL,
                                                      &dataSize,
                                                      bufferList);
                    
                    if(status == noErr) {
                        UInt32 totalChannels = 0;
                        for(UInt32 i = 0; i < bufferList->mNumberBuffers; i++) {
                            totalChannels += bufferList->mBuffers[i].mNumberChannels;
                            for(UInt32 ch = 0; ch < bufferList->mBuffers[i].mNumberChannels; ch++) {
                                info.outputChannels.push_back(ch);
                            }
                        }
                        
                        if(totalChannels > 0) {
                            deviceNames.push_back(info.name);
                            devices.push_back(info);
                            
                            ofLogNotice("RadioStationVLC") << "Found device: " << info.name
                                                         << " with " << totalChannels << " channels";
                        }
                    }
                    free(bufferList);
                }
            }
        }
        CFRelease(deviceName);
    }
}

ofLogNotice("RadioStationVLC") << "Loaded " << deviceNames.size() << " audio devices";
}

bool RadioStationVLC::recreateAudioUnit() {
    std::lock_guard<std::mutex> lock(audioMutex);
    
    try {
        cleanupAudioUnit();
        
        AudioComponentDescription desc;
        desc.componentType = kAudioUnitType_Output;
        desc.componentSubType = kAudioUnitSubType_HALOutput;
        desc.componentManufacturer = kAudioUnitManufacturer_Apple;
        desc.componentFlags = 0;
        desc.componentFlagsMask = 0;
        
        audioComponent = AudioComponentFindNext(NULL, &desc);
        if(!audioComponent) return false;
        
        OSStatus status = AudioComponentInstanceNew(audioComponent, &audioUnit);
        if(status != noErr) return false;
        
        // Set device
        AudioDeviceID deviceId = kAudioObjectSystemObject;
        if(deviceSelector >= 0 && deviceSelector < devices.size()) {
            deviceId = devices[deviceSelector].deviceId;
        }
        
        status = AudioUnitSetProperty(audioUnit,
                                    kAudioOutputUnitProperty_CurrentDevice,
                                    kAudioUnitScope_Global,
                                    0,
                                    &deviceId,
                                    sizeof(deviceId));
        
        if(status != noErr) {
            cleanupAudioUnit();
            return false;
        }

        // Get device's native format
        AudioStreamBasicDescription deviceFormat;
        UInt32 size = sizeof(deviceFormat);
        status = AudioUnitGetProperty(audioUnit,
                                    kAudioUnitProperty_StreamFormat,
                                    kAudioUnitScope_Output,
                                    0,
                                    &deviceFormat,
                                    &size);
        
        // Set format that matches device but ensures we can handle our stereo input
        AudioStreamBasicDescription audioFormat;
        audioFormat.mSampleRate = deviceFormat.mSampleRate;
        audioFormat.mFormatID = kAudioFormatLinearPCM;
        audioFormat.mFormatFlags = kAudioFormatFlagIsFloat |
                                 kAudioFormatFlagIsPacked |
                                 kAudioFormatFlagIsNonInterleaved;
        audioFormat.mFramesPerPacket = 1;
        audioFormat.mChannelsPerFrame = deviceFormat.mChannelsPerFrame;
        audioFormat.mBitsPerChannel = 32;
        audioFormat.mBytesPerPacket = 4;
        audioFormat.mBytesPerFrame = 4;
        
        status = AudioUnitSetProperty(audioUnit,
                                    kAudioUnitProperty_StreamFormat,
                                    kAudioUnitScope_Input,
                                    0,
                                    &audioFormat,
                                    sizeof(audioFormat));
        
        if(status != noErr) {
            cleanupAudioUnit();
            return false;
        }
        
        // Set callback
        AURenderCallbackStruct callbackStruct;
        callbackStruct.inputProc = audioCallback;
        callbackStruct.inputProcRefCon = this;
        
        status = AudioUnitSetProperty(audioUnit,
                                    kAudioUnitProperty_SetRenderCallback,
                                    kAudioUnitScope_Input,
                                    0,
                                    &callbackStruct,
                                    sizeof(callbackStruct));
        
        if(status != noErr) {
            cleanupAudioUnit();
            return false;
        }
        
        // Initialize
        status = AudioUnitInitialize(audioUnit);
        if(status != noErr) {
            cleanupAudioUnit();
            return false;
        }

        return true;
        
    } catch (const std::exception& e) {
        ofLogError("RadioStationVLC") << "Error recreating audio unit: " << e.what();
        cleanupAudioUnit();
        return false;
    }
}

void RadioStationVLC::updateChannelCount() {
    if(deviceSelector < 0 || deviceSelector >= devices.size()) return;
    
    const auto& device = devices[deviceSelector];
    AudioDeviceID deviceId = device.deviceId;
    
    AudioObjectPropertyAddress propertyAddress = {
        kAudioDevicePropertyStreamConfiguration,
        kAudioDevicePropertyScopeOutput,
        0
    };
    
    UInt32 dataSize = 0;
    OSStatus status = AudioObjectGetPropertyDataSize(deviceId, &propertyAddress, 0, NULL, &dataSize);
    if(status != noErr) {
        ofLogError("RadioStationVLC") << "Failed to get stream configuration size";
        return;
    }
    
    AudioBufferList* bufferList = (AudioBufferList*)malloc(dataSize);
    status = AudioObjectGetPropertyData(deviceId, &propertyAddress, 0, NULL, &dataSize, bufferList);
    
    if(status == noErr) {
        UInt32 totalChannels = 0;
        for(UInt32 i = 0; i < bufferList->mNumberBuffers; i++) {
            totalChannels += bufferList->mBuffers[i].mNumberChannels;
        }
        
        int maxChannels = totalChannels;
        if(maxChannels < 2) maxChannels = 2;
        
        // Calculate maximum starting channel (ensuring space for stereo pair)
        int maxStartChannel = maxChannels - 1;
        
        // Store current channel
        int currentChannel = channelSelector.get();
        
        // Update channel selector range
        channelSelector.setMax(maxStartChannel);
        
        // If current channel is out of range, adjust it
        if(currentChannel > maxStartChannel) {
            channelSelector = 1;
        }
    }
    
    free(bufferList);
}

void RadioStationVLC::cleanupAudioUnit() {
if(audioUnit) {
    AudioOutputUnitStop(audioUnit);
    AudioUnitUninitialize(audioUnit);
    AudioComponentInstanceDispose(audioUnit);
    audioUnit = nullptr;
}
}

///
///
///

bool RadioStationVLC::setupAudioUnit() {
try {
    AudioComponentDescription desc;
    desc.componentType = kAudioUnitType_Output;
    desc.componentSubType = kAudioUnitSubType_HALOutput;
    desc.componentManufacturer = kAudioUnitManufacturer_Apple;
    desc.componentFlags = 0;
    desc.componentFlagsMask = 0;
    
    audioComponent = AudioComponentFindNext(NULL, &desc);
    if(!audioComponent) {
        ofLogError("RadioStationVLC") << "Failed to find audio component";
        return false;
    }
    
    OSStatus status = AudioComponentInstanceNew(audioComponent, &audioUnit);
    if(status != noErr) {
        ofLogError("RadioStationVLC") << "Failed to create audio unit";
        return false;
    }
    
    // Set up the audio format
    AudioStreamBasicDescription audioFormat;
    audioFormat.mSampleRate = 44100;
    audioFormat.mFormatID = kAudioFormatLinearPCM;
    audioFormat.mFormatFlags = kAudioFormatFlagIsFloat |
                             kAudioFormatFlagIsPacked |
                             kAudioFormatFlagIsNonInterleaved;
    audioFormat.mFramesPerPacket = 1;
    audioFormat.mChannelsPerFrame = 2;  // Stereo
    audioFormat.mBitsPerChannel = 32;
    audioFormat.mBytesPerPacket = 4;
    audioFormat.mBytesPerFrame = 4;
    
    status = AudioUnitSetProperty(audioUnit,
                                kAudioUnitProperty_StreamFormat,
                                kAudioUnitScope_Input,
                                0,
                                &audioFormat,
                                sizeof(audioFormat));
    
    if(status != noErr) {
        ofLogError("RadioStationVLC") << "Failed to set audio format";
        cleanupAudioUnit();
        return false;
    }
    
    // Set callback
    AURenderCallbackStruct callbackStruct;
    callbackStruct.inputProc = audioCallback;
    callbackStruct.inputProcRefCon = this;
    
    status = AudioUnitSetProperty(audioUnit,
                                kAudioUnitProperty_SetRenderCallback,
                                kAudioUnitScope_Input,
                                0,
                                &callbackStruct,
                                sizeof(callbackStruct));
    
    if(status != noErr) {
        ofLogError("RadioStationVLC") << "Failed to set audio callback";
        cleanupAudioUnit();
        return false;
    }
    
    // Initialize
    status = AudioUnitInitialize(audioUnit);
    if(status != noErr) {
        ofLogError("RadioStationVLC") << "Failed to initialize audio unit";
        cleanupAudioUnit();
        return false;
    }
    
    ofLogNotice("RadioStationVLC") << "Audio unit setup successful";
    return true;
    
} catch (const std::exception& e) {
    ofLogError("RadioStationVLC") << "Error in audio unit setup: " << e.what();
    cleanupAudioUnit();
    return false;
}
}

OSStatus RadioStationVLC::audioCallback(void *inRefCon,
                                      AudioUnitRenderActionFlags *ioActionFlags,
                                      const AudioTimeStamp *inTimeStamp,
                                      UInt32 inBusNumber,
                                      UInt32 inNumberFrames,
                                      AudioBufferList *ioData) {
    RadioStationVLC* radio = static_cast<RadioStationVLC*>(inRefCon);
    if(!radio || radio->isChangingDevice) return noErr;
    
    try {
        // Clear all channels first
        for(UInt32 i = 0; i < ioData->mNumberBuffers; i++) {
            float* buffer = static_cast<float*>(ioData->mBuffers[i].mData);
            memset(buffer, 0, sizeof(float) * inNumberFrames);
        }

        // Calculate target channels (0-based)
        int leftChannelIdx = radio->channelSelector - 1;
        int rightChannelIdx = leftChannelIdx + 1;

        // Validate target channels
        if(leftChannelIdx < 0 ||
           rightChannelIdx >= (int)ioData->mNumberBuffers ||
           leftChannelIdx >= (int)ioData->mNumberBuffers) {
            return noErr;
        }

        // Get pointers to target channel buffers
        float* leftBuffer = static_cast<float*>(ioData->mBuffers[leftChannelIdx].mData);
        float* rightBuffer = static_cast<float*>(ioData->mBuffers[rightChannelIdx].mData);

        if(!leftBuffer || !rightBuffer) return noErr;

        static std::vector<float> tempBuffer;
        if(tempBuffer.size() < inNumberFrames * 2) {
            tempBuffer.resize(inNumberFrames * 2);
        }

        size_t framesRead = radio->audioBuffer->read(tempBuffer.data(), inNumberFrames);

        float volume = radio->volume;
        for(UInt32 i = 0; i < inNumberFrames; i++) {
            if(i < framesRead) {
                leftBuffer[i] = tempBuffer[i * 2] * volume;
                rightBuffer[i] = tempBuffer[i * 2 + 1] * volume;
            }
        }

        return noErr;
    }
    catch (const std::exception& e) {
        ofLogError("RadioStationVLC") << "Exception in audio callback: " << e.what();
        return kAudio_ParamError;
    }
}
