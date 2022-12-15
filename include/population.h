#ifndef __POPULATION_H__
#define __POPULATION_H__
#include <map>
#include <vector>
#include "include/chromosome_base.h"
#include "include/common.h"
#include "include/job.h"
#include "include/job_base.h"
#include "include/lot.h"
#include "include/machine.h"
#include "include/machine_base.h"
#include "include/parameters.h"

typedef struct pop_parameters {
    int AMOUNT_OF_CHROMOSOMES;
    int AMOUNT_OF_R_CHROMOSOMES;
    double EVOLUTION_RATE;
    double SELECTION_RATE;
    int GENERATIONS;
    int MAX_SETUP_TIMES;
    weights_t weights;
    setup_time_parameters_t setup_times_parameters;
    scheduling_parameters_t scheduling_parameters;
    std::map<std::pair<std::string, std::string>, double>
        transportation_time_table;
} pop_parameters_t;

typedef struct pop_operations {
    list_operations_t *list_ops;
    job_base_operations_t *job_ops;
    machine_base_operations_t *machine_ops;
} pop_operations_t;

typedef struct pop_objects {
    job_t **jobs;
    machine_t **machines;
    int NUMBER_OF_JOBS;
    int NUMBER_OF_MACHINES;
} pop_objects_t;

struct population_t {
    unsigned int no;

    pop_parameters_t parameters;
    pop_operations_t operations;
    pop_objects_t objects;

    chromosome_base_t *chromosomes;
};

class population_base_t
{
public:
    population_base_t(unsigned int no,
                      chromosome_base_t *chromosomes,
                      int AMOUNT_OF_CHROMOSOMES,
                      int AMOUNT_OF_R_CHROMOSOMES,
                      int GENERATIONS,
                      job_t **jobs,
                      machine_t **machines,
                      int NUMBER_OF_JOBS,
                      int NUMBER_OF_MACHINES);

    unsigned int no;
    chromosome_base_t *chromosomes;

    struct {
        int AMOUNT_OF_CHROMOSOMES;
        int AMOUNT_OF_R_CHROMOSOMES;
        int GENERATIONS;
    } parameters;

    struct {
        job_t **jobs;
        machine_t **machines;
        int NUMBER_OF_JOBS;
        int NUMBER_OF_MACHINES;
    } objects;
};

class population_prepare_t : public population_base_t
{
public:
    population_prepare_t(population_base_t &base);
};

class population_selection_t : public population_base_t
{
public:
    population_selection_t(population_base_t &base, double SELECTION_RATE);
    double SELECTION_RATE;
};

class population_crossover_t : public population_base_t
{
public:
    population_crossover_t(population_base_t &base, double EVOLUTION_RATE);
    double EVOLUTION_RATE;
};

class population_decoding_t : public population_base_t
{
public:
    population_decoding_t(population_base_t &base,
                          weights_t weights,
                          setup_time_parameters_t setup_times_parameters,
                          scheduling_parameters_t scheduling_parameters,
                          std::map<std::pair<std::string, std::string>, double>
                              transportation_time_table,
                          list_operations_t *list_ops,
                          job_base_operations_t *job_ops,
                          machine_base_operations_t *machine_ops);
    struct {
        weights_t weights;
        setup_time_parameters_t setup_times_parameters;
        scheduling_parameters_t scheduling_parameters;
        std::map<std::pair<std::string, std::string>, double>
            transportation_time_table;
    } decoding_parameters;

    struct {
        list_operations_t *list_ops;
        job_base_operations_t *job_ops;
        machine_base_operations_t *machine_ops;
    } operations;
};

#endif
