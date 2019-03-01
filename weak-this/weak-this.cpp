// weak-this.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"

using namespace std;
using namespace std::chrono;

struct Scheduler
{
	using Task = function<void()>;

	~Scheduler()
	{
		FinishCurrentTask();
	}

	void Post(Task task)
	{
		FinishCurrentTask();
		m_f = async(launch::async, move(task));
	}

	void FinishCurrentTask()
	{
		if (m_f.valid())
		{
			m_f.get();
		}
	}

private:
	future<void> m_f;
};

struct CancellationToken
{
	void Cancel()
	{
		*m_cancelled = true;
	}

	bool IsCancelled() const
	{
		return *m_cancelled;
	}

private:
	shared_ptr<atomic<bool>> m_cancelled = make_shared<atomic<bool>>();
};

struct Generator
{
	using Callback = function<void(unsigned)>;

	CancellationToken Start(milliseconds interval, Callback callback)
	{
		CancellationToken token;
		scheduler.Post([token, interval, callback = move(callback)] {
			random_device rd;
			while (!token.IsCancelled())
			{
				this_thread::sleep_for(interval);
				callback(rd());
			}
		});

		return token;
	}

private:
	Scheduler scheduler;
};

struct Indicator : enable_shared_from_this<Indicator>
{
	Indicator(shared_ptr<Generator> gen, string name)
		: m_generator(move(gen))
		, m_name(move(name))
	{
	}

	~Indicator()
	{
		m_cancellationToken.Cancel();
	}

	void Run(milliseconds interval)
	{
		m_cancellationToken.Cancel();
		m_cancellationToken = m_generator->Start(interval,
			[this, weakThis = weak_from_this()](unsigned value) {
				if (auto strongThis = weakThis.lock())
				{
					cout << strongThis->m_name << ": " << value << endl;
				}
				else
				{
					cout << "Indicator has been destroyed\n";
				}
			});
	}

private:
	CancellationToken m_cancellationToken;
	shared_ptr<Generator> m_generator;
	string m_name;
};

int main()
{
	auto generator = make_shared<Generator>();
	{
		auto indicator = make_shared<Indicator>(generator, "#1"s);
		indicator->Run(1s);
		cin.get();
	}
	cout << "Finish\n";
	cin.get();
}
