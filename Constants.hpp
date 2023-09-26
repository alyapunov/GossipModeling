/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2023, Aleksandr Lyapunov
 */
#pragma once

#include <cstddef>

//
//  μsμsμsμsμs  μs
//      μs            μsμs  μs        μsμsμs
//      μs      μs    μs  μs  μs    μs      μs
//      μs      μs    μs  μs  μs    μsμsμsμsμs
//      μs      μs    μs  μs  μs    μs
//      μs      μs    μs  μs  μs      μsμsμsμs
//
//  All time constants are in microseconds!
//

// PhysicalTopology
constexpr size_t NUM_DC = 3;
constexpr size_t NUM_RACKS = 100;

constexpr size_t BAD_PEER_LATENCY = 10000;
constexpr size_t CROSS_DC_LATENCY = 4000;
constexpr size_t CROSS_RACK_LATENCY = 2000;
constexpr size_t MINIMAL_LATENCY = 500;

// Coefficient of lognormal distribution of ping time.
// Must be >= 1.
// Examples:
// 1.0 - no random, ping time is fixed.
// 1.1 - randomly plus around 10%.
// 2.0 - randomly plus around 100%.
constexpr double LATENCY_RANDOM_COEF = 1.1;

// Cluster settings
constexpr size_t INITIAL_CONNECT_COUNT = 3;
constexpr double CONN_COEF = 1.5;
constexpr size_t THINK_INTERVAL = 10000;
constexpr size_t HEARTBEAT_INTERVAL = 1000;
constexpr size_t GOSSIP_INTERVAL = 5000;
//constexpr size_t FAREWELL_INTERVAL = 1000000;
constexpr double INTERVAL_RANDOM_COEF = 1.1;

