#include <algorithm>
#include <array>
#include <cstring>
#include <ctime>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include <fcntl.h>
#include <sys/stat.h>

/* Directory needed : output_X-X */
/* Files needed in same directory */
#include "include/csv.h"
#include "include/time_converter.h"

#define DIR_OUTPUT "result"

/* Files needed in same directory */
//#define PATH_MACHINE "machines.csv"
#define PATH_CONFIG "config.csv"
#define PATH_RECORD_GAP "record_gap.csv"
#define PATH_NCKU_RESULT "result.csv"

#define PATH_OUTPUT "indicator.csv"

#define BASE_TIME "07:30"
#define START_DATE "22-02-01"
#define END_DATE "22-02-02"

#define MAX(x, y) ((x) ^ (((x) ^ (y)) & -((x) < (y))))

#define MIN(x, y) ((x) ^ (((x) ^ (y)) & -((x) > (y))))

/*
 * Refactor
 * 1. encapsulate same info
 * 2. repeated code in function
 */

static inline long fconvert_time(long time, long bias)
{
    return (time * 60) - bias;
}

typedef struct assign_info_args {
    csv_t data;
    std::vector<time_t> rtime;
    int dir;
} raw_info_t;

class base_info_t
{
public:
    base_info_t() : start(0){};
    base_info_t(std::string _entity, const time_t _start)
        : entity(_entity), start(_start){};
    std::string entity;
    const time_t start;

    virtual void assign_info_vec(std::vector<base_info_t *> &info_vec,
                                 raw_info_t raw_info) = 0;
    void assign_info_map(
        std::map<std::string, std::vector<base_info_t *>> &info,
        std::vector<base_info_t *> info_vec);
    std::map<std::string, std::vector<base_info_t *>> init_map(
        raw_info_t raw_info);
};

std::map<std::string, std::vector<base_info_t *>> base_info_t::init_map(
    raw_info_t raw_info)
{
    /* Assume start_time, end_time currently in minutes */
    std::vector<base_info_t *> info_vec;
    assign_info_vec(info_vec, raw_info);

    /* Insert entity information */
    std::map<std::string, std::vector<base_info_t *>> info;
    assign_info_map(info, info_vec);
    return info;
}

void base_info_t::assign_info_map(
    std::map<std::string, std::vector<base_info_t *>> &info,
    std::vector<base_info_t *> info_vec)
{
    std::vector<base_info_t *> temp;
    for (auto &cur : info_vec) {
        if (info.find(cur->entity) == info.end())
            info.insert(std::pair<std::string, std::vector<base_info_t *>>(
                cur->entity, temp));
        info[cur->entity].push_back(cur);
    }
}

class wlth_info_t : public base_info_t
{
public:
    wlth_info_t() : base_info_t(), qty(0), end(0){};
    wlth_info_t(std::map<std::string, std::string> const &elements, long rtime)
        : base_info_t(elements.at("entity"),
                      fconvert_time(std::stoll(elements.at("start")), rtime)),
          qty(std::stoi(elements.at("qty"))),
          lot(elements.at("lot")),
          end(fconvert_time(std::stoll(elements.at("end")), rtime)){};

    const int qty;
    std::string lot;
    const time_t end;

    void assign_info_vec(std::vector<base_info_t *> &info_vec,
                         raw_info_t raw_info);
};

class setup_info_t : public base_info_t
{
public:
    setup_info_t() : base_info_t(){};
    setup_info_t(std::map<std::string, std::string> const &elements, long rtime)
        : base_info_t(elements.at("entity"),
                      fconvert_time(std::stoll(elements.at("start")), rtime)){};
    std::string entity;

    void assign_info_vec(std::vector<base_info_t *> &info_vec,
                         raw_info_t raw_info);
};

void wlth_info_t::assign_info_vec(std::vector<base_info_t *> &info_vec,
                                  raw_info_t raw_info)
{
    for (int i = 0, size = raw_info.data.nrows(); i < size; ++i)
        info_vec.push_back(new wlth_info_t(raw_info.data.getElements(i),
                                           raw_info.rtime.at(raw_info.dir)));
}

void setup_info_t::assign_info_vec(std::vector<base_info_t *> &info_vec,
                                   raw_info_t raw_info)
{
    for (int i = 0, size = raw_info.data.nrows(); i < size; ++i)
        info_vec.push_back(new setup_info_t(raw_info.data.getElements(i),
                                            raw_info.rtime.at(raw_info.dir)));
}


int main()
{
    // mkdir(DIR_OUTPUT, 0777);
    /* Get relative time */
    csv_t conf(PATH_CONFIG, "r", true, true);
    conf.dropNullRow();
    conf.trim(" ");
    conf.setHeaders(std::map<std::string, std::string>({
        {"base_time", "std_time"},
    }));

    std::vector<time_t> rtime;
    for (int i = 0, size = conf.nrows(); i < size; ++i) {
        std::string rtimes = conf.getElements(i)["base_time"];
        rtimes.replace(rtimes.find(" ") + 1, 5, BASE_TIME);
        rtime.push_back(
            timeConverter(conf.getElements(i)["base_time"])(rtimes));
    }

    std::ofstream outfile(PATH_OUTPUT);
    outfile << "No,Utilization,Outplan,Setup" << std::endl;
    std::cout << "No,Utilization,Outplan,Setup" << std::endl;
    /* Read result.csv */
    for (int dir = 0, size = conf.nrows(); dir < size; ++dir) {
        csv_t data(
            "output_" + conf.getElements(dir)["no"] + "/" + PATH_NCKU_RESULT,
            "r", true, true);
        data.dropNullRow();
        data.trim(" ");
        data.setHeaders(
            std::map<std::string, std::string>({{"lot", "lot_number"},
                                                {"start", "start_time"},
                                                {"end", "end_time"}}));

        wlth_info_t *wlth_dummy = new wlth_info_t();
        std::map<std::string, std::vector<base_info_t *>> info =
            wlth_dummy->init_map({data, rtime, dir});

        /* output per simulation */
        time_t base_start = 0, base_end = timeConverter(START_DATE)(END_DATE);
        int gqty = 0;
        time_t gtotal = 0;
        for (auto &cur : info) {
            int lqty = 0;
            time_t total = 0;

            for (auto &it : cur.second) {
                wlth_info_t *it2 = static_cast<wlth_info_t *>(it);
                if (it2->end < base_start || it2->start > base_end)
                    continue;
                total += MIN(it2->end, base_end) - MAX(it2->start, base_start);
                if (it2->end <= base_end)
                    lqty += it2->qty;
            }

            gtotal += total;
            gqty += lqty;
        }

        int nmachine = 0;
        std::string line;
        std::string PATH_MACHINE = conf.getElements(dir)["machines"];
        std::ifstream infile(PATH_MACHINE);
        while (std::getline(infile, line)) {
            if (line.compare(0, 2, "BB") == 0)
                ++nmachine;
        }

        int gitotal =
            (static_cast<int>((long double) (gtotal) /
                              ((long double) (nmachine * base_end)) * 100000));
        /* output no, utility, quantity */
        outfile << conf.getElements(dir)["no"] << "," << gitotal / 1000 << "."
                << (((gitotal + 5) % 1000)) / 10 << "%,"
                << ((gqty + 50) / 100) / 10 << "." << ((gqty + 50) / 100) % 10
                << "K";

        std::cout << conf.getElements(dir)["no"] << "," << gitotal / 1000 << "."
                  << (((gitotal + 5) % 1000)) / 10 << "%,"
                  << ((gqty + 50) / 100) / 10 << "." << ((gqty + 50) / 100) % 10
                  << "K";

        // TODO : repeated code
        /* Read record_gap.csv */
        csv_t setup(
            "output_" + conf.getElements(dir)["no"] + "/" + PATH_RECORD_GAP,
            "r", true, true);
        setup.dropNullRow();
        setup.trim(" ");
        setup.setHeaders(
            std::map<std::string, std::string>({{"start", "start_time"}}));

        setup_info_t *setup_dummy = new setup_info_t();
        std::map<std::string, std::vector<base_info_t *>> sinfo =
            setup_dummy->init_map({setup, rtime, dir});

        std::ofstream outfile_setup("result/setup_" +
                                    conf.getElements(dir)["no"] + ".csv");

        /* output setup */
        int gsetup = 0;
        for (auto &cur : sinfo) {
            int lsetup = 0;
            for (auto &it : cur.second) {
                if (it->start < base_start || it->start > base_end)
                    continue;
                ++lsetup;
            }
            outfile_setup << cur.first << "," << lsetup << std::endl;
            gsetup += lsetup;
        }

        outfile << "," << gsetup << std::endl;
        std::cout << "," << gsetup << std::endl;
    }

    return 0;
}
