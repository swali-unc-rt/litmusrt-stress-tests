#pragma once

struct task {
	double cost; // Task's WCET
	double period; // Task's period (exact time between job releases)
	double deadline; // Task's relative deadline
};