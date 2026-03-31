#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <iostream>
#include <chrono>
#include <thread>
#include <fstream>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <vector>
#include <map>
#include <set>
#include <mutex>
#include <zmq.hpp>
#include <nlohmann/json.hpp>
#include "backends/imgui_impl_opengl3.h"
#include "backends/imgui_impl_sdl2.h"
#include "imgui.h"
#include "implot.h"
#include "database.h"

using json = nlohmann::json;

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

LocationData g_locationData;
LocationHistory g_locationHistory;
SignalHistory g_signalHistory;
Database g_database;
int g_measurementCounter=0;
const int INT32_MAX_VAL=2147483647;

bool isValidCell(const CellTowerData& cell){
    if (cell.mcc==INT32_MAX_VAL||cell.mnc==INT32_MAX_VAL) return false;
    if (cell.type=="LTE"&&(cell.rsrp==INT32_MAX_VAL||cell.rsrq==INT32_MAX_VAL)) return false;
    return true;}

bool isValidLocation(const LocationData& data){
    if (data.latitude==0.0f&&data.longitude==0.0f) return false;
    if (data.latitude<-90.0f||data.latitude>90.0f) return false;
    if (data.longitude<-180.0f||data.longitude>180.0f) return false;
    return true;}

static const ImVec4 pciColors[5]={
    ImVec4(0.00f,0.80f,1.00f,1.00f),
    ImVec4(0.95f,0.60f,0.00f,1.00f),
    ImVec4(0.00f,0.95f,0.60f,1.00f),
    ImVec4(1.00f,0.00f,0.80f,1.00f),
    ImVec4(1.00f,0.80f,0.00f,1.00f)};

ImVec4 getColorForPCI(int pci,int index){
    if (index>=0&&index<5) return pciColors[index];
    float hue=(pci*0.618f)-(int)(pci*0.618f);
    return ImVec4(hue,0.7f,0.8f,1.0f);}

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

        const std::string filename="location_data.json";

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

void run_gui(LocationData* loc){
    SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER);
    SDL_Window* window=SDL_CreateWindow(
        "LOCATION & TELEPHONY",
        SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,
        1400,900,SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE);
    SDL_GLContext gl_context=SDL_GL_CreateContext(window);
    ImGui::CreateContext();
    ImPlot::CreateContext();
    ImGuiIO& io=ImGui::GetIO();
    (void)io;
    ImGui::StyleColorsDark();
    ImGuiStyle& style=ImGui::GetStyle();
    style.WindowRounding=8.0f;
    style.FrameRounding=6.0f;
    ImVec4* colors=style.Colors;
    colors[ImGuiCol_WindowBg]=ImVec4(0.09f,0.10f,0.12f,1.00f);
    colors[ImGuiCol_TitleBg]=ImVec4(0.00f,0.40f,0.70f,1.00f);
    colors[ImGuiCol_TitleBgActive]=ImVec4(0.00f,0.50f,0.80f,1.00f);
    colors[ImGuiCol_FrameBg]=ImVec4(0.15f,0.17f,0.20f,1.00f);
    colors[ImGuiCol_Button]=ImVec4(0.00f,0.50f,0.80f,0.60f);
    colors[ImGuiCol_ButtonHovered]=ImVec4(0.00f,0.60f,0.90f,1.00f);
    colors[ImGuiCol_ButtonActive]=ImVec4(0.00f,0.70f,1.00f,1.00f);
    colors[ImGuiCol_Separator]=ImVec4(0.00f,0.50f,0.80f,0.50f);
    style.WindowPadding=ImVec2(20,20);
    style.FramePadding=ImVec2(15,10);
    style.ItemSpacing=ImVec2(15,15);
    ImGui_ImplSDL2_InitForOpenGL(window,gl_context);
    ImGui_ImplOpenGL3_Init("#version 330");
    io.FontGlobalScale=1.4f;
    bool running=true;
    while(running){
        SDL_Event event;
        while(SDL_PollEvent(&event)){
            ImGui_ImplSDL2_ProcessEvent(&event);
            if(event.type==SDL_QUIT)running=false;}
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();
        ImGui::SetNextWindowPos(ImVec2(0,0));
        ImGui::SetNextWindowSize(ImVec2(1400,900));
        ImGui::Begin("LOCATION & TELEPHONY",nullptr,
            ImGuiWindowFlags_NoResize|
            ImGuiWindowFlags_NoMove|
            ImGuiWindowFlags_NoCollapse|
            ImGuiWindowFlags_NoSavedSettings);
        ImGui::Columns(2,"main_columns",false);
        ImGui::SetColumnWidth(0,400);
        float left_start_y=ImGui::GetCursorPosY();
        ImGui::TextColored(ImVec4(0.00f,0.70f,1.00f,1.00f),"CURRENT POSITION");
        ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.80f,0.80f,0.80f,1.00f),"Latitude:");
        ImGui::SameLine(120);
        ImGui::PushStyleColor(ImGuiCol_Text,ImVec4(0.00f,0.80f,1.00f,1.00f));
        ImGui::Text("%.6f°",loc->latitude);
        ImGui::PopStyleColor();
        ImGui::TextColored(ImVec4(0.80f,0.80f,0.80f,1.00f),"Longitude:");
        ImGui::SameLine(120);
        ImGui::PushStyleColor(ImGuiCol_Text,ImVec4(0.00f,0.80f,1.00f,1.00f));
        ImGui::Text("%.6f°",loc->longitude);
        ImGui::PopStyleColor();
        ImGui::TextColored(ImVec4(0.80f,0.80f,0.80f,1.00f),"Altitude:");
        ImGui::SameLine(120);
        ImGui::PushStyleColor(ImGuiCol_Text,ImVec4(0.00f,0.95f,0.60f,1.00f));
        ImGui::Text("%.2f m",loc->altitude);
        ImGui::PopStyleColor();
        ImGui::TextColored(ImVec4(0.80f,0.80f,0.80f,1.00f),"Accuracy:");
        ImGui::SameLine(120);
        ImGui::PushStyleColor(ImGuiCol_Text,ImVec4(1.00f,0.80f,0.00f,1.00f));
        ImGui::Text("%.2f m",loc->accuracy);
        ImGui::PopStyleColor();
        ImGui::TextColored(ImVec4(0.80f,0.80f,0.80f,1.00f),"Last Update:");
        ImGui::SameLine(150);
        ImGui::PushStyleColor(ImGuiCol_Text,ImVec4(1.00f,1.00f,1.00f,1.00f));
        ImGui::Text("%s",loc->time.c_str());
        ImGui::PopStyleColor();
        ImGui::Spacing();
        ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.00f,0.70f,1.00f,1.00f),"TRAFFIC");
        ImGui::Spacing();
        float total_mb=loc->traffic.total/(1024.0*1024.0);
        float rx_mb=loc->traffic.total_rx/(1024.0*1024.0);
        float tx_mb=loc->traffic.total_tx/(1024.0*1024.0);
        ImGui::TextColored(ImVec4(0.80f,0.80f,0.80f,1.00f),"Total:");
        ImGui::SameLine(90);
        ImGui::PushStyleColor(ImGuiCol_Text,ImVec4(0.00f,0.95f,0.60f,1.00f));
        ImGui::Text("%.2f MB",total_mb);
        ImGui::PopStyleColor();
        ImGui::TextColored(ImVec4(0.80f,0.80f,0.80f,1.00f),"Download (RX):");
        ImGui::SameLine(160);
        ImGui::PushStyleColor(ImGuiCol_Text,ImVec4(0.00f,0.80f,1.00f,1.00f));
        ImGui::Text("%.2f MB",rx_mb);
        ImGui::PopStyleColor();
        ImGui::TextColored(ImVec4(0.80f,0.80f,0.80f,1.00f),"Upload (TX):");
        ImGui::SameLine(150);
        ImGui::PushStyleColor(ImGuiCol_Text,ImVec4(0.00f,0.80f,1.00f,1.00f));
        ImGui::Text("%.2f MB",tx_mb);
        ImGui::PopStyleColor();
        ImGui::Spacing();
        ImGui::Spacing();
        ImGui::Spacing();
        ImGui::NextColumn();
        ImGui::SetColumnWidth(1,960);
        ImGui::SetCursorPosY(left_start_y);
        ImGui::BeginChild("Graphs and Towers",ImVec2(0,0),true);
        ImGui::TextColored(ImVec4(0.00f,0.70f,1.00f,1.00f),"SIGNAL POWER GRAPHICS");
        ImGui::Separator();
        ImGui::Spacing();

        std::vector<double> rsrp,rsrq,rssnr,ss_rsrp,ss_rsrq,ss_sinr,dbm;
        int data_count=0;
        {   std::lock_guard<std::mutex> lock(g_signalHistory.mutex);
            data_count=g_signalHistory.timestamps.size();}

        ImGui::Text("Data points: %d",data_count);
        ImGui::Spacing();

        if (data_count>0){
            float graph_height=200.0f;
            if (!g_signalHistory.rsrp_values.empty()){
                double min_val=999999,max_val=-999999;
                for (const auto& pair:g_signalHistory.rsrp_values){
                    if (!pair.second.empty()){
                        double min_pair=*std::min_element(pair.second.begin(),pair.second.end());
                        double max_pair=*std::max_element(pair.second.begin(),pair.second.end());
                        min_val=std::min(min_val, min_pair);
                        max_val=std::max(max_val, max_pair);}}

                if (min_val<=max_val&&ImPlot::BeginPlot("RSRP (dBm) - LTE",ImVec2(-1,graph_height))){
                    ImPlot::SetupAxes("Measurement number","dBm");
                    ImPlot::SetupAxisLimits(ImAxis_X1,0,data_count-1,ImGuiCond_Always);
                    ImPlot::SetupAxisLimits(ImAxis_Y1,min_val-5,max_val+5,ImGuiCond_Always);
                    int idx=0;
                    for (const auto& pair:g_signalHistory.rsrp_values){
                        if (idx>=5) break;
                        std::string label="PCI "+std::to_string(pair.first);
                        ImVec4 color=getColorForPCI(pair.first,idx);
                        ImPlot::PlotLine(label.c_str(),pair.second.data(),(int)pair.second.size(),1,0,{ImPlotProp_LineColor,color});
                        idx++;}
                    ImPlot::EndPlot();}
                ImGui::Spacing();}

            if (!g_signalHistory.rsrq_values.empty()){
                double min_val=999999,max_val=-999999;
                for (const auto& pair:g_signalHistory.rsrq_values){
                    if (!pair.second.empty()){
                        double min_pair=*std::min_element(pair.second.begin(), pair.second.end());
                        double max_pair=*std::max_element(pair.second.begin(), pair.second.end());
                        min_val=std::min(min_val,min_pair);
                        max_val=std::max(max_val,max_pair);}}

                if (min_val<=max_val&&ImPlot::BeginPlot("RSRQ (dB) - LTE",ImVec2(-1,graph_height))){
                    ImPlot::SetupAxes("Measurement number","dB");
                    ImPlot::SetupAxisLimits(ImAxis_X1,0,data_count-1,ImGuiCond_Always);
                    ImPlot::SetupAxisLimits(ImAxis_Y1,min_val-5,max_val+5,ImGuiCond_Always);
                    int idx=0;
                    for (const auto& pair:g_signalHistory.rsrq_values){
                        if (idx>=5) break;
                        std::string label="PCI "+std::to_string(pair.first);
                        ImVec4 color=getColorForPCI(pair.first,idx);
                        ImPlot::PlotLine(label.c_str(),pair.second.data(),(int)pair.second.size(),1,0,{ImPlotProp_LineColor,color});
                        idx++;}
                    ImPlot::EndPlot();}
                ImGui::Spacing();}

            if (!g_signalHistory.rssi_values.empty()){
                double min_val=999999,max_val=-999999;
                for (const auto& pair:g_signalHistory.rssi_values){
                    if (!pair.second.empty()){
                        double min_pair=*std::min_element(pair.second.begin(),pair.second.end());
                        double max_pair=*std::max_element(pair.second.begin(),pair.second.end());
                        min_val=std::min(min_val,min_pair);
                        max_val=std::max(max_val,max_pair);}}

                if (min_val<=max_val&&ImPlot::BeginPlot("RSSI (dBm) - LTE",ImVec2(-1,graph_height))){
                    ImPlot::SetupAxes("Measurement number","dBm");
                    ImPlot::SetupAxisLimits(ImAxis_X1,0,data_count-1,ImGuiCond_Always);
                    ImPlot::SetupAxisLimits(ImAxis_Y1,min_val-5,max_val+5,ImGuiCond_Always);
                    int idx=0;
                    for (const auto& pair:g_signalHistory.rssi_values){
                        if (idx>=5) break;
                        std::string label="PCI "+std::to_string(pair.first);
                        ImVec4 color=getColorForPCI(pair.first,idx);
                        ImPlot::PlotLine(label.c_str(),pair.second.data(),(int)pair.second.size(),1,0,{ImPlotProp_LineColor,color});
                        idx++;}
                    ImPlot::EndPlot();}
                ImGui::Spacing();}

            if (!g_signalHistory.rssnr_values.empty()){
                double min_val=999999,max_val=-999999;
                for (const auto& pair:g_signalHistory.rssnr_values){
                    if (!pair.second.empty()){
                        double min_pair=*std::min_element(pair.second.begin(),pair.second.end());
                        double max_pair=*std::max_element(pair.second.begin(),pair.second.end());
                        min_val=std::min(min_val,min_pair);
                        max_val=std::max(max_val,max_pair);}}
                if (min_val<=max_val&&ImPlot::BeginPlot("RSSNR (dB) - LTE",ImVec2(-1,graph_height))){
                    ImPlot::SetupAxes("Measurement number","dB");
                    ImPlot::SetupAxisLimits(ImAxis_X1,0,data_count-1,ImGuiCond_Always);
                    ImPlot::SetupAxisLimits(ImAxis_Y1,min_val-5,max_val+5,ImGuiCond_Always);
                    int idx=0;
                    for (const auto& pair:g_signalHistory.rssnr_values){
                        if (idx>=5) break;
                        std::string label="PCI "+std::to_string(pair.first);
                        ImVec4 color=getColorForPCI(pair.first,idx);
                        ImPlot::PlotLine(label.c_str(),pair.second.data(),(int)pair.second.size(),1,0,{ImPlotProp_LineColor,color});
                        idx++;}
                    ImPlot::EndPlot();}
                ImGui::Spacing();}
            ImGui::Separator();
            ImGui::Spacing();
            ImGui::Text("Latest values:");
            std::vector<int> pcis;
            for (const auto& pair:g_signalHistory.rsrp_values){
                if (!pair.second.empty()) pcis.push_back(pair.first);}
            int idx=0;
            for (int pci:pcis){
                if (idx>=5) break;
                ImVec4 color=getColorForPCI(pci,idx);
                ImGui::TextColored(color,"PCI %d",pci);
                auto rsrp_it=g_signalHistory.rsrp_values.find(pci);
                if (rsrp_it!=g_signalHistory.rsrp_values.end()&&!rsrp_it->second.empty()){ImGui::TextColored(color,"  RSRP: %.1f dBm",rsrp_it->second.back());}
                auto rsrq_it=g_signalHistory.rsrq_values.find(pci);
                if (rsrq_it!=g_signalHistory.rsrq_values.end()&&!rsrq_it->second.empty()){ImGui::TextColored(color,"  RSRQ: %.1f dB",rsrq_it->second.back());}
                auto rssi_it=g_signalHistory.rssi_values.find(pci);
                if (rssi_it!=g_signalHistory.rssi_values.end()&&!rssi_it->second.empty()){ImGui::TextColored(color,"  RSSI: %.1f dBm",rssi_it->second.back());}
                auto rssnr_it=g_signalHistory.rssnr_values.find(pci);
                if (rssnr_it!=g_signalHistory.rssnr_values.end()&&!rssnr_it->second.empty()){ImGui::TextColored(color,"  SINR: %.1f dB",rssnr_it->second.back());}
                idx++;
                ImGui::Spacing();}
        }else{
            ImGui::TextColored(ImVec4(1.0f,0.5f,0.5f,1.0f),"No data to display");
            ImGui::Text("Send data from phone to see signal strength graphs");}
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::TextColored(ImVec4(0.00f,0.70f,1.00f,1.00f),"TELEPHONY");
        ImGui::Separator();
        ImGui::Spacing();
        if(!loc->cellTowers.empty()){
            for(size_t i=0;i<loc->cellTowers.size();++i){
                const auto& cell=loc->cellTowers[i];
                ImVec4 typeColor;
                if(cell.type=="LTE")typeColor=ImVec4(0.00f,0.80f,1.00f,1.00f);
                else if(cell.type=="5G"||cell.type=="NR")typeColor=ImVec4(0.95f,0.60f,0.00f,1.00f);
                else typeColor=ImVec4(0.70f,0.70f,0.70f,1.00f);
                ImGui::PushStyleColor(ImGuiCol_Text,typeColor);
                ImGui::Text("Tower %zu [%s]",i+1,cell.type.c_str());
                ImGui::PopStyleColor();
                ImGui::Columns(2,"tower_columns",false);
                ImGui::SetColumnWidth(0,400);
                if(cell.type=="LTE"){
                    ImGui::TextColored(ImVec4(0.80f,0.80f,0.80f,1.00f),"Band:");
                    ImGui::SameLine(100);
                    ImGui::Text("%d",cell.band);
                    ImGui::TextColored(ImVec4(0.80f,0.80f,0.80f,1.00f),"Cell ID:");
                    ImGui::SameLine(100);
                    ImGui::Text("%d",cell.cell_identity);
                    ImGui::TextColored(ImVec4(0.80f,0.80f,0.80f,1.00f),"EARFCN:");
                    ImGui::SameLine(100);
                    ImGui::Text("%d",cell.earfcn);
                    ImGui::TextColored(ImVec4(0.80f,0.80f,0.80f,1.00f),"MCC:");
                    ImGui::SameLine(100);
                    ImGui::Text("%d",cell.mcc);
                    ImGui::TextColored(ImVec4(0.80f,0.80f,0.80f,1.00f),"MNC:");
                    ImGui::SameLine(100);
                    ImGui::Text("%d",cell.mnc);
                    ImGui::TextColored(ImVec4(0.80f,0.80f,0.80f,1.00f),"PCI:");
                    ImGui::SameLine(100);
                    ImGui::Text("%d",cell.pci);
                    ImGui::TextColored(ImVec4(0.80f,0.80f,0.80f,1.00f),"TAC:");
                    ImGui::SameLine(100);
                    ImGui::Text("%d",cell.tac);}
                else if(cell.type=="GSM"){
                    ImGui::TextColored(ImVec4(0.80f,0.80f,0.80f,1.00f),"Cell ID:");
                    ImGui::SameLine(100);
                    ImGui::Text("%d",cell.cell_identity);
                    ImGui::TextColored(ImVec4(0.80f,0.80f,0.80f,1.00f),"ARFCN:");
                    ImGui::SameLine(100);
                    ImGui::Text("%d",cell.arfcn);
                    ImGui::TextColored(ImVec4(0.80f,0.80f,0.80f,1.00f),"BSIC:");
                    ImGui::SameLine(100);
                    ImGui::Text("%d",cell.bsic);
                    ImGui::TextColored(ImVec4(0.80f,0.80f,0.80f,1.00f),"LAC:");
                    ImGui::SameLine(100);
                    ImGui::Text("%d",cell.lac);
                    ImGui::TextColored(ImVec4(0.80f,0.80f,0.80f,1.00f),"MCC:");
                    ImGui::SameLine(100);
                    ImGui::Text("%d",cell.mcc);
                    ImGui::TextColored(ImVec4(0.80f,0.80f,0.80f,1.00f),"MNC:");
                    ImGui::SameLine(100);
                    ImGui::Text("%d",cell.mnc);
                    ImGui::TextColored(ImVec4(0.80f,0.80f,0.80f,1.00f),"PSC:");
                    ImGui::SameLine(100);
                    ImGui::Text("%d",cell.pci);}
                else if(cell.type=="NR"){
                    ImGui::TextColored(ImVec4(0.80f,0.80f,0.80f,1.00f),"Band:");
                    ImGui::SameLine(100);
                    ImGui::Text("%d",cell.band);
                    ImGui::TextColored(ImVec4(0.80f,0.80f,0.80f,1.00f),"NCI:");
                    ImGui::SameLine(100);
                    ImGui::Text("%s",cell.nci.c_str());
                    ImGui::TextColored(ImVec4(0.80f,0.80f,0.80f,1.00f),"NRARFCN:");
                    ImGui::SameLine(100);
                    ImGui::Text("%d",cell.nrarfcn);
                    ImGui::TextColored(ImVec4(0.80f,0.80f,0.80f,1.00f),"PCI:");
                    ImGui::SameLine(100);
                    ImGui::Text("%d",cell.pci);
                    ImGui::TextColored(ImVec4(0.80f,0.80f,0.80f,1.00f),"TAC:");
                    ImGui::SameLine(100);
                    ImGui::Text("%d",cell.tac);
                    ImGui::TextColored(ImVec4(0.80f,0.80f,0.80f,1.00f),"MCC:");
                    ImGui::SameLine(100);
                    ImGui::Text("%d",cell.mcc);
                    ImGui::TextColored(ImVec4(0.80f,0.80f,0.80f,1.00f),"MNC:");
                    ImGui::SameLine(100);
                    ImGui::Text("%d",cell.mnc);}
                ImGui::NextColumn();
                if(cell.type=="LTE"){
                    ImGui::TextColored(ImVec4(0.00f,0.95f,0.60f,1.00f),"ASU Level:");
                    ImGui::SameLine(120);
                    ImGui::Text("%d",cell.asu_level);
                    ImGui::TextColored(ImVec4(0.00f,0.95f,0.60f,1.00f),"CQI:");
                    ImGui::SameLine(100);
                    ImGui::Text("%d",cell.cqi);
                    ImGui::TextColored(ImVec4(0.00f,0.95f,0.60f,1.00f),"RSRP:");
                    ImGui::SameLine(100);
                    ImGui::Text("%d dBm",cell.rsrp);
                    ImGui::TextColored(ImVec4(0.00f,0.95f,0.60f,1.00f),"RSRQ:");
                    ImGui::SameLine(100);
                    ImGui::Text("%d",cell.rsrq);
                    ImGui::TextColored(ImVec4(0.00f,0.95f,0.60f,1.00f),"RSSI:");
                    ImGui::SameLine(100);
                    ImGui::Text("%d",cell.rssi);
                    ImGui::TextColored(ImVec4(0.00f,0.95f,0.60f,1.00f),"RSSNR:");
                    ImGui::SameLine(100);
                    ImGui::Text("%d",cell.rssnr);
                    ImGui::TextColored(ImVec4(0.80f,0.80f,0.80f,1.00f),"TA:");
                    ImGui::SameLine(100);
                    ImGui::Text("%d",cell.timing_advance);}
                else if(cell.type=="GSM"){
                    ImGui::TextColored(ImVec4(0.00f,0.95f,0.60f,1.00f),"Dbm:");
                    ImGui::SameLine(100);
                    ImGui::Text("%d dBm",cell.dbm);
                    ImGui::TextColored(ImVec4(0.00f,0.95f,0.60f,1.00f),"RSSI:");
                    ImGui::SameLine(100);
                    ImGui::Text("%d",cell.rssi);
                    ImGui::TextColored(ImVec4(0.80f,0.80f,0.80f,1.00f),"TA:");
                    ImGui::SameLine(100);
                    ImGui::Text("%d",cell.timing_advance);}
                else if(cell.type=="NR"){
                    ImGui::TextColored(ImVec4(0.00f,0.95f,0.60f,1.00f),"SS-RSRP:");
                    ImGui::SameLine(100);
                    ImGui::Text("%d dBm",cell.ss_rsrp);
                    ImGui::TextColored(ImVec4(0.00f,0.95f,0.60f,1.00f),"SS-RSRQ:");
                    ImGui::SameLine(100);
                    ImGui::Text("%d",cell.ss_rsrq);
                    ImGui::TextColored(ImVec4(0.00f,0.95f,0.60f,1.00f),"SS-SINR:");
                    ImGui::SameLine(100);
                    ImGui::Text("%d",cell.ss_sinr);
                    ImGui::TextColored(ImVec4(0.80f,0.80f,0.80f,1.00f),"TA:");
                    ImGui::SameLine(100);
                    ImGui::Text("%ld",cell.timing_advance_micros);}
                ImGui::Columns(1);
                if(i<loc->cellTowers.size()-1){ImGui::Spacing();ImGui::Separator();ImGui::Spacing();}}
        }else{ImGui::TextColored(ImVec4(1.00f,0.50f,0.50f,1.00f),"No cell tower data");}
        ImGui::EndChild();
        ImGui::Columns(1);
        ImGui::End();
        ImGui::Render();
        glClearColor(0.08f,0.09f,0.10f,1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);}
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();
    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();}

CellTowerData parseCellTower(const json& cellJson){
    CellTowerData cell;
    cell.type=cellJson.value("type","Unknown");
    cell.mcc=cellJson.value("mcc",0);
    cell.mnc=cellJson.value("mnc",0);
    cell.pci=cellJson.value("pci",0);
    cell.tac=cellJson.value("tac",0);
    cell.timing_advance=cellJson.value("timing_advance",0);

    if (cell.type=="LTE"){
        cell.band=cellJson.value("band",0);
        cell.cell_identity=cellJson.value("cell_identity",0);
        cell.earfcn=cellJson.value("earfcn",0);
        cell.asu_level=cellJson.value("asu_level",0);
        cell.cqi=cellJson.value("cqi",0);
        cell.rsrp=cellJson.value("rsrp",0);
        cell.rsrq=cellJson.value("rsrq",0);
        cell.rssi=cellJson.value("rssi",0);
        cell.rssnr=cellJson.value("rssnr",0);}
    else if (cell.type=="GSM"){
        cell.cell_identity=cellJson.value("cell_identity",0);
        cell.bsic=cellJson.value("bsic",0);
        cell.arfcn=cellJson.value("arfcn",0);
        cell.lac=cellJson.value("lac",0);
        cell.dbm=cellJson.value("dbm",0);}
    else if (cell.type=="NR"){
        cell.nci=cellJson.value("nci","");
        cell.nrarfcn=cellJson.value("nrarfcn",0);
        cell.ss_rsrp=cellJson.value("ss_rsrp",0);
        cell.ss_rsrq=cellJson.value("ss_rsrq",0);
        cell.ss_sinr=cellJson.value("ss_sinr",0);
        cell.timing_advance_micros=cellJson.value("timing_advance_micros",0L);}
    return cell;}

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
        int open_count=0;
        int close_count=0;
        for (char c:msg){
            if (c=='{') open_count++;
            if (c=='}') close_count++;}
        
        while (close_count>open_count&&!msg.empty()&&msg.back()=='}'){
            msg.pop_back();
            close_count--;}
        
        while (!msg.empty()&&(msg.back()==' '||msg.back()=='\n'||msg.back()=='\r'||msg.back()=='\t')){msg.pop_back();}

        try{
            json j=json::parse(msg);
            {   std::lock_guard<std::mutex> lock(loc->mutex);
                if (j.contains("location") && j["location"].is_object()){
                    auto& loc_data = j["location"];
                    loc->latitude=loc_data.value("latitude",0.0f);
                    loc->longitude=loc_data.value("longitude",0.0f);
                    loc->altitude=loc_data.value("altitude",0.0f);
                    loc->accuracy=loc_data.value("accuracy",0.0f);
                    loc->time=loc_data.value("time","No data");
                    loc->time_milliseconds=loc_data.value("current_time",0LL);}
                if (j.contains("traffic")){
                    loc->traffic.total_rx=j["traffic"].value("total_rx",0LL);
                    loc->traffic.total_tx=j["traffic"].value("total_tx",0LL);
                    loc->traffic.total=j["traffic"].value("total",0LL);}
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
                            if (isValidCell(cell)){g_database.insertCell(measurement_id,cell);
                            }else{std::cout<<"  [!] Пропуск записи, кривые значения"<<std::endl;}}}
                    std::cout<<"\033[32m[БД]\033[0m Измерение #"<<g_measurementCounter<<" сохранено"<<std::endl;}
                else{std::cout<<"\033[33m[БД]\033[0m Не подключено, измерение #"<<g_measurementCounter<<" сохранено только в JSON"<<std::endl;}}
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

int main(){
    if (g_database.connect()){std::cout<<"\033[32mПодключение к БД успешно\033[0m"<<std::endl;}
    else{std::cout<<"\033[33mОШИБКА ПОДКЛЮЧЕНИЯ К БД\033[0m"<<std::endl;}
    std::thread gui_thread(run_gui,&g_locationData);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    run_zmq_server(&g_locationData);
    gui_thread.join();}