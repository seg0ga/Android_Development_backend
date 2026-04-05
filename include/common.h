#ifndef COMMON_H
#define COMMON_H

#include <vector>
#include <string>
#include <map>
#include <mutex>

struct CellTowerData{
    std::string type;
    int mcc=0;int mnc=0;
    int pci=0;int tac=0;
    int timing_advance=0;int band=0;
    int cell_identity=0;int earfcn=0;
    int asu_level=0;int cqi=0;
    int rsrp=0;int rsrq=0;
    int rssi=0;int rssnr=0;
    int bsic=0;int arfcn=0;
    int lac=0;int dbm=0;
    std::string nci;
    int nrarfcn=0;int ss_rsrp=0;
    int ss_rsrq=0;int ss_sinr=0;
    long timing_advance_micros=0;};

struct TrafficData{
    long long total_rx=0;
    long long total_tx=0;
    long long total=0;};

struct LocationData{
    float latitude=0.0f;
    float longitude=0.0f;
    float altitude=0.0f;
    float accuracy=0.0f;
    std::string time="No data";
    long long time_milliseconds=0;
    TrafficData traffic;
    std::vector<CellTowerData> cellTowers;
    std::mutex mutex;};

struct LocationHistory {
    std::vector<float>latitudes;
    std::vector<float>longitudes;
    std::vector<float>altitudes;
    std::vector<float>accuracies;
    std::vector<std::string> times;
    std::vector<long long> time_milliseconds;
    std::vector<TrafficData> trafficHistory;
    std::vector<std::vector<CellTowerData>> cellTowersHistory;
    std::mutex mutex;};

struct SignalHistory{
    std::vector<double>timestamps;
    std::map<int,std::vector<double>> rsrp_values;
    std::map<int,std::vector<double>> rsrq_values;
    std::map<int,std::vector<double>> rssi_values;
    std::map<int,std::vector<double>> rssnr_values;
    std::map<int,std::vector<double>> ss_rsrp_values;
    std::map<int,std::vector<double>> ss_rsrq_values;
    std::map<int,std::vector<double>> ss_sinr_values;
    std::map<int,std::vector<double>> dbm_values;
    std::mutex mutex;};

extern LocationData g_locationData;
extern LocationHistory g_locationHistory;
extern SignalHistory g_signalHistory;
extern int g_measurementCounter;
extern const int INT32_MAX_VAL;

#endif
