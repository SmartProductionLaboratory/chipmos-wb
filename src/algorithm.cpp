//
// Created by YuChunLin on 2021/8/21.
//

#include "include/algorithm.h"
#ifdef WIN32
#include <winsock.h>
#else
#include <sys/socket.h>
#endif
// #include <unistd.h>
#include <vector>

using namespace std;

void multiple_stage_schedule::schedule()
{
    prescheduling(_machines, _lots);

    int stage2_setup_times = stage2Scheduling(
        _machines, _lots, _pop->parameters.scheduling_parameters.PEAK_PERIOD);

    _pop->parameters.scheduling_parameters.MAX_SETUP_TIMES -=
        stage2_setup_times;
    stage3Scheduling(_machines, _lots, _pop, _fd);
}

void multiple_stage_schedule::prescheduling(machines_t *machines, lots_t *lots)
{
    vector<lot_t *> prescheduled_lots = lots->prescheduledLots();
    vector<job_t *> prescheduled_jobs;

    foreach (prescheduled_lots, i) {
        job_t *job = new job_t();
        try {
            string prescheduled_model = machines->getModelByEntityName(
                prescheduled_lots[i]->preScheduledEntity());
            prescheduled_lots[i]->setPrescheduledModel(prescheduled_model);

            *job = prescheduled_lots[i]->job();
            machines->addPrescheduledJob(job);
            prescheduled_jobs.push_back(job);
        } catch (out_of_range &e) {
            // cout << "Error " << prescheduled_lots[i]->preScheduledEntity()
            //      << endl;
            delete job;
            lots->pushBackNotPrescheduledLot(prescheduled_lots[i]);
        }
    }

    machines->prescheduleJobs();
}

int multiple_stage_schedule::stage2Scheduling(machines_t *machines,
                                              lots_t *lots,
                                              double peak_period)
{
    machines->setupToolAndWire(lots->amountOfTools(), lots->amountOfWires());

    map<string, vector<lot_t *> > groups = lots->getLotsRecipeGroups();
    for (auto it = groups.begin(); it != groups.end(); it++) {
        vector<job_t *> jobs;
        string current_group = it->first;
        foreach (it->second, i) {
            lot_t *current_lot = it->second[i];

            current_lot->setCanRunLocation(machines->getModelLocations());
            machines->addJobLocation(current_lot->lotNumber(),
                                     current_lot->getCanRunLocations());
            machines->addJobProcessTimes(current_lot->lotNumber(),
                                         current_lot->getModelProcessTimes());

            job_t *job = new job_t();
            *job = current_lot->job();
            job->base.ptr_derived_object = job->list.ptr_derived_object = job;
            jobs.push_back(job);
        }
        machines->addGroupJobs(current_group, jobs);
    }
    machines->distributeOrphanMachines(peak_period);
    // TODO: add entity_limit
    int stage2_setup_times = machines->scheduleGroups();
    return stage2_setup_times;
}


void multiple_stage_schedule::stage3Scheduling(machines_t *machines,
                                               lots_t *lots,
                                               population_t *pop,
                                               int fd)
{
    machines->groupJobsByToolAndWire();
    machines->distributeTools();
    machines->distributeWires();
    // TODO: add entity limit
    machines->chooseMachinesForGroups();

    machine_t **machine_array;
    job_t **jobs;

    int NUMBER_OF_MACHINES;
    int NUMBER_OF_JOBS;

    machines->prepareMachines(&NUMBER_OF_MACHINES, &machine_array);
    machines->prepareJobs(&NUMBER_OF_JOBS, &jobs);

    pop->objects.NUMBER_OF_JOBS = NUMBER_OF_JOBS;
    pop->objects.NUMBER_OF_MACHINES = NUMBER_OF_MACHINES;
    pop->objects.jobs = jobs;
    pop->objects.machines = machine_array;

    pop->operations.machine_ops =
        machines->getInitilizedMachineBaseOperations();
    pop->operations.job_ops = machines->getInitializedJobBaseOperations();
    pop->operations.list_ops = machines->getInitializedListOperations();

    prepareChromosomes(&pop->chromosomes, pop->objects.NUMBER_OF_JOBS,
                       pop->parameters.AMOUNT_OF_R_CHROMOSOMES);

    // cout << "Number of machines : " << pop->objects.NUMBER_OF_MACHINES <<
    // endl; cout << "Number of jobs : " << pop->objects.NUMBER_OF_JOBS << endl;

    geneticAlgorithm(pop, fd);
}

void prepareChromosomes(chromosome_base_t **_chromosomes,
                        int NUMBER_OF_JOBS,
                        int NUMBER_OF_R_CHROMOSOMES)
{
    chromosome_base_t *chromosomes = (chromosome_base_t *) malloc(
        sizeof(chromosome_base_t) * NUMBER_OF_R_CHROMOSOMES);

    for (int i = 0; i < NUMBER_OF_R_CHROMOSOMES; ++i) {
        chromosomes[i].chromosome_no = i;
        chromosomes[i].gene_size = NUMBER_OF_JOBS << 1;
        chromosomes[i].genes =
            (double *) malloc(sizeof(double) * chromosomes[i].gene_size);
        chromosomes[i].ms_genes = chromosomes[i].genes;
        chromosomes[i].os_genes = chromosomes[i].genes + NUMBER_OF_JOBS;

        random(chromosomes[i].genes, chromosomes[i].gene_size);
    }
    *_chromosomes = chromosomes;
}

chromosome_base_t searchChromosome(double rnd,
                                   vector<chromosome_linker_t> linkers)
{
    foreach (linkers, i) {
        if (linkers[i].value > rnd)
            return linkers[i].chromosome;
    }
    return linkers[0].chromosome;
}

void chromosomeSelection(chromosome_base_t *chromosomes,
                         chromosome_base_t *tmp_chromosomes,
                         double elites_rate,
                         int AMOUNT_OF_CHROMOSOMES,
                         int AMOUNT_OF_R_CHROMOSOMES)
{
    double sum0 = 0, sum1 = 0;
    double accumulate = 0;
    double rnd = 0;

    int elites_amount = AMOUNT_OF_CHROMOSOMES * elites_rate;
    int random_amount = AMOUNT_OF_CHROMOSOMES - elites_amount;
    vector<chromosome_linker_t> linkers;

    for (int i = elites_amount; i < AMOUNT_OF_R_CHROMOSOMES; ++i)
        sum0 += chromosomes[i].fitnessValue;

    for (int i = elites_amount; i < AMOUNT_OF_R_CHROMOSOMES; ++i) {
        chromosomes[i].fitnessValue = sum0 / chromosomes[i].fitnessValue;
        sum1 += chromosomes[i].fitnessValue;
    }

    for (int i = elites_amount, j = 0; i < AMOUNT_OF_R_CHROMOSOMES; ++i, ++j) {
        copyChromosome(tmp_chromosomes[j], chromosomes[i]);
        linkers.push_back(chromosome_linker_t{
            .chromosome = tmp_chromosomes[j],
            .value = accumulate += chromosomes[i].fitnessValue / sum1});
    }

    for (int i = 0, j = elites_amount; i < random_amount; ++i, ++j) {
        rnd = randomDouble();
        chromosome_base_t selected_chromosome = searchChromosome(rnd, linkers);
        copyChromosome(chromosomes[j], selected_chromosome);
    }
}

void geneticAlgorithm(population_t *pop, int fd)
{
    int AMOUNT_OF_JOBS = pop->objects.NUMBER_OF_JOBS;
    int NUMBER_OF_MACHINES = pop->objects.NUMBER_OF_MACHINES;
    int MAX_SETUP_TIMES = pop->parameters.MAX_SETUP_TIMES;

    job_t **jobs = pop->objects.jobs;

    chromosome_base_t *chromosomes = pop->chromosomes;
    chromosome_base_t *tmp_chromosomes;

    machine_t **machines = pop->objects.machines;
    // map<unsigned int, machine_t *> machines = pop->round.machines;

    prepareChromosomes(&tmp_chromosomes, AMOUNT_OF_JOBS,
                       pop->parameters.AMOUNT_OF_R_CHROMOSOMES);

    // ops
    machine_base_operations_t *machine_ops = pop->operations.machine_ops;
    list_operations_t *list_ops = pop->operations.list_ops;
    job_base_operations_t *job_ops = pop->operations.job_ops;

    // initialize machine_op
    char output_string[1024];
    int string_length = 0;

    int generations = pop->parameters.GENERATIONS;
    int AMOUNT_OF_R_CHROMOSOMES = pop->parameters.AMOUNT_OF_R_CHROMOSOMES;
    int AMOUNT_OF_CHROMOSOMES = pop->parameters.AMOUNT_OF_CHROMOSOMES;

    for (int k = 0; k < generations && !stop; ++k) {
        for (int i = 0; i < AMOUNT_OF_R_CHROMOSOMES;
             ++i) {  // for all chromosomes
            chromosomes[i].fitnessValue = decoding(
                chromosomes[i], pop->objects, pop->operations, pop->parameters);
        }
        // sort the chromosomes
        qsort(chromosomes, pop->parameters.AMOUNT_OF_R_CHROMOSOMES,
              sizeof(chromosomes[0]), chromosomeCmp);
        string_length = printf("%d/%d-%lf\n", k, pop->parameters.GENERATIONS,
                               chromosomes[0].fitnessValue);
        // fprintf(stdout, output_string);
        // write(1, output_string, string_length);
        // statistic
        chromosomeSelection(chromosomes, tmp_chromosomes,
                            pop->parameters.SELECTION_RATE,
                            pop->parameters.AMOUNT_OF_CHROMOSOMES,
                            pop->parameters.AMOUNT_OF_CHROMOSOMES);

        // evolution
        // crossover
        int crossover_amount = pop->parameters.AMOUNT_OF_CHROMOSOMES *
                               pop->parameters.EVOLUTION_RATE;
        for (int l = AMOUNT_OF_CHROMOSOMES;
             l < crossover_amount + AMOUNT_OF_CHROMOSOMES; l += 2) {
            int rnd1 = randomRange(0, AMOUNT_OF_CHROMOSOMES, -1);
            int rnd2 = randomRange(0, AMOUNT_OF_CHROMOSOMES, rnd1);
            crossover(chromosomes[rnd1], chromosomes[rnd2], chromosomes[l],
                      chromosomes[l + 1]);
        }
        // mutation
        for (int l = pop->parameters.AMOUNT_OF_CHROMOSOMES + crossover_amount;
             l < pop->parameters.AMOUNT_OF_R_CHROMOSOMES; ++l) {
            int rnd = randomRange(0, pop->parameters.AMOUNT_OF_CHROMOSOMES, -1);
            mutation(chromosomes[rnd], chromosomes[l]);
        }
    }

    decoding(chromosomes[0], pop->objects, pop->operations, pop->parameters);

    // update machines' avaliable time and set the last job
    for (int i = 0; i < NUMBER_OF_MACHINES; ++i) {
        setLastJobInMachine(machines[i]);
        machines[i]->base.available_time =
            machines[i]->current_job.base.end_time;
    }
}
