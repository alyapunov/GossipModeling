/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2023, Aleksandr Lyapunov
 */
#pragma once

#include <cmath>

#include <Cluster.hpp>
#include <Job.hpp>
#include <Utils.hpp>

struct Topology {
	size_t known_count;
	size_t conn_count;
	size_t max_hops;
	size_t max_latency;
	size_t inaccessible_count;

	NodeId extra_jump;
	NodeId extra_drop;
	std::vector<std::pair<NodeId, double>> tmp_jumps;

	const Node& node;
	const std::unordered_map<NodeId, KnownInfoNode>& known_nodes;

	Topology(Node *node_) : node(*node_), known_nodes(node_->prepageKnowledge())
	{
		known_count = known_nodes.size();
		conn_count = node.getConns().size();
		calcHopsAndLatency();
	}

	static size_t getOptimalConnCount(size_t cluster_size)
	{
		double base = cluster_size + INITIAL_CONNECT_COUNT;
		size_t count = size_t(CONN_COEF * std::pow(base, .5) + .5);
		if (count < INITIAL_CONNECT_COUNT)
			count = INITIAL_CONNECT_COUNT;
		if (count > cluster_size - 1)
			count = cluster_size - 1;
		return count;
	}

	std::vector<std::pair<NodeId, double>>& operator()(NodeId id)
	{
		tmp_jumps.clear();

		auto itr = known_nodes.find(id);
		if (itr == known_nodes.end())
			return tmp_jumps;

		const KnownInfoNode &info = itr->second;
		bool need_extra_jump = id == node.getId() && extra_jump.isSet();
		for (const auto& [peer_id, conn_info] : info.conns) {
			if (id == node.getId() && peer_id == extra_drop)
				continue;
			if (need_extra_jump && peer_id == extra_jump)
				need_extra_jump = false;
			tmp_jumps.emplace_back(peer_id, conn_info.latency);
		}
		if (need_extra_jump) {
			double lat = 2 * CROSS_DC_LATENCY;
			tmp_jumps.emplace_back(extra_jump, lat);
		}
		return tmp_jumps;
	}

	void calcHopsAndLatency()
	{
		auto scan = scanGraph(node.getId(), known_nodes, *this);
		max_hops = scan.max_hops;
		max_latency = scan.max_latency;
		inaccessible_count = scan.inaccessible_nodes.size();
	}

	double prosperity() const {
		double k1 = 1.;
		double k2 = 1.;
		double k3 = 1.;
		double res = k1 * (CROSS_DC_LATENCY + CROSS_RACK_LATENCY +
			      MINIMAL_LATENCY) / max_latency;
		if (max_hops > 2)
			res += k2 / (max_hops - 1) * (max_hops - 1);
		else
			res += k2;
		size_t opt_count = getOptimalConnCount(known_count);
		if (conn_count > opt_count)
			res += k3 * double(opt_count) / double(conn_count);
		else
			res += k3;
		return res;
	}

	double urgency() const {
		double p = prosperity();
		if (p < 0.05)
			return 0.05;
		else if (p > 1.)
			return 1.;
		else
			return p;
	}
};

struct JobTopology {
	NodeId node_id;

	size_t delay() const
	{
		double rnd = Rnd::getPessimistLogNormal(INTERVAL_RANDOM_COEF);
		return THINK_INTERVAL * rnd;
	}

	void operator()()
	{
		Node *node = Cluster::findNode(node_id);
		if (node == nullptr)
			return;
		jobSchedule(*this);

		Topology t(node);
		if (Rnd::getDbl(1.) > t.urgency())
			return;

		double cur_prosp = t.prosperity();
		const KnownInfoNode &this_info = t.known_nodes.at(node_id);
		NodeId best;

		t.conn_count++;
		for (const auto& [anode_id, info] : t.known_nodes) {
			if (this_info.conns.count(anode_id) != 0)
				continue;
			if (anode_id == node_id)
				continue;
			t.extra_jump = anode_id;
			t.calcHopsAndLatency();
			double prosp = t.prosperity();

			if (prosp > cur_prosp) {
				best = anode_id;
				cur_prosp = prosp;
			}
		}
		t.extra_jump.reset();
		t.conn_count--;

		if (t.conn_count >= t.getOptimalConnCount(t.known_count)) {
			t.conn_count--;
			for (const auto& [anode_id, info] : t.known_nodes) {
				if (this_info.conns.count(anode_id) == 0)
					continue;
				t.extra_drop = anode_id;
				t.calcHopsAndLatency();
				double prosp = t.prosperity();
				if (prosp > cur_prosp) {
					best = anode_id;
					cur_prosp = prosp;
				}
			}
			t.extra_drop.reset();
			t.conn_count++;
		}

		if (!best.isSet())
			return;

		if (this_info.conns.count(best) == 0) {
			jobSchedule(JobConnect{node_id, best});
		} else {
			for (ConnId conn_id : node->getPeerConns(best)) {
				jobSchedule(JobDisconnect{node_id, conn_id});
			}
		}

		for (const auto& [peer_id, conns] : node->getPeersRaw()) {
			size_t established_count = 0;
			for (ConnId conn_id : conns) {
				Conn& conn = node->getConn(conn_id);
				if (conn.isEstablished())
					established_count++;
			}
			if (established_count == 0)
				continue;

			for (ConnId conn_id : conns) {
				Conn& conn = node->getConn(conn_id);
				(void)conn;
				assert(conn.isEstablished() ||
				       conn.isIncoming());
			}
		}
	}
};