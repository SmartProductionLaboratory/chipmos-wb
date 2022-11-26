//
// Created by eugene on 2021/7/5.
//

#ifndef __ALGORITHM_H__
#define __ALGORITHM_H__

#include "include/chromosome.h"
#include "include/chromosome_base.h"
#include "include/lots.h"
#include "include/machines.h"
#include "include/population.h"

extern bool stop;

class algorithm_base_t
{
public:
    machines_t *_machines;
    lots_t *_lots;
    double _peak_period;

    algorithm_base_t(machines_t *machines, lots_t *lots, double peak_period = 0)
        : _machines(machines), _lots(lots), _peak_period(peak_period){};

    virtual void schedule() = 0;
};

class multiple_stage_schedule : public algorithm_base_t
{
public:
    multiple_stage_schedule(machines_t *machines,
                            lots_t *lots,
                            double peak_period,
                            population_t *pop,
                            int fd)
        : algorithm_base_t(machines, lots, peak_period), _pop(pop), _fd(fd){};
    void schedule();

private:
    population_t *_pop;
    int _fd;

    void prescheduling(machines_t *machines, lots_t *lots);

    int stage2Scheduling(machines_t *machines,
                         lots_t *lots,
                         double peak_period);

    void stage3Scheduling(machines_t *machines,
                          lots_t *lots,
                          population_t *pop,
                          int fd);
};

void prepareChromosomes(chromosome_base_t **_chromosomes,
                        int NUMBER_OF_JOBS,
                        int NUMBER_OF_R_CHROMOSOMES);

void geneticAlgorithm(population_t *pop, int fd);

#endif
