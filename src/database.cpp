#include "database.h"
#include "common.h"
#include <sstream>

int Database::insertMeasurement(const LocationData& data,int counter){
    if (!conn) return -1;
    std::ostringstream query;
    query<<"INSERT INTO based (counter, \"current_time\", \"time\", latitude, longitude, accuracy, altitude, traffic_total, traffic_total_rx, traffic_total_tx) "
          <<"VALUES ($1, $2, $3, $4, $5, $6, $7, $8, $9, $10) RETURNING id";

    const char* params[10];
    std::string p1=std::to_string(counter);
    std::string p2=std::to_string(data.time_milliseconds);
    std::string p3=data.time;
    std::string p4=std::to_string(data.latitude);
    std::string p5=std::to_string(data.longitude);
    std::string p6=std::to_string(data.accuracy);
    std::string p7=std::to_string(data.altitude);
    std::string p8=std::to_string(data.traffic.total);
    std::string p9=std::to_string(data.traffic.total_rx);
    std::string p10=std::to_string(data.traffic.total_tx);
    params[0]=p1.c_str();
    params[1]=p2.c_str();
    params[2]=p3.c_str();
    params[3]=p4.c_str();
    params[4]=p5.c_str();
    params[5]=p6.c_str();
    params[6]=p7.c_str();
    params[7]=p8.c_str();
    params[8]=p9.c_str();
    params[9]=p10.c_str();
    PGresult* res=PQexecParams(conn,query.str().c_str(),10,nullptr,params,nullptr,nullptr,0);

    int measurement_id=-1;
    if (PQresultStatus(res)==PGRES_TUPLES_OK&&PQntuples(res)>0){measurement_id=std::stoi(PQgetvalue(res,0,0));
    }else{std::cerr<<"\033[31mОШИБКА\033[0m вставки measurement: "<<PQresultErrorMessage(res)<<"\n";}
    PQclear(res);
    return measurement_id;}

bool Database::insertCell(int measurement_id,const CellTowerData& cell){
    if (!conn) return false;

    std::ostringstream query;
    query<<"INSERT INTO cells (measurement_id, type, mcc, mnc, cell_identity, tac, earfcn, band, pci, rsrp, rsrq, rssi, rssnr, asu_level, cqi, timing_advance) "
          <<"VALUES ($1, $2, $3, $4, $5, $6, $7, $8, $9, $10, $11, $12, $13, $14, $15, $16)";

    const char* params[16];
    std::string p1=std::to_string(measurement_id);
    std::string p2=cell.type;
    std::string p3=std::to_string(cell.mcc);
    std::string p4=std::to_string(cell.mnc);
    std::string p5=std::to_string(cell.cell_identity);
    std::string p6=std::to_string(cell.tac);
    std::string p7=std::to_string(cell.earfcn);
    std::string p8=std::to_string(cell.band);
    std::string p9=std::to_string(cell.pci);
    std::string p10=std::to_string(cell.rsrp);
    std::string p11=std::to_string(cell.rsrq);
    std::string p12=std::to_string(cell.rssi);
    std::string p13=std::to_string(cell.rssnr);
    std::string p14=std::to_string(cell.asu_level);
    std::string p15=std::to_string(cell.cqi);
    std::string p16=std::to_string(cell.timing_advance);
    params[0]=p1.c_str();
    params[1]=p2.c_str();
    params[2]=p3.c_str();
    params[3]=p4.c_str();
    params[4]=p5.c_str();
    params[5]=p6.c_str();
    params[6]=p7.c_str();
    params[7]=p8.c_str();
    params[8]=p9.c_str();
    params[9]=p10.c_str();
    params[10]=p11.c_str();
    params[11]=p12.c_str();
    params[12]=p13.c_str();
    params[13]=p14.c_str();
    params[14]=p15.c_str();
    params[15]=p16.c_str();
    PGresult* res=PQexecParams(conn,query.str().c_str(),16,nullptr,params,nullptr,nullptr,0);
    bool success=(PQresultStatus(res)==PGRES_COMMAND_OK);
    if (!success){std::cerr<<"\033[31mОШИБКА\033[0m вставки cell: "<<PQresultErrorMessage(res)<<"\n";}
    PQclear(res);
    return success;}

std::vector<std::vector<std::string>> Database::getMeasurements(int limit){
    std::vector<std::vector<std::string>> results;
    if (!conn) return results;
    std::ostringstream query;
    query<<"SELECT id, counter, time, latitude, longitude, accuracy, altitude FROM based ORDER BY id DESC LIMIT "<<limit;

    PGresult* res=PQexec(conn,query.str().c_str());
    if (PQresultStatus(res)==PGRES_TUPLES_OK){
        int nFields=PQnfields(res);
        for (int i=0;i<PQntuples(res);i++){
            std::vector<std::string> row;
            for (int j=0;j<nFields;j++){
                if (PQgetisnull(res,i,j)){row.push_back("NULL");
                }else{row.push_back(PQgetvalue(res,i,j));}}
            results.push_back(row);}}
    PQclear(res);
    return results;}
