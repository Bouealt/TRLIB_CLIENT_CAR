#include "Audio.h"
#include <sys/time.h> // �������� gettimeofday ��ͷ�ļ�
#include <cstring>    // �������� strlen ��ͷ�ļ�
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
    data.samplesPerInterval = sampleRate * saveIntervalMs / 1000; // ÿ�α���Ĳ�����
}

AudioCapture::~AudioCapture()
{
    stop();
    terminatePortAudio();
}

void AudioCapture::initPortAudio()
{
    PaError err = Pa_Initialize();
    // if (err != paNoError)
    // {
    //     std::cerr << "PortAudio error: " << Pa_GetErrorText(err) << std::endl;
    //     throw std::runtime_error("Failed to initialize PortAudio.");
    // }

    err = Pa_OpenDefaultStream(&stream, numChannels, 0, paInt16, sampleRate, framesPerBuffer, recordCallback, &data);
    // if (err != paNoError)
    // {
    //     std::cerr << "PortAudio error: " << Pa_GetErrorText(err) << std::endl;
    //     throw std::runtime_error("Failed to open audio stream.");
    // }
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
    // ��ʼ¼��
    PaError err = Pa_StartStream(stream);
    if (err != paNoError)
    {
        std::cerr << "PortAudio error: " << Pa_GetErrorText(err) << std::endl;
        throw std::runtime_error("Failed to start audio stream.");
    }

    std::string baseDir = std::filesystem::current_path().string(); // �������ǰĿ¼Ϊ����Ŀ¼

    std::string lastSecond = "";
    while (!runningFlag || runningFlag->load())
    {
        Pa_Sleep(saveIntervalMs); // ÿ�εȴ�������

        if (runningFlag && !runningFlag->load())
        {
            break;
        }
        std::string currentDateTime = getCurrentTime();
        std::string curDateTime = currentDateTime.substr(0, 19);
        std::string seconds = currentDateTime.substr(17, 2); // �и����
        std::string msTime = currentDateTime.substr(20, 23); // �и������

        std::string file_path = baseDir + "/dataCapture/Car0001/Audio/" + curDateTime;
        // ��������ı䣬�����µ��ļ���
        if (seconds != lastSecond)
        {
            std::string folderPath = baseDir + "/dataCapture/Car0001/Audio/" + curDateTime;
            fs::create_directories(folderPath);
            lastSecond = seconds;
        }

        std::string filename = file_path + "/audio" + "-" + msTime + ".wav";

        // ���浱ǰ��Ƶ���ݵ��ļ�
        saveAudioData(filename);
        // std::cout << "Saved audio to: " << filename << std::endl;
        SensorData audio_data;
        audio_data.sensor_type = "audio";
        audio_data.readable_timestamp = currentDateTime;
        audio_data.file_path = filename;

        // // ��ӡ SensorData �ṹ�������
        // std::cout << "Sensor Type: " << audio_data.sensor_type << "\n"
        //           << "Timestamp: " << audio_data.readable_timestamp << "\n"
        //           << "File Path: " << audio_data.file_path << "\n";
        {
            std::lock_guard<std::mutex> lock(captureToProcessingQueueMutex);
            captureToProcessingQueue.push(audio_data);      // ����Ŀ¼·��
            captureToProcessingQueueCondition.notify_one(); // ֪ͨ����ģ����������
        }
        data.recordedSamples.clear(); // ����Ѿ��������Ƶ����
    }
    
    stop();
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

// ������Ƶ����ΪWAV�ļ�
void AudioCapture::saveAudioData(const std::string &filePath)
{
    std::ofstream outFile(filePath, std::ios::binary);
    if (!outFile.is_open())
    {
        std::cerr << "Failed to open file for writing: " << filePath << std::endl;
        return;
    }

    // д�� WAV �ļ�ͷ��
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

    // д����Ƶ����
    outFile.write(reinterpret_cast<const char *>(data.recordedSamples.data()), subchunk2Size);
    outFile.close();
}

// ��ȡ��ǰʱ�䣬�����ļ��и�ʽ "2024-10-09" ��ʱ��� "14-24-34"
std::string AudioCapture::getCurrentTime()
{
    // ��ȡ��ǰʱ���
    auto now = std::chrono::system_clock::now();
    auto now_time_t = std::chrono::system_clock::to_time_t(now);
    auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

    // ��ȡ��ǰʱ���tm�ṹ
    std::tm tm;
    localtime_r(&now_time_t, &tm);

    // ��ʱ����Ϣ��ʽ��Ϊ�ַ���
    std::stringstream ss;
    ss << std::put_time(&tm, "%Y-%m-%d/%H-%M-%S") << '-' << std::setfill('0') << std::setw(3) << now_ms.count();
    // std::cout << ss.str() << std::endl;     // ��ӡ��ǰ��ʱ�䣬ʱ-��-��-����
    return ss.str();
}
