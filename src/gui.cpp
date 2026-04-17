#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <iostream>
#include "imgui.h"
#include "implot.h"
#include "backends/imgui_impl_opengl3.h"
#include "backends/imgui_impl_sdl2.h"
#include "gui.h"
#include "json.h"
#include "database.h"
#include "common.h"
#include "tile_manager.h"

extern SignalHistory g_signalHistory;

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

void run_gui(LocationData* loc){
	static TileManager g_tileManager;
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
		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();
		ImGui::TextColored(ImVec4(0.00f,0.70f,1.00f,1.00f),"OSM MAP");
		ImGui::Separator();
		ImGui::Spacing();

		if (isValidLocation(*loc)){
		    ImGui::BeginChild("MapContainer",ImVec2(0, 450),true);

		    if (ImPlot::BeginPlot("##OSM_Map",ImVec2(-1, -1))){
		        ImPlot::SetupAxes("Longitude","Latitude");
 		        ImPlot::SetupAxisFormat(ImAxis_X1,"%.6f°");
		        ImPlot::SetupAxisFormat(ImAxis_Y1,"%.6f°");
    		    double lon=loc->longitude;
    		    double lat=loc->latitude;
    		    double offset=0.015;
    		    ImPlot::SetupAxisLimits(ImAxis_X1,lon-offset,lon+offset);
    		    ImPlot::SetupAxisLimits(ImAxis_Y1,lat-offset,lat+offset);
    		    ImPlotRect limits=ImPlot::GetPlotLimits();
     		    g_tileManager.update();

        		if (limits.X.Min>-180&&limits.X.Max<180&&limits.Y.Min>-90&&limits.Y.Max<90){
            		g_tileManager.renderTiles(limits.X.Min,limits.X.Max,limits.Y.Min,limits.Y.Max);}
		        double positions[2]={loc->longitude,loc->latitude};
		        ImPlot::PlotScatter("You are here",positions,positions+1,1);
		        ImPlot::EndPlot();}
		    ImGui::EndChild();
			}else{
		    	ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.00f),
        		"Waiting for location data...");}
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
