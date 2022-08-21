#include <queue>
#include <mutex>
#include <condition_variable>

using namespace std;

#define QUEUE_SIZE 10  // tamaño máximo de la cola

template <typename T> class Queue{
    private:
        queue<T> queue_msg;
        mutex q_mutex;
        condition_variable q_condv;
    public:

    Queue() = default;
    ~Queue() = default;

    void push(T task);
    T pop();
};

template <typename T> void Queue<T>::push(T task){
    unique_lock<mutex> lock_queue(q_mutex);
    while (queue_msg.size() >= QUEUE_SIZE) {
        q_condv.wait(lock_queue);
    }
    queue_msg.push(task);
    lock_queue.unlock();

    if(queue_msg.size() == 1)
        q_condv.notify_one();
}

template <typename T> T Queue<T>::pop(){
    unique_lock<mutex> lock_queue(q_mutex);
    while(queue_msg.empty())
        q_condv.wait(lock_queue);

    T msg = queue_msg.front();
    queue_msg.pop();
    lock_queue.unlock();

    if(queue_msg.size() == QUEUE_SIZE-1)
        q_condv.notify_one();

    return msg;
}
