#ifndef HEATMAP_H
#define HEATMAP_H

#include <cstdint>
#include <map>
#include <string>
#include <vector>

enum class HeatmapMetric{
    RSRP=0,
    RSRQ,
    RSSI,
    Altitude};

struct HeatmapSelection{
    bool enabled=true;
    HeatmapMetric metric=HeatmapMetric::RSRP;
    int earfcn=0;
    int radiusMeters=40;
    int displayRadiusMeters=400;
    double power=2.0;
    float opacity=0.55f;};

class HeatmapData{
public:
    bool load();
    bool isLoaded() const;
    std::vector<int> getAvailableEarfcns(HeatmapMetric metric) const;
    std::string getTilePath(const HeatmapSelection& selection,int zoom,int x,int y) const;
    bool renderTile(const HeatmapSelection& selection,int zoom,int x,int y,
                    std::vector<uint8_t>& rgbaData, int& width, int& height) const;
    static const char* metricName(HeatmapMetric metric);

private:
    struct Sample{
        double latitude=0.0;
        double longitude=0.0;
        double value=0.0;
        double accuracyMeters=0.0;};

    struct MetricConfig{
        double minValue=0.0;
        double maxValue=1.0;
        double transparentBelow=0.0;
        bool useDynamicRange=false;};

    struct DynamicRange{
        double minValue=0.0;
        double maxValue=1.0;};

    bool loadFromDatabase();
    static MetricConfig getMetricConfig(HeatmapMetric metric);
    static double haversineMeters(double lat1, double lon1, double lat2, double lon2);
    static double tilePixelToLongitude(int zoom, int tileX, int pixelX, int tileSize);
    static double tilePixelToLatitude(int zoom, int tileY, int pixelY, int tileSize);
    static double tileTopLatitude(int zoom, int tileY);
    static double tileBottomLatitude(int zoom, int tileY);
    static double tileLeftLongitude(int zoom, int tileX);
    static double tileRightLongitude(int zoom, int tileX);
    static void setPixel(std::vector<uint8_t>& rgbaData, int width, int x, int y, uint8_t r, uint8_t g, uint8_t b, uint8_t a);
    static void colorize(double ratio, float opacity, uint8_t& r, uint8_t& g, uint8_t& b, uint8_t& a);

    std::map<HeatmapMetric, std::map<int, std::vector<Sample>>> m_samples;
    std::map<HeatmapMetric, std::map<int, DynamicRange>> m_dynamicRanges;
    bool m_loaded=false;};

#endif
