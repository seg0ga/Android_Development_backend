#include "tile_manager.h"
#include <curl/curl.h>
#include <png.h>
#include <thread>
#include <cmath>
#include <iostream>
#include <algorithm>
#include <cstring>
#include <filesystem>
#include <fstream>
#include "implot.h"

size_t WriteCallback(void* contents,size_t size,size_t nmemb,std::string* data){
    size_t totalSize=size*nmemb;
    data->append((char*)contents,totalSize);
    return totalSize;}

TileManager::TileManager():m_running(true){
    curl_global_init(CURL_GLOBAL_DEFAULT);
    std::filesystem::create_directories("build");
    std::thread(&TileManager::fetchWorker,this).detach();}

TileManager::~TileManager(){
    m_running=false;
    curl_global_cleanup();}

std::string TileManager::getTilePath(int zoom,int x,int y){
    std::string path="build/"+std::to_string(zoom)+"/"+std::to_string(x);
    std::filesystem::create_directories(path);
    return path+"/"+std::to_string(y)+".png";}

bool TileManager::saveTileToDisk(const std::string& path,const std::vector<uint8_t>& pngData){
    std::ofstream file(path,std::ios::binary);
    if(!file) return false;
    file.write(reinterpret_cast<const char*>(pngData.data()),pngData.size());
    return true;}

bool TileManager::loadTileFromDisk(const std::string& path,std::vector<uint8_t>& pngData){
    std::ifstream file(path,std::ios::binary|std::ios::ate);
    if(!file) return false;
    size_t size=file.tellg();
    file.seekg(0,std::ios::beg);
    pngData.resize(size);
    file.read(reinterpret_cast<char*>(pngData.data()),size);
    return true;}

double TileManager::mercatorXToTileX(double mercatorX,int zoom){
    return (0.5+mercatorX/360.0)*(1<<zoom);}

double TileManager::mercatorYToTileY(double mercatorY,int zoom){
    double lat_rad=mercatorY*M_PI/180.0;
    double y_normalized=0.5-log(tan(M_PI/4.0+lat_rad/2.0))/(2.0*M_PI);
    return y_normalized*(1<<zoom);}

double TileManager::tileXToMercatorX(int tileX,int zoom){
    return (tileX/static_cast<double>(1<<zoom)-0.5)*360.0;}

double TileManager::tileYToMercatorY(int tileY,int zoom){
    double y_normalized=tileY/static_cast<double>(1<<zoom);
    double lat_rad=atan(sinh(M_PI*(1.0-2.0*y_normalized)));
    return lat_rad*180.0/M_PI;}

int TileManager::getZoomForLimits(double minLon,double maxLon){
    double diff=maxLon-minLon;

    if (diff>90.0) return 4;
    else if (diff>45.0) return 5;
    else if (diff>22.5) return 6;
    else if (diff>11.25) return 7;
    else if (diff>5.625) return 8;
    else if (diff>2.8125) return 9;
    else if (diff>1.40625) return 10;
    else if (diff>0.703125) return 11;
    else if (diff>0.3515625) return 12;
    else if (diff>0.17578125) return 13;
    else if (diff>0.087890625) return 14;
    else return 15;}

void TileManager::clearQueue(){
    std::lock_guard<std::mutex> lock(m_jobMutex);
    while (!m_jobQueue.empty()){
        m_jobQueue.pop();}}

bool TileManager::downloadTile(int zoom,int x,int y,std::vector<uint8_t>& rgbaData,int& width,int& height){
    std::string url="https://tile.openstreetmap.org/"+std::to_string(zoom)+"/"+std::to_string(x)+"/"+std::to_string(y)+".png";
    CURL* curl=curl_easy_init();
    if (!curl) return false;

    std::string response;
    curl_easy_setopt(curl,CURLOPT_URL,url.c_str());
    curl_easy_setopt(curl,CURLOPT_WRITEFUNCTION,WriteCallback);
    curl_easy_setopt(curl,CURLOPT_WRITEDATA,&response);
    curl_easy_setopt(curl,CURLOPT_USERAGENT,"Android_Development_backend");
    curl_easy_setopt(curl,CURLOPT_TIMEOUT,10L);
    CURLcode res=curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    if (res!=CURLE_OK||response.empty()){return false;}
    std::string tilePath=getTilePath(zoom,x,y);
    saveTileToDisk(tilePath,std::vector<uint8_t>(response.begin(),response.end()));
    png_image image;
    memset(&image,0,sizeof(image));
    image.version=PNG_IMAGE_VERSION;

    if (!png_image_begin_read_from_memory(&image,response.data(),response.size())){return false;}
    width=image.width;
    height=image.height;
    image.format=PNG_FORMAT_RGBA;
    rgbaData.resize(PNG_IMAGE_SIZE(image));
    if (!png_image_finish_read(&image,NULL,rgbaData.data(),0,NULL)){return false;}
    png_image_free(&image);
    return true;}

void TileManager::fetchWorker(){
    while (m_running){
        TileJob job;
        bool hasJob=false;

        {   std::lock_guard<std::mutex> lock(m_jobMutex);
            if (!m_jobQueue.empty()){
                job=m_jobQueue.front();
                m_jobQueue.pop();
                hasJob=true;}}

        if (hasJob){
            std::vector<uint8_t> pngData;
            std::string tilePath=getTilePath(job.zoom,job.x,job.y);
            std::vector<uint8_t> rgbaData;
            int width,height;
            if (loadTileFromDisk(tilePath,pngData)){
                png_image image;
                memset(&image,0,sizeof(image));
                image.version=PNG_IMAGE_VERSION;
                if (png_image_begin_read_from_memory(&image,pngData.data(),pngData.size())){
                    width=image.width;
                    height=image.height;
                    image.format=PNG_FORMAT_RGBA;
                    rgbaData.resize(PNG_IMAGE_SIZE(image));
                    if (png_image_finish_read(&image,NULL,rgbaData.data(),0,NULL)){
                        std::lock_guard<std::mutex> lock(m_cacheMutex);
                        auto& tex=m_tileCache[job.id];
                        tex.rgbaBlob=std::move(rgbaData);
                        tex.width=width;
                        tex.height=height;
                        tex.isLoading=false;}
                    png_image_free(&image);}
            }else if(downloadTile(job.zoom,job.x,job.y,rgbaData,width,height)){
                std::lock_guard<std::mutex> lock(m_cacheMutex);
                auto& tex=m_tileCache[job.id];
                tex.rgbaBlob=std::move(rgbaData);
                tex.width=width;
                tex.height=height;
                tex.isLoading=false;
            }else{
                std::lock_guard<std::mutex> lock(m_cacheMutex);
                auto& tex=m_tileCache[job.id];
                tex.isLoading=false;}
        }else{std::this_thread::sleep_for(std::chrono::milliseconds(10));}}}

void TileManager::update(){
    std::lock_guard<std::mutex> lock(m_cacheMutex);
    for (auto& pair:m_tileCache){
        auto& tex=pair.second;
        if (!tex.rgbaBlob.empty()&&tex.id==0){
            glGenTextures(1,&tex.id);
            glBindTexture(GL_TEXTURE_2D,tex.id);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
            glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,tex.width,tex.height,0,GL_RGBA,GL_UNSIGNED_BYTE,tex.rgbaBlob.data());
            tex.rgbaBlob.clear();
            tex.rgbaBlob.shrink_to_fit();}}}

void TileManager::renderTiles(double minX,double maxX,double minY,double maxY){
    int zoom=getZoomForLimits(minX,maxX);

    if (zoom!=m_currentZoom){
        m_currentZoom=zoom;
        clearQueue();}

    int minTileX=static_cast<int>(std::floor(mercatorXToTileX(minX,zoom)));
    int minTileY=static_cast<int>(std::floor(mercatorYToTileY(maxY,zoom)));
    int maxTileX=static_cast<int>(std::floor(mercatorXToTileX(maxX,zoom)));
    int maxTileY=static_cast<int>(std::floor(mercatorYToTileY(minY,zoom)));

    int maxTileCount=(1<<zoom)-1;
    minTileX=std::max(0,minTileX);
    maxTileX=std::min(maxTileCount,maxTileX);
    minTileY=std::max(0,minTileY);
    maxTileY=std::min(maxTileCount,maxTileY);

    for (int x=minTileX;x<=maxTileX;x++){
        for (int y=minTileY;y<=maxTileY;y++){
            std::string tileId=std::to_string(zoom)+"/"+std::to_string(x)+"/"+std::to_string(y);
            GLuint gpuId=0;
            {   std::lock_guard<std::mutex> lock(m_cacheMutex);
                auto it=m_tileCache.find(tileId);
                if (it!=m_tileCache.end()){
                    gpuId=it->second.id;
                    if (it->second.id==0&&!it->second.isLoading&&it->second.rgbaBlob.empty()){
                        it->second.isLoading=true;
                        TileJob job{tileId,zoom,x,y};
                        std::lock_guard<std::mutex> lock2(m_jobMutex);
                        m_jobQueue.push(job);}
                }else{
                    TextureData tex;
                    tex.isLoading=true;
                    m_tileCache[tileId]=tex;
                    TileJob job{tileId,zoom,x,y};
                    std::lock_guard<std::mutex> lock2(m_jobMutex);
                    m_jobQueue.push(job);}}

            if (gpuId!=0){
                ImPlotPoint minPoint{tileXToMercatorX(x,zoom),tileYToMercatorY(y+1,zoom)};
                ImPlotPoint maxPoint{tileXToMercatorX(x+1,zoom),tileYToMercatorY(y,zoom)};
                ImPlot::PlotImage(("##tile_"+tileId).c_str(),(ImTextureID)(intptr_t)gpuId,minPoint,maxPoint);}}}}