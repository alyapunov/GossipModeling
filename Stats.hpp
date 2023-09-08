/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2023, Aleksandr Lyapunov
 */
#pragma once

#include <cmath>

constexpr double EXP_AVG_ALPHA = 0.05;

class ExpAvg {
public:
	void update(double val)
	{
		avg = EXP_AVG_ALPHA * val +
		      (1. - EXP_AVG_ALPHA) * (is_set ? avg : val);
	}

	double get() const
	{
		return avg;
	}

	bool is() const
	{
		return is_set;
	}

private:
	bool is_set = false;
	double avg = 0;
};
