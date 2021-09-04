#include "include/machines.h"
#include "include/info.h"
#include "include/job_base.h"
#include "include/linked_list.h"
#include "include/machine.h"
#include "include/machine_base.h"
#include "include/parameters.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <set>
#include <stdexcept>
#include <vector>


using namespace std;

machines_t::machines_t()
{
    setup_time_parameters_t param;
    weights_t weights;
    memset(&param, 0, sizeof(param));
    _param = param;

    memset(&weights, 0, sizeof(weights));
    _weights = weights;

    threshold = 2500;

    _init(_param);
}

machines_t::machines_t(setup_time_parameters_t param, weights_t weights)
{
    threshold = 2500;
    _init(param);
    _weights = weights;
}

machines_t::~machines_t()
{
    for (map<string, machine_t *>::iterator it = _machines.begin();
         it != _machines.end(); it++) {
        delete it->second;
    }

    free(list_ops);
    free(machine_ops);
    free(job_ops);
}

machines_t::machines_t(machines_t &other)
{
    throw invalid_argument("copy constructor hasn't been implemented");
}

void machines_t::_init(setup_time_parameters_t parameters)
{
    list_ops = (list_operations_t *) malloc(sizeof(list_operations_t));
    *list_ops = LINKED_LIST_OPS;

    job_ops = (job_base_operations_t *) malloc(sizeof(job_base_operations_t));
    *job_ops = JOB_BASE_OPS;

    size_t num_of_setup_time_units =
        sizeof(setup_time_parameters_t) / sizeof(double);

    machine_ops = (machine_base_operations_t *) malloc(
        sizeof(machine_base_operations_t) +
        sizeof(setup_time_unit_t) * num_of_setup_time_units);

    machine_ops->add_job = machineBaseAddJob;
    machine_ops->sort_job = machineBaseSortJob;
    machine_ops->setup_time_functions[0] = {setupTimeCWN, parameters.TIME_CWN};
    machine_ops->setup_time_functions[1] = {setupTimeCK, parameters.TIME_CK};
    machine_ops->setup_time_functions[2] = {setupTimeEU, parameters.TIME_EU};
    machine_ops->setup_time_functions[3] = {setupTimeMC, parameters.TIME_MC};
    machine_ops->setup_time_functions[4] = {setupTimeSC, parameters.TIME_SC};
    machine_ops->setup_time_functions[5] = {setupTimeCSC, parameters.TIME_CSC};
    machine_ops->setup_time_functions[6] = {setupTimeUSC, parameters.TIME_USC};
    machine_ops->sizeof_setup_time_function_array =
        num_of_setup_time_units - 1;  // -1 is for ICSI
    machine_ops->reset = machineReset;
}


void machines_t::addMachine(machine_t machine)
{
    // check if machine exist
    string machine_name(machine.base.machine_no.data.text);
    string model_name(machine.model_name.data.text);
    string location(machine.location.data.text);
    if (_machines.count(machine_name) == 1) {
        throw std::invalid_argument("[" + machine_name + "] is added twice");
    }


    machine_t *machine_ptr = nullptr;
    machine_ptr = new machine_t;
    if (machine_ptr == nullptr) {
        perror("Failed to new a machine instance");
        exit(EXIT_FAILURE);
    }
    *machine_ptr = machine;

    machine_ptr->current_job.base.ptr_derived_object =
        &machine_ptr->current_job;
    machine_ptr->current_job.list.ptr_derived_object =
        &machine_ptr->current_job;

    machine_ptr->tools.areses = nullptr;
    machine_ptr->tools.number = 0;

    machine_ptr->wires.areses = nullptr;
    machine_ptr->wires.number = 0;


    // add into container
    _machines[machine_name] = machine_ptr;

    _v_machines.push_back(machine_ptr);


    string part_id(machine_ptr->current_job.part_id.data.text),
        part_no(machine_ptr->current_job.part_no.data.text);

    _tool_machines[part_no].push_back(machine_ptr);
    _wire_machines[part_id].push_back(machine_ptr);
    _tool_wire_machines[part_no + "_" + part_id].push_back(machine_ptr);

    if (_model_locations.count(model_name) == 0) {
        _model_locations[model_name] = vector<string>();
    }

    if (find(_model_locations[model_name].begin(),
             _model_locations[model_name].end(),
             location) == _model_locations[model_name].end()) {
        _model_locations[model_name].push_back(location);
    }
}

void machines_t::addPrescheduledJob(job_t *job)
{
    job->base.ptr_derived_object = job;
    job->list.ptr_derived_object = job;
    string machine_no(job->base.machine_no.data.text);
    machine_ops->add_job(&_machines.at(machine_no)->base, &job->list);
}

void machines_t::prescheduleJobs()
{
    job_t *job_on_machine;

    for (map<string, machine_t *>::iterator it = _machines.begin();
         it != _machines.end(); ++it) {
        machine_ops->sort_job(&it->second->base, list_ops);
        scheduling(it->second, machine_ops, _weights, _param);
        setJob2Scheduled(it->second);

        // collect the job which is on the machine
        job_on_machine = new job_t();
        *job_on_machine = it->second->current_job;
        _scheduled_jobs.push_back(job_on_machine);

        setLastJobInMachine(
            it->second);  // if the machine is scheduled, it will be set
    }

    // collect scheduled jobs
    list_ele_t *list;
    for (map<string, machine_t *>::iterator it = _machines.begin();
         it != _machines.end(); ++it) {
        list = it->second->base.root;
        while (list) {
            _scheduled_jobs.push_back((job_t *) list->ptr_derived_object);
            list = list->next;
        }
    }
}

void machines_t::_collectScheduledJobs(machine_t *machine,
                                       std::vector<job_t *> &scheduled_jobs)
{
    list_ele_t *list;
    list = machine->base.root;
    while (list) {
        scheduled_jobs.push_back((job_t *) list->ptr_derived_object);
        list = list->next;
    }
}

bool machinePtrComparison(machine_t *m1, machine_t *m2)
{
    return m1->base.available_time < m2->base.available_time;
}

bool jobPtrComparison(job_t *j1, job_t *j2)
{
    return j1->base.arriv_t < j2->base.arriv_t;
}

void machines_t::addGroupJobs(string recipe, vector<job_t *> jobs)
{
    if (_dispatch_groups.count(recipe) != 0)
        cerr << "Warning : you add group of jobs twice, recipe is [" << recipe
             << "]" << endl;

    iter(jobs, i)
    {
        jobs[i]->base.ptr_derived_object = jobs[i];
        jobs[i]->list.ptr_derived_object = jobs[i];
    }

    vector<machine_t *> machines;
    for (auto it = _machines.begin(); it != _machines.end(); it++) {
        if (strcmp(it->second->current_job.bdid.data.text, recipe.c_str()) ==
            0) {
            machines.push_back(it->second);
        }
    }

    _dispatch_groups[recipe] =
        (struct __machine_group_t){.machines = machines,
                                   .unscheduled_jobs = jobs,
                                   .scheduled_jobs = vector<job_t *>()};
}

vector<machine_t *> machines_t::_sortedMachines(vector<machine_t *> &ms)
{
    sort(ms.begin(), ms.end(), machinePtrComparison);
    return ms;
}

vector<job_t *> machines_t::_sortedJobs(std::vector<job_t *> &jobs)
{
    sort(jobs.begin(), jobs.end(), jobPtrComparison);
    return jobs;
}


void machines_t::_scheduleAGroup(struct __machine_group_t *group)
{
    vector<machine_t *> machines = group->machines;
    vector<job_t *> unscheduled_jobs = group->unscheduled_jobs;

    sort(machines.begin(), machines.end(), machinePtrComparison);
    sort(unscheduled_jobs.begin(), unscheduled_jobs.end(), jobPtrComparison);
    bool end = true;
    while (end) {
        int num_scheduled_jobs = 0;
        iter(machines, i)
        {
            string location(machines[i]->location.data.text);
            string model(machines[i]->model_name.data.text);

            iter(unscheduled_jobs, j)
            {
                if (unscheduled_jobs[j]->is_scheduled)
                    continue;

                // check the location is suitable
                string lot_number(unscheduled_jobs[j]->base.job_info.data.text);
                vector<string> locations = _job_can_run_locations[lot_number];

                if (find(locations.begin(), locations.end(), location) !=
                        locations.end() &&
                    (unscheduled_jobs[j]->base.arriv_t -
                         machines[i]->base.available_time <=
                     60)) {
                    unscheduled_jobs[j]->base.ptime =
                        _job_process_times[lot_number][model];
                    staticAddJob(machines[i], unscheduled_jobs[j], machine_ops);
                    unscheduled_jobs[j]->is_scheduled = true;
                    unscheduled_jobs[j]->base.machine_no =
                        machines[i]->base.machine_no;
                    num_scheduled_jobs += 1;
                    break;
                }
            }
        }
        if (num_scheduled_jobs == 0)
            end = false;
    }

    group->unscheduled_jobs.clear();
    group->scheduled_jobs.clear();
    iter(unscheduled_jobs, i)
    {
        if (unscheduled_jobs[i]->is_scheduled) {
            group->scheduled_jobs.push_back(unscheduled_jobs[i]);
        } else {
            group->unscheduled_jobs.push_back(unscheduled_jobs[i]);
        }
    }
    iter(machines, i) { setLastJobInMachine(machines[i]); }
}

void machines_t::scheduleGroups()
{
    std::map<std::string, struct __machine_group_t> ngroups;
    for (map<string, struct __machine_group_t>::iterator it =
             _dispatch_groups.begin();
         it != _dispatch_groups.end(); it++) {
        _scheduleAGroup(&it->second);
        if (it->second.unscheduled_jobs.size() != 0) {
            ngroups[it->first] = it->second;
            ngroups[it->first].scheduled_jobs.clear();
            ngroups[it->first].machines.clear();
        }
    }
    _dispatch_groups = ngroups;
    // for (map<string, struct __machine_group_t>::iterator it =
    //          _dispatch_groups.begin();
    //      it != _dispatch_groups.end(); it++) {
    //     printf("[%s] : %lu jobs\n", it->first.c_str(),
    //            it->second.unscheduled_jobs.size());
    // }
}

bool machines_t::_isThereAnyUnusedResource(
    std::map<std::string, std::vector<ares_t *>> _resources,
    std::string resource_name,
    int threshold)
{
    vector<ares_t *> resources = _resources.at(resource_name);
    vector<ares_t *> result;
    iter(resources, i)
    {
        if (resources[i]->available_time == 0) {
            result.push_back(resources[i]);
        }
    }

    return result.size() > threshold;
}

vector<job_t *> machines_t::_reconsiderJobsOnAMachine(machine_t *machine,
                                                      int threshold)
{
    vector<job_t *> jobs;
    list_ele_t *iterator = machine->base.root;
    while (iterator) {
        job_t *job = (job_t *) iterator->ptr_derived_object;
        if (job->base.end_time >= threshold) {
            if (iterator->prev) {
                iterator->prev->next = NULL;
                iterator->prev = NULL;
            }
            jobs.push_back(job);
        }
        iterator = iterator->next;
    }

    // make the information in machine correct
    iterator = machine->base.root;
    while (iterator->next) {
        iterator = iterator->next;
    }
    machine->base.tail = iterator;

    setLastJobInMachine(machine);

    return jobs;
}

void machines_t::reconsiderJobs()
{
    iter(_v_machines, i)
    {
        // bigger then threshold -> check the tool and wire
        if (_v_machines[i]->base.available_time > threshold) {
            string part_no(_v_machines[i]->current_job.part_no.data.text);
            string part_id(_v_machines[i]->current_job.part_id.data.text);
            string bd_id(_v_machines[i]->current_job.bdid.data.text);
            if (_isThereAnyUnusedResource(_tools, part_no) &&
                _isThereAnyUnusedResource(_wires, part_id)) {
                // take out the jobs
                vector<job_t *> jobs =
                    _reconsiderJobsOnAMachine(_v_machines[i], threshold);
                _dispatch_groups[bd_id].unscheduled_jobs = jobs;
            }
        }

        // collect the scheduled job
        _collectScheduledJobs(_v_machines[i], _scheduled_jobs);
    }


    // update the tools and the wires
    _updateAllKindOfResourcesAvailableTime(_tools, _tool_machines);
    _updateAllKindOfResourcesAvailableTime(_wires, _wire_machines);
}

void machines_t::groupJobsByToolAndWire()
{
    // collected unmatched jobs
    for (map<string, struct __machine_group_t>::iterator it =
             _dispatch_groups.begin();
         it != _dispatch_groups.end(); it++) {
        iter(it->second.unscheduled_jobs, i)
        {
            string part_id(it->second.unscheduled_jobs[i]->part_id.data.text);
            string part_no(it->second.unscheduled_jobs[i]->part_no.data.text);
            string key = part_no + "_" + part_id;
            if (_tool_wire_jobs_groups.count(key) == 0) {
                _tool_wire_jobs_groups[key] = new (struct __job_group_t);
                *(_tool_wire_jobs_groups[key]) = (struct __job_group_t){
                    .part_no = part_no,
                    .part_id = part_id,
                    .number_of_tools = _number_of_tools.count(part_no) == 0
                                           ? 0
                                           : _number_of_tools[part_no],
                    .number_of_wires = _number_of_wires.count(part_id) == 0
                                           ? 0
                                           : _number_of_wires[part_id]};
            }
            _tool_wire_jobs_groups[key]->orphan_jobs.push_back(
                it->second.unscheduled_jobs[i]);
        }
    }

    //

    for (map<string, struct __job_group_t *>::iterator it =
             _tool_wire_jobs_groups.begin();
         it != _tool_wire_jobs_groups.end(); it++) {
        string part_id = it->second->part_id;
        string part_no = it->second->part_no;
        it->second->number_of_jobs = it->second->orphan_jobs.size();
        _wire_jobs_groups[part_id].push_back(it->second);
        _tool_jobs_groups[part_no].push_back(it->second);
        _jobs_groups.push_back(it->second);
    }
}


bool distEntryComparison(struct __distribution_entry_t ent1,
                         struct __distribution_entry_t ent2)
{
    return ent1.ratio < ent2.ratio;
}

map<string, int> machines_t::_distributeAResource(
    int number_of_resources,
    map<string, int> groups_statistic)
{
    vector<struct __distribution_entry_t> data;

    double sum = 0;
    for (map<string, int>::iterator it = groups_statistic.begin();
         it != groups_statistic.end(); ++it) {
        sum += it->second;
    }

    for (map<string, int>::iterator it = groups_statistic.begin();
         it != groups_statistic.end(); ++it) {
        data.push_back((struct __distribution_entry_t){
            .name = it->first, .ratio = it->second / sum});
    }

    sort(data.begin(), data.end(), distEntryComparison);

    int original_number_of_resources = number_of_resources;
    int i = 0;
    map<string, int> result;
    while (i < data.size() - 1 && number_of_resources > 0) {
        int _n_res = data[i].ratio * original_number_of_resources;
        if (_n_res == 0) {
            _n_res = 1;
        }
        result[data[i].name] = _n_res;
        number_of_resources -= _n_res;
        ++i;
    }

    result[data.back().name] = number_of_resources;

    return result;
}

void machines_t::distributeTools()
{
    // distribute tools
    for (map<string, std::vector<struct __job_group_t *>>::iterator it =
             _tool_jobs_groups.begin();
         it != _tool_jobs_groups.end(); ++it) {
        string part_no = it->first;
        map<string, int> data;
        // collect the data
        iter(it->second, i)
        {
            // set part_id to a key because I am distributing the number of
            // tools All the groups in it->second have the same part_no, but
            // part_id is different
            data[it->second[i]->part_id] = it->second[i]->number_of_jobs;
        }

        // calculate
        data = _distributeAResource(_number_of_tools.at(it->first), data);

        // setup
        for (map<string, int>::iterator it2 = data.begin(); it2 != data.end();
             it2++) {
            string part_id = it2->first;
            string key = part_no + "_" + part_id;
            _tool_wire_jobs_groups.at(key)->number_of_tools = it2->second;
        }
    }
}

void machines_t::distributeWires()
{
    // distribute tools
    for (map<string, std::vector<struct __job_group_t *>>::iterator it =
             _wire_jobs_groups.begin();
         it != _wire_jobs_groups.end(); ++it) {
        string part_id = it->first;
        map<string, int> data;
        // collect the data
        iter(it->second, i)
        {
            // set part_no to a key because I am distributing the number of
            // wires All the groups in it->second have the same part_id, but
            // part_no is different
            data[it->second[i]->part_no] = it->second[i]->number_of_jobs;
        }

        // calculate
        data = _distributeAResource(_number_of_wires.at(it->first), data);

        // setup
        for (map<string, int>::iterator it2 = data.begin(); it2 != data.end();
             it2++) {
            string part_no = it2->first;
            string key = part_no + "_" + part_id;
            _tool_wire_jobs_groups.at(key)->number_of_wires = it2->second;
        }
    }

    for (map<string, struct __job_group_t *>::iterator it =
             _tool_wire_jobs_groups.begin();
         it != _tool_wire_jobs_groups.end(); ++it) {
        printf("[%s]-[%s] : (%d)#(%d) -> %lu\n", it->second->part_no.c_str(),
               it->second->part_id.c_str(), it->second->number_of_tools,
               it->second->number_of_wires, it->second->jobs.size());
    }
}

bool machines_t::_canJobRunOnTheMachine(job_t *job, machine_t *machine)
{
    string lot_number(job->base.job_info.data.text);
    string location(machine->location.data.text);
    string model(machine->model_name.data.text);
    vector<string> locations = _job_can_run_locations[lot_number];
    map<string, double> process_times = _job_process_times[lot_number];
    return find(locations.begin(), locations.end(), location) !=
               locations.end() &&
           process_times.count(model) != 0;
}

bool machines_t::_addNewResource(
    machine_t *machine,
    std::string resource_name,
    std::map<std::string, std::vector<std::string>> &container)
{
    string name(machine->base.machine_no.data.text);
    vector<string> &resources = container[name];
    if (find(resources.begin(), resources.end(), resource_name) ==
        resources.end()) {
        resources.push_back(resource_name);
        return true;
    } else
        return false;
}

void machines_t::_chooseMachinesForAGroup(
    struct __job_group_t *group,
    vector<machine_t *> candidate_machines)
{
    // FIXME : need to be well test
    vector<job_t *> good_jobs;  // which means that the job has more than one
                                // can_run machines
    vector<job_t *> bad_jobs;   // the job has no any can-run machines

    bad_jobs = group->orphan_jobs;
    group->orphan_jobs.clear();

    candidate_machines = _sortedMachines(candidate_machines);

    // FIXME : need to delete the testing variable
    map<string, vector<string>> suitable_machines;

    int number_of_tools = group->number_of_tools;
    int number_of_wires = group->number_of_wires;
    string part_id = group->part_id;
    string part_no = group->part_no;

    vector<string> can_run_machines;

    // go through the machines
    // until running out of tools or wires
    int i = 0;
    for (i = 0; i < candidate_machines.size() && number_of_tools > 0 &&
                number_of_wires > 0;
         ++i) {
        vector<job_t *> nbad_jobs;
        vector<job_t *> ngood_jobs;
        iter(bad_jobs, j)
        {
            string lot_number(bad_jobs[j]->base.job_info.data.text);
            if (_canJobRunOnTheMachine(bad_jobs[j], candidate_machines[i])) {
                _job_can_run_machines[lot_number].push_back(
                    string(candidate_machines[i]->base.machine_no.data.text));

                suitable_machines[lot_number].push_back(
                    string(candidate_machines[i]->base.machine_no.data.text));

                ngood_jobs.push_back(bad_jobs[j]);
            } else {
                nbad_jobs.push_back(bad_jobs[j]);
            }
        }

        string model_name(candidate_machines[i]->base.machine_no.data.text);
        bool used = false;  // a flag to describe if the machine is choose below
        // if ngood_jobs has jobs means that the machine is a good machine
        if (ngood_jobs.size() || bad_jobs.size() == 0) {
            iter(good_jobs, j)
            {
                string lot_number(good_jobs[j]->base.job_info.data.text);
                if (_canJobRunOnTheMachine(good_jobs[j],
                                           candidate_machines[i])) {
                    _job_can_run_machines[lot_number].push_back(string(
                        candidate_machines[i]->base.machine_no.data.text));

                    suitable_machines[lot_number].push_back(string(
                        candidate_machines[i]->base.machine_no.data.text));
                    used = true;
                }
            }
            good_jobs += ngood_jobs;  // update the good_jobs container

            // update the tool and wire carried by the machines
            // update tool
            if (used || ngood_jobs.size()) {  // if the machine is chosen in
                                              // first round or second round
                if (_addNewResource(candidate_machines[i], part_no,
                                    _machines_tools)) {
                    number_of_tools -= 1;
                    // printf("%s\n",
                    // candidate_machines[i]->base.machine_no.data.text);
                }
                if (_addNewResource(candidate_machines[i], part_id,
                                    _machines_wires)) {
                    number_of_wires -= 1;
                }
            }
        }
        bad_jobs = nbad_jobs;
    }
    group->number_of_tools = number_of_tools;
    group->number_of_wires = number_of_wires;
    group->orphan_jobs += bad_jobs;
    group->jobs += good_jobs;
}

void machines_t::_initializeNumberOfExpectedMachines()
{
    for (map<string, struct __job_group_t *>::iterator it =
             _tool_wire_jobs_groups.begin();
         it != _tool_wire_jobs_groups.end(); ++it) {
        int expected_num_of_machines =
            min(it->second->number_of_wires, it->second->number_of_tools);
        it->second->number_of_machines =
            expected_num_of_machines > 0 ? expected_num_of_machines : 0;
    }
}

// sorting comparison function from big to small
bool jobGroupComparisonByNumberOfMachines(struct __job_group_t *g1,
                                          struct __job_group_t *g2)
{
    return g1->number_of_machines > g2->number_of_machines;
}

void machines_t::chooseMachinesForGroups()
{
    // initialize the number of expected machines
    _initializeNumberOfExpectedMachines();

    // sort the the group by num_of_machines in decreasing order
    sort(_jobs_groups.begin(), _jobs_groups.end(),
         jobGroupComparisonByNumberOfMachines);

    vector<machine_t *> out_of_range_machines;
    vector<machine_t *> in_the_range_machines;
    iter(_v_machines, i)
    {
        if (_v_machines[i]->base.available_time < threshold) {
            in_the_range_machines.push_back(_v_machines[i]);
        } else {
            out_of_range_machines.push_back(_v_machines[i]);
        }
    }

    // Two stages machine selection
    // In the first stage choose the machine whose available time is less then
    // threshold. If the all the jobs in stage 1 are able to choose their
    // favorite machines, they won't be allowed to choose the machine in stage3.
    // If the jobs
    // In the second stage choose the machine whose available time is bigger
    // then threshold
    iter(_jobs_groups, i)
    {
        _jobs_groups[i]->jobs.clear();
        _chooseMachinesForAGroup(_jobs_groups[i], in_the_range_machines);
        if (_jobs_groups[i]->orphan_jobs.size()) {
            _chooseMachinesForAGroup(_jobs_groups[i], out_of_range_machines);
        }
    }

    iter(_jobs_groups, i)
    {
        if (_jobs_groups[i]->orphan_jobs.size()) {
            printf("[%s]-[%s] loss : %lu\n", _jobs_groups[i]->part_no.c_str(),
                   _jobs_groups[i]->part_id.c_str(),
                   _jobs_groups[i]->orphan_jobs.size());
        }
    }

    map<string, int> selected_model_statistic;
    map<string, int> available_model_statistic;
    vector<string> selected_machines;
    iter(_jobs_groups, i)
    {
        iter(_jobs_groups[i]->jobs, j)
        {
            string lot_number(
                _jobs_groups[i]->jobs[j]->base.job_info.data.text);
            vector<string> can_run_machines = _job_can_run_machines[lot_number];
            selected_machines += can_run_machines;

            map<string, double> process_times = _job_process_times[lot_number];
            for (map<string, double>::iterator it = process_times.begin();
                 it != process_times.end(); ++it) {
                if (available_model_statistic.count(it->first) == 0) {
                    available_model_statistic[it->first] = 0;
                }

                available_model_statistic[it->first] += 1;
            }
            // iter(can_run_machines, k){
            //     machine_t *machine = _machines.at(can_run_machines[k]);
            //     string model_name(machine->model_name.data.text);
            //     if(data.count(model_name) == 0){
            //         data[model_name] = 0;
            //     }

            //     data[model_name] += 1;
            // }
        }
    }

    set<string> selected_machines_set(selected_machines.begin(),
                                      selected_machines.end());
    selected_machines = vector<string>(selected_machines_set.begin(),
                                       selected_machines_set.end());
    iter(selected_machines, i)
    {
        machine_t *machine = _machines.at(selected_machines[i]);
        string model_name(machine->model_name.data.text);
        if (selected_model_statistic.count(model_name) == 0) {
            selected_model_statistic[model_name] = 0;
        }
        selected_model_statistic[model_name] += 1;
    }
    cout << "Determined Machine Statistic" << endl;
    for (map<string, int>::iterator it = selected_model_statistic.begin();
         it != selected_model_statistic.end(); ++it) {
        cout << "\"" << it->first << "\" : " << it->second << endl;
    }

    cout << "Available Machine Statistic" << endl;
    for (map<string, int>::iterator it = available_model_statistic.begin();
         it != available_model_statistic.end(); ++it) {
        cout << "\"" << it->first << "\" : " << it->second << endl;
    }
}

void machines_t::_setupContainersForMachines()
{
    _tool_machines.clear();
    _wire_machines.clear();
    _tool_wire_machines.clear();
    iter(_v_machines, i)
    {
        string part_no(_v_machines[i]->current_job.part_no.data.text);
        string part_id(_v_machines[i]->current_job.part_id.data.text);
        string key = part_no + "_" + part_id;
        _tool_machines[part_no].push_back(_v_machines[i]);
        _wire_machines[part_id].push_back(_v_machines[i]);
        _tool_wire_machines[key].push_back(_v_machines[i]);
    }
}

void machines_t::_createResources(
    std::map<std::string, int> &number_of_resource,
    std::map<std::string, std::vector<ares_t *>> &resource_instance_container)
{
    if (number_of_resource.size() == 0)
        throw logic_error(
            "You haven't set the number of this kind of resource");

    for (map<string, int>::iterator it = number_of_resource.begin();
         it != number_of_resource.end(); ++it) {
        int number_of_resource = it->second;
        vector<ares_t *> areses;
        for (int i = 0; i < number_of_resource; ++i) {
            ares_t *ares = new ares_t();
            *ares = ares_t{.name = stringToInfo(it->first),
                           .available_time = 0,
                           .used = false,
                           .time = 0};
            areses.push_back(ares);
        }
        resource_instance_container[it->first] = areses;
    }
}

void machines_t::_updateAKindOfResourceAvailableTime(
    std::vector<ares_t *> &resource_instances,
    std::vector<machine_t *> &resource_machines)
{
    // sort the machines by available time in increasing order
    sort(resource_machines.begin(), resource_machines.end(),
         machinePtrComparison);

    int i = 0, j = 0;
    while (i < resource_instances.size() && j < resource_machines.size()) {
        resource_instances[i]->available_time =
            resource_machines[j]->base.available_time > 0
                ? resource_machines[j]->base.available_time
                : 0;
        ++i;
        ++j;
    }

    while (i < resource_instances.size()) {
        resource_instances[i]->available_time = 0;
        ++i;
    }

    sort(resource_instances.begin(), resource_instances.end(), aresPtrComp);
}

void machines_t::_updateAllKindOfResourcesAvailableTime(
    std::map<std::string, std::vector<ares_t *>> &resource_instances,
    std::map<std::string, std::vector<machine_t *>> &resource_machines)
{
    for (map<string, std::vector<ares_t *>>::iterator it =
             resource_instances.begin();
         it != resource_instances.end(); ++it) {
        string resource_name = it->first;
        _updateAKindOfResourceAvailableTime(resource_instances[resource_name],
                                            resource_machines[resource_name]);
    }
}

void machines_t::_setupResources(
    map<string, int> &number_of_resource,
    map<string, vector<ares_t *>> &resources_instance_container,
    map<string, vector<machine_t *>> &resource_machines)
{
    // create tool
    _createResources(number_of_resource, resources_instance_container);

    // update the resources' available time
    _updateAllKindOfResourcesAvailableTime(resources_instance_container,
                                           resource_machines);
}


void machines_t::setupToolAndWire()
{
    _setupContainersForMachines();
    _setupResources(_number_of_tools, _tools, _tool_machines);
    _setupResources(_number_of_wires, _wires, _wire_machines);
}

resources_t machines_t::_loadResource(
    std::vector<std::string> list,
    std::map<std::string, std::vector<ares_t *>> &resource_instances,
    std::map<std::string, std::vector<ares_t *>> &used_resources)
{
    int number_of_resources = list.size();
    resources_t resources;
    resources.areses =
        (ares_t **) malloc(sizeof(ares_t *) * number_of_resources);
    resources.number = number_of_resources;
    iter(list, i)
    {
        // TODO :should find the resource which satisfy with
        //  min(|res->available->time - machine->available->time|)
        std::string res_name = list[i];
        ares_t *res = resource_instances[res_name].back();
        resources.areses[i] = res;
        resource_instances[res_name].pop_back();
        used_resources[res_name].push_back(res);
    }

    return resources;
}


void machines_t::_loadResourcesOnTheMachine(machine_t *machine)
{
    string name(machine->base.machine_no.data.text);
    vector<string> tool_list = _machines_tools[name];
    vector<string> wire_list = _machines_wires[name];

    // load resource
    // cout<<"Machine : " << name << endl;
    machine->tools = _loadResource(tool_list, _tools, _loaded_tools);
    machine->wires = _loadResource(wire_list, _wires, _loaded_wires);
}


void machines_t::prepareMachines(int *number, machine_t ***machine_array)
{
    machine_t **machines;
    vector<string> machine_lists;
    for (map<string, vector<string>>::iterator it = _machines_tools.begin();
         it != _machines_tools.end(); ++it) {
        machine_lists.push_back(it->first);
    }

    int num_of_machines = machine_lists.size();

    machines = (machine_t **) malloc(sizeof(machine_t *) * num_of_machines);

    iter(machine_lists, i)
    {
        machine_t *machine = _machines[machine_lists[i]];
        string machine_no(machine->base.machine_no.data.text);
        _loadResourcesOnTheMachine(machine);
        machines[i] = machine;
        machines[i]->base.ptr_derived_object = machines[i];
    }



    *number = num_of_machines;
    *machine_array = machines;
}

void machines_t::_linkMachineToAJob(job_t *job)
{
    string lot_number(job->base.job_info.data.text);

    // FIXME : delete the variables
    // string part_no(job->part_no.data.text);
    vector<string> can_run_machines = _job_can_run_machines.at(lot_number);
    process_time_t *process_times = nullptr;
    process_times = (process_time_t *) malloc(sizeof(process_time_t) *
                                              can_run_machines.size());

    iter(can_run_machines, i)
    {
        string machine_name = can_run_machines[i];
        machine_t *machine = _machines[machine_name];
        string model_name(machine->model_name.data.text);
        double ptime = _job_process_times[lot_number][model_name];
        process_time_t pt =
            (process_time_t){.machine_no = machine->base.machine_no,
                             .machine = machine,
                             .process_time = ptime};
        process_times[i] = pt;
    }
    job_ops->set_process_time(&job->base, process_times,
                              can_run_machines.size());
}

void machines_t::prepareJobs(int *number, job_t ***job_array)
{
    vector<job_t *> jobs;
    iter(_jobs_groups, i)
    {
        iter(_jobs_groups[i]->jobs, j)
        {
            jobs.push_back(_jobs_groups[i]->jobs[j]);
        }
    }
    job_t **arr = (job_t **) malloc(sizeof(job_t *) * jobs.size());
    iter(jobs, i)
    {
        _linkMachineToAJob(jobs[i]);
        arr[i] = jobs[i];
        arr[i]->base.ptr_derived_object = arr[i];
        arr[i]->list.ptr_derived_object = arr[i];
    }

    *number = jobs.size();
    *job_array = arr;
}