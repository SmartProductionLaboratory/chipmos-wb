#ifndef __ROUTE_H__
#define __ROUTE_H__

#include <map>
#include <set>
#include <string>
#include <vector>


#include <include/csv.h>
#include <include/lot.h>

#define WB1 2200
#define WB2 3200
#define WB3 3400
#define WB4 3600

#define DA1 2070
#define DA2 2130
#define DA3 3130
#define DA4 3330
#define DA5 4130
#define DA6 4330
#define DA7 4530
#define DA8 4730
#define DA9 5130
#define DA10 5330
#define DA11 5530
#define DA12 5730
#define DA13 6130
#define DA14 6330
#define DA15 6530


#define CURE 2080


typedef struct station_t station_t;
struct station_t {
    std::string route_name;
    std::string station_name;
    int oper;
    int seq;
};

class route_t
{
private:
    /// route_name map to its array of stations
    std::map<std::string, std::vector<station_t> > _routes;

    std::map<std::string, std::set<int> > _beforeWB;

    /// route_name map to station map to the queue_time
    std::map<int, std::map<int, int> > _queue_time;

    // std::set use R-B tree, which provides optimal time complexity on
    // searching data.
    std::set<int> _wb_stations;  // store the opers of W/B stations.
    std::set<int> _da_stations;  // store the opers of D/A stations.


private:
    /**
     * setupBeforeStation() - set up the number of  stations(nstations) before
     * ...(stations list).
     *
     * setupBeforeStation is used to setup the @b nstations stations before
     * given stations list. The scheduling plan of W/B only considers the
     * station in the range, [W/B - nstations, W/B]. The opers of route which
     * are in the range will be pushed in @b _beforWB container. _beforeWB is a
     * containers which maps the route name to its the opers, which are in [WB -
     * nstations, WB] range.
     *
     * @param routename : route name
     * @param remove : if remove is true, remove the station which isn't is the
     * range.
     * @param nstations : range from the specified station
     * @param nopts : number of optional station
     * @param ... : the optional station
     */
    std::vector<station_t> setupBeforeStation(std::string routename,
                                              bool remove,
                                              int nstations,
                                              int nopts,
                                              ...);

    /**
     * findStationIdx () - locate the oper in the route
     *
     * @param routename : route name
     * @param oper : operation
     */
    int findStationIdx(std::string routename, int oper);

public:
    route_t();

    void setRoute(csv_t all_routes);

    /**
     * setRoute() - set routes from dataframe
     *
     * @b dataframe is a csv_t type that store stations of @b routename. The
     * headers of dataframe must have these headers, "route", "desc", "oper",
     * "seq" for initializing station_t object. @b route is for the route name
     * of dataframe. @b desc is description of the station. For example : A31,
     * D/A, W/B and etc... @b oper is the operation number of the station, For
     * example : D/A -> 2070, W/B -> 2200. @b seq is the sequence number of
     * station. setRoute will convert all entries of dataframe to be station_t
     * objects.
     *
     * setRoute () will remove the stations which are not in the scheduling
     * range, [W/B, W/B - 7].
     *
     * @param routename : route name
     * @param dataframe : the datathat contains the stations and their
     * information of the route.
     *
     */
    void setRoute(std::string routename, csv_t dataframe);

    /**
     * setQueueTime () - set queue time dataframe
     *
     * set queue time to be 2-D a mapping container _queue_time. _queue_time is
     * a std::map container and the mapping relationship is [START_OPER,
     * END_OPER] map to queue time
     *
     * @param queue_time_df : csv_t type dataframe
     */
    void setQueueTime(csv_t queue_time_df);

    /**
     * isLotInStations () - check if lot is in the stations range[WB - 7, WB]
     */
    bool isLotInStations(lot_t lot);

    /**
     * calculateQueueTime() - sum the queue time of lot
     *
     * The function is going to dealing with queue time issue. The lot will
     * traverse all of stations in the route until the route is in W/B, and also
     * sum the queue time of each station. If the lot traverse to D/A, the
     * traversal routine pauses and return the flag. The condition in the
     * traversal is as follow:
     * 1. If the lot is originally in W/B
     *  1.1 if lot originally has moved in : start traversing from next station
     *      until traversing to next W/B station.
     *  1.2 if lot hasn't moved in : the traversal is finished, return 0.
     * 2. If the lot is in D/A:
     *  2.1 if lot has moved in : start traversing from next station until
     *      traversing to next W/B station
     *  2.2 if lot hasn't moved in : the traversal pauses and lot is
     *      going to be dispatched. In this situation, return 2.
     * 3. If lot is not in W/B either in D/A, the traversal starts from the
     *    station where lot is.
     *
     * Traversal routine is as follow:
     * 1. locate the station index where the lot is
     * 2. start from the station in 1., if station is "BIG" station, sum the
     * queue time.
     * 3. if stations is CURE, record the times of passing CURE station.
     * 4. if station is D/A, lot.tmp_oper move to next station. return 1.
     * 5. if station is W/B, the traversal is finished, and return 0.
     *
     * @return : return flag, the meaning of flag is as follow:
     *          0 : finished -> lot is in WB
     *          1 : dispatched -> lot traverses to D/A
     *          2 : dispatched -> lot is waiting on D/A
     *          -1 : error
     */
    int calculateQueueTime(lot_t &lot);
};

#endif
