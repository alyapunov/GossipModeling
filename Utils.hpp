/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2023, Aleksandr Lyapunov
 */
#pragma once

#include <array>
#include <cassert>
#include <cstdlib>
#include <cmath>
#include <numeric>
#include <unordered_set>
#include <vector>

#define PI 3.14159265358979323846

class Rnd {
public:
	static int getInt(int lim)
	{
		return rand() % lim;
	}

	static double getDbl(double lim)
	{
		return (double)rand() / RAND_MAX * lim;
	}

	static double getNormal(double deviation = 1.)
	{
		double a = (double)(rand() + 1) / RAND_MAX;
		double r = (double)(rand() + 1) / RAND_MAX;
		return cos(2 * PI * a) * pow(-2 * log(r), 0.5) * deviation;
	}

	static double getLogNormal(double relative_deviation = 1.1)
	{
		assert(relative_deviation >= 1.);
		assert(relative_deviation < 15.); // Don't work correctly.
		// Magic formula, created by myself using excel.
		double x = log(relative_deviation) / log(2.48);
		double deviation = log(x + 1) / log(2.48);
		double n = Rnd::getNormal(deviation);
		return exp(n);
	}

	static double getPessimistLogNormal(double relative_deviation = 1.1)
	{
		assert(relative_deviation >= 1.);
		assert(relative_deviation < 15.); // Don't work correctly.
		// Magic formula, created by myself using excel.
		double x = log(relative_deviation) / log(2.48);
		double deviation = log(x + 1) / log(2.48);
		double n = Rnd::getNormal(deviation);
		return exp(fabs(n));
	}

	template <class CONTAINER>
	static size_t choose(const CONTAINER& container)
	{
		assert(std::size(container) > 0);
		return Rnd::getInt(std::size(container));
	}

	template <class CONTAINER>
	static size_t chooseNot(const CONTAINER& container, size_t not_me)
	{
		assert(std::size(container) > 1);
		while (true) {
			size_t res = choose(container);
			if (res != not_me)
				return res;
		}
	}

	template <class CONTAINER, class WEIGHT>
	static size_t chooseByWeight(const CONTAINER& container, WEIGHT weight)
	{
		auto op = [&weight](double sum, auto x) {
			return sum + weight(x);
		};
		double total_weight = std::accumulate(std::begin(container),
						      std::end(container),
						      0., op);
		double rnd = Rnd::getDbl(total_weight);
		size_t res = 0;
		for (const auto& x: container) {
			double w = weight(x);
			if (rnd < w)
				return res;
			rnd -= w;
			res++;
		}
		return std::size(container) - 1;
	}
};

template <class T>
T copy(const T& t)
{
	return t;
}

template <class T, class U>
void updMax(T& t, const U& u)
{
	if (t < u)
		t = u;
}

template <class NODE_ID>
struct GraphScanResult {
	size_t max_hops = 0;
	double max_latency = 0;
	std::vector<NODE_ID> inaccessible_nodes;
};

template <class NODE_ID, class ALL_NODES_MAP, class JUMP_F>
GraphScanResult<NODE_ID> scanGraph(NODE_ID origin, const ALL_NODES_MAP& all,
				   JUMP_F&& jump)
{
	std::unordered_set<NODE_ID> visited;
	std::unordered_map<NODE_ID, double> wave1, wave2;
	visited.insert(origin);
	wave1.emplace(origin, 0);
	GraphScanResult<NODE_ID> res;
	while (true) {
		for (auto [node_id, cur_lat] : wave1) {
			const auto& edges = jump(node_id);
			for (const auto& [peer_id, next_lat] : edges) {
				if (visited.count(peer_id) != 0)
					continue;
				double lat = cur_lat + next_lat;
				auto itr = wave2.find(peer_id);
				if (itr == wave2.end() || itr->second > lat)
					wave2.emplace(peer_id, lat);
			}
		}
		if (wave2.empty())
			break;
		res.max_hops++;
		for (auto [node_id, lat] : wave2) {
			if (res.max_latency < lat)
				res.max_latency = lat;
			visited.insert(node_id);
		}
		std::swap(wave1, wave2);
		wave2.clear();
	}
	for (const auto& [node_id, thing] : all) {
		if (visited.count(node_id) == 0)
			res.inaccessible_nodes.push_back(node_id);
	}
	return res;
}
