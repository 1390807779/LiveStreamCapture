#ifndef MEDIAQUEUE_HPP
#define MEDIAQUEUE_HPP

#include <iostream>
#include <queue>
#include <memory>
#include <shared_mutex>
#include <utility>
#include <mutex>

enum MediaQueueType
{
    QueueUnknowType = -1,
    QueueAudioType,
    QueueVideoType
};

template <typename T>

class MediaQueue
{
private: 
    std::queue<std::shared_ptr<T>> audioQueue;
    std::queue<std::shared_ptr<T>> videoQueue;
    std::shared_mutex mutex;
public:
    MediaQueue();
    ~MediaQueue();
    void updateQueue(MediaQueueType type, std::queue<std::shared_ptr<T>> queue);
    void pushWithType(MediaQueueType type, std::shared_ptr<T> t);
    std::shared_ptr<T> getTopWithType(MediaQueueType type);
    std::shared_ptr<T> popWithType(MediaQueueType type);
    int getQueueSizeWithType(MediaQueueType type);
    std::queue<std::shared_ptr<T>> getQueueWithType(MediaQueueType type);
};

template <typename T>
MediaQueue<T>::MediaQueue():
    audioQueue(std::queue<std::shared_ptr<T>>()),
    videoQueue(std::queue<std::shared_ptr<T>>())
{
}

template <typename T>
MediaQueue<T>::~MediaQueue()
{
}

template <typename T>
void MediaQueue<T>::updateQueue(MediaQueueType type, std::queue<std::shared_ptr<T>> queue)
{
    std::unique_lock<std::shared_mutex> lock(mutex);
    switch (type)
    {
    case QueueAudioType:
        audioQueue = queue;
        break;
    case QueueVideoType:
        videoQueue = queue;
        break;
    default:
        break;
    }
}

template <typename T>
void MediaQueue<T>::pushWithType(MediaQueueType type, std::shared_ptr<T> t)
{
    std::unique_lock<std::shared_mutex> lock(mutex);
    switch (type)
    {
    case QueueAudioType:
        audioQueue.push(t);
        break;
    case QueueVideoType:
        videoQueue.push(t);
        break;
    default:
        break;
    }
}

template <typename T>
std::shared_ptr<T> MediaQueue<T>::getTopWithType(MediaQueueType type)
{
    std::shared_lock<std::shared_mutex> lock(mutex);
    switch (type)
    {
    case QueueAudioType:
        return audioQueue.empty() ? nullptr : audioQueue.front();
        break;
    case QueueVideoType:
        return videoQueue.empty() ? nullptr : videoQueue.front();
        break;
    default:
        break;
    }
    return nullptr;
}

template <typename T>
std::shared_ptr<T> MediaQueue<T>::popWithType(MediaQueueType type)
{
    std::shared_ptr<T> t = getTopWithType(type);
    if (!t)
    {
        return t;
    }
    std::unique_lock<std::shared_mutex> lock(mutex);
    switch (type)
    {
    case QueueAudioType:
        audioQueue.pop();
        break;
    case QueueVideoType:
        videoQueue.pop();
        break;
    default:
        break;
    }
    return t;
}

template <typename T>
int MediaQueue<T>::getQueueSizeWithType(MediaQueueType type)
{
    std::shared_lock<std::shared_mutex> lock(mutex);
    switch (type)
    {
    case QueueAudioType:
        return audioQueue.size();
        break;
    case QueueVideoType:
        return videoQueue.size();
        break;
    default:
        break;
    }
    return 0;
}

template <typename T>
std::queue<std::shared_ptr<T>> MediaQueue<T>::getQueueWithType(MediaQueueType type)
{
    std::unique_lock<std::shared_mutex> lock(mutex);
    switch (type)
    {
    case QueueAudioType:
        return audioQueue;
        break;
    case QueueVideoType:
        return videoQueue;
        break;
    default:
        break;
    }
    return std::queue<std::shared_ptr<T>>();
}

#endif