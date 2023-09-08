/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2023, Aleksandr Lyapunov
 *
 */
#pragma once

#include <Scheduler.hpp>
#include <Utils.hpp>

template <class F>
void
jobSchedule(F&& f, bool now = false)
{
	Scheduler::add(now ? 0 : f.delay(), std::forward<F>(f));
}

inline size_t
pingDelay(NodeId node_id, NodeId peer_id)
{
	Node *node = Cluster::findNode(node_id);
	assert(node != nullptr);
	Node *peer = Cluster::findNode(peer_id);
	return node->getLatency(peer);
}