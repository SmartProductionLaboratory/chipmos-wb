
#ifndef CHIPMOSWB_ENTITIES_H
#define CHIPMOSWB_ENTITIES_H

#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

#include "include/csv.h"
#include "include/entity.h"
#include "include/machine_base.h"
#include "include/machine_constraint_a.h"
#include "include/machine_constraint_r.h"

class entities_t
{
private:
    std::vector<entity_t *> _ents;

    // _entities[MODEL][AREA] is a vector of entity_t object.
    std::map<std::string, std::map<std::string, std::vector<entity_t *> > >
        _entities;

    std::map<std::string, std::vector<std::string> > _model_locations;

    std::map<std::string, std::vector<entity_t *> > _loc_ents;

    std::vector<std::map<std::string, std::string> > _faulty_machine;

    std::map<std::string, entity_t *> name_entity;

    std::map<std::string, std::string> prod_map_to_pid;
    std::map<std::string, std::string> prod_map_to_bom_id;
    std::map<std::string, std::string> bom_id_map_to_part_id;
    std::map<std::string, std::string> pid_map_to_part_no;

    std::map<std::string, std::map<std::string, bool> > _dedicate_machines;

    // std::vector<machine_constraint_t *> _machine_constraints;
    machine_constraint_t *_mcs_a;
    machine_constraint_t *_mcs_r;

    time_t _time;

    void _readProcessIdFile(std::string filename);

    void _readPartNoFile(std::string filename);

    void _readPartIdFile(std::string filename);

    void _readDedicateMachines(std::string filename);

    void _setupMachineConstraints(std::string filename);

public:
    entities_t();

    /**
     * entities_t () - constructor of entities_t
     *
     * The constructor will convert @b _time to time_t type
     */
    entities_t(std::map<std::string, std::string> arguments);

    /**
     * addMachine() - add a machine
     *
     * @b elements is a std::map container which store the relationship between
     * header and data. For example, elements[ENTITY] == "BB211",
     * elements[MODEL] = "UTC3000" and etc... This function will convert
     * elements to entity_t object.
     */
    entity_t *addMachine(std::map<std::string, std::string> elements,
                         machine_base_operations_t *ops);

    /**
     * addMachines() - add machines from dataframe
     *
     * add machines from @csv_t type dataframe.
     */
    void addMachines(csv_t machines_csv,
                     csv_t location_csv,
                     machine_base_operations_t *ops);


    std::vector<entity_t *> allEntities();

    void setTime(std::string text);

    entity_t *getEntityByName(std::string entity_name);

    std::map<std::string, std::map<std::string, bool> > getDedicateMachines();

    inline std::vector<std::map<std::string, std::string> > getFaultyEntities();

    // inline std::vector<machine_constraint_t *> getMachineConstraints();
    machine_constraint_t *getMachineConstraintA();
    machine_constraint_t *getMachineConstraintR();
};

inline machine_constraint_t *entities_t::getMachineConstraintA()
{
    return _mcs_a;
}

inline machine_constraint_t *entities_t::getMachineConstraintR()
{
    return _mcs_r;
}

inline std::map<std::string, std::map<std::string, bool> >
entities_t::getDedicateMachines()
{
    return _dedicate_machines;
}

inline entity_t *entities_t::getEntityByName(std::string entity_name)
{
    entity_t *ent = nullptr;
    try {
        ent = name_entity.at(entity_name);
    } catch (std::out_of_range &e) {
        ent = nullptr;
    }
    return ent;
}

inline std::vector<entity_t *> entities_t::allEntities()
{
    return _ents;
}

inline std::vector<std::map<std::string, std::string> >
entities_t::getFaultyEntities()
{
    return _faulty_machine;
}


#endif  // CHIPMOSWB_ENTITIES_H
