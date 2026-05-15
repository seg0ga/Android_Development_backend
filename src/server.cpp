#include <iostream>
#include <thread>
#include <zmq.hpp>
#include <nlohmann/json.hpp>
#include "server.h"
#include "json.h"
#include "database.h"

using json = nlohmann::json;

extern Database g_database;
extern int g_measurementCounter;

void updateSignalHistory(const LocationData& data){
    std::lock_guard<std::mutex> lock(g_signalHistory.mutex);
    g_signalHistory.timestamps.push_back(g_signalHistory.timestamps.size());
    for (const auto& cell:data.cellTowers){
        if (cell.type=="LTE"){
            g_signalHistory.rsrp_values[cell.pci].push_back(cell.rsrp);
            g_signalHistory.rsrq_values[cell.pci].push_back(cell.rsrq);
            g_signalHistory.rssi_values[cell.pci].push_back(cell.rssi);
            g_signalHistory.rssnr_values[cell.pci].push_back(cell.rssnr);}
        else if (cell.type=="NR"){
            g_signalHistory.ss_rsrp_values[cell.pci].push_back(cell.ss_rsrp);
            g_signalHistory.ss_rsrq_values[cell.pci].push_back(cell.ss_rsrq);
            g_signalHistory.ss_sinr_values[cell.pci].push_back(cell.ss_sinr);}
        else if (cell.type=="GSM"){
            g_signalHistory.dbm_values[cell.pci].push_back(cell.dbm);}}}

void run_zmq_server(LocationData* loc){
    zmq::context_t context(1);
    zmq::socket_t socket(context,ZMQ_REP);
    socket.bind("tcp://*:443");
    std::cout<<"\033[32mZMQ сервер запущен на порту 443\033[0m"<<std::endl;

    while(true){
        zmq::message_t request;
        socket.recv(request);
        std::string msg=static_cast<char*>(request.data());

        int brace_count=0;
        size_t first_complete_pos=0;
        bool in_string=false;
        bool escape=false;
        bool started=false;
        size_t start_pos=0;

        for (size_t i=0;i<msg.size();++i){
            char c=msg[i];
            if (escape){escape=false;continue;}
            if (c=='\\'){escape=true;continue;}
            if (c=='"'){in_string=!in_string;continue;}
            if (in_string) continue;
            if (c=='{'&&!started){
                started=true;
                start_pos=i;}

            if (c=='{') brace_count++;
            else if (c=='}'){
                brace_count--;
                if (brace_count==0&&started){
                    first_complete_pos=i+1;
                    break;}}}

        if (started&&first_complete_pos>start_pos){msg=msg.substr(start_pos,first_complete_pos-start_pos);}

        while (!msg.empty()&&(msg.back()==' '||msg.back()=='\n'||msg.back()=='\r'||msg.back()=='\t')){msg.pop_back();}

        try{
            json j=json::parse(msg);
            {   std::lock_guard<std::mutex> lock(loc->mutex);
                if (j.contains("location") && j["location"].is_object()){
                    auto& loc_data = j["location"];
                    auto getFloatValue=[&](const json& obj,const std::string& key,float defaultVal=0.0f)->float{
                        auto it=obj.find(key);
                        if (it==obj.end()) return defaultVal;
                        if (it->is_number_float()) return it->get<float>();
                        if (it->is_number()) return static_cast<float>(it->get<double>());
                        if (it->is_string()){
                            try{return std::stof(it->get<std::string>());}
                            catch(...){return defaultVal;}}
                        return defaultVal;};

                    auto getLongLongValue=[&](const json& obj,const std::string& key,long long defaultVal=0LL)->long long{
                        auto it=obj.find(key);
                        if (it==obj.end()) return defaultVal;
                        if (it->is_number_integer()) return it->get<long long>();
                        if (it->is_string()){
                            try{return std::stoll(it->get<std::string>());}
                            catch(...){return defaultVal;}}
                        if (it->is_number()) return static_cast<long long>(it->get<double>());
                        return defaultVal;};

                    loc->latitude=getFloatValue(loc_data,"latitude",0.0f);
                    loc->longitude=getFloatValue(loc_data,"longitude",0.0f);
                    loc->altitude=getFloatValue(loc_data,"altitude",0.0f);
                    loc->accuracy=getFloatValue(loc_data,"accuracy",0.0f);
                    loc->time=loc_data.value("time","No data");
                    loc->time_milliseconds=getLongLongValue(loc_data,"current_time",0LL);}
                if (j.contains("traffic")){
                    auto& traffic=j["traffic"];
                    auto getLL=[&](const json& obj,const std::string& key,long long defaultVal=0LL)->long long{
                        auto it=obj.find(key);
                        if (it==obj.end()) return defaultVal;
                        if (it->is_number_integer()) return it->get<long long>();
                        if (it->is_string()){
                            try{return std::stoll(it->get<std::string>());}
                            catch(...){return defaultVal;}}
                        if (it->is_number()) return static_cast<long long>(it->get<double>());
                        return defaultVal;};
                    loc->traffic.total_rx=getLL(traffic,"total_rx",0LL);
                    loc->traffic.total_tx=getLL(traffic,"total_tx",0LL);
                    loc->traffic.total=getLL(traffic,"total",0LL);}
                loc->cellTowers.clear();
                if (j.contains("cells")){
                    for (const auto& cellJson:j["cells"]){
                        CellTowerData cell=parseCellTower(cellJson);
                        loc->cellTowers.push_back(cell);}}
                g_measurementCounter++;
                saveToJsonFile(*loc,g_measurementCounter);
                updateSignalHistory(*loc);
                if (g_database.isConnected()){
                    int measurement_id=g_database.insertMeasurement(*loc,g_measurementCounter);
                    if (measurement_id>0){
                        std::cout<<"  Cells: "<<loc->cellTowers.size()<<std::endl;
                        for (const auto& cell:loc->cellTowers){
                            std::cout<<"  ["<<cell.type<<"] mcc="<<cell.mcc<<" rsrp="<<cell.rsrp<<" cqi="<<cell.cqi<<" band="<<cell.band<<std::endl;
                            g_database.insertCell(measurement_id,cell);}
                        std::cout<<"\033[32m[БД]\033[0m Измерение #"<<g_measurementCounter<<" сохранено"<<std::endl;}
                }else{std::cout<<"\033[33m[БД]\033[0m Не подключено, измерение #"<<g_measurementCounter<<" сохранено только в JSON"<<std::endl;}}
            zmq::message_t reply(5);
            memcpy(reply.data(),"OK",2);
            socket.send(reply,zmq::send_flags::none);
        }catch(const json::parse_error& e){
            std::cerr<<"Ошибка JSON: "<<e.what()<<std::endl;
            std::cerr<<"Длина: "<<msg.size()<<", позиция: "<<e.byte<<std::endl;
            if (e.byte>0&&e.byte<=msg.size()){
                size_t start=(e.byte>50)?e.byte-50:0;
                size_t len=std::min(size_t(100),msg.size()-start);
                std::cerr<<"Фрагмент: "<<msg.substr(start,len)<<std::endl;}
            zmq::message_t error(5);
            memcpy(error.data(),"ОШИБКА",5);
            socket.send(error,zmq::send_flags::none);
        }catch(const std::exception& e){
            std::cerr<<"Ошибка: "<<e.what()<<std::endl;
            zmq::message_t error(5);
            memcpy(error.data(),"ОШИБКА",5);
            socket.send(error,zmq::send_flags::none);}}}
