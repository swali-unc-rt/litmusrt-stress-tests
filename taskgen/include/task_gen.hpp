#pragma once
#include <vector>
#include <random>

#include "structs.hpp"

/* Generates a task set with a target utilization U.
 *  We use a technique from Emberson et. al. 2010, "Techniques for the synthesis of multiprocessor tasksets" WATERS'10
 */

class TaskGen {
public:
	TaskGen( unsigned int M, // number of processors guaranteed to component
		double Tmin, // minimum period
		double Tmax, // maximum period
		unsigned int n, // number of tasks
		double U, // target utilization
        double tg = 1.0 // Ti must be divisible by this much
        );
	~TaskGen();

	void outputTaskSet(std::vector<task*>* tasksOut) const;
	void outputTaskSet(char* outCsvFileName) const;
	double getActualUtilization() const { return actualU; }
private:
	unsigned int M; // number of processors guaranteed to component
	double Tmin; // minimum period
	double Tmax; // maximum period
	unsigned int n; // number of tasks
	double U; // target utilization
	std::default_random_engine gen;
	double actualU; // actual utilization
	std::uniform_real_distribution<double> ri_dist; // Samples Ri
    double tg; // tmin/tmax/ti will be disible by this amount

	// List of tasks. Cleaned up in destructor.
	std::vector<task*> tasks;

	void generateTaskSet();
	double* generate_utilizations();

	double sample_Ti();
	double sample_ri();
};