#include <stdio.h>
#include <stdlib.h>

#include <ctime>

/* Usage: taskgen m Tmin Tmax n U csvfilename
 * m = num processors
 * [Tmin,Tmax] = range for the period (integers only)
 * n = number of tasks
 * U = target utilization (0,1)
 * csvfilename = the output file name
 */

#include "task_gen.hpp"

int main( int argc, char* argv[] ) {
    if( argc < 8 ) {
        printf("Usage: %s m Tmin Tmax n U tg csvfilename\n", argv[0]);
        printf(
            "\t m= number of processors\n"
            "\t [Tmin,Tmax]= the range of allowed periods (must be divisible by 1)\n"
            "\t n= number of tasks to generate\n"
            "\t U= target normalized utilization from (0,1)\n"
            "\t tg= Tmin, Tmax, and generated Ti must be divisible by tg (e.g. 0.5 means 1.5 period is possible)\n"
            "\t csvfilename= where to output the task set csv\n"
        );
        printf( "\n Only %d args given.\n", argc);
        return 1;
    }

    srand(time(nullptr));

    unsigned int m = atoi(argv[1]);
    double Tmin = (double)atof(argv[2]);
    double Tmax = (double)atof(argv[3]);
    unsigned int n = atoi(argv[4]);
    double U = atof(argv[5]);
    double tg = atof(argv[6]);

    printf("m, Tmin, Tmax, n, U, tg: %d %f %f %d %f %f\n", m, Tmin, Tmax, n, U, tg);

    if( n <= 0 || U <= 0 || m <= 0 || Tmin <= 0 || Tmax <= 0 ) {
        printf("Bad argument.\n");
        return 1;
    }

    TaskGen tgen(m,Tmin,Tmax,n,U,tg);
    tgen.outputTaskSet(argv[7]);

    printf("Generated %d tasks with actual utilization %f\n", n, tgen.getActualUtilization() / m);
    return 0;
}