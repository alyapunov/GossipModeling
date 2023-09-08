/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2023, Aleksandr Lyapunov
 */
#pragma once

#include <Cluster.hpp>
#include <Job.hpp>
#include <Utils.hpp>

struct JobDisconnectPeer {
	NodeId node_id;
	NodeId peer_id;
	ConnId conn_id;

	size_t delay() const
	{
		return pingDelay(node_id, peer_id);
	}

	void operator()()
	{
		Node *peer = Cluster::findNode(peer_id);
		if (peer == nullptr)
			return;
		peer->disconnect(conn_id);
	}
};

struct JobDisconnect {
	NodeId node_id;
	ConnId conn_id;

	size_t delay() const
	{
		return 0;
	}

	void operator()()
	{
		Node *node = Cluster::findNode(node_id);
		if (node == nullptr)
			return;
		auto& conns = node->getConns();
		if (conns.find(conn_id) == conns.end())
			return;
		NodeId peer_id = conns.at(conn_id).getPeerId();
		node->disconnect(conn_id);
		jobSchedule(JobDisconnectPeer{node_id, peer_id, conn_id});
	}
};

struct JobConnectNotifyPeer {
	NodeId node_id;
	NodeId peer_id;
	ConnId conn_id;
	size_t time_accept;

	size_t delay() const
	{
		return pingDelay(node_id, peer_id);
	}

	void operator()()
	{
		Node *peer = Cluster::findNode(peer_id);
		if (peer == nullptr || !peer->hasConn(conn_id)) {
			jobSchedule(JobDisconnect{node_id, conn_id});
			return;
		}
		size_t time_roundtrip = Scheduler::now() - time_accept;
		peer->establish(conn_id).latency.update(time_roundtrip);
		peer->known_direct_latency[node_id].update(time_roundtrip);
	}
};

struct JobConnectNotifyNode {
	NodeId node_id;
	NodeId peer_id;
	ConnId conn_id;
	size_t time_start;
	size_t time_accept;

	size_t delay() const
	{
		return pingDelay(peer_id, node_id);
	}

	void operator()()
	{
		Node *node = Cluster::findNode(node_id);
		if (node == nullptr || !node->hasConn(conn_id)) {
			jobSchedule(JobDisconnect{peer_id, conn_id});
			return;
		}
		size_t time_roundtrip = Scheduler::now() - time_start;
		node->establish(conn_id).latency.update(time_roundtrip);
		node->known_direct_latency[peer_id].update(time_roundtrip);
		jobSchedule(JobConnectNotifyPeer{node_id, peer_id,
						 conn_id, time_accept});
	}
};

struct JobConnectAccept {
	NodeId node_id;
	NodeId peer_id;
	ConnId conn_id;
	size_t time_start;

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
		peer->accept(conn_id, node_id);
		jobSchedule(JobConnectNotifyNode{node_id, peer_id, conn_id,
						 time_start, Scheduler::now()});
	}
};

struct JobConnect {
	NodeId node_id;
	NodeId peer_id;

	size_t delay() const
	{
		return 0;
	}

	void operator()()
	{
		Node *node = Cluster::findNode(node_id);
		if (node == nullptr)
			return;
		ConnId conn_id = node->connect(peer_id);
		jobSchedule(JobConnectAccept{node_id, peer_id,
					     conn_id, Scheduler::now()});
	}
};
