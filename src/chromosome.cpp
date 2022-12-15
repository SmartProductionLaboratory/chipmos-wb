#include <cstdlib>

#include "include/chromosome.h"
#include "include/infra.h"
#include "include/job_base.h"
#include "include/linked_list.h"
#include "include/machine.h"
#include "include/parameters.h"

using namespace std;

void copyChromosome(chromosome_base_t c1, chromosome_base_t c2)
{
    memcpy(c1.genes, c2.genes, sizeof(double) * c1.gene_size);
}


void crossover(chromosome_base_t p1,
               chromosome_base_t p2,
               chromosome_base_t c1,
               chromosome_base_t c2)
{
    memcpy(c1.genes, p1.genes, sizeof(double) * p1.gene_size);
    memcpy(c2.genes, p2.genes, sizeof(double) * p2.gene_size);
    int cutpoint1 = randomRange(0, p1.gene_size, -1);
    int cutpoint2 = randomRange(0, p2.gene_size, cutpoint1);

    if (cutpoint1 > cutpoint2)
        std::swap(cutpoint1, cutpoint2);

    int size = cutpoint2 - cutpoint1;

    memcpy(c1.genes + cutpoint1, p2.genes + cutpoint1, sizeof(double) * size);
    memcpy(c2.genes + cutpoint1, p1.genes + cutpoint1, sizeof(double) * size);
}


void mutation(chromosome_base_t p, chromosome_base_t c)
{
    memcpy(c.genes, p.genes, sizeof(double) * p.gene_size);
    int pos = randomRange(0, p.gene_size, -1);
    double rnd = randomDouble();
    c.genes[pos] = rnd;
}

double decoding(chromosome_base_t chromosome,
                population_decoding_t decoding_obj)
{
    job_t **jobs = decoding_obj.objects.jobs;
    machine_t **machines = decoding_obj.objects.machines;
    int NUMBER_OF_JOBS = decoding_obj.objects.NUMBER_OF_JOBS;
    int NUMBER_OF_MACHINES = decoding_obj.objects.NUMBER_OF_MACHINES;

    machine_base_operations_t *machine_ops =
        decoding_obj.operations.machine_ops;
    list_operations_t *list_ops = decoding_obj.operations.list_ops;
    job_base_operations_t *job_ops = decoding_obj.operations.job_ops;

    weights_t weights = decoding_obj.decoding_parameters.weights;
    std::map<std::pair<std::string, std::string>, double>
        transportation_time_table =
            decoding_obj.decoding_parameters.transportation_time_table;
    setup_time_parameters_t setup_times_parameters =
        decoding_obj.decoding_parameters.setup_times_parameters;
    int MAX_SETUP_TIMES =
        decoding_obj.decoding_parameters.scheduling_parameters.MAX_SETUP_TIMES;

    unsigned int machine_idx;
    machine_t *machine;

    for (int i = 0; i < NUMBER_OF_MACHINES; ++i) {
        machine_ops->reset(&(machines[i]->base));
    }

    // machine selection
    for (int j = 0; j < NUMBER_OF_JOBS; ++j) {
        job_ops->set_ms_gene_addr(&(jobs[j]->base), chromosome.ms_genes + j);
        job_ops->set_os_gene_addr(&(jobs[j]->base), chromosome.os_genes + j);
        machine_idx = job_ops->machine_selection(&jobs[j]->base);
        machine = (machine_t *) jobs[j]->base.process_time[machine_idx].machine;
        // // machine_no = jobs[j].base.machine_no;
        // machine = (machine_t *) jobs[j].base.current_machine;
        machine_ops->add_job(&machine->base, &jobs[j]->list);
    }

    // sorting;
    for (int i = 0; i < NUMBER_OF_MACHINES; ++i) {
        machine_ops->sort_job(&(machines[i]->base), list_ops);
    }

    // scheduling
    double value = 0;
    int setup_times_in1440 = 0;
    for (int i = 0; i < NUMBER_OF_MACHINES; ++i) {
        scheduling(machines[i], machine_ops, weights, transportation_time_table,
                   setup_times_parameters);
        insertAlgorithm(machines[i], machine_ops, weights,
                        transportation_time_table, setup_times_parameters);
        value += machines[i]->quality;
        setup_times_in1440 += machines[i]->setup_times;
    }

    if (setup_times_in1440 > MAX_SETUP_TIMES)
        value += weights.WEIGHT_MAX_SETUP_TIMES * setup_times_in1440;

    return value;
}



int chromosomeCmp(const void *_c1, const void *_c2)
{
    chromosome_base_t *c1 = (chromosome_base_t *) _c1;
    chromosome_base_t *c2 = (chromosome_base_t *) _c2;
    if (c1->fitnessValue > c2->fitnessValue)
        return 1;
    else
        return -1;
}
