#!/bin/bash

# 创建性能分析目录
TIMESTAMP=$(date +"%Y%m%d_%H%M%S")
PERF_DIR="performance_analysis_${TIMESTAMP}"
mkdir -p $PERF_DIR

echo "Starting comprehensive performance analysis..."

# CPU性能分析
echo "Running Callgrind analysis..."
valgrind --tool=callgrind --callgrind-out-file=${PERF_DIR}/callgrind.out ./VehicleClient  &
PROG_PID=$!
sleep 2  # 给程序启动一点时间

# 系统资源监控 (10秒采样)
echo "Monitoring system resources..."
mpstat 1 10 > ${PERF_DIR}/cpu_stats.log &
iostat -x 1 10 > ${PERF_DIR}/io_stats.log &
vmstat 1 10 > ${PERF_DIR}/memory_stats.log &

# 进程特定监控
echo "Monitoring process resources..."
pidstat -p $PROG_PID 1 10 > ${PERF_DIR}/process_stats.log &

# 等待程序完成
wait $PROG_PID

# 分析Callgrind结果
callgrind_annotate ${PERF_DIR}/callgrind.out > ${PERF_DIR}/callgrind_report.txt

echo "Performance analysis complete. Results stored in ${PERF_DIR}/"