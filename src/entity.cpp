#include <cstdio>
#include <iterator>
#include <stdexcept>

#include "include/entity.h"
#include "include/infra.h"
#include "include/lot.h"
#include "include/machine.h"
//#include "include/machines.h"

using namespace std;

double setupTime(job_t *prev, job_t *next, machine_base_operations_t *ops)
{
    // for all setup time function
    double time = 0;
    if (isSameInfo(prev->bdid, next->bdid))
        return 0.0;
    for (unsigned int i = 0; i < ops->sizeof_setup_time_function_array; ++i) {
        if (prev) {
            time += ops->setup_time_functions[i].function(
                &prev->base, &next->base, ops->setup_time_functions[i].minute);
        } else {
            time += ops->setup_time_functions[i].function(
                NULL, &next->base, ops->setup_time_functions[i].minute);
        }
    }
    return time;
}

machine_status::machine_status(job_t &current_job,
                               job_t &prev_job,
                               std::map<std::string, std::string> elements,
                               bool has_lot,
                               machine_base_operations_t *ops)
    : current_job(current_job),
      prev_job(prev_job),
      elements(elements),
      has_lot(has_lot),
      ops(ops)
{
}

setup_machine_status::setup_machine_status(
    job_t &current_job,
    job_t &prev_job,
    std::map<std::string, std::string> elements,
    bool has_lot,
    machine_base_operations_t *ops)
    : machine_status(current_job, prev_job, elements, has_lot, ops)
{
}

qc_machine_status::qc_machine_status(
    job_t &current_job,
    job_t &prev_job,
    std::map<std::string, std::string> elements,
    bool has_lot,
    machine_base_operations_t *ops)
    : machine_status(current_job, prev_job, elements, has_lot, ops)
{
}

wait_setup_machine_status::wait_setup_machine_status(
    job_t &current_job,
    job_t &prev_job,
    std::map<std::string, std::string> elements,
    bool has_lot,
    machine_base_operations_t *ops)
    : machine_status(current_job, prev_job, elements, has_lot, ops)
{
}

eng_machine_status::eng_machine_status(
    job_t &current_job,
    job_t &prev_job,
    std::map<std::string, std::string> elements,
    bool has_lot,
    machine_base_operations_t *ops)
    : machine_status(current_job, prev_job, elements, has_lot, ops)
{
}

stop_machine_status::stop_machine_status(
    job_t &current_job,
    job_t &prev_job,
    std::map<std::string, std::string> elements,
    bool has_lot,
    machine_base_operations_t *ops)
    : machine_status(current_job, prev_job, elements, has_lot, ops)
{
}

wait_repair_machine_status::wait_repair_machine_status(
    job_t &current_job,
    job_t &prev_job,
    std::map<std::string, std::string> elements,
    bool has_lot,
    machine_base_operations_t *ops)
    : machine_status(current_job, prev_job, elements, has_lot, ops)
{
}

in_repair_machine_status::in_repair_machine_status(
    job_t &current_job,
    job_t &prev_job,
    std::map<std::string, std::string> elements,
    bool has_lot,
    machine_base_operations_t *ops)
    : machine_status(current_job, prev_job, elements, has_lot, ops)
{
}

idle_machine_status::idle_machine_status(
    job_t &current_job,
    job_t &prev_job,
    std::map<std::string, std::string> elements,
    bool has_lot,
    machine_base_operations_t *ops)
    : machine_status(current_job, prev_job, elements, has_lot, ops)
{
}

pm_machine_status::pm_machine_status(
    job_t &current_job,
    job_t &prev_job,
    std::map<std::string, std::string> elements,
    bool has_lot,
    machine_base_operations_t *ops)
    : machine_status(current_job, prev_job, elements, has_lot, ops)
{
}

running_machine_status::running_machine_status(
    job_t &current_job,
    job_t &prev_job,
    std::map<std::string, std::string> elements,
    bool has_lot,
    machine_base_operations_t *ops)
    : machine_status(current_job, prev_job, elements, has_lot, ops)
{
}

void machine_status::handle_status(double &_outplan_time,
                                   double &_ptime,
                                   double &_setup_time,
                                   time_t &base_time)
{
}

void setup_machine_status::handle_status(double &_outplan_time,
                                         double &_ptime,
                                         double &_setup_time,
                                         time_t &base_time)
{
    if (elements["recover_time"].length() != 0) {  // has oupplan time
        if (elements["qty"].length() != 0 && elements["uph"].length() != 0) {
            _ptime = stod(elements["qty"]) / stod(elements["uph"]) * 60;
            _outplan_time += _ptime;
        }
    } else {
        _outplan_time = 0;
    }
}

void qc_machine_status::handle_status(double &_outplan_time,
                                      double &_ptime,
                                      double &_setup_time,
                                      time_t &base_time)
{
    if (elements["recover_time"].length() != 0) {
        if (elements["qty"].length() != 0 && elements["uph"].length() != 0) {
            _ptime = stod(elements["qty"]) / stod(elements["uph"]) * 60;
            _outplan_time += _ptime;
        }
    } else {
        _outplan_time = 0 + 2 * 60;
    }
}

void wait_setup_machine_status::handle_status(double &_outplan_time,
                                              double &_ptime,
                                              double &_setup_time,
                                              time_t &base_time)
{
    if (has_lot) {
        if (elements["recover_time"].length() != 0) {
            _ptime = stod(elements["qty"]) / stod(elements["uph"]) * 60;
            if (string(current_job.bdid.data.text)
                    .compare(elements["Last bd id"]) != 0) {
                _setup_time = setupTime(&prev_job, &current_job, ops);
            } else
                _setup_time = 84;  // mc_code
            _outplan_time += (_setup_time + _ptime);
        } else {
            _outplan_time = 0 + 2 * 60;
        }
    }
}

void eng_machine_status::handle_status(double &_outplan_time,
                                       double &_ptime,
                                       double &_setup_time,
                                       time_t &base_time)
{
    if (has_lot) {
        if (elements["recover_time"].length() != 0) {
            _ptime = stod(elements["qty"]) / stod(elements["uph"]) * 60;
            if (string(current_job.bdid.data.text)
                    .compare(elements["Last bd id"]) != 0) {
                _setup_time = setupTime(&prev_job, &current_job, ops);
            } else
                _setup_time = 84;  // mc_code
            _outplan_time += (6 * 60 + _setup_time + _ptime);
        } else {
            _outplan_time = 0 + 6 * 60;
        }
    }
}

void stop_machine_status::handle_status(double &_outplan_time,
                                        double &_ptime,
                                        double &_setup_time,
                                        time_t &base_time)
{
    if (elements["recover_time"].length() == 0)
        throw std::invalid_argument("No outplan time provided\n");
    _ptime = stod(elements["qty"]) / stod(elements["uph"]) * 60;
    _outplan_time =
        (timeConverter()(elements["wip_outplan_time"]) - base_time) / 60.0;
}

void wait_repair_machine_status::handle_status(double &_outplan_time,
                                               double &_ptime,
                                               double &_setup_time,
                                               time_t &base_time)
{
    if (elements["recover_time"].length() == 0)
        throw std::invalid_argument("No outplan time provided\n");
    _ptime = stod(elements["qty"]) / stod(elements["uph"]) * 60;
    _outplan_time =
        (timeConverter()(elements["wip_outplan_time"]) - base_time) / 60.0;
}

void in_repair_machine_status::handle_status(double &_outplan_time,
                                             double &_ptime,
                                             double &_setup_time,
                                             time_t &base_time)
{
    _outplan_time = 0 + 2 * 60;
}

void idle_machine_status::handle_status(double &_outplan_time,
                                        double &_ptime,
                                        double &_setup_time,
                                        time_t &base_time)
{
    _outplan_time = 0;
}

void pm_machine_status::handle_status(double &_outplan_time,
                                      double &_ptime,
                                      double &_setup_time,
                                      time_t &base_time)
{
}

void running_machine_status::handle_status(double &_outplan_time,
                                           double &_ptime,
                                           double &_setup_time,
                                           time_t &base_time)
{
}

void set_start(double &r, double arg1, double arg2)
{
    r = arg1 - arg2;
}

void machine_status::set_start(double &start_time,
                               double _recover_time,
                               double _ptime)
{
}

void setup_machine_status::set_start(double &start_time,
                                     double _recover_time,
                                     double _ptime)
{
    set_start(start_time, _recover_time, _ptime);
}

void qc_machine_status::set_start(double &start_time,
                                  double _recover_time,
                                  double _ptime)
{
    set_start(start_time, _recover_time, _ptime);
}

void wait_setup_machine_status::set_start(double &start_time,
                                          double _recover_time,
                                          double _ptime)
{
    set_start(start_time, _recover_time, _ptime);
}

void eng_machine_status::set_start(double &start_time,
                                   double _recover_time,
                                   double _ptime)
{
    set_start(start_time, _recover_time, _ptime);
}

void stop_machine_status::set_start(double &start_time,
                                    double _recover_time,
                                    double _ptime)
{
    set_start(start_time, _recover_time, _ptime);
}

void wait_repair_machine_status::set_start(double &start_time,
                                           double _recover_time,
                                           double _ptime)
{
    set_start(start_time, _recover_time, _ptime);
}

void machine_status::set_base(double &_outplan_time,
                              double &_recover_time,
                              double &_intime,
                              time_t &base_time)
{
    _outplan_time = _recover_time = base_time;
}

void qc_machine_status::set_base(double &_outplan_time,
                                 double &_recover_time,
                                 double &_intime,
                                 time_t &base_time)
{
    _outplan_time = _recover_time = _intime;
}

void machine_status::wait(double &_outplan_time) {}

void wait_setup_machine_status::wait(double &_outplan_time)
{
    if (elements["lot_number"].length() == 0) {
        has_lot = false;
        elements["lot_number"] = elements["Last Wip Lot"];
        elements["customer"] = elements["Last Cust"];
        elements["pin_package"] = elements["Last Pin Package"];
        elements["bd_id"] = elements["Last bd id"];
        elements["part_id"] = elements["Last Part ID"];
    }
}

void eng_machine_status::wait(double &_outplan_time)
{
    if (elements["lot_number"].length() == 0) {
        has_lot = false;
        elements["lot_number"] = elements["Last Wip Lot"];
        elements["customer"] = elements["Last Cust"];
        elements["pin_package"] = elements["Last Pin Package"];
        elements["bd_id"] = elements["Last bd id"];
        elements["part_id"] = elements["Last Part ID"];

        _outplan_time += 6 * 60;
    }
}

entity_t::entity_t()
{
    _intime = _outplan_time = _recover_time = 0;
}

entity_t::entity_t(map<string, string> elements,
                   machine_base_operations_t *ops,
                   time_t base_time)
{
    _current_lot = nullptr;
    _entity_name = elements["entity"];
    _model_name = elements["model"];
    _location = elements["location"];
    _ent_weight = stod(elements["ent_weight"]);
    _setup_time = 0;
    _ptime = 0;

    if (elements["in_time"].length() != 0)
        _intime = timeConverter()(elements["in_time"]);
    else
        _intime = base_time;

    if (elements["recover_time"].length() != 0)
        _outplan_time = _recover_time =
            timeConverter()(elements["recover_time"]);

    _status->set_base(_outplan_time, _recover_time, _intime, base_time);
    setBaseTime(base_time);

    bool has_lot = true;
    /*
    if ((_status == WAIT_SETUP || _status == ENG) &&
        elements["lot_number"].length() == 0) {
        has_lot = false;
        elements["lot_number"] = elements["Last Wip Lot"];
        elements["customer"] = elements["Last Cust"];
        elements["pin_package"] = elements["Last Pin Package"];
        elements["bd_id"] = elements["Last bd id"];
        elements["part_id"] = elements["Last Part ID"];
        if (_status == ENG)
            _outplan_time += 6 * 60;
    }
    */

    try {
        _current_lot = new lot_t(elements);
    } catch (invalid_argument &e) {
        throw invalid_argument(
            "throw up invalid_argument exception when creating on-machine "
            "lot_t instance");
    }
    _current_lot->setCanRunModel(_model_name);
    try {
        _current_lot->setUph(_model_name, stod(elements["uph"]));
    } catch (std::invalid_argument &e) {
    }

    if (_current_lot == nullptr) {
        perror("new current_lot error");
        exit(EXIT_FAILURE);
    }

    job_t current_job = _current_lot->job();
    current_job.base.ptr_derived_object = &current_job;
    job_t prev_job = {
        .pin_package = stringToInfo(elements["Last Pin Package"]),
        .customer = stringToInfo(elements["Last Cust"]),
        .part_id = stringToInfo(elements["Last Part ID"]),
        .bdid = stringToInfo(elements["Last bd id"]),
    };
    prev_job.base.ptr_derived_object = &prev_job;

    if (elements["STATUS"].compare("SETUP") == 0)
        _status = new setup_machine_status(current_job, prev_job, elements,
                                           has_lot, ops);
    else if (elements["STATUS"].compare("QC") == 0)
        _status = new qc_machine_status(current_job, prev_job, elements,
                                        has_lot, ops);
    else if (elements["STATUS"].compare("WAIT-SETUP") == 0)
        _status = new wait_setup_machine_status(current_job, prev_job, elements,
                                                has_lot, ops);
    else if (elements["STATUS"].compare("ENG") == 0)
        _status = new eng_machine_status(current_job, prev_job, elements,
                                         has_lot, ops);
    else if (elements["STATUS"].compare("STOP") == 0)
        _status = new stop_machine_status(current_job, prev_job, elements,
                                          has_lot, ops);
    else if (elements["STATUS"].compare("WAIT-REPAIR") == 0)
        _status = new wait_repair_machine_status(current_job, prev_job,
                                                 elements, has_lot, ops);
    else if (elements["STATUS"].compare("IN-REPAIR") == 0)
        _status = new in_repair_machine_status(current_job, prev_job, elements,
                                               has_lot, ops);
    else if (elements["STATUS"].compare("IDLE") == 0)
        _status = new idle_machine_status(current_job, prev_job, elements,
                                          has_lot, ops);
    else if (elements["STATUS"].compare("PM") == 0)
        _status = new pm_machine_status(current_job, prev_job, elements,
                                        has_lot, ops);
    else if (elements["STATUS"].compare("RUNNING") == 0)
        _status = new running_machine_status(current_job, prev_job, elements,
                                             has_lot, ops);

    _status->handle_status(_outplan_time, _ptime, _setup_time, base_time);
    _recover_time = _outplan_time;

    string lot_number = elements["lot_number"];

    // if (elements["qty"].length()) {
    //     if (_outplan_time <= 0) {
    //         elements["qty"] = to_string(0);
    //     }
    // }
}

void entity_t::setBaseTime(time_t base_time)
{
    double tmp_time = _recover_time - base_time;
    _recover_time = tmp_time / 60;

    _outplan_time = _recover_time;

    _intime = (_intime - base_time) / 60;
}

machine_t entity_t::machine()
{
    machine_t machine =
        machine_t{.base = {.machine_no = stringToInfo(_entity_name),
                           .size_of_jobs = 0,
                           .available_time = _recover_time},
                  .model_name = stringToInfo(_model_name),
                  .location = stringToInfo(_location),
                  .current_job = _current_lot->job(),
                  .makespan = 0,
                  .total_completion_time = 0,
                  .quality = 0,
                  .setup_times = 0,
                  .setup_weight = _ent_weight,
                  .ptr_derived_object = nullptr};

    machine.current_job.base.end_time = _recover_time;
    machine.current_job.base.start_time = _intime;
    machine.current_job.base.machine_no = stringToInfo(_entity_name);

    _status->set_start(machine.current_job.base.start_time, _recover_time,
                       _ptime);

    return machine;
}
