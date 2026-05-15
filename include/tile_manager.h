#ifndef TILE_MANAGER_H
#define TILE_MANAGER_H

#include <GL/glew.h>
#include <map>
#include <queue>
#include <string>
#include <mutex>
#include <vector>
#include <cstdint>
#include "heatmap.h"

struct TileJob{
    std::string id;
    int zoom;
    int x;
    int y;};

struct TextureData{
    GLuint id=0;
    bool isLoading=false;
    bool isMissing=false;
    std::vector<uint8_t> rgbaBlob;
    int width=0;
    int height=0;};

class TileManager{
public:
    TileManager();
    ~TileManager();

    void update();
    int getZoomForLimits(double minLon,double maxLon);
    void renderTiles(double minX,double maxX,double minY,double maxY);
    void clearQueue();
    void setHeatmapSelection(const HeatmapSelection& selection);
    std::vector<int> getAvailableHeatmapEarfcns(HeatmapMetric metric) const;
    void clearHeatmapCache();

private:
    std::string getTilePath(int zoom,int x,int y);
    bool saveTileToDisk(const std::string& path,const std::vector<uint8_t>& pngData);
    bool loadTileFromDisk(const std::string& path,std::vector<uint8_t>& pngData);
    bool decodePngToRgba(const std::vector<uint8_t>& pngData,std::vector<uint8_t>& rgbaData,int& width,int& height);
    bool saveRgbaTileToDisk(const std::string& path,const std::vector<uint8_t>& rgbaData,int width,int height);
    GLuint getHeatmapTextureId(int zoom,int x,int y);
    std::map<std::string, TextureData> m_tileCache;
    std::map<std::string, TextureData> m_heatmapCache;
    std::queue<TileJob> m_jobQueue;
    std::mutex m_jobMutex;
    std::mutex m_cacheMutex;
    int m_currentZoom=-1;
    bool m_running;
    HeatmapData m_heatmapData;
    HeatmapSelection m_heatmapSelection;

    double mercatorXToTileX(double mercatorX,int zoom);
    double mercatorYToTileY(double mercatorY,int zoom);
    double tileXToMercatorX(int tileX,int zoom);
    double tileYToMercatorY(int tileY,int zoom);
    void fetchWorker();
    bool downloadTile(int zoom,int x,int y,std::vector<uint8_t>& rgbaData,int& width,int& height);};

#endif