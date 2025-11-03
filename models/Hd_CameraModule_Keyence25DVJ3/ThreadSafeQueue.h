#pragma once
#include <mutex>
#include <condition_variable>
#include <list>
#include <chrono>
#include <thread>
#include <type_traits>
template <typename T,typename Container=std::list<T>>
class ThreadSafeQueue
{
public:
	ThreadSafeQueue() = default;
	ThreadSafeQueue(const ThreadSafeQueue &r) = delete;
	ThreadSafeQueue(ThreadSafeQueue &&r) = delete;
	ThreadSafeQueue& operator=(const ThreadSafeQueue &r) = delete;
	ThreadSafeQueue& operator=(ThreadSafeQueue &&r) = delete;
public:
	typedef Container ContainerType;
	typedef T ValueType;
public:
	Container container;
private:
	std::mutex mux;
	std::condition_variable datacond;
	bool stopbit = false;
	bool popwaitflag = false;
	bool wakeupbit = true;
	std::condition_variable popwaitdatacond;
public:
	bool isEmpty();
	void push(const T &value);
	void push(T &&value);
	template <typename... Args>
	void emplace(Args&&... args);
	void pop(T &value);
	bool try_pop(T &value);
	bool wait_for_pop(int milisecond,T &value);
	bool top(T &value);
	void start() { std::lock_guard<std::mutex> lock(mux); stopbit = false; wakeupbit = true; };
	void end() { std::lock_guard<std::mutex> lock(mux); stopbit = true; datacond.notify_all(); };
	void clear() {
		std::lock_guard<std::mutex> lock(mux);
		container.clear();
	};
	void popwait();	//当pop函数进入休眠时此函数非阻塞，当pop未进入休眠时此函数阻塞直到pop进入休眠
	bool isstop() { return stopbit; };
	void DisableWakeup();
};

template<typename T, typename Container>
bool ThreadSafeQueue<T, Container>::isEmpty()
{
	std::lock_guard<std::mutex> lock(mux);
	return container.empty();
}

template<typename T, typename Container>
void ThreadSafeQueue<T, Container>::push(const T & value)
{
	std::lock_guard<std::mutex> lock(mux);
	if (wakeupbit)
	{
		container.push_back(value);
		datacond.notify_one();
	}
}

template<typename T, typename Container>
void ThreadSafeQueue<T, Container>::push(T && value)
{
	std::lock_guard<std::mutex> lock(mux);
	if (wakeupbit)
	{
		container.push_back(std::move(value));
		datacond.notify_one();
	}
}

template<typename T, typename Container>
void ThreadSafeQueue<T, Container>::pop(T & value)
{
	std::unique_lock<std::mutex> lock(mux);
	datacond.wait(lock, [&]() {
		if (!stopbit && container.empty())
		{
			popwaitflag = true;
			popwaitdatacond.notify_one();
		}
		else
			popwaitflag = false;
		return stopbit || !container.empty();
		});
	if (stopbit)
		return;
	value = std::move(container.front());
	container.pop_front();
}

template<typename T, typename Container>
bool ThreadSafeQueue<T, Container>::try_pop(T & value)
{
	std::unique_lock<std::mutex> lock(mux);
	if (container.empty())
		return false;
	value = container.front();
	container.pop_front();
	return true;
}

template<typename T, typename Container>
bool ThreadSafeQueue<T, Container>::wait_for_pop(int milisecond,T & value)
{
	std::unique_lock<std::mutex> lock(mux);
	std::chrono::milliseconds timeout(milisecond);
	if (datacond.wait_for(lock, timeout, [=]() {return stopbit || !container.empty(); }))
	{
		if (stopbit)
			return false;
		value = container.front();
		container.pop_front();
		return true;
	}
	else
		return false;
}

template<typename T, typename Container>
bool ThreadSafeQueue<T, Container>::top(T & value)
{
	std::lock_guard<std::mutex> lock(mux);
	if (container.empty())
		return false;
	value = container.front();
	return true;
}

template<typename T, typename Container>
void ThreadSafeQueue<T, Container>::popwait()
{
	std::unique_lock<std::mutex> lock(mux);
	popwaitdatacond.wait(lock, [&]() {
		return popwaitflag;
		});
}

template<typename T, typename Container>
inline void ThreadSafeQueue<T, Container>::DisableWakeup()
{
	wakeupbit = false;
}

template<typename T, typename Container>
template<typename ...Args>
void ThreadSafeQueue<T, Container>::emplace(Args && ...args)
{
	std::lock_guard<std::mutex> lock(mux);
	container.emplace_back(std::forward<Args>(args)...);
	datacond.notify_one();
}
