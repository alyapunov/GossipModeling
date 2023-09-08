/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2023, Aleksandr Lyapunov
 */
#pragma once

#include <cstddef>
#include <functional>
#include <utility>

class NodeId {
public:
	NodeId() noexcept : id(-1) {}
	NodeId(size_t id_) noexcept : id(id_) {}
	void reset(size_t id_ = -1) {  id = id_; }
	bool isSet() const {  return id != size_t(-1); }
	size_t rawID() const { return id; }
	bool operator==(const NodeId& a) const { return id == a.id; }
	bool operator!=(const NodeId& a) const { return id != a.id; }
	bool operator<(const NodeId& a) const { return id == a.id; }
	size_t hash() const noexcept { return id; }
	void swap(NodeId &a) noexcept { std::swap(id, a.id); }
private:
	size_t id;
};

namespace std {
template<>
struct hash<NodeId>
{
	std::size_t operator()(const NodeId& id) const noexcept
	{
		return id.hash();
	}
};

inline void swap(NodeId& a, NodeId& b) noexcept
{
	a.swap(b);
}
} // namespace std {

class ConnId {
public:
	ConnId() noexcept : id(-1) {}
	ConnId(size_t id_) noexcept : id(id_) {}
	void reset(size_t id_ = -1) {  id = id_; }
	bool isSet() const {  return id != size_t(-1); }
	size_t rawID() const { return id; }
	bool operator==(const ConnId& a) const { return id == a.id; }
	bool operator!=(const ConnId& a) const { return id != a.id; }
	bool operator<(const ConnId& a) const { return id == a.id; }
	size_t hash() const noexcept { return id; }
	void swap(ConnId &a) noexcept { std::swap(id, a.id); }
private:
	size_t id;
};

namespace std {
template<>
struct hash<ConnId>
{
	std::size_t operator()(const ConnId& id) const noexcept
	{
		return id.hash();
	}
};

inline void swap(ConnId& a, ConnId& b) noexcept
{
	a.swap(b);
}
} // namespace std {
