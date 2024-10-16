#ifndef AUDIO_H
#define AUDIO_H

#include <vector>
#include <string>
#include <portaudio.h>
#include <filesystem>
#include "../shared/SharedQueue.h"

typedef short SAMPLE;
namespace fs = std::filesystem;

class AudioCapture {
public:
    AudioCapture(int sampleRate, int framesPerBuffer, int channels, int saveIntervalMs);
    ~AudioCapture();

    void start();
    void stop();
    void saveAudioData(const std::string &filePath);
    std::string getCurrentTime();

private:
    struct AudioData {
        std::vector<SAMPLE> recordedSamples;
        int samplesPerInterval;  // 每次保存的采样数
    };

    AudioData data;
    int sampleRate;
    int framesPerBuffer;
    int numChannels;
    int saveIntervalMs;
    PaStream *stream;
    std::string baseDir;
    std::string currentSecondFolder;

    static int recordCallback(const void *inputBuffer, void *outputBuffer,
                              unsigned long framesPerBuffer,
                              const PaStreamCallbackTimeInfo* timeInfo,
                              PaStreamCallbackFlags statusFlags,
                              void *userData);

    void initPortAudio();
    void terminatePortAudio();
};

#endif // AUDIO_CAPTURE_H
