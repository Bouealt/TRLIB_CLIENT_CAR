#include "SharedQueue.h"

// ��ʼ�����ڴӲɼ�ģ�鵽����ģ��Ĺ�����к�ͬ��ԭ��
std::queue<std::string> captureToProcessingQueue;
std::mutex captureToProcessingQueueMutex;
std::condition_variable captureToProcessingQueueCondition;

// ��ʼ�����ڴӴ���ģ�鵽����ģ��Ĺ�����к�ͬ��ԭ��
std::queue<std::string> processingToSendingQueue;
std::mutex processingToSendingQueueMutex;
std::condition_variable processingToSendingQueueCondition;

// ��ʼ��ȫ�ֿ��Ʊ���
std::atomic<bool> cKeepRunning(true);
