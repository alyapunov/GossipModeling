/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2023, Aleksandr Lyapunov
 */
#pragma once

#include <Constants.hpp>
#include <Utils.hpp>

struct PhysicalNode {
	size_t dc;
	size_t rack;
	PhysicalNode() noexcept;
	~PhysicalNode() noexcept;
	PhysicalNode(const PhysicalNode& n) noexcept;
	PhysicalNode& operator=(const PhysicalNode& n) noexcept;
	size_t getBaseLatency(const PhysicalNode* n) const;
	size_t getLatency(const PhysicalNode* n) const;
};

class PhysicalTopology {
private:
	friend struct PhysicalNode;
	static void create(PhysicalNode& n);
	static void reg(PhysicalNode& n);
	static void unreg(PhysicalNode& n);

	size_t counts[NUM_DC * NUM_RACKS] = {};
	static PhysicalTopology& Instance();
};

PhysicalNode::PhysicalNode() noexcept
{
	PhysicalTopology::create(*this);
}

PhysicalNode::~PhysicalNode() noexcept
{
	PhysicalTopology::unreg(*this);
}

PhysicalNode::PhysicalNode(const PhysicalNode& n) noexcept : dc(n.dc), rack(n.rack)
{
	PhysicalTopology::reg(*this);
}

PhysicalNode&
PhysicalNode::operator=(const PhysicalNode& n) noexcept
{
	PhysicalTopology::unreg(*this);
	dc = n.dc;
	rack = n.rack;
	PhysicalTopology::reg(*this);
	return *this;
}

size_t
PhysicalNode::getBaseLatency(const PhysicalNode* n) const
{
	size_t base_latency = 0;
	if (n == nullptr)
		base_latency = BAD_PEER_LATENCY;
	else if (dc != n->dc)
		base_latency = CROSS_DC_LATENCY;
	else if (rack != n->rack)
		base_latency = CROSS_RACK_LATENCY;
	else
		base_latency = MINIMAL_LATENCY;
	return base_latency;
}

size_t
PhysicalNode::getLatency(const PhysicalNode* n) const
{
	size_t base_latency = getBaseLatency(n);
	return base_latency * Rnd::getPessimistLogNormal(LATENCY_RANDOM_COEF);
}

PhysicalTopology&
PhysicalTopology::Instance()
{
	static PhysicalTopology instance;
	return instance;
}

void
PhysicalTopology::create(PhysicalNode& n)
{
	size_t i = Rnd::chooseByWeight(Instance().counts, [](size_t c){
		return 1. / (c + 0.5);
	});
	Instance().counts[i]++;
	n.dc = i / NUM_RACKS;
	n.rack = i % NUM_RACKS;
}

void
PhysicalTopology::reg(PhysicalNode& n)
{
	Instance().counts[n.dc * NUM_RACKS + n.rack]++;
}

void
PhysicalTopology::unreg(PhysicalNode& n)
{
	Instance().counts[n.dc * NUM_RACKS + n.rack]--;
}