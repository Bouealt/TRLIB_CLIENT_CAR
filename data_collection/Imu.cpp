#include "Imu.h"
#include <iostream>
#include <vector>
#include <iomanip>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <thread>

// 定义全局变量
const int buf_length = 11;
std::vector<uint8_t> RxBuff(buf_length, 0);
std::vector<uint8_t> ACCData(6, 0);
std::vector<uint8_t> GYROData(6, 0);
std::vector<uint8_t> AngleData(6, 0);
int FrameState = 0;
int CheckSum = 0;

int data_length = 0;
std::vector<float> acc(3, 0.0);
std::vector<float> gyro(3, 0.0);
std::vector<float> Angle(3, 0.0);
int start = 0;

Imu::Imu() {}
Imu::~Imu() {}

bool Imu::activate()
{   
    //sudo chmod 666 /dev/ttyUSB0 保证运行
    //sudo chmod 666 /dev/video0 保证运行
    const char *port = "/dev/ttyUSB0"; // Linux serial port
    int baud_rate = B9600;             // Same baud rate as the INERTIAL navigation module

    int serial_port = open_serial(port, baud_rate);
    if (serial_port < 0)
        return false;

    std::cout << "Serial is Opened\n";

    std::string baseDir = std::filesystem::current_path().string(); // 假设程序当前目录为基础目录
    std::string lastSecond = "";
    while (true)
    {
        auto start_time = std::chrono::steady_clock::now(); // 记录开始时间
        
        std::string currentDateTime = getCurrentTime();
        std::string curDateTime = currentDateTime.substr(0, 19);
        std::string seconds = currentDateTime.substr(17, 2); // 切割出秒
        std::string msTime = currentDateTime.substr(20, 23); // 切割出毫秒

        std::string filename = baseDir + "/dataCapture/Car0001/Imu/" + curDateTime+ "/imu" + "-" + msTime + ".txt";
        // 如果秒数改变，创建新的文件夹
        if (seconds != lastSecond)
        {
            std::string folderPath = baseDir + "/dataCapture/Car0001/Imu/" + curDateTime;
            fs::create_directories(folderPath);
            lastSecond = seconds;
        }
        uint8_t RXdata;

        if (read(serial_port, &RXdata, 1) > 0)
        {
            DueData(RXdata);

            // 保存数据到文件
            save_data_to_file(filename ,acc, gyro, Angle);
        }

        // 等待直到200ms过去，确保循环严格控制在每200ms执行一次
        auto end_time = std::chrono::steady_clock::now();
        std::chrono::milliseconds elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        if (elapsed.count() < 200)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(200 - elapsed.count()));
        }

        SensorData imu_data;
        imu_data.sensor_type = "imu";
        imu_data.readable_timestamp = currentDateTime;
        imu_data.file_path = filename;
        {
            std::lock_guard<std::mutex> lock(captureToProcessingQueueMutex);
            captureToProcessingQueue.push(imu_data);            // 推送目录路径
            captureToProcessingQueueCondition.notify_one(); // 通知处理模块有新数据
        }
    }

    close(serial_port);
    return true;
}

// 获取加速度数据
std::vector<float> Imu::get_acc(const std::vector<uint8_t> &datahex)
{
    int axl = datahex[0], axh = datahex[1];
    int ayl = datahex[2], ayh = datahex[3];
    int azl = datahex[4], azh = datahex[5];
    float k_acc = 16.0;

    float acc_x = (axh << 8 | axl) / 32768.0 * k_acc;
    float acc_y = (ayh << 8 | ayl) / 32768.0 * k_acc;
    float acc_z = (azh << 8 | azl) / 32768.0 * k_acc;

    if (acc_x >= k_acc)
        acc_x -= 2 * k_acc;
    if (acc_y >= k_acc)
        acc_y -= 2 * k_acc;
    if (acc_z >= k_acc)
        acc_z -= 2 * k_acc;

    return {acc_x, acc_y, acc_z};
}

// 获取陀螺仪数据
std::vector<float> Imu::get_gyro(const std::vector<uint8_t> &datahex)
{
    int wxl = datahex[0], wxh = datahex[1];
    int wyl = datahex[2], wyh = datahex[3];
    int wzl = datahex[4], wzh = datahex[5];
    float k_gyro = 2000.0;

    float gyro_x = (wxh << 8 | wxl) / 32768.0 * k_gyro;
    float gyro_y = (wyh << 8 | wyl) / 32768.0 * k_gyro;
    float gyro_z = (wzh << 8 | wzl) / 32768.0 * k_gyro;

    if (gyro_x >= k_gyro)
        gyro_x -= 2 * k_gyro;
    if (gyro_y >= k_gyro)
        gyro_y -= 2 * k_gyro;
    if (gyro_z >= k_gyro)
        gyro_z -= 2 * k_gyro;

    return {gyro_x, gyro_y, gyro_z};
}

// 获取角度数据
std::vector<float> Imu::get_angle(const std::vector<uint8_t> &datahex)
{
    int rxl = datahex[0], rxh = datahex[1];
    int ryl = datahex[2], ryh = datahex[3];
    int rzl = datahex[4], rzh = datahex[5];
    float k_angle = 180.0;

    float angle_x = (rxh << 8 | rxl) / 32768.0 * k_angle;
    float angle_y = (ryh << 8 | ryl) / 32768.0 * k_angle;
    float angle_z = (rzh << 8 | rzl) / 32768.0 * k_angle;

    if (angle_x >= k_angle)
        angle_x -= 2 * k_angle;
    if (angle_y >= k_angle)
        angle_y -= 2 * k_angle;
    if (angle_z >= k_angle)
        angle_z -= 2 * k_angle;

    return {angle_x, angle_y, angle_z};
}

// 保存数据到文件
void Imu::save_data_to_file(const std::string &folder_path, const std::vector<float> &acc, const std::vector<float> &gyro, const std::vector<float> &angle)
{
    std::ofstream file(folder_path);
    if (file.is_open())
    {
        file << "acc: " << acc[0] << " " << acc[1] << " " << acc[2] << "\n";
        file << "gyro: " << gyro[0] << " " << gyro[1] << " " << gyro[2] << "\n";
        file << "angle: " << angle[0] << " " << angle[1] << " " << angle[2] << "\n";
        file.close();
    }
    else
    {
        std::cerr << "Failed to open file at " << folder_path ;
    }
}

// 处理接收到的数据
void Imu::GetDataDeal(const std::vector<uint8_t> &list_buf)
{
    if (list_buf[buf_length - 1] != CheckSum)
        return; // 校验码不正确

    if (list_buf[1] == 0x51)
    { // 加速度输出
        std::copy(list_buf.begin() + 2, list_buf.begin() + 8, ACCData.begin());
        acc = get_acc(ACCData);
    }
    else if (list_buf[1] == 0x52)
    { // 角速度输出
        std::copy(list_buf.begin() + 2, list_buf.begin() + 8, GYROData.begin());
        gyro = get_gyro(GYROData);
    }
    else if (list_buf[1] == 0x53)
    { // 姿态角度输出
        std::copy(list_buf.begin() + 2, list_buf.begin() + 8, AngleData.begin());
        Angle = get_angle(AngleData);
    }

}
// 处理输入的单字节数据
void Imu::DueData(uint8_t inputdata)
{
    if (inputdata == 0x55 && start == 0)
    {
        start = 1;
        data_length = 11;
        CheckSum = 0;
        std::fill(RxBuff.begin(), RxBuff.end(), 0);
    }

    if (start == 1)
    {
        CheckSum += inputdata;                        // 校验码计算
        RxBuff[buf_length - data_length] = inputdata; // 保存数据
        data_length--;
        if (data_length == 0)
        { // 接收到完整的数据
            CheckSum = (CheckSum - inputdata) & 0xFF;
            start = 0;
            GetDataDeal(RxBuff); // 处理数据
        }
    }
}

// 打开串口并配置参数
int Imu::open_serial(const char *port, int baud_rate)
{
    int serial_port = open(port, O_RDWR);
    if (serial_port < 0)
    {
        std::cerr << "Error opening port " << port << "\n";
        return -1;
    }
    struct termios tty;
    memset(&tty, 0, sizeof(tty));

    if (tcgetattr(serial_port, &tty) != 0)
    {
        std::cerr << "Error getting terminal attributes\n";
        return -1;
    }

    cfsetispeed(&tty, baud_rate);
    cfsetospeed(&tty, baud_rate);

    tty.c_cflag |= (CLOCAL | CREAD);
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;
    tty.c_cflag &= ~PARENB;
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CRTSCTS;

    tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    tty.c_iflag &= ~(IXON | IXOFF | IXANY);
    tty.c_oflag &= ~OPOST;

    tcsetattr(serial_port, TCSANOW, &tty);

    return serial_port;
}

// 获取当前时间，返回文件夹格式 "2024-10-09" 和时间戳 "14-24-34"
std::string Imu::getCurrentTime()
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
