#ifndef ThreadSafeQueue_hh
#define ThreadSafeQueue_hh

#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>

template <typename T>
class ThreadSafeQueue {
public:
    ThreadSafeQueue() : _stop(false) {}
    ~ThreadSafeQueue() = default;

    // 데이터 삽입 후 대기 중인 Consumer 스레드 깨움
    void Push(T item) {
        std::lock_guard<std::mutex> lock(_mutex);
        _queue.push(item);
        _cv.notify_one(); 
    }

    // 데이터 추출 (큐가 비어있으면 데이터가 들어올 때까지 슬립 상태로 대기)
    bool WaitAndPop(T& popped_item) {
        std::unique_lock<std::mutex> lock(_mutex);
        _cv.wait(lock, [this]() { return !_queue.empty() || _stop.load(); });

        if (_queue.empty() && _stop.load()) {
            return false; // 프로그램 종료 시 안전 탈출
        }

        popped_item = _queue.front();
        _queue.pop();
        return true;
    }

    // [핵심] 락이 걸리지 않게 즉시 추출 시도 (Object Pool 재활용에 사용)
    bool TryPop(T& popped_item) {
        std::lock_guard<std::mutex> lock(_mutex);
        if (_queue.empty()) return false;
        popped_item = _queue.front();
        _queue.pop();
        return true;
    }

    // 강제 종료 시그널 발생 시 무한 대기 중인 모든 스레드 깨우기
    void Stop() {
        _stop.store(true);
        _cv.notify_all();
    }

    size_t Size() const {
        std::lock_guard<std::mutex> lock(_mutex);
        return _queue.size();
    }

private:
    std::queue<T> _queue;
    mutable std::mutex _mutex;
    std::condition_variable _cv;
    std::atomic<bool> _stop;
};
#endif
