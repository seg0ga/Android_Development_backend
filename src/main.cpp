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
#include <mutex>
#include <zmq.hpp>
#include <nlohmann/json.hpp>
#include "backends/imgui_impl_opengl3.h"
#include "backends/imgui_impl_sdl2.h"
#include "imgui.h"
#include "implot.h"

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
    std::string time="Нет данных";
    long long time_milliseconds=0;
    TrafficData traffic;
    std::vector<CellTowerData> cellTowers;
    std::mutex mutex;};

struct LocationHistory{
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
    std::vector<double>rsrp_values;
    std::vector<double>rsrq_values;
    std::vector<double>rssnr_values;
    std::vector<double>ss_rsrp_values;
    std::vector<double>ss_rsrq_values;
    std::vector<double>ss_sinr_values;
    std::vector<double>dbm_values;
    std::mutex mutex;};

LocationData g_locationData;
LocationHistory g_locationHistory;
SignalHistory g_signalHistory;

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

    }catch(const std::exception& e){std::cerr<<"Ошибка сохранения файла: "<<e.what()<<std::endl;}}

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
            rsrp=g_signalHistory.rsrp_values;
            rsrq=g_signalHistory.rsrq_values;
            rssnr=g_signalHistory.rssnr_values;
            ss_rsrp=g_signalHistory.ss_rsrp_values;
            ss_rsrq=g_signalHistory.ss_rsrq_values;
            ss_sinr=g_signalHistory.ss_sinr_values;
            dbm=g_signalHistory.dbm_values;
            data_count=rsrp.size();}

        ImGui::Text("Data points: %d",data_count);
        ImGui::Spacing();
        if (data_count>0){
            double min_rsrp=*std::min_element(rsrp.begin(),rsrp.end());
            double max_rsrp=*std::max_element(rsrp.begin(),rsrp.end());
            double min_rsrq=*std::min_element(rsrq.begin(),rsrq.end());
            double max_rsrq=*std::max_element(rsrq.begin(),rsrq.end());
            double min_rssnr=*std::min_element(rssnr.begin(),rssnr.end());
            double max_rssnr=*std::max_element(rssnr.begin(),rssnr.end());
            double padding=5.0;
            float graph_height=200.0f;
            if (ImPlot::BeginPlot("RSRP (dBm) - LTE",ImVec2(-1,graph_height))){
                ImPlot::SetupAxes("Measurement number","dBm");
                ImPlot::SetupAxisLimits(ImAxis_X1,0,data_count-1,ImGuiCond_Always);
                ImPlot::SetupAxisLimits(ImAxis_Y1,min_rsrp-padding,max_rsrp+padding,ImGuiCond_Always);
                ImPlot::PlotLine("RSRP",rsrp.data(),data_count);
                ImPlot::EndPlot();}
            ImGui::Spacing();

            if (ImPlot::BeginPlot("RSRQ (dB) - LTE", ImVec2(-1,graph_height))){
                ImPlot::SetupAxes("Measurement number", "dB");
                ImPlot::SetupAxisLimits(ImAxis_X1,0,data_count-1,ImGuiCond_Always);
                ImPlot::SetupAxisLimits(ImAxis_Y1,min_rsrq-padding,max_rsrq+padding,ImGuiCond_Always);
                ImPlot::PlotLine("RSRQ",rsrq.data(),data_count);
                ImPlot::EndPlot();}
            ImGui::Spacing();

            if (ImPlot::BeginPlot("RSSNR (dB) - LTE",ImVec2(-1,graph_height))){
                ImPlot::SetupAxes("Measurement number", "dB");
                ImPlot::SetupAxisLimits(ImAxis_X1,0,data_count-1,ImGuiCond_Always);
                ImPlot::SetupAxisLimits(ImAxis_Y1,min_rssnr-padding,max_rssnr+padding,ImGuiCond_Always);
                ImPlot::PlotLine("RSSNR",rssnr.data(),data_count);
                ImPlot::EndPlot();}
            ImGui::Spacing();

            if (!ss_rsrp.empty()&&ss_rsrp.size()==data_count){
                double min_ss_rsrp=*std::min_element(ss_rsrp.begin(),ss_rsrp.end());
                double max_ss_rsrp=*std::max_element(ss_rsrp.begin(),ss_rsrp.end());
                if (ImPlot::BeginPlot("SS-RSRP (dBm) - 5G NR",ImVec2(-1,graph_height))){
                    ImPlot::SetupAxes("Measurement number","dBm");
                    ImPlot::SetupAxisLimits(ImAxis_X1,0,data_count-1,ImGuiCond_Always);
                    ImPlot::SetupAxisLimits(ImAxis_Y1,min_ss_rsrp-padding,max_ss_rsrp+padding,ImGuiCond_Always);
                    ImPlot::PlotLine("SS-RSRP",ss_rsrp.data(),data_count);
                    ImPlot::EndPlot();}ImGui::Spacing();}

            if (!ss_rsrq.empty()&&ss_rsrq.size()==data_count){
                double min_ss_rsrq=*std::min_element(ss_rsrq.begin(),ss_rsrq.end());
                double max_ss_rsrq=*std::max_element(ss_rsrq.begin(),ss_rsrq.end());

                if (ImPlot::BeginPlot("SS-RSRQ (dB) - 5G NR",ImVec2(-1,graph_height))){
                    ImPlot::SetupAxes("Measurement number", "dB");
                    ImPlot::SetupAxisLimits(ImAxis_X1,0,data_count-1,ImGuiCond_Always);
                    ImPlot::SetupAxisLimits(ImAxis_Y1,min_ss_rsrq-padding,max_ss_rsrq+padding,ImGuiCond_Always);
                    ImPlot::PlotLine("SS-RSRQ",ss_rsrq.data(),data_count);
                    ImPlot::EndPlot();}ImGui::Spacing();}

            if (!ss_sinr.empty()&&ss_sinr.size()==data_count){
                double min_ss_sinr=*std::min_element(ss_sinr.begin(),ss_sinr.end());
                double max_ss_sinr=*std::max_element(ss_sinr.begin(),ss_sinr.end());

                if (ImPlot::BeginPlot("SS-SINR (dB) - 5G NR",ImVec2(-1,graph_height))){
                    ImPlot::SetupAxes("Measurement number", "dB");
                    ImPlot::SetupAxisLimits(ImAxis_X1,0,data_count-1,ImGuiCond_Always);
                    ImPlot::SetupAxisLimits(ImAxis_Y1,min_ss_sinr-padding,max_ss_sinr+padding,ImGuiCond_Always);
                    ImPlot::PlotLine("SS-SINR",ss_sinr.data(),data_count);
                    ImPlot::EndPlot();}ImGui::Spacing();}

            if (!dbm.empty()&&dbm.size()==data_count){
                double min_dbm=*std::min_element(dbm.begin(),dbm.end());
                double max_dbm=*std::max_element(dbm.begin(),dbm.end());

                if (ImPlot::BeginPlot("DBM - GSM",ImVec2(-1,graph_height))){
                    ImPlot::SetupAxes("Measurement number", "dBm");
                    ImPlot::SetupAxisLimits(ImAxis_X1,0,data_count-1,ImGuiCond_Always);
                    ImPlot::SetupAxisLimits(ImAxis_Y1,min_dbm-padding,max_dbm+padding,ImGuiCond_Always);
                    ImPlot::PlotLine("DBM",dbm.data(),data_count);
                    ImPlot::EndPlot();}ImGui::Spacing();}

            ImGui::Separator();
            ImGui::Spacing();
            ImGui::Text("Latest values:");
            ImGui::Text("LTE RSRP: %.1f dBm",rsrp.back());
            ImGui::Text("LTE RSRQ: %.1f dB",rsrq.back());
            ImGui::Text("LTE RSSNR: %.1f dB",rssnr.back());

            if (!ss_rsrp.empty()){
                ImGui::Text("5G SS-RSRP: %.1f dBm", ss_rsrp.back());
                ImGui::Text("5G SS-RSRQ: %.1f dB", ss_rsrq.back());
                ImGui::Text("5G SS-SINR: %.1f dB", ss_sinr.back());}

            if (!dbm.empty()){ImGui::Text("GSM DBM: %.1f dBm", dbm.back());}

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
        cell.timing_advance_micros = cellJson.value("timing_advance",0);}
    return cell;}

void run_server(){
    zmq::context_t context(1);
    zmq::socket_t socket(context,zmq::socket_type::rep);
    try {
        socket.bind("tcp://*:443");
        std::cout<<"Сервер запущен на порту 443..."<<std::endl;
        std::cout<<"Ожидание данных от Android устройства..."<<std::flush;
        int counter=0;

        while (true){
            try {
                zmq::message_t request;
                socket.set(zmq::sockopt::rcvtimeo, 1000);

                if (socket.recv(request,zmq::recv_flags::none)){
                    std::string received(static_cast<char*>(request.data()),request.size());
                    std::cout<<"Получены данные: "<<received<<std::endl;

                    try {
                        json j=json::parse(received);
                        LocationData newData;
                        if (j.contains("location")&&j["location"].is_object()){
                            auto& loc=j["location"];
                            newData.latitude=loc.value("latitude",0.0);
                            newData.longitude=loc.value("longitude",0.0);
                            newData.altitude=loc.value("altitude",0.0);
                            newData.accuracy=loc.value("accuracy",0.0);
                            long long time_milliseconds=0;
                            if (loc.contains("current_time")){
                                if (loc["current_time"].is_string()){
                                    std::string time_str=loc["current_time"];
                                    time_milliseconds=std::stoll(time_str);
                                }else{time_milliseconds=loc["current_time"].get<long long>();}}

                            newData.time_milliseconds = time_milliseconds;
                            std::time_t time_seconds=static_cast<std::time_t>(time_milliseconds/1000);
                            int milliseconds=static_cast<int>(time_milliseconds%1000);
                            std::stringstream ss;
                            ss<<std::put_time(std::localtime(&time_seconds),"%Y-%m-%d %H:%M:%S");
                            ss<<"."<<std::setfill('0')<<std::setw(3)<<milliseconds;
                            newData.time=ss.str();}

                        if (j.contains("traffic")&&j["traffic"].is_object()){
                            auto& traffic=j["traffic"];
                            newData.traffic.total_rx=traffic.value("total_rx",0LL);
                            newData.traffic.total_tx=traffic.value("total_tx",0LL);
                            newData.traffic.total=traffic.value("total",0LL);}

                        if (j.contains("cells")&&j["cells"].is_array()){
                            for (const auto& cellJson:j["cells"]){newData.cellTowers.push_back(parseCellTower(cellJson));}}
                        {   std::lock_guard<std::mutex> lock(g_locationData.mutex);
                            g_locationData.latitude=newData.latitude;
                            g_locationData.longitude=newData.longitude;
                            g_locationData.altitude=newData.altitude;
                            g_locationData.accuracy=newData.accuracy;
                            g_locationData.time=newData.time;
                            g_locationData.time_milliseconds=newData.time_milliseconds;
                            g_locationData.traffic=newData.traffic;
                            g_locationData.cellTowers=newData.cellTowers;}
                        {   std::lock_guard<std::mutex> lock(g_locationHistory.mutex);
                            g_locationHistory.latitudes.push_back(newData.latitude);
                            g_locationHistory.longitudes.push_back(newData.longitude);
                            g_locationHistory.altitudes.push_back(newData.altitude);
                            g_locationHistory.accuracies.push_back(newData.accuracy);
                            g_locationHistory.times.push_back(newData.time);
                            g_locationHistory.time_milliseconds.push_back(newData.time_milliseconds);
                            g_locationHistory.trafficHistory.push_back(newData.traffic);
                            g_locationHistory.cellTowersHistory.push_back(newData.cellTowers);}
                        {   std::lock_guard<std::mutex> lock(g_signalHistory.mutex);
                            double timestamp=static_cast<double>(newData.time_milliseconds);
                            g_signalHistory.timestamps.push_back(timestamp);

                            bool lte_found=false;
                            for (const auto& cell:newData.cellTowers){
                                if (cell.type=="LTE"){
                                    g_signalHistory.rsrp_values.push_back(static_cast<double>(cell.rsrp));
                                    g_signalHistory.rsrq_values.push_back(static_cast<double>(cell.rsrq));
                                    g_signalHistory.rssnr_values.push_back(static_cast<double>(cell.rssnr));
                                    lte_found = true;
                                    break;}}

                            if (!lte_found){
                                g_signalHistory.rsrp_values.push_back(-140.0);
                                g_signalHistory.rsrq_values.push_back(-20.0);
                                g_signalHistory.rssnr_values.push_back(0.0);}

                            const size_t MAX_HISTORY=50;
                            if (g_signalHistory.timestamps.size()>MAX_HISTORY){
                                g_signalHistory.timestamps.erase(g_signalHistory.timestamps.begin());
                                g_signalHistory.rsrp_values.erase(g_signalHistory.rsrp_values.begin());
                                g_signalHistory.rsrq_values.erase(g_signalHistory.rsrq_values.begin());
                                g_signalHistory.rssnr_values.erase(g_signalHistory.rssnr_values.begin());}}
                        counter++;
                        saveToJsonFile(newData,counter);
                        std::string response="OK";
                        zmq::message_t reply(response.size());
                        memcpy(reply.data(),response.c_str(),response.size());
                        socket.send(reply, zmq::send_flags::none);

                    }catch (const json::parse_error& e){
                        std::cerr<<"Ошибка обработки JSON: "<<e.what()<<std::endl;
                        std::string response="Неправильный JSON";
                        zmq::message_t reply(response.size());
                        memcpy(reply.data(),response.c_str(),response.size());
                        socket.send(reply, zmq::send_flags::none);}}
            } catch (const zmq::error_t& e){
                if (e.num()!=EAGAIN){std::cerr<<"Ошибка ZMQ: "<<e.what()<<std::endl;}}
            std::this_thread::sleep_for(std::chrono::milliseconds(10));}
    }catch (const std::exception& e){
        std::cerr<<"Ошибка сервера: "<<e.what()<<std::endl;}
    socket.close();
    context.close();}

int main(int argc, char *argv[]) {
//   std::thread gui_thread(run_gui, &g_locationData);
    std::thread server_thread(run_server);
//    gui_thread.join();
    server_thread.join();}