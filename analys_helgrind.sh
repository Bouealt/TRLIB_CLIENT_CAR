#!/bin/bash
# 分析Helgrind日志的脚本
# 使用方法: ./analyze_helgrind.sh helgrind_log.txt

LOG_FILE=$1

if [ ! -f "$LOG_FILE" ]; then
    echo "日志文件不存在: $LOG_FILE"
    exit 1
fi

# 创建输出目录
OUTPUT_DIR="helgrind_analysis"
mkdir -p $OUTPUT_DIR

echo "=== Helgrind日志分析 ===" > "$OUTPUT_DIR/summary.txt"
echo "分析文件: $LOG_FILE" >> "$OUTPUT_DIR/summary.txt"
echo "分析时间: $(date)" >> "$OUTPUT_DIR/summary.txt"
echo "" >> "$OUTPUT_DIR/summary.txt"

# 统计竞态条件数量
DATA_RACE_COUNT=$(grep -c "Possible data race" "$LOG_FILE")
echo "数据竞争总数: $DATA_RACE_COUNT" >> "$OUTPUT_DIR/summary.txt"

# 统计锁问题
LOCK_ORDER_COUNT=$(grep -c "lock order" "$LOG_FILE")
echo "锁顺序问题: $LOCK_ORDER_COUNT" >> "$OUTPUT_DIR/summary.txt"

# 统计条件变量问题
COND_VAR_COUNT=$(grep -c "Condition variable" "$LOG_FILE")
echo "条件变量问题: $COND_VAR_COUNT" >> "$OUTPUT_DIR/summary.txt"

# 其他常见问题
UNINIT_MEM_COUNT=$(grep -c "uninitialised value" "$LOG_FILE")
echo "未初始化内存访问: $UNINIT_MEM_COUNT" >> "$OUTPUT_DIR/summary.txt"

echo "" >> "$OUTPUT_DIR/summary.txt"
echo "=== 问题按模块分类 ===" >> "$OUTPUT_DIR/summary.txt"

# 分析关键模块问题
modules=("opencv" "camera" "imu" "mic" "network" "file" "thread" "mutex" "queue")
for module in "${modules[@]}"; do
    count=$(grep -i "$module" "$LOG_FILE" | grep -c "Possible data race")
    echo "$module 相关数据竞争: $count" >> "$OUTPUT_DIR/summary.txt"
    
    # 提取每个模块的前5个不同类型的问题用于详细分析
    if [ $count -gt 0 ]; then
        echo "提取 $module 相关的关键问题示例..."
        mkdir -p "$OUTPUT_DIR/$module"
        
        # 提取数据竞争问题
        grep -n "Possible data race" "$LOG_FILE" | grep -i "$module" | head -5 > "$OUTPUT_DIR/$module/data_races.txt"
        
        # 为每个问题提取上下文
        while read line; do
            line_num=$(echo $line | cut -d':' -f1)
            start=$((line_num - 5))
            [ $start -lt 1 ] && start=1
            end=$((line_num + 25))
            sed -n "${start},${end}p" "$LOG_FILE" > "$OUTPUT_DIR/$module/context_${line_num}.txt"
        done < "$OUTPUT_DIR/$module/data_races.txt"
    fi
done

echo "" >> "$OUTPUT_DIR/summary.txt"
echo "=== 最常见的调用栈模式 ===" >> "$OUTPUT_DIR/summary.txt"

# 提取最常见的10种调用栈模式
grep -A 5 "Possible data race" "$LOG_FILE" | sort | uniq -c | sort -nr | head -10 > "$OUTPUT_DIR/top_stacks.txt"
cat "$OUTPUT_DIR/top_stacks.txt" >> "$OUTPUT_DIR/summary.txt"

# 提取关键文件和行号
echo "" >> "$OUTPUT_DIR/summary.txt"
echo "=== 项目代码中的热点问题 ===" >> "$OUTPUT_DIR/summary.txt"

# 假设您的项目代码文件有特定的命名模式或路径
# 这里我们尝试找出您项目代码中出现最多的文件和行号
grep -o "[a-zA-Z0-9_]\+\.cpp:[0-9]\+" "$LOG_FILE" | sort | uniq -c | sort -nr | head -20 > "$OUTPUT_DIR/hotspots.txt"
cat "$OUTPUT_DIR/hotspots.txt" >> "$OUTPUT_DIR/summary.txt"

echo "" >> "$OUTPUT_DIR/summary.txt"
echo "=== OpenCV相关问题分析 ===" >> "$OUTPUT_DIR/summary.txt"

# 特别分析OpenCV问题，因为从您的日志片段看这是主要问题来源
grep -A 10 -B 2 "opencv" "$LOG_FILE" | grep "Possible data race" | sort | uniq -c | sort -nr | head -10 > "$OUTPUT_DIR/opencv_issues.txt"
cat "$OUTPUT_DIR/opencv_issues.txt" >> "$OUTPUT_DIR/summary.txt"

echo "" >> "$OUTPUT_DIR/summary.txt"
echo "=== 竞争访问的内存地址分析 ===" >> "$OUTPUT_DIR/summary.txt"

# 分析最常被竞争访问的内存地址
grep -o "Address 0x[0-9a-f]\+" "$LOG_FILE" | sort | uniq -c | sort -nr | head -20 > "$OUTPUT_DIR/addresses.txt"
cat "$OUTPUT_DIR/addresses.txt" >> "$OUTPUT_DIR/summary.txt"

echo "分析完成，结果保存在 $OUTPUT_DIR 目录下"
echo "摘要文件: $OUTPUT_DIR/summary.txt"

# 打印摘要文件内容
cat "$OUTPUT_DIR/summary.txt"