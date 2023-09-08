/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2023, Aleksandr Lyapunov
 */
#include <iostream>

#include <Cluster.hpp>
#include <Job.hpp>
#include <JobConnect.hpp>
#include <JobHeartbeat.hpp>
#include <JobGossip.hpp>
#include <JobTopology.hpp>
#include <Scheduler.hpp>

void addNode(size_t num)
{
	std::vector<NodeId> initial_conns;
	initial_conns.reserve(INITIAL_CONNECT_COUNT);
	auto& nodes = Cluster::getNodes();
	if (nodes.size() <= INITIAL_CONNECT_COUNT) {
		for (auto& node : nodes)
			initial_conns.push_back(node.getId());
	} else {
		while (initial_conns.size() < INITIAL_CONNECT_COUNT) {
			size_t node_idx = Rnd::choose(nodes);
			NodeId node_id = nodes[node_idx].getId();
			bool has = false;
			for (NodeId test : initial_conns)
				if (node_id == test)
					has = true;
			if (has)
				continue;
			initial_conns.push_back(node_id);
		}
	}

	for (size_t i = 0; i < num; i++) {
		NodeId node_id = Cluster::addNode();

		for (NodeId peer_id : initial_conns)
			jobSchedule(JobConnect{node_id, peer_id});

		jobSchedule(JobHeartbeat{node_id});
		jobSchedule(JobGossip{node_id});
		jobSchedule(JobTopology{node_id});
		if (initial_conns.size() < INITIAL_CONNECT_COUNT) {
			initial_conns.push_back(node_id);
		}
	}
}

void delNode(size_t num)
{
	for (size_t i = 0; i < num; i++)
		Cluster::delNode();
}

std::ostream& operator<<(std::ostream &strm, const ClusterStatus &status)
{
	strm << "{max_hops = " << status.max_hops
	     << ", max_conns = " << status.max_conns
	     << ", max_latency = " << status.max_latency
	     << ", unknown_node_count = " << status.inaccessible_node_count
	     << "}";
	return strm;
}

int main()
{
	std::string str;
	while (true) {
		std::cin >> str;
		if (std::cin.eof() || str == "end" || str == "exit") {
			return 0;
		} else if (str == "add") {
			size_t num;
			std::cin >> num;
			std::cout << "adding " << num << std::endl;
			addNode(num);
		} else if (str == "del") {
			size_t num;
			std::cin >> num;
			std::cout << "deleting " << num << std::endl;
			delNode(num);
		} else if (str == "wait") {
			size_t num;
			std::cin >> num;
			if (!Scheduler::more()) {
				std::cout << "No more to do\n";
				continue;
			}
			std::cout << "waiting " << num << " microseconds\n";
			size_t t = Scheduler::now();
			while (Scheduler::more() && Scheduler::now() < t + num)
				Scheduler::next();
			std::cout << getClusterStatus() << std::endl;

//			for (const Node& node : Cluster::getNodes()) {
//				std::cout << node.getId().rawID();
//				std::cout << "(" << node.known_nodes.size();
//				std::cout << ")";
//				for (const auto& [conn_id, conn] : node.getConns()) {
//					std::cout << " " << conn_id.rawID();
//					std::cout << ":" << conn.getPeerId().rawID();
//					std::cout << "(";
//					if (conn.isIncoming())
//						std::cout << "I";
//					else if (conn.isOutgoing())
//						std::cout << "O";
//					if (conn.isEstablished())
//						std::cout << "E";
//					std::cout << ")";
//				}
//				std::cout << std::endl;
//			}
		} else {
			std::cout << "unknown command " << str << std::endl;
		}
	}
}
