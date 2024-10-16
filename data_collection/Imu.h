#ifndef IMU_H
#define IMU_H

#include <vector>
#include <string>
#include <cstdint>
#include <filesystem>
#include "../shared/SharedQueue.h"

namespace fs = std::filesystem;



class Imu
{
public:
    Imu();
    ~Imu();
    bool activate();

private:
    // 获取加速度数据
    std::vector<float> get_acc(const std::vector<uint8_t> &datahex);
    // 获取陀螺仪数据
    std::vector<float> get_gyro(const std::vector<uint8_t> &datahex);
    // 获取角度数据
    std::vector<float> get_angle(const std::vector<uint8_t> &datahex);
    // 保存数据到文件
    void save_data_to_file(const std::string &folder_path, const std::vector<float> &acc, const std::vector<float> &gyro, const std::vector<float> &angle);
    // 处理接收到的数据
    void GetDataDeal(const std::vector<uint8_t> &list_buf);
    // 处理输入的单字节数据
    void DueData(uint8_t inputdata);
    // 打开串口并配置参数
    int open_serial(const char *port, int baud_rate);
    std::string getCurrentTime();

    
};

#endif // IMU_H
