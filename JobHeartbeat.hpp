/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2023, Aleksandr Lyapunov
 */
#pragma once

#include <Cluster.hpp>
#include <Job.hpp>
#include <JobConnect.hpp>
#include <Utils.hpp>

struct JobHeartbeatBack {
	NodeId node_id;
	NodeId peer_id;
	ConnId conn_id;
	size_t time_start;

	size_t delay() const
	{
		return pingDelay(peer_id, node_id);
	}

	void operator()()
	{
		Node *node = Cluster::findNode(node_id);
		if (node == nullptr) {
			jobSchedule(JobDisconnect{peer_id, conn_id});
			return;
		}
		size_t latency = Scheduler::now() - time_start;
		if (node->hasConn(conn_id))
			node->getConn(conn_id).latency.update(latency);
		node->known_direct_latency[peer_id].update(latency);
	}
};

struct JobHeartbeatForth {
	NodeId node_id;
	NodeId peer_id;
	ConnId conn_id;
	size_t time_start = Scheduler::now();

	size_t delay() const
	{
		return pingDelay(node_id, peer_id);
	}

	void operator()()
	{
		Node *peer = Cluster::findNode(peer_id);
		if (peer == nullptr) {
			jobSchedule(JobDisconnect{node_id, conn_id});
			return;
		}
		jobSchedule(JobHeartbeatBack{node_id, peer_id,
					     conn_id, time_start});
	}

};

struct JobHeartbeat {
	NodeId node_id;

	size_t delay() const
	{
		double rnd = Rnd::getPessimistLogNormal(INTERVAL_RANDOM_COEF);
		return HEARTBEAT_INTERVAL * rnd;
	}

	void operator()()
	{
		Node *node = Cluster::findNode(node_id);
		if (node == nullptr)
			return;
		jobSchedule(*this);

		const auto& conns = node->getConns();
		for (const auto& [conn_id, conn] : conns)
			jobSchedule(JobHeartbeatForth{node_id,
						      conn.getPeerId(),
						      conn_id});
	}
};