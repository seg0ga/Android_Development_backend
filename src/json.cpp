#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include "json.h"

using json = nlohmann::json;

CellTowerData parseCellTower(const json& cellJson){
    CellTowerData cell;
    cell.type=cellJson.value("type","Unknown");

    auto getIntValue=[&](const std::string& key,int defaultVal=0)->int{
        auto it=cellJson.find(key);
        if (it==cellJson.end()) return defaultVal;
        if (it->is_number_integer()) return it->get<int>();
        if (it->is_string()){
            try{return std::stoi(it->get<std::string>());}
            catch(...){return defaultVal;}}
        if (it->is_number()) return static_cast<int>(it->get<double>());
        return defaultVal;};

    auto getLongValue=[&](const std::string& key,long long defaultVal=0LL)->long long{
        auto it=cellJson.find(key);
        if (it==cellJson.end()) return defaultVal;
        if (it->is_number_integer()) return it->get<long long>();
        if (it->is_string()){
            try{return std::stoll(it->get<std::string>());}
            catch(...){return defaultVal;}}
        if (it->is_number()) return static_cast<long long>(it->get<double>());
        return defaultVal;};

    cell.mcc=getIntValue("mcc",0);
    cell.mnc=getIntValue("mnc",0);
    cell.pci=getIntValue("pci",0);
    cell.tac=getIntValue("tac",0);
    cell.timing_advance=getIntValue("timing_advance",0);

    if (cell.type=="LTE"){
        cell.band=getIntValue("band",0);
        cell.cell_identity=getIntValue("cell_identity",0);
        cell.earfcn=getIntValue("earfcn",0);
        cell.asu_level=getIntValue("asu_level",0);
        cell.cqi=getIntValue("cqi",0);
        cell.rsrp=getIntValue("rsrp",0);
        cell.rsrq=getIntValue("rsrq",0);
        cell.rssi=getIntValue("rssi",0);
        cell.rssnr=getIntValue("rssnr",0);}
    else if (cell.type=="GSM"){
        cell.cell_identity=getIntValue("cell_identity",0);
        cell.bsic=getIntValue("bsic",0);
        cell.arfcn=getIntValue("arfcn",0);
        cell.lac=getIntValue("lac",0);
        cell.dbm=getIntValue("dbm",0);}
    else if (cell.type=="NR"){
        cell.nci=cellJson.value("nci","");
        cell.nrarfcn=getIntValue("nrarfcn",0);
        cell.ss_rsrp=getIntValue("ss_rsrp",0);
        cell.ss_rsrq=getIntValue("ss_rsrq",0);
        cell.ss_sinr=getIntValue("ss_sinr",0);
        cell.timing_advance_micros=getLongValue("timing_advance_micros",0L);}
    return cell;}

void saveToJsonFile(const LocationData& data,int counter){
    try {
        json j;
        j["counter"]=counter;
        j["latitude"]=data.latitude;
        j["longitude"]=data.longitude;
        j["altitude"]=data.altitude;
        j["accuracy"]=data.accuracy;
        j["time"]=data.time;
        j["current_time"]=data.time_milliseconds;

        json traffic;
        traffic["total_rx"]=data.traffic.total_rx;
        traffic["total_tx"]=data.traffic.total_tx;
        traffic["total"]=data.traffic.total;
        j["traffic"]=traffic;

        json cells=json::array();
        for (const auto& cell:data.cellTowers){
            json cellJson;
            cellJson["type"]=cell.type;
            cellJson["mcc"]=cell.mcc;
            cellJson["mnc"]=cell.mnc;
            cellJson["pci"]=cell.pci;
            cellJson["tac"]=cell.tac;
            cellJson["timing_advance"]=cell.timing_advance;

            if (cell.type=="LTE"){
                cellJson["band"]=cell.band;
                cellJson["cell_identity"]=cell.cell_identity;
                cellJson["earfcn"]=cell.earfcn;
                cellJson["asu_level"]=cell.asu_level;
                cellJson["cqi"]=cell.cqi;
                cellJson["rsrp"]=cell.rsrp;
                cellJson["rsrq"]=cell.rsrq;
                cellJson["rssi"]=cell.rssi;
                cellJson["rssnr"]=cell.rssnr;}

            else if (cell.type=="GSM"){
                cellJson["bsic"]=cell.bsic;
                cellJson["arfcn"]=cell.arfcn;
                cellJson["lac"]=cell.lac;
                cellJson["dbm"]=cell.dbm;}

            else if (cell.type=="NR"){
                cellJson["nci"]=cell.nci;
                cellJson["nrarfcn"]=cell.nrarfcn;
                cellJson["ss_rsrp"]=cell.ss_rsrp;
                cellJson["ss_rsrq"]=cell.ss_rsrq;
                cellJson["ss_sinr"]=cell.ss_sinr;
                cellJson["timing_advance_micros"]=cell.timing_advance_micros;}

            cells.push_back(cellJson);}
        j["cells"]=cells;

        const std::string filename="location_data666.json";

        json root;
        std::ifstream inputFile(filename);

        if (inputFile.good()){
            try {inputFile>>root;inputFile.close();
            }catch (...){root=json::array();}
        }else{root=json::array();}

        if (!root.is_array()){root=json::array();}
        root.push_back(j);
        std::ofstream outputFile(filename);
        outputFile<<root.dump(4);
        outputFile.close();
    }catch(const std::exception& e){std::cerr<<"Ошибка сохранения в файл: "<<e.what()<<std::endl;}}
