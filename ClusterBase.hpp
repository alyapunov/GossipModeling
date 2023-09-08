/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2023, Aleksandr Lyapunov
 */
#pragma once

#include <cassert>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include <PhysicalTopology.hpp>
#include <Types.hpp>

enum ConnType_t {
	CONN_INCOMING,
	CONN_OUTGOING,
};

enum ConnStatus_t {
	CONN_PENDING,
	CONN_ESTABLISHED,
};

class ConnBase {
public:
	ConnId getConnId() const { return conn_id; }
	NodeId getPeerId() const { return peer_id; }
	bool isEstablished() const { return status == CONN_ESTABLISHED; }
	bool isIncoming() const { return type == CONN_INCOMING; }
	bool isOutgoing() const { return type == CONN_OUTGOING; }

	ConnBase(ConnId conn_id_, NodeId peer_id_, ConnType_t type_) noexcept
		: conn_id(conn_id_), peer_id(peer_id_), type(type_) {}
	~ConnBase() noexcept = default;
	ConnBase(const ConnBase&) = delete;
	ConnBase& operator=(const ConnBase&) = delete;
	ConnBase(ConnBase&& c) noexcept;
	ConnBase& operator=(ConnBase&& c) noexcept;
	void dispose() noexcept;

private:
	template <class CONN>
	friend class NodeBase;

	ConnId conn_id;
	NodeId peer_id;
	ConnType_t type;
	ConnStatus_t status = CONN_PENDING;
};

template <class CONN>
class NodeBase : public PhysicalNode {
public:
	using Conn_t = CONN;
	static_assert(std::is_base_of_v<ConnBase, Conn_t>);

	NodeId getId() const { return id; }
	size_t getIdx() const { return idx; }

	bool isConnected(ConnId conn_id) const;
	template <class... ARGS>
	ConnId connect(NodeId peer_id, ARGS&& ...args);
	template <class... ARGS>
	void accept(ConnId conn_id, NodeId peer_id, ARGS&& ...args);
	Conn_t& establish(ConnId conn_id);
	void disconnect(ConnId conn_id);

	size_t getConnCount() const;
	const std::unordered_map<ConnId, Conn_t>& getConns() const;
	bool hasConn(ConnId conn_id) const;
	Conn_t& getConn(ConnId conn_id);
	const Conn_t& getConn(ConnId conn_id) const;

	size_t getPeerCount() const;
	const std::unordered_map<NodeId, std::unordered_set<ConnId>>& getPeersRaw() const;
	size_t getEstablishedPeerCount() const;
	void getPeers(std::vector<NodeId>& res) const;
	void getEstablishedPeers(std::vector<NodeId>& res) const;

	bool hasPeer(NodeId peer_id) const;
	bool hasEstablishedPeer(NodeId peer_id) const;
	const std::unordered_set<ConnId>& getPeerConns(NodeId peer_id) const;
	ConnId getEstablishedPeerConn(NodeId peer_id) const;

	NodeBase(NodeId id_, size_t idx_) noexcept;
	~NodeBase() noexcept;
	NodeBase(const NodeBase&) = delete;
	NodeBase& operator=(const NodeBase&) = delete;
	NodeBase(NodeBase&& n) noexcept;
	NodeBase& operator=(NodeBase&& n) noexcept;
	void dispose() noexcept;
private:
	template <class NODE>
	friend class ClusterBase;

	NodeId id = SIZE_MAX;
	size_t idx  = SIZE_MAX;

	static inline size_t conn_id_generator = 0;
	std::unordered_map<ConnId, Conn_t> conn_by_id;
	std::unordered_map<NodeId, std::unordered_set<ConnId>> conn_by_peer;

	bool hasEstablishedPeer(const std::unordered_set<ConnId>& conns) const;
};

template <class NODE>
class ClusterBase {
public:
	using Node_t = NODE;
	using Conn_t = typename NODE::Conn_t;
	static_assert(std::is_base_of_v<NodeBase<Conn_t>, Node_t>);

	template <class... ARGS>
	static NodeId addNode(ARGS&&... args);
	static NodeId delNode();
	static NODE *findNode(NodeId id);
	static const std::vector<NODE>& getNodes() { return instance().nodes; }
	static const std::unordered_map<NodeId, size_t>& getNodeMap() { return instance().id_to_idx; }
	static size_t getNodeCount() { return instance().nodes.size(); }

	ClusterBase(const ClusterBase&) = delete;
	ClusterBase& operator=(const ClusterBase&) = delete;
private:
	ClusterBase() = default;
	~ClusterBase();
	static ClusterBase& instance();

	size_t max_node_id = 0;
	std::vector<NODE> nodes;
	std::unordered_map<NodeId, size_t> id_to_idx;
};

ConnBase::ConnBase(ConnBase&& c) noexcept
	: conn_id(c.conn_id), peer_id(c.peer_id), type(c.type), status(c.status)
{
	c.dispose();
}

ConnBase&
ConnBase::operator=(ConnBase&& c) noexcept
{
	std::swap(conn_id, c.conn_id);
	std::swap(peer_id, c.peer_id);
	std::swap(type, c.type);
	std::swap(status, c.status);
	return *this;
}

void
ConnBase::dispose() noexcept
{
#ifndef NDEBUG
	conn_id.reset();
	peer_id.reset();
	status = CONN_PENDING;
#endif
}

template <class CONN>
NodeBase<CONN>::NodeBase(NodeId id_, size_t idx_) noexcept
	: id(id_), idx(idx_)
{
}

template <class CONN>
NodeBase<CONN>::~NodeBase() noexcept
{
	assert(id == SIZE_MAX);
	assert(idx == SIZE_MAX);
}

template <class CONN>
NodeBase<CONN>::NodeBase(NodeBase&& n) noexcept
	: PhysicalNode(std::move(n)), id(n.id), idx(n.idx)
{
	n.dispose();
}

template <class CONN>
NodeBase<CONN>&
NodeBase<CONN>::operator=(NodeBase&& n) noexcept
{
	std::swap(id, n.id);
	std::swap(idx, n.idx);
	return *this;
}

template <class CONN>
void
NodeBase<CONN>::dispose() noexcept
{
	for (auto& [conn_id, conn] : conn_by_id)
		conn.dispose();
#ifndef NDEBUG
	id = SIZE_MAX;
	idx = SIZE_MAX;
#endif
}

template <class CONN>
bool
NodeBase<CONN>::isConnected(ConnId conn_id) const
{
	return conn_by_id.count(conn_id) != 0;
}

template <class CONN>
template <class... ARGS>
ConnId
NodeBase<CONN>::connect(NodeId peer_id, ARGS&& ...args)
{
	ConnId conn_id = conn_id_generator++;
	conn_by_id.emplace(std::piecewise_construct,
			   std::forward_as_tuple(conn_id),
			   std::forward_as_tuple(conn_id, peer_id,
						 CONN_OUTGOING, args...));
	conn_by_peer[peer_id].insert(conn_id);
	return conn_id;
}

template <class CONN>
template <class... ARGS>
void
NodeBase<CONN>::accept(ConnId conn_id, NodeId peer_id, ARGS&& ...args)
{
	assert(!isConnected(conn_id));
	conn_by_id.emplace(std::piecewise_construct,
			   std::forward_as_tuple(conn_id),
			   std::forward_as_tuple(conn_id, peer_id,
						 CONN_INCOMING, args...));
	conn_by_peer[peer_id].insert(conn_id);
}

template <class CONN>
CONN&
NodeBase<CONN>::establish(ConnId conn_id)
{
	assert(isConnected(conn_id));
	Conn_t& conn = conn_by_id.at(conn_id);
	conn.status = CONN_ESTABLISHED;
	return conn;
}

template <class CONN>
void
NodeBase<CONN>::disconnect(ConnId conn_id)
{
	auto itr1 = conn_by_id.find(conn_id);
	if (itr1 == conn_by_id.end())
		return;
	auto& conn = itr1->second;
	auto itr2 = conn_by_peer.find(conn.getPeerId());
	auto& conns = itr2->second;
	conns.erase(conn_id);
	if (conns.empty())
		conn_by_peer.erase(itr2);
	conn_by_id.erase(itr1);
}

template <class CONN>
size_t
NodeBase<CONN>::getConnCount() const
{
	return conn_by_id.size();
}

template <class CONN>
const std::unordered_map<ConnId, CONN>&
NodeBase<CONN>::getConns() const
{
	return conn_by_id;
}

template <class CONN>
bool
NodeBase<CONN>::hasConn(ConnId conn_id) const
{
	return conn_by_id.count(conn_id) != 0;
}

template <class CONN>
const CONN&
NodeBase<CONN>::getConn(ConnId conn_id) const
{
	return conn_by_id.at(conn_id);
}

template <class CONN>
CONN&
NodeBase<CONN>::getConn(ConnId conn_id)
{
	return conn_by_id.at(conn_id);
}

template <class CONN>
size_t
NodeBase<CONN>::getPeerCount() const
{
	return conn_by_peer.size();
}

template <class CONN>
const std::unordered_map<NodeId, std::unordered_set<ConnId>>&
NodeBase<CONN>::getPeersRaw() const
{
	return conn_by_peer;
}

template <class CONN>
size_t
NodeBase<CONN>::getEstablishedPeerCount() const
{
	size_t res = 0;
	for (const auto& [peer_id, conns] : conn_by_peer)
		if (hasEstablishedPeer(conns))
			res++;
	return res;
}

template <class CONN>
void
NodeBase<CONN>::getPeers(std::vector<NodeId>& res) const
{
	res.clear();
	for (const auto& [peer_id, conns] : conn_by_peer)
		res.push_back(peer_id);
}

template <class CONN>
void
NodeBase<CONN>::getEstablishedPeers(std::vector<NodeId>& res) const
{
	res.clear();
	for (const auto& [peer_id, conns] : conn_by_peer)
		if (hasEstablishedPeer(conns))
			res.push_back(peer_id);
}

template <class CONN>
bool
NodeBase<CONN>::hasPeer(NodeId peer_id) const
{
	return conn_by_peer.count(peer_id) > 0;
}

template <class CONN>
bool
NodeBase<CONN>::hasEstablishedPeer(const std::unordered_set<ConnId>& conns) const
{
	for (ConnId conn_id : conns) {
		const auto &conn = conn_by_id.at(conn_id);
		if (conn.status == CONN_ESTABLISHED)
			return true;
	}
	return false;
}

template <class CONN>
bool
NodeBase<CONN>::hasEstablishedPeer(NodeId peer_id) const
{
	if (conn_by_peer.count(peer_id) == 0)
		return false;
	const auto& conns = conn_by_peer.at(peer_id);
	return hasEstablishedPeer(conns);
}

template <class CONN>
const std::unordered_set<ConnId>&
NodeBase<CONN>::getPeerConns(NodeId peer_id) const
{
	assert(conn_by_peer.count(peer_id) != 0);
	return conn_by_peer.at(peer_id);
}

template <class CONN>
ConnId
NodeBase<CONN>::getEstablishedPeerConn(NodeId peer_id) const
{
	assert(conn_by_peer.count(peer_id) != 0);
	const auto& conns =  conn_by_peer.at(peer_id);
	for (ConnId conn_id : conns) {
		const auto &conn = conn_by_id.at(conn_id);
		if (conn.status == CONN_ESTABLISHED)
			return conn_id;
	}
	return ConnId{};
}

template <class NODE>
ClusterBase<NODE>::~ClusterBase()
{
	for (NODE& node : nodes)
		node.dispose();
}

template <class NODE>
ClusterBase<NODE>& ClusterBase<NODE>::instance()
{
	static ClusterBase<NODE> inst;
	return inst;
}

template <class NODE>
template <class... ARGS>
NodeId
ClusterBase<NODE>::addNode(ARGS&&... args)
{
	ClusterBase<NODE>& inst = instance();
	NodeId id = inst.max_node_id++;
	size_t idx = inst.nodes.size();
	inst.nodes.emplace_back(id, idx, std::forward<ARGS>(args)...);
	inst.id_to_idx.emplace(id, idx);
	return id;
}

template <class NODE>
NodeId
ClusterBase<NODE>::delNode()
{
	ClusterBase<NODE>& inst = instance();
	assert(inst.nodes.size() > 0);
	size_t idx = Rnd::choose(inst.nodes);
	NodeId id = inst.nodes[idx].id;
	assert(inst.nodes[idx].idx == idx);
	inst.id_to_idx.erase(inst.nodes[idx].id);
	inst.nodes[idx] = std::move(inst.nodes.back());
	inst.nodes[idx].idx = idx;
	inst.id_to_idx[inst.nodes[idx].id] = idx;
	inst.nodes.back().dispose();
	inst.nodes.pop_back();
	return id;
}

template <class NODE>
NODE *
ClusterBase<NODE>::findNode(NodeId id)
{
	ClusterBase<NODE>& inst = instance();
	auto itr = inst.id_to_idx.find(id);
	if (itr == inst.id_to_idx.end())
		return nullptr;
	size_t idx = itr->second;
	assert(inst.nodes[idx].idx == idx);
	return &inst.nodes[idx];
}