/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2023, Aleksandr Lyapunov
 */
#pragma once

#include <unordered_map>

#include <ClusterBase.hpp>
#include <Stats.hpp>

struct KnownInfoConnection {
	double latency;
};

struct KnownInfoNode {
	std::unordered_map<NodeId, KnownInfoConnection> conns;
	size_t info_version;
};

struct Conn : public ConnBase {
	using ConnBase::ConnBase;

	ExpAvg latency;
};

struct Node : public NodeBase<Conn> {
	using NodeBase<Conn>::NodeBase;

	size_t self_info_version = 0;
	std::unordered_map<NodeId, KnownInfoNode> known_nodes;
	std::unordered_map<NodeId, ExpAvg> known_direct_latency;

	const std::unordered_map<NodeId, KnownInfoNode>& prepageKnowledge();
	void applyKnowledge(const std::unordered_map<NodeId, KnownInfoNode>& m);
	double getKnownLatency(NodeId peer_id) const;

};

using Cluster = ClusterBase<Node>;

double Node::getKnownLatency(NodeId peer_id) const
{
	auto itr = known_direct_latency.find(peer_id);
	if (itr != known_direct_latency.end())
		return itr->second.get();
	return 2 * CROSS_DC_LATENCY;
}

const std::unordered_map<NodeId, KnownInfoNode>& Node::prepageKnowledge()
{
	KnownInfoNode me;
	me.info_version = ++self_info_version;
	std::vector<NodeId> peers;
	getPeers(peers);
	for (NodeId peer_id : peers) {
		if (known_nodes.count(peer_id) == 0)
			continue;

		KnownInfoConnection conn;
		conn.latency = getKnownLatency(peer_id);
		me.conns[peer_id] = std::move(conn);
	}
	known_nodes[getId()] = std::move(me);
	return known_nodes;
}

void Node::applyKnowledge(const std::unordered_map<NodeId, KnownInfoNode>& more)
{
	for (const auto& [node_id, info] : more) {
		if (known_nodes.count(node_id) == 0 ||
		    known_nodes.at(node_id).info_version < info.info_version)
			known_nodes[node_id] = info;
	}
}

struct ClusterStatus {
	size_t max_hops;
	size_t max_conns;
	double max_latency;
	size_t inaccessible_node_count;
};

ClusterStatus
getClusterStatus()
{
	ClusterStatus res{};
	const auto& nodes = Cluster::getNodes();
	const auto& node_map = Cluster::getNodeMap();
	std::vector<NodeId> tmp_peers;
	std::vector<std::pair<NodeId, double>> tmp_jumps;
	auto jump = [&tmp_peers, &tmp_jumps](NodeId id) -> std::vector<std::pair<NodeId, double>>& {
		tmp_peers.clear();
		tmp_jumps.clear();
		Node *node = Cluster::findNode(id);
		node->getEstablishedPeers(tmp_peers);
		for (NodeId peer_id : tmp_peers) {
			ConnId conn_id = node->getEstablishedPeerConn(peer_id);
			auto& conn = node->getConn(conn_id);
			double lat = conn.latency.get();
			tmp_jumps.emplace_back(peer_id, lat);
		}
		return tmp_jumps;
	};
	for (const auto& node: nodes) {
		if (res.max_conns < node.getConnCount())
			res.max_conns = node.getConnCount();

		auto scan = scanGraph(node.getId(), node_map, jump);
		updMax(res.max_hops, scan.max_hops);
		updMax(res.max_latency, scan.max_latency);
		res.inaccessible_node_count += scan.inaccessible_nodes.size();
	}
	return res;
}

