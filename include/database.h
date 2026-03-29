#ifndef DATABASE_H
#define DATABASE_H

#include <libpq-fe.h>
#include <string>
#include <vector>
#include <iostream>

#define DB_HOST "localhost"
#define DB_PORT "5432"
#define DB_NAME "mobile_network_db"
#define DB_USER "postgres"
#define DB_PASSWORD "postgres1234"

struct CellTowerData;
struct TrafficData;
struct LocationData;

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

    int insertMeasurement(const LocationData& data,int counter);
    bool insertCell(int measurement_id,const CellTowerData& cell);
    std::vector<std::vector<std::string>> getMeasurements(int limit=10);

private:
    PGconn* conn;};

#endif
