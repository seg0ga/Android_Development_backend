#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include "json.h"

using json = nlohmann::json;

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
