#ifndef __ENTITY_H__
#define __ENTITY_H__

#include <stdio.h>
#include <stdlib.h>
#include <ctime>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

#include "include/csv.h"
#include "include/lot.h"
#include "include/machine.h"

class machine_status
{
protected:
    bool has_lot;
    job_t &current_job, &prev_job;
    std::map<std::string, std::string> elements;
    machine_base_operations_t *ops;

public:
    machine_status(job_t &current_job,
                   job_t &prev_job,
                   std::map<std::string, std::string> elements,
                   bool has_lot,
                   machine_base_operations_t *ops);
    void handle_status(double &_outplan_time,
                       double &_ptime,
                       double &_setup_time,
                       time_t &base_time);
    void set_start(double &start_time, double _recover_time, double _ptime);
    void set_base(double &_outplan_time,
                  double &_recover_time,
                  double &_intime,
                  time_t &base_time);
    void wait(double &_outplan_time);
};

class setup_machine_status : public machine_status
{
public:
    setup_machine_status(job_t &current_job,
                         job_t &prev_job,
                         std::map<std::string, std::string> elements,
                         bool has_lot,
                         machine_base_operations_t *ops);
    void handle_status(double &_outplan_time,
                       double &_ptime,
                       double &_setup_time,
                       time_t &base_time);
    void set_start(double &start_time, double _recover_time, double _ptime);
};

class qc_machine_status : public machine_status
{
public:
    qc_machine_status(job_t &current_job,
                      job_t &prev_job,
                      std::map<std::string, std::string> elements,
                      bool has_lot,
                      machine_base_operations_t *ops);
    void handle_status(double &_outplan_time,
                       double &p_time,
                       double &_setup_time,
                       time_t &base_time);
    void set_start(double &start_time, double _recover_time, double _ptime);
    void set_base(double &_outplan_time,
                  double &_recover_time,
                  double &_intime,
                  time_t &base_time);
};

class wait_setup_machine_status : public machine_status
{
public:
    wait_setup_machine_status(job_t &current_job,
                              job_t &prev_job,
                              std::map<std::string, std::string> elements,
                              bool has_lot,
                              machine_base_operations_t *ops);
    void handle_status(double &_outplan_time,
                       double &p_time,
                       double &_setup_time,
                       time_t &base_time);
    void set_start(double &start_time, double _recover_time, double _ptime);
    void wait(double &_outplan_time);
};

class eng_machine_status : public machine_status
{
public:
    eng_machine_status(job_t &current_job,
                       job_t &prev_job,
                       std::map<std::string, std::string> elements,
                       bool has_lot,
                       machine_base_operations_t *ops);
    void handle_status(double &_outplan_time,
                       double &p_time,
                       double &_setup_time,
                       time_t &base_time);
    void set_start(double &start_time, double _recover_time, double _ptime);
    void wait(double &_outplan_time);
};

class stop_machine_status : public machine_status
{
public:
    stop_machine_status(job_t &current_job,
                        job_t &prev_job,
                        std::map<std::string, std::string> elements,
                        bool has_lot,
                        machine_base_operations_t *ops);
    void handle_status(double &_outplan_time,
                       double &p_time,
                       double &_setup_time,
                       time_t &base_time);
    void set_start(double &start_time, double _recover_time, double _ptime);
};

class wait_repair_machine_status : public machine_status
{
public:
    wait_repair_machine_status(job_t &current_job,
                               job_t &prev_job,
                               std::map<std::string, std::string> elements,
                               bool has_lot,
                               machine_base_operations_t *ops);
    void handle_status(double &_outplan_time,
                       double &p_time,
                       double &_setup_time,
                       time_t &base_time);
    void set_start(double &start_time, double _recover_time, double _ptime);
};

class in_repair_machine_status : public machine_status
{
public:
    in_repair_machine_status(job_t &current_job,
                             job_t &prev_job,
                             std::map<std::string, std::string> elements,
                             bool has_lot,
                             machine_base_operations_t *ops);
    void handle_status(double &_outplan_time,
                       double &p_time,
                       double &_setup_time,
                       time_t &base_time);
    void set_start(double &start_time, double _recover_time, double _ptime);
};

class idle_machine_status : public machine_status
{
public:
    idle_machine_status(job_t &current_job,
                        job_t &prev_job,
                        std::map<std::string, std::string> elements,
                        bool has_lot,
                        machine_base_operations_t *ops);
    void handle_status(double &_outplan_time,
                       double &p_time,
                       double &_setup_time,
                       time_t &base_time);
    void set_start(double &start_time, double _recover_time, double _ptime);
};

class pm_machine_status : public machine_status
{
public:
    pm_machine_status(job_t &current_job,
                      job_t &prev_job,
                      std::map<std::string, std::string> elements,
                      bool has_lot,
                      machine_base_operations_t *ops);
    void handle_status(double &_outplan_time,
                       double &p_time,
                       double &_setup_time,
                       time_t &base_time);
    void set_start(double &start_time, double _recover_time, double _ptime);
};

class running_machine_status : public machine_status
{
public:
    running_machine_status(job_t &current_job,
                           job_t &prev_job,
                           std::map<std::string, std::string> elements,
                           bool has_lot,
                           machine_base_operations_t *ops);
    void handle_status(double &_outplan_time,
                       double &p_time,
                       double &_setup_time,
                       time_t &base_time);
    void set_start(double &start_time, double _recover_time, double _ptime);
};

class entity_t
{
private:
    machine_status *_status;

    double _recover_time;
    double _outplan_time;
    double _intime;
    double _setup_time;
    double _ptime;
    double _ent_weight;

    std::string _entity_name;
    std::string _model_name;
    std::string _location;

    lot_t *_current_lot;

    std::vector<lot_t *> _prescheduled_lots;

public:
    entity_t();

    entity_t(std::map<std::string, std::string> elements,
             machine_base_operations_t *ops,
             time_t base_time = 0);

    double getRecoverTime() const;
    double getOutplanTime() const;
    std::string getEntityName();
    std::string getRecipe();

    void setBaseTime(time_t time);

    virtual machine_t machine();

    bool hold;
};

inline double entity_t::getRecoverTime() const
{
    return _recover_time;
}

inline double entity_t::getOutplanTime() const
{
    return _outplan_time;
}

inline std::string entity_t::getEntityName()
{
    return _entity_name;
}

inline std::string entity_t::getRecipe()
{
    return _current_lot->recipe();
}


#endif
