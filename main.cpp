#include <ctime>
#include <exception>
#include <iostream>
#include <iterator>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <system_error>

#include <include/arrival.h>
#include <include/infra.h>
#include <include/condition_card.h>
#include <include/csv.h>
#include <include/da.h>
#include <include/lot.h>
#include <include/machine.h>
#include <include/route.h>


using namespace std;



int main(int argc, const char *argv[])
{
    lots_t lots;
    lots.addLots(createLots("WipOutPlanTime_.csv", "product_find_process_id.csv",
                   "process_find_lot_size_and_entity.csv", "fcst.csv",
                   "routelist.csv", "newqueue_time.csv",
                   "BOM List_20210521.csv", "Process find heatblock.csv",
                   "EMS Heatblock data.csv", "GW Inventory.csv"));
    
    // csv_t out("out.csv", "w");
    // iter(lots, i) { out.addData(lots[i].data()); }
    // out.write();

    csv_t machine_csv("machines.csv", "r", true, true);
    machine_csv.trim(" ");
    machine_csv.setHeaders(map<string, string>({{"entity", "ENTITY"},
                                                {"model", "MODEL"},
                                                {"recover_time", "OUTPLAN"}}));

    csv_t location_csv("locations.csv", "r", true, true);
    location_csv.trim(" ");
    location_csv.setHeaders(
        map<string, string>({{"entity", "Entity"}, {"location", "Location"}}));


    char *text = strdup("2020/12/19 10:50");
    machines_t machines(text);
    machines.addMachines(machine_csv, location_csv);
    
    vector<lot_group_t> group = lots.round(machines);
    vector<job_t> jobs;
    vector<vector<string> > can_run_entities;
    iter(group, i){
        iter(group[i].lots, j){
            jobs.push_back(group[i].lots[j]->job());
            can_run_entities.push_back(group[i].lots[j]->getCanRunEntities());
        }
    }
    

    return 0;
}
