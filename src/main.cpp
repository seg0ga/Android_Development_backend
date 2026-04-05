#include <iostream>
#include <chrono>
#include <thread>
#include <zmq.hpp>
#include <nlohmann/json.hpp>
#include "common.h"
#include "gui.h"
#include "server.h"
#include "database.h"

LocationData g_locationData;
LocationHistory g_locationHistory;
SignalHistory g_signalHistory;
Database g_database;
int g_measurementCounter=0;
const int INT32_MAX_VAL=2147483647;

int main(){
    if (g_database.connect()){std::cout<<"\033[32mПодключение к БД успешно\033[0m"<<std::endl;}
    else{std::cout<<"\033[33mОШИБКА ПОДКЛЮЧЕНИЯ К БД\033[0m"<<std::endl;}
    std::thread gui_thread(run_gui,&g_locationData);
    std::thread server_thread(run_zmq_server,&g_locationData);
    gui_thread.join();}
