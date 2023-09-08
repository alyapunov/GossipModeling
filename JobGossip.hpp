/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2023, Aleksandr Lyapunov
 */
#pragma once

#include <Cluster.hpp>
#include <Job.hpp>
#include <Utils.hpp>

struct JobGossipSend {
	NodeId node_id;
	NodeId peer_id;
	std::unordered_map<NodeId, KnownInfoNode> knowledge;

	size_t delay() const
	{
		return pingDelay(node_id, peer_id);
	}

	void operator()()
	{
		Node *peer = Cluster::findNode(peer_id);
		if (peer == nullptr)
			return;

		peer->applyKnowledge(knowledge);
	}
};

struct JobGossip {
	NodeId node_id;

	size_t delay() const
	{
		double rnd = Rnd::getPessimistLogNormal(INTERVAL_RANDOM_COEF);
		return GOSSIP_INTERVAL * rnd;
	}

	void operator()()
	{
		Node *node = Cluster::findNode(node_id);
		if (node == nullptr)
			return;
		jobSchedule(*this);

		auto knowledge = node->prepageKnowledge();
		const auto& conns = node->getConns();
		for (const auto& [conn_id, conn] : conns)
			jobSchedule(JobGossipSend{node_id, conn.getPeerId(),
						  knowledge});
	}
};