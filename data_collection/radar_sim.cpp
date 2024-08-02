#include <iostream>
#include <fstream>
#include <pcl/point_types.h>
#include <pcl/io/pcd_io.h>
#include <random>
#include <string>
#include <filesystem>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <thread>

namespace fs = std::filesystem;

struct PointXYZI {
    float x;
    float y;
    float z;
    float intensity;
};

// 保存点云数据到二进制文件
void saveBinFile(const std::string& filename, const pcl::PointCloud<pcl::PointXYZI>::Ptr& cloud) {
    std::ofstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "Error opening file: " << filename << std::endl;
        return;
    }

    for (const auto& point : cloud->points) {
        PointXYZI p { point.x, point.y, point.z, point.intensity };
        file.write(reinterpret_cast<const char*>(&p), sizeof(PointXYZI));
    }

    file.close();
}

// 生成随机点云数据
pcl::PointCloud<pcl::PointXYZI>::Ptr generatePointCloud(int num_points) {
    pcl::PointCloud<pcl::PointXYZI>::Ptr cloud(new pcl::PointCloud<pcl::PointXYZI>);
    std::default_random_engine generator;
    std::uniform_real_distribution<float> distribution(0.0, 100.0);

    for (int i = 0; i < num_points; ++i) {
        pcl::PointXYZI point;
        point.x = distribution(generator);
        point.y = distribution(generator);
        point.z = distribution(generator);
        point.intensity = distribution(generator);
        cloud->points.push_back(point);
    }

    cloud->width = cloud->points.size();
    cloud->height = 1;
    cloud->is_dense = true;

    return cloud;
}

// 获取当前时间并格式化
std::string getCurrentTimeString() {
    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&in_time_t), "%H-%M-%S") << '-' << std::setfill('0') << std::setw(3) << ms.count();
    return ss.str();
}

// 创建目录并保存点云数据
void savePointCloudData(const std::string& base_dir, int num_points, int frequency, int duration_seconds) {
    auto current_time = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(current_time);
    std::stringstream date_ss;
    date_ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d");

    std::string date_dir = base_dir + "/" + date_ss.str();
    fs::create_directories(date_dir);

    int num_files = frequency * duration_seconds;
    for (int i = 0; i < num_files; ++i) {
        std::string time_str = getCurrentTimeString();
        std::string file_path = date_dir + "/" + time_str + ".bin";

        auto cloud = generatePointCloud(num_points);
        saveBinFile(file_path, cloud);

        std::cout << "Saved " << cloud->points.size() << " data points to " << file_path << std::endl;

        std::this_thread::sleep_for(std::chrono::milliseconds(1000 / frequency));
    }
}

int main() {
    // 可调整的参数
    std::string base_dir = "radar_sim";
    int num_points = 1000;       // 点的数量
    int frequency = 5;           // 采集频率（每秒采集次数）
    int duration_seconds = 10;   // 采集持续时间（秒）

    savePointCloudData(base_dir, num_points, frequency, duration_seconds);

    return 0;
}
