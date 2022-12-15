#include "include/population.h"

population_base_t::population_base_t(unsigned int no,
                                     chromosome_base_t *chromosomes,
                                     int AMOUNT_OF_CHROMOSOMES,
                                     int AMOUNT_OF_R_CHROMOSOMES,
                                     int GENERATIONS,
                                     job_t **jobs,
                                     machine_t **machines,
                                     int NUMBER_OF_JOBS,
                                     int NUMBER_OF_MACHINES)
    : no(no),
      chromosomes(chromosomes),
      parameters({
          .AMOUNT_OF_CHROMOSOMES = AMOUNT_OF_CHROMOSOMES,
          .AMOUNT_OF_R_CHROMOSOMES = AMOUNT_OF_R_CHROMOSOMES,
          .GENERATIONS = GENERATIONS,
      }),
      objects({
          .jobs = jobs,
          .machines = machines,
          .NUMBER_OF_JOBS = NUMBER_OF_JOBS,
          .NUMBER_OF_MACHINES = NUMBER_OF_MACHINES,
      })
{
}

population_prepare_t::population_prepare_t(population_base_t &base)
    : population_base_t(base)
{
}

population_selection_t::population_selection_t(population_base_t &base,
                                               double SELECTION_RATE)
    : population_base_t(base), SELECTION_RATE(SELECTION_RATE)
{
}

population_crossover_t::population_crossover_t(population_base_t &base,
                                               double EVOLUTION_RATE)
    : population_base_t(base), EVOLUTION_RATE(EVOLUTION_RATE)
{
}

population_decoding_t::population_decoding_t(
    population_base_t &base,
    weights_t weights,
    setup_time_parameters_t setup_times_parameters,
    scheduling_parameters_t scheduling_parameters,
    std::map<std::pair<std::string, std::string>, double>
        transportation_time_table,
    list_operations_t *list_ops,
    job_base_operations_t *job_ops,
    machine_base_operations_t *machine_ops)
    : population_base_t(base),
      decoding_parameters(
          {.weights = weights,
           .setup_times_parameters = setup_times_parameters,
           .scheduling_parameters = scheduling_parameters,
           .transportation_time_table = transportation_time_table}),
      operations({
          .list_ops = list_ops,
          .job_ops = job_ops,
          .machine_ops = machine_ops,
      })
{
}
