#!/usr/bin/env python3
"""
Helgrind日志模式识别脚本 - 帮助识别和分类数据竞争问题
用法: python3 helgrind_patterns.py helgrind_log.txt
"""

import sys
import re
import os
from collections import defaultdict, Counter

class RacePattern:
    def __init__(self):
        self.access_type = ""  # read/write
        self.size = 0
        self.address = ""
        self.thread1 = ""
        self.thread2 = ""
        self.stack1 = []
        self.stack2 = []
        self.key_functions1 = set()
        self.key_functions2 = set()
    
    def __str__(self):
        return f"Race: {self.access_type} of size {self.size} at {self.address} between threads {self.thread1} and {self.thread2}"
    
    def get_summary(self):
        """返回关键信息摘要"""
        return (
            f"Race: {self.access_type} of size {self.size} bytes\n"
            f"Thread {self.thread1} vs Thread {self.thread2}\n"
            f"Key functions in thread {self.thread1}: {', '.join(self.key_functions1)}\n"
            f"Key functions in thread {self.thread2}: {', '.join(self.key_functions2)}\n"
        )

def extract_key_function(line):
    """从调用栈行中提取关键函数名"""
    match = re.search(r'at 0x[0-9a-f]+: ([a-zA-Z0-9_:]+)', line)
    if match:
        return match.group(1)
    match = re.search(r'by 0x[0-9a-f]+: ([a-zA-Z0-9_:]+)', line)
    if match:
        return match.group(1)
    return None

def parse_helgrind_log(filename):
    """解析Helgrind日志文件，提取数据竞争模式"""
    if not os.path.exists(filename):
        print(f"错误: 文件 {filename} 不存在")
        return []
    
    with open(filename, 'r') as f:
        content = f.read()
    
    # 按问题块分割
    race_blocks = re.split(r'==\d+== -+', content)
    
    race_patterns = []
    for block in race_blocks:
        if "Possible data race during" not in block:
            continue
        
        pattern = RacePattern()
        
        # 提取基本信息
        race_match = re.search(r'Possible data race during ([a-z]+) of size (\d+) at (0x[0-9a-f]+) by thread #(\d+)', block)
        if race_match:
            pattern.access_type = race_match.group(1)
            pattern.size = int(race_match.group(2))
            pattern.address = race_match.group(3)
            pattern.thread1 = race_match.group(4)
        
        # 提取第二个线程信息
        conflict_match = re.search(r'This conflicts with a previous [a-z]+ .* by thread #(\d+)', block)
        if conflict_match:
            pattern.thread2 = conflict_match.group(1)
        
        # 提取调用栈
        lines = block.split('\n')
        current_thread = None
        for line in lines:
            if "by thread #" in line and "at 0x" in line:
                current_thread = pattern.thread1
            elif "This conflicts with" in line:
                current_thread = pattern.thread2
            
            if current_thread == pattern.thread1 and "at 0x" in line or "by 0x" in line:
                pattern.stack1.append(line.strip())
                func = extract_key_function(line)
                if func and not func.startswith('???'):
                    pattern.key_functions1.add(func)
            
            elif current_thread == pattern.thread2 and "at 0x" in line or "by 0x" in line:
                pattern.stack2.append(line.strip())
                func = extract_key_function(line)
                if func and not func.startswith('???'):
                    pattern.key_functions2.add(func)
        
        race_patterns.append(pattern)
    
    return race_patterns

def classify_patterns(patterns):
    """根据模式将数据竞争问题分类"""
    classifications = defaultdict(list)
    
    for pattern in patterns:
        # 检查OpenCV相关问题
        if any("opencv" in func.lower() for func in pattern.key_functions1.union(pattern.key_functions2)):
            classifications["OpenCV"].append(pattern)
        
        # 检查摄像头相关问题
        if any("camera" in func.lower() or "videocapture" in func.lower() for func in pattern.key_functions1.union(pattern.key_functions2)):
            classifications["Camera"].append(pattern)
        
        # 检查并行处理相关问题
        if any("parallel" in func.lower() or "tbb" in func.lower() for func in pattern.key_functions1.union(pattern.key_functions2)):
            classifications["Parallel"].append(pattern)
        
        # 检查队列相关问题
        if any("queue" in func.lower() or "push" in func.lower() or "pop" in func.lower() for func in pattern.key_functions1.union(pattern.key_functions2)):
            classifications["Queue"].append(pattern)
        
        # 其他问题归为未分类
        if len([cat for cat, pats in classifications.items() if pattern in pats]) == 0:
            classifications["Uncategorized"].append(pattern)
    
    return classifications

def find_root_causes(patterns):
    """尝试识别根本原因并生成修复建议"""
    causes = defaultdict(int)
    solutions = {}
    
    # 常见问题模式
    for pattern in patterns:
        funcs = pattern.key_functions1.union(pattern.key_functions2)
        
        # OpenCV并行处理问题
        if any("parallel_for_" in func for func in funcs) and any("cv::" in func for func in funcs):
            causes["OpenCV并行处理"] += 1
            solutions["OpenCV并行处理"] = "使用cv::setNumThreads(1)禁用OpenCV并行处理"
        
        # 摄像头数据竞争
        if any("VideoCapture" in func for func in funcs) and pattern.access_type == "write":
            causes["摄像头数据写入竞争"] += 1
            solutions["摄像头数据写入竞争"] = "使用互斥锁保护VideoCapture对象和帧数据"
        
        # 队列操作竞争
        if any("queue" in func.lower() or "push" in func.lower() or "pop" in func.lower() for func in funcs):
            causes["队列操作竞争"] += 1
            solutions["队列操作竞争"] = "使用线程安全队列或添加互斥锁保护队列操作"
        
        # TBB线程冲突
        if any("tbb" in func.lower() for func in funcs):
            causes["TBB线程库冲突"] += 1
            solutions["TBB线程库冲突"] = "控制TBB线程数量或避免并行访问共享数据"
    
    return causes, solutions

def generate_report(filename, output_dir="helgrind_patterns"):
    """生成综合报告"""
    os.makedirs(output_dir, exist_ok=True)
    
    patterns = parse_helgrind_log(filename)
    classifications = classify_patterns(patterns)
    causes, solutions = find_root_causes(patterns)
    
    with open(f"{output_dir}/report.txt", "w") as f:
        f.write(f"Helgrind日志模式分析报告\n")
        f.write(f"分析文件: {filename}\n")
        f.write(f"找到的数据竞争总数: {len(patterns)}\n\n")
        
        f.write("问题分类统计:\n")
        for category, pats in classifications.items():
            f.write(f"  {category}: {len(pats)} 个问题\n")
        
        f.write("\n根本原因分析:\n")
        for cause, count in causes.items():
            f.write(f"  {cause}: {count} 个问题\n")
            if cause in solutions:
                f.write(f"    建议解决方案: {solutions[cause]}\n")
        
        f.write("\n按类别的问题详情:\n")
        for category, pats in classifications.items():
            f.write(f"\n{category} 类别 ({len(pats)} 个问题):\n")
            # 只显示每类前5个问题
            for i, pattern in enumerate(pats[:5]):
                f.write(f"  问题 {i+1}:\n")
                f.write(f"    {pattern.get_summary()}\n")
                if i >= 4 and len(pats) > 5:
                    f.write(f"  ... 以及其他 {len(pats) - 5} 个问题\n")
    
    # 为每个类别生成详细报告
    for category, pats in classifications.items():
        with open(f"{output_dir}/{category.lower()}_details.txt", "w") as f:
            f.write(f"{category} 类别的详细问题报告 ({len(pats)} 个问题)\n\n")
            for i, pattern in enumerate(pats):
                f.write(f"问题 {i+1}:\n")
                f.write(f"{pattern.get_summary()}\n")
                f.write("Thread 1 调用栈:\n")
                for line in pattern.stack1:
                    f.write(f"  {line}\n")
                f.write("Thread 2 调用栈:\n")
                for line in pattern.stack2:
                    f.write(f"  {line}\n")
                f.write("\n" + "-"*50 + "\n\n")
    
    # 生成解决方案文件
    with open(f"{output_dir}/solutions.txt", "w") as f:
        f.write("数据竞争问题的建议解决方案\n\n")
        
        # OpenCV解决方案
        if "OpenCV并行处理" in causes:
            f.write("1. OpenCV并行处理问题解决方案:\n")
            f.write("   - 在主程序开始时添加: cv::setNumThreads(1);\n")
            f.write("   - 这将禁用OpenCV的内部并行处理，避免TBB创建额外线程\n\n")
        
        # 摄像头解决方案
        if "摄像头数据写入竞争" in causes:
            f.write("2. 摄像头数据竞争解决方案:\n")
            f.write("   - 使用互斥锁保护VideoCapture对象\n")
            f.write("   - 使用帧的深拷贝(clone)而非浅拷贝\n")
            f.write("   - 实现示例代码:\n")
            f.write("""
   std::mutex camera_mutex;
   
   void captureFrames() {
       cv::Mat frame;
       {
           std::lock_guard<std::mutex> lock(camera_mutex);
           cap >> frame;
       }
       // 使用frame的深拷贝
       cv::Mat frame_copy = frame.clone();
       // 处理frame_copy...
   }
   """)
            f.write("\n")
        
        # 队列解决方案
        if "队列操作竞争" in causes:
            f.write("3. 队列操作竞争解决方案:\n")
            f.write("   - 实现线程安全的队列类\n")
            f.write("   - 使用互斥锁和条件变量\n")
            f.write("   - 实现示例代码:\n")
            f.write("""
   template<typename T>
   class ThreadSafeQueue {
   private:
       std::queue<T> queue;
       mutable std::mutex mutex;
       std::condition_variable cond;
   
   public:
       void push(T value) {
           std::lock_guard<std::mutex> lock(mutex);
           queue.push(std::move(value));
           cond.notify_one();
       }
       
       bool tryPop(T& value) {
           std::lock_guard<std::mutex> lock(mutex);
           if(queue.empty()) {
               return false;
           }
           value = std::move(queue.front());
           queue.pop();
           return true;
       }
       
       // 更多方法...
   };
   """)
            f.write("\n")
    
    print(f"分析完成，报告已保存到 {output_dir} 目录")
    print(f"主报告文件: {output_dir}/report.txt")
    print(f"建议解决方案: {output_dir}/solutions.txt")

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print(f"用法: {sys.argv[0]} helgrind_log.txt")
        sys.exit(1)
    
    generate_report(sys.argv[1])