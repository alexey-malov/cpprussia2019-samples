#include "pch.h"

using namespace std;
using namespace std::chrono;

template <typename T>
class ThreadSafeQueue
{
public:
	ThreadSafeQueue() = default;
	ThreadSafeQueue(const ThreadSafeQueue&) = delete;
	ThreadSafeQueue& operator=(const ThreadSafeQueue&) = delete;

	void Push(T value)
	{
		auto data = make_shared<T>(std::move(value));
		lock_guard lk(m_sync);
		m_queue.push(data);
		m_cond.notify_one();
	}

	[[nodiscard]] bool TryPop(T& value)
	{
		lock_guard lk{ m_sync };
		if (m_queue.empty())
		{
			return false;
		}
		value = std::move(*m_queue.front());
		m_queue.pop();
		return true;
	}

	[[nodiscard]] bool IsEmpty() const
	{
		lock_guard lk(m_sync);
		return m_queue.empty();
	}

private:
	mutable mutex m_sync;
	queue<shared_ptr<T>> m_queue;
	condition_variable m_cond;
};

struct Joiner
{
	Joiner(thread& t) noexcept
		: m_thread(t)
	{
	}
	Joiner(const Joiner&) = delete;
	Joiner& operator=(const Joiner&) = delete;
	~Joiner()
	{
		if (m_thread.joinable())
		{
			try
			{
				m_thread.join();
			}
			catch (...)
			{
			}
		}
	}

private:
	thread& m_thread;
};

struct Scheduler
{
	using Task = function<void()>;

	Scheduler()
		: m_thread(&Scheduler::WorkerThread, this)
	{
	}

	Scheduler(const Scheduler&) = delete;
	Scheduler& operator=(const Scheduler&) = delete;

	~Scheduler()
	{
		m_finish = true;
		if (m_thread.joinable())
		{
			m_thread.join();
		}
	}

	void Post(Task task)
	{
		if (m_finish)
		{
			throw logic_error("scheduler is terminating");
		}
		m_tasks.Push(std::move(task));
	}

private:
	void WorkerThread()
	{
		while (!m_finish)
		{
			Task task;
			if (m_tasks.TryPop(task))
			{
				task();
			}
			else
			{
				this_thread::yield();
			}
		}
	}
	atomic_bool m_finish = false;
	ThreadSafeQueue<Task> m_tasks;
	thread m_thread;
};

struct AsyncIO
{
	using Callback = function<void(string&& data)>;

	AsyncIO(shared_ptr<Scheduler> scheduler)
		: m_scheduler(scheduler)
	{
	}

	void AsyncRead(Callback callback)
	{
		m_scheduler->Post([callback = move(callback)] {
			this_thread::sleep_for(1s);
			random_device rnd;
			callback("some data: "s + to_string(rnd()));
		});
	}

private:
	shared_ptr<Scheduler> m_scheduler;
};

struct Consumer : enable_shared_from_this<Consumer>
{
public:
	Consumer(shared_ptr<AsyncIO> io)
		: m_io(move(io))
	{
	}

	void Run()
	{
		m_io->AsyncRead([self = shared_from_this()](string&& data) {
			self->OnRead(move(data));
		});
	}

private:
	void OnRead(string&& data)
	{
		cout << "First read data: " << data << "\n";
		m_io->AsyncRead([self = shared_from_this()](string&& data) {
			self->OnSecondRead(move(data));
		});
	}

	void OnSecondRead(string&& data)
	{
		cout << "Second read data: " << data << "\n";
	}

	shared_ptr<AsyncIO> m_io;
};

int main()
{
	auto scheduler = make_shared<Scheduler>();
	{
		auto asyncIO = make_shared<AsyncIO>(scheduler);
		make_shared<Consumer>(move(asyncIO))->Run();
		cout << "Press enter to quit\n";
		cin.get();
	}
	cout << "Exiting\n";
}
