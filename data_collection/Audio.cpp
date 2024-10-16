#include "Audio.h"
#include <sys/time.h> // 包含用于 gettimeofday 的头文件
#include <cstring>    // 包含用于 strlen 的头文件
#include <iostream>
#include <fstream>
#include <ctime>
#include <sys/stat.h>
#include <unistd.h>
#include <sstream>
#include <iomanip>

AudioCapture::AudioCapture(int sampleRate, int framesPerBuffer, int channels, int saveIntervalMs)
    : sampleRate(sampleRate), framesPerBuffer(framesPerBuffer), numChannels(channels), saveIntervalMs(saveIntervalMs), stream(nullptr)
{
    data.samplesPerInterval = sampleRate * saveIntervalMs / 1000; // 每次保存的采样数
}

AudioCapture::~AudioCapture()
{
    stop();
    terminatePortAudio();
}

void AudioCapture::initPortAudio()
{
    PaError err = Pa_Initialize();
    if (err != paNoError)
    {
        std::cerr << "PortAudio error: " << Pa_GetErrorText(err) << std::endl;
        throw std::runtime_error("Failed to initialize PortAudio.");
    }

    err = Pa_OpenDefaultStream(&stream, numChannels, 0, paInt16, sampleRate, framesPerBuffer, recordCallback, &data);
    if (err != paNoError)
    {
        std::cerr << "PortAudio error: " << Pa_GetErrorText(err) << std::endl;
        throw std::runtime_error("Failed to open audio stream.");
    }
}

void AudioCapture::terminatePortAudio()
{
    if (stream)
    {
        Pa_StopStream(stream);
        Pa_CloseStream(stream);
    }
    Pa_Terminate();
}

void AudioCapture::start()
{
    initPortAudio();
    // 开始录音
    PaError err = Pa_StartStream(stream);
    if (err != paNoError)
    {
        std::cerr << "PortAudio error: " << Pa_GetErrorText(err) << std::endl;
        throw std::runtime_error("Failed to start audio stream.");
    }

    std::string baseDir = std::filesystem::current_path().string(); // 假设程序当前目录为基础目录

    std::string lastSecond = "";
    while (true)
    {
        Pa_Sleep(saveIntervalMs); // 每次等待保存间隔

        std::string currentDateTime = getCurrentTime();
        std::string curDateTime = currentDateTime.substr(0, 19);
        std::string seconds = currentDateTime.substr(17, 2); // 切割出秒
        std::string msTime = currentDateTime.substr(20, 23); // 切割出毫秒

        std::string file_path = baseDir + "/dataCapture/Car0001/Audio/" + curDateTime;
        // 如果秒数改变，创建新的文件夹
        if (seconds != lastSecond)
        {
            std::string folderPath = baseDir + "/dataCapture/Car0001/Audio/" + curDateTime;
            fs::create_directories(folderPath);
            lastSecond = seconds;
        }

        std::string filename = file_path + "/audio" + "-" + msTime + ".wav";

        // 保存当前音频数据到文件
        saveAudioData(filename);
        // std::cout << "Saved audio to: " << filename << std::endl;
        SensorData audio_data;
        audio_data.sensor_type = "audio";
        audio_data.readable_timestamp = currentDateTime;
        audio_data.file_path = filename;

        // // 打印 SensorData 结构体的内容
        // std::cout << "Sensor Type: " << audio_data.sensor_type << "\n"
        //           << "Timestamp: " << audio_data.readable_timestamp << "\n"
        //           << "File Path: " << audio_data.file_path << "\n";
        {
            std::lock_guard<std::mutex> lock(captureToProcessingQueueMutex);
            captureToProcessingQueue.push(audio_data);            // 推送目录路径
            captureToProcessingQueueCondition.notify_one(); // 通知处理模块有新数据
        }
        data.recordedSamples.clear(); // 清除已经保存的音频数据
    }
}

void AudioCapture::stop()
{
    if (stream)
    {
        Pa_StopStream(stream);
    }
}

int AudioCapture::recordCallback(const void *inputBuffer, void *outputBuffer,
                                 unsigned long framesPerBuffer,
                                 const PaStreamCallbackTimeInfo *timeInfo,
                                 PaStreamCallbackFlags statusFlags,
                                 void *userData)
{
    AudioData *data = (AudioData *)userData;
    const SAMPLE *rptr = (const SAMPLE *)inputBuffer;

    if (inputBuffer != nullptr)
    {
        for (unsigned long i = 0; i < framesPerBuffer; ++i)
        {
            data->recordedSamples.push_back(*rptr++);
        }
    }

    return paContinue;
}

// 保存音频数据为WAV文件
void AudioCapture::saveAudioData(const std::string &filePath)
{
    std::ofstream outFile(filePath, std::ios::binary);
    if (!outFile.is_open()) {
        std::cerr << "Failed to open file for writing: " << filePath << std::endl;
        return;
    }

    // 写入 WAV 文件头部
    outFile.write("RIFF", 4);
    uint32_t fileSize = 36 + data.recordedSamples.size() * sizeof(int16_t);
    outFile.write(reinterpret_cast<const char *>(&fileSize), 4);
    outFile.write("WAVE", 4);

    outFile.write("fmt ", 4);
    uint32_t subchunk1Size = 16;
    outFile.write(reinterpret_cast<const char *>(&subchunk1Size), 4);
    uint16_t audioFormat = 1; // PCM
    outFile.write(reinterpret_cast<const char *>(&audioFormat), 2);
    uint16_t numChannels = this->numChannels;
    outFile.write(reinterpret_cast<const char *>(&numChannels), 2);
    uint32_t sampleRate = this->sampleRate;
    outFile.write(reinterpret_cast<const char *>(&sampleRate), 4);
    uint16_t bitsPerSample = 16;
    uint32_t byteRate = sampleRate * numChannels * (bitsPerSample / 8);
    outFile.write(reinterpret_cast<const char *>(&byteRate), 4);
    uint16_t blockAlign = numChannels * (bitsPerSample / 8);
    outFile.write(reinterpret_cast<const char *>(&blockAlign), 2);
    outFile.write(reinterpret_cast<const char *>(&bitsPerSample), 2);

    outFile.write("data", 4);
    uint32_t subchunk2Size = data.recordedSamples.size() * sizeof(int16_t);
    outFile.write(reinterpret_cast<const char *>(&subchunk2Size), 4);

    // 写入音频数据
    outFile.write(reinterpret_cast<const char *>(data.recordedSamples.data()), subchunk2Size);
    outFile.close();
}


// 获取当前时间，返回文件夹格式 "2024-10-09" 和时间戳 "14-24-34"
std::string AudioCapture::getCurrentTime()
{
    // 获取当前时间点
    auto now = std::chrono::system_clock::now();
    auto now_time_t = std::chrono::system_clock::to_time_t(now);
    auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

    // 获取当前时间的tm结构
    std::tm tm;
    localtime_r(&now_time_t, &tm);

    // 将时间信息格式化为字符串
    std::stringstream ss;
    ss << std::put_time(&tm, "%Y-%m-%d/%H-%M-%S") << '-' << std::setfill('0') << std::setw(3) << now_ms.count();
    // std::cout << ss.str() << std::endl;     // 打印当前的时间，时-分-秒-毫秒
    return ss.str();
}
