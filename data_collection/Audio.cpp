#include "Audio.h"
#include <sys/time.h>   // 包含用于 gettimeofday 的头文件
#include <cstring>      // 包含用于 strlen 的头文件
#include <iostream>
#include <fstream>
#include <ctime>
#include <sys/stat.h>
#include <unistd.h>
#include <sstream>
#include <iomanip>

AudioCapture::AudioCapture(int sampleRate, int framesPerBuffer, int channels, int saveIntervalMs)
    : sampleRate(sampleRate), framesPerBuffer(framesPerBuffer), numChannels(channels), saveIntervalMs(saveIntervalMs), fileIndex(0), stream(nullptr) {
    data.samplesPerInterval = sampleRate * saveIntervalMs / 1000;  // 每次保存的采样数
}

AudioCapture::~AudioCapture() {
    stop();
    terminatePortAudio();
}

void AudioCapture::initPortAudio() {
    PaError err = Pa_Initialize();
    if (err != paNoError) {
        std::cerr << "PortAudio error: " << Pa_GetErrorText(err) << std::endl;
        throw std::runtime_error("Failed to initialize PortAudio.");
    }

    err = Pa_OpenDefaultStream(&stream, numChannels, 0, paInt16, sampleRate, framesPerBuffer, recordCallback, &data);
    if (err != paNoError) {
        std::cerr << "PortAudio error: " << Pa_GetErrorText(err) << std::endl;
        throw std::runtime_error("Failed to open audio stream.");
    }
}

void AudioCapture::terminatePortAudio() {
    if (stream) {
        Pa_StopStream(stream);
        Pa_CloseStream(stream);
    }
    Pa_Terminate();
}

void AudioCapture::start() {
    initPortAudio();

    // 开始录音
    PaError err = Pa_StartStream(stream);
    if (err != paNoError) {
        std::cerr << "PortAudio error: " << Pa_GetErrorText(err) << std::endl;
        throw std::runtime_error("Failed to start audio stream.");
    }

    // 获取当前日期，创建日期文件夹
    baseDir = "dataCapture/Car0001/" + getCurrentTime(true, false);
    createDirectory("dataCapture");
    createDirectory("dataCapture/Car0001");
    createDirectory(baseDir);

    std::cout << "Recording... Press Ctrl+C to stop." << std::endl;

    std::string lastSecond = "";
    while (true) {
        Pa_Sleep(saveIntervalMs);  // 每次等待保存间隔

        // 获取当前时间（秒和毫秒）
        std::string currentSecond = getCurrentTime(false, false); // 精确到秒的时间
        std::string milliseconds = getCurrentTime(false, true);   // 精确到毫秒的时间

        // 如果秒数改变，创建新的文件夹
        if (currentSecond != lastSecond) {
            currentSecondFolder = baseDir + "/" + currentSecond;
            createDirectory(currentSecondFolder);
            lastSecond = currentSecond;
        }

        // 生成文件名，带有毫秒信息
        std::stringstream ss;
        ss << currentSecondFolder << "/audio-" << milliseconds << ".wav";
        std::string fileName = ss.str();

        // 保存当前音频数据到文件
        saveAudioData(fileName);
        std::cout << "Saved audio to: " << fileName << std::endl;

        data.recordedSamples.clear();  // 清除已经保存的音频数据
        fileIndex++;
    }
}

void AudioCapture::stop() {
    if (stream) {
        Pa_StopStream(stream);
    }
}

int AudioCapture::recordCallback(const void *inputBuffer, void *outputBuffer,
                                 unsigned long framesPerBuffer,
                                 const PaStreamCallbackTimeInfo* timeInfo,
                                 PaStreamCallbackFlags statusFlags,
                                 void *userData) {
    AudioData *data = (AudioData *)userData;
    const SAMPLE *rptr = (const SAMPLE *)inputBuffer;

    if (inputBuffer != nullptr) {
        for (unsigned long i = 0; i < framesPerBuffer; ++i) {
            data->recordedSamples.push_back(*rptr++);
        }
    }

    return paContinue;
}

// 保存音频数据为WAV文件
void AudioCapture::saveAudioData(const std::string &filePath) {
    std::ofstream outFile(filePath, std::ios::binary);
    
    // 写入 WAV 文件头部
    outFile.write("RIFF", 4);
    uint32_t fileSize = 36 + data.recordedSamples.size() * sizeof(SAMPLE);
    outFile.write(reinterpret_cast<const char*>(&fileSize), 4);
    outFile.write("WAVE", 4);

    outFile.write("fmt ", 4);
    uint32_t subchunk1Size = 16;
    outFile.write(reinterpret_cast<const char*>(&subchunk1Size), 4);
    uint16_t audioFormat = 1;
    outFile.write(reinterpret_cast<const char*>(&audioFormat), 2);
    uint16_t numChannels = numChannels;
    outFile.write(reinterpret_cast<const char*>(&numChannels), 2);
    uint32_t sampleRate = sampleRate;
    outFile.write(reinterpret_cast<const char*>(&sampleRate), 4);
    uint32_t byteRate = sampleRate * numChannels * sizeof(SAMPLE);
    outFile.write(reinterpret_cast<const char*>(&byteRate), 4);
    uint16_t blockAlign = numChannels * sizeof(SAMPLE);
    outFile.write(reinterpret_cast<const char*>(&blockAlign), 2);
    uint16_t bitsPerSample = 16;
    outFile.write(reinterpret_cast<const char*>(&bitsPerSample), 2);

    outFile.write("data", 4);
    uint32_t subchunk2Size = data.recordedSamples.size() * sizeof(SAMPLE);
    outFile.write(reinterpret_cast<const char*>(&subchunk2Size), 4);

    // 写入音频数据
    outFile.write(reinterpret_cast<const char*>(data.recordedSamples.data()), subchunk2Size);
    outFile.close();
}

// 获取当前时间，返回文件夹格式 "2024-10-09" 和时间戳 "14-24-34"
std::string AudioCapture::getCurrentTime(bool folderFormat, bool includeMilliseconds) {
    time_t rawtime;
    struct tm *timeinfo;
    char buffer[80];
    struct timeval tv;

    time(&rawtime);
    timeinfo = localtime(&rawtime);
    
    if (folderFormat) {
        strftime(buffer, sizeof(buffer), "%Y-%m-%d", timeinfo);
    } else {
        if (includeMilliseconds) {
            gettimeofday(&tv, nullptr);
            strftime(buffer, sizeof(buffer), "%H-%M-%S", timeinfo);
            sprintf(buffer + strlen(buffer), "-%03ld", tv.tv_usec / 1000);
        } else {
            strftime(buffer, sizeof(buffer), "%H-%M-%S", timeinfo);
        }
    }

    return std::string(buffer);
}

// 创建文件夹路径
void AudioCapture::createDirectory(const std::string &dir) {
    if (mkdir(dir.c_str(), 0777) != 0 && errno != EEXIST) {
        std::cerr << "Error creating directory: " << dir << std::endl;
    }
}

