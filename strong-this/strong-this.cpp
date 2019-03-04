#include "pch.h"

using namespace std;
using namespace std::chrono;

struct Scheduler
{
	using Task = function<void()>;

	Scheduler()
	{
		m_f = async(launch::async, &Scheduler::Run, this);
	}

	~Scheduler()
	{
		m_finish = true;
		if (m_f.valid())
		{
			m_f.get();
		}
	}

	void Post(Task task)
	{
		if (m_finish)
		{
			throw logic_error("scheduler is terminating");
		}
	}
private:
	void Run()
	{
	}
	atomic<bool> m_finish = false;
	future<void> m_f;

	deque<Task> m_tasks;

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
		m_io->AsyncRead([self = shared_from_this()](string&& data){
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
	{
		auto scheduler = make_shared<Scheduler>();
		auto asyncIO = make_shared<AsyncIO>(move(scheduler));
		make_shared<Consumer>(move(asyncIO))->Run();
		cout << "Press enter to quit\n";
		cin.get();
	}
	cout << "Exiting\n";
}
