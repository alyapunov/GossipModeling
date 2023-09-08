/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2023, Aleksandr Lyapunov
 */
#pragma once

#include <functional>
#include <set>
#include <memory>

class Scheduler {
public:
	template <class F>
	static void add(size_t wait, F&& f);
	static void next();
	static bool more();
	static size_t now();

	Scheduler(const Scheduler&) = delete;
	Scheduler& operator=(const Scheduler&) = delete;
private:
	Scheduler() = default;
	static Scheduler& instance();

	struct Task {
		template <class F>
		Task(size_t time, F&& f);

		size_t time;
		std::function<void()> func;
	};

	struct TaskCmp {
		bool operator()(const std::unique_ptr<Task>& a,
				const std::unique_ptr<Task>& b) const
		{
			return operator()(a.get(), b.get());
		}
		bool operator()(const Task *a, const Task *b) const;
	};

	size_t cur_time;
	std::set<std::unique_ptr<Task>, TaskCmp> tasks;
};

template <class F>
Scheduler::Task::Task(size_t t, F&& f)
{
	time = t;
	func = std::forward<F>(f);
}

bool
Scheduler::TaskCmp::operator()(const Task *a, const Task *b) const
{
	return std::tie(a->time, a) < std::tie(b->time, b);
}

Scheduler&
Scheduler::instance()
{
	static Scheduler inst;
	return inst;
}

template <class F>
void
Scheduler::add(size_t wait, F&& f)
{
	Scheduler &inst = instance();
	size_t time = inst.cur_time + wait;
	auto t = std::make_unique<Task>(time, std::forward<F>(f));
	inst.tasks.insert(std::move(t));
}

void
Scheduler::next()
{
	Scheduler &inst = instance();
	auto handle = inst.tasks.extract(inst.tasks.begin());
	std::unique_ptr<Task>& task = handle.value();
	inst.cur_time = task->time;
	task->func();
}

bool
Scheduler::more()
{
	Scheduler &inst = instance();
	return !inst.tasks.empty();
}

size_t
Scheduler::now()
{
	Scheduler &inst = instance();
	return inst.cur_time;
}
