#ifndef DATABASE_H
#define DATABASE_H

#include <libpq-fe.h>
#include <string>
#include <vector>
#include <iostream>
#include "common.h"

#define DB_HOST "localhost"
#define DB_PORT "5432"
#define DB_NAME "mobile_network_db"
#define DB_USER "postgres"
#define DB_PASSWORD "postgres1234"

struct HeatmapDbRow{
    double latitude=0.0;
    double longitude=0.0;
    double accuracy=0.0;
    double altitude=0.0;
    int earfcn=0;
    int rsrp=0;
    int rsrq=0;
    int rssi=0;
};

class Database{
public:
    Database():conn(nullptr) {}

    ~Database(){
        if (conn){
            PQfinish(conn);
            conn=nullptr;}}

    bool connect(){
        const char* info="host=" DB_HOST " port=" DB_PORT " dbname=" DB_NAME " user=" DB_USER " password=" DB_PASSWORD;
        conn=PQconnectdb(info);

        if (PQstatus(conn)!=CONNECTION_OK){
            std::cerr<<"\033[31mОШИБКА\033[0m подключения к БД: "<<PQerrorMessage(conn)<<"\n";
            PQfinish(conn);
            conn=nullptr;
            return false;}
        std::cout<<"Подключение к БД \033[32mУСПЕШНО!\033[0m\n";
        return true;}

    bool isConnected() const {return conn!=nullptr&&PQstatus(conn)==CONNECTION_OK;}

    bool executeCommand(const std::string& sql);
    int insertMeasurement(const LocationData& data,int counter);
    bool insertCell(int measurement_id,const CellTowerData& cell);
    std::vector<std::vector<std::string>> getMeasurements(int limit=10);
    std::vector<HeatmapDbRow> getHeatmapRows();

private:
    PGconn* conn;};

extern Database g_database;

#endif
