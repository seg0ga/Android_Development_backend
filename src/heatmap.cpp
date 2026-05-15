#include "heatmap.h"
#include "database.h"
#include <algorithm>
#include <cmath>
#include <limits>

namespace {

constexpr int kTileSize=256;
constexpr double kEarthRadiusMeters=6371000.0;
constexpr double kPi=3.14159265358979323846;
constexpr int kInvalidInt=2147483647;

double clamp01(double value){
    if (value<0.0) return 0.0;
    if (value>1.0) return 1.0;
    return value;}

bool isFiniteNumber(double value){
    return std::isfinite(value);}

double smoothstep(double edge0,double edge1,double x){
    const double t=clamp01((x-edge0)/(edge1-edge0));
    return t*t*(3.0-2.0*t);}

}

bool HeatmapData::load(){
    if (m_loaded) return true;
    m_loaded=loadFromDatabase();
    return m_loaded;}

bool HeatmapData::isLoaded() const{
    return m_loaded;}

std::vector<int> HeatmapData::getAvailableEarfcns(HeatmapMetric metric) const{
    std::vector<int> result;
    auto metricIt=m_samples.find(metric);
    if (metricIt==m_samples.end()) return result;
    for (const auto& [earfcn,samples]:metricIt->second){
        if (!samples.empty()) result.push_back(earfcn);}
    return result;}

std::string HeatmapData::getTilePath(const HeatmapSelection& selection,int zoom,int x,int y) const{
    return "build/heatmap/v3/"+std::string(metricName(selection.metric))+"/"+
           std::to_string(selection.earfcn)+"/"+
           "idw_"+std::to_string(selection.radiusMeters)+"_draw_"+std::to_string(selection.displayRadiusMeters)+"/"+
           std::to_string(zoom)+"/"+std::to_string(x)+"/"+std::to_string(y)+".png";}

bool HeatmapData::renderTile(const HeatmapSelection& selection,int zoom,int x,int y,
                             std::vector<uint8_t>& rgbaData,int& width,int& height) const{
    width=kTileSize;
    height=kTileSize;
    rgbaData.assign(width*height*4,0);

    auto metricIt=m_samples.find(selection.metric);
    if (metricIt==m_samples.end()) return false;
    auto earfcnIt=metricIt->second.find(selection.earfcn);
    if (earfcnIt==metricIt->second.end()||earfcnIt->second.empty()) return false;

    const auto& samples=earfcnIt->second;
    MetricConfig config=getMetricConfig(selection.metric);
    DynamicRange dynamicRange{config.minValue,config.maxValue};

    if (config.useDynamicRange){
        auto rangeMetricIt=m_dynamicRanges.find(selection.metric);
        if (rangeMetricIt==m_dynamicRanges.end()) return false;
        auto rangeIt=rangeMetricIt->second.find(selection.earfcn);
        if (rangeIt==rangeMetricIt->second.end()) return false;
        dynamicRange=rangeIt->second;}

    const double minLat=tileBottomLatitude(zoom,y);
    const double maxLat=tileTopLatitude(zoom,y);
    const double minLon=tileLeftLongitude(zoom,x);
    const double maxLon=tileRightLongitude(zoom,x);
    const double centerLat=(minLat+maxLat)*0.5;
    double maxSampleRadius=std::max<double>(1.0,selection.displayRadiusMeters);
    for (const auto& sample:samples){
        if (isFiniteNumber(sample.accuracyMeters)&&sample.accuracyMeters>0.0){
            maxSampleRadius=std::max(maxSampleRadius,sample.accuracyMeters);}}
    const double latMargin=maxSampleRadius/111320.0;
    const double lonDenominator=std::max(0.1,std::cos(centerLat*kPi/180.0));
    const double lonMargin=maxSampleRadius/(111320.0*lonDenominator);

    std::vector<const Sample*> candidates;
    candidates.reserve(samples.size());
    for (const auto& sample:samples){
        if (sample.latitude<minLat-latMargin||sample.latitude>maxLat+latMargin) continue;
        if (sample.longitude<minLon-lonMargin||sample.longitude>maxLon+lonMargin) continue;
        candidates.push_back(&sample);}
    if (candidates.empty()) return false;

    bool hasVisiblePixels=false;
    for (int pixelY=0;pixelY<height;++pixelY){
        const double latitude=tilePixelToLatitude(zoom,y,pixelY,height);
        for (int pixelX=0;pixelX<width;++pixelX){
            const double longitude=tilePixelToLongitude(zoom,x,pixelX,width);
            double weightedSum=0.0;
            double totalWeight=0.0;
            double influenceSum=0.0;
            double nearestEdgeRatio=1.0;
            bool directHit=false;
            double interpolatedValue=0.0;
            bool hasSamplesInDisplayRadius=false;

            for (const Sample* sample:candidates){
                const double distance=haversineMeters(latitude,longitude,sample->latitude,sample->longitude);
                const double sampleRadius=
                    (isFiniteNumber(sample->accuracyMeters)&&sample->accuracyMeters>0.0)
                    ? sample->accuracyMeters
                    : std::max<double>(1.0,selection.displayRadiusMeters);
                if (distance>sampleRadius) continue;
                nearestEdgeRatio=std::min(nearestEdgeRatio,distance/std::max(1.0,sampleRadius));
                hasSamplesInDisplayRadius=true;
                influenceSum+=std::max(0.0,1.0-distance/sampleRadius);

                if (distance<0.5){
                    interpolatedValue=sample->value;
                    directHit=true;
                    break;}

                const double softenedDistance=std::max(1.0,distance);
                const double radialWeight=std::max(0.05,1.0-distance/sampleRadius);
                const double weight=radialWeight/std::pow(softenedDistance,std::max(1.0,selection.power*0.7));
                weightedSum+=sample->value*weight;
                totalWeight+=weight;}

            if (!directHit){
                if (totalWeight<=0.0) continue;
                interpolatedValue=weightedSum/totalWeight;}

            if (!isFiniteNumber(interpolatedValue)) continue;
            if (!config.useDynamicRange&&interpolatedValue<=config.transparentBelow) continue;
            const double valueRange=dynamicRange.maxValue-dynamicRange.minValue;
            if (valueRange<=0.0) continue;
            if (!hasSamplesInDisplayRadius) continue;

            const double ratio=clamp01((interpolatedValue-dynamicRange.minValue)/valueRange);
            const double edgeFade=1.0-smoothstep(0.35,1.0,nearestEdgeRatio);
            const double influenceFade=clamp01(influenceSum/2.0);
            uint8_t r=0,g=0,b=0,a=0;
            colorize(ratio,static_cast<float>(selection.opacity*edgeFade*influenceFade),r,g,b,a);
            setPixel(rgbaData,width,pixelX,pixelY,r,g,b,a);
            hasVisiblePixels=true;}}

    return hasVisiblePixels;}

const char* HeatmapData::metricName(HeatmapMetric metric){
    switch (metric){
        case HeatmapMetric::RSRP: return "RSRP";
        case HeatmapMetric::RSRQ: return "RSRQ";
        case HeatmapMetric::RSSI: return "RSSI";
        case HeatmapMetric::Altitude: return "Altitude";}
    return "RSRP";}

bool HeatmapData::loadFromDatabase(){
    m_samples.clear();
    m_dynamicRanges.clear();
    if (!g_database.isConnected()) return false;

    const std::vector<HeatmapDbRow> rows=g_database.getHeatmapRows();
    if (rows.empty()) return false;

    for (const auto& row:rows){
        const double latitude=row.latitude;
        const double longitude=row.longitude;
        const double accuracy=row.accuracy;
        const double altitude=row.altitude;
        if (!isFiniteNumber(latitude)||!isFiniteNumber(longitude)) continue;
        if (latitude<-90.0||latitude>90.0||longitude<-180.0||longitude>180.0) continue;

        const int earfcn=row.earfcn;
        if (earfcn<=0||earfcn==kInvalidInt) continue;

        if (row.rsrp!=kInvalidInt){m_samples[HeatmapMetric::RSRP][earfcn].push_back({latitude,longitude,static_cast<double>(row.rsrp),accuracy});}
        if (row.rsrq!=kInvalidInt){m_samples[HeatmapMetric::RSRQ][earfcn].push_back({latitude,longitude,static_cast<double>(row.rsrq),accuracy});}
        if (row.rssi!=kInvalidInt){m_samples[HeatmapMetric::RSSI][earfcn].push_back({latitude,longitude,static_cast<double>(row.rssi),accuracy});}

        m_samples[HeatmapMetric::Altitude][earfcn].push_back({latitude,longitude,altitude,accuracy});
        auto& altitudeRanges=m_dynamicRanges[HeatmapMetric::Altitude];
        auto rangeIt=altitudeRanges.find(earfcn);
        if (rangeIt==altitudeRanges.end()){
            altitudeRanges[earfcn]={altitude,altitude};
        }else{
            auto& range=rangeIt->second;
            range.minValue=std::min(range.minValue,altitude);
            range.maxValue=std::max(range.maxValue,altitude);}}

    return !m_samples.empty();}

HeatmapData::MetricConfig HeatmapData::getMetricConfig(HeatmapMetric metric){
    switch (metric){
        case HeatmapMetric::RSRP: return {-110.0,-80.0,-110.0,false};
        case HeatmapMetric::RSRQ: return {-20.0,-3.0,-20.0,false};
        case HeatmapMetric::RSSI: return {-110.0,-65.0,-110.0,false};
        case HeatmapMetric::Altitude: return {0.0,1.0,std::numeric_limits<double>::lowest(),true};}
    return {-110.0,-80.0,-110.0,false};}

double HeatmapData::haversineMeters(double lat1,double lon1,double lat2,double lon2){
    const double lat1Rad=lat1*kPi/180.0;
    const double lat2Rad=lat2*kPi/180.0;
    const double deltaLat=(lat2-lat1)*kPi/180.0;
    const double deltaLon=(lon2-lon1)*kPi/180.0;
    const double sinLat=std::sin(deltaLat*0.5);
    const double sinLon=std::sin(deltaLon*0.5);
    const double a=sinLat*sinLat+std::cos(lat1Rad)*std::cos(lat2Rad)*sinLon*sinLon;
    const double c=2.0*std::atan2(std::sqrt(a),std::sqrt(1.0-a));
    return kEarthRadiusMeters*c;}

double HeatmapData::tilePixelToLongitude(int zoom,int tileX,int pixelX,int tileSize){
    const double tilesCount=static_cast<double>(1<<zoom);
    const double x=(static_cast<double>(tileX)+(pixelX+0.5)/tileSize)/tilesCount;
    return x*360.0-180.0;}

double HeatmapData::tilePixelToLatitude(int zoom,int tileY,int pixelY,int tileSize){
    const double tilesCount=static_cast<double>(1<<zoom);
    const double y=(static_cast<double>(tileY)+(pixelY+0.5)/tileSize)/tilesCount;
    const double n=kPi-2.0*kPi*y;
    return 180.0/kPi*std::atan(std::sinh(n));}

double HeatmapData::tileTopLatitude(int zoom,int tileY){
    const double tilesCount=static_cast<double>(1<<zoom);
    const double y=static_cast<double>(tileY)/tilesCount;
    const double n=kPi-2.0*kPi*y;
    return 180.0/kPi*std::atan(std::sinh(n));}

double HeatmapData::tileBottomLatitude(int zoom,int tileY){
    const double tilesCount=static_cast<double>(1<<zoom);
    const double y=static_cast<double>(tileY+1)/tilesCount;
    const double n=kPi-2.0*kPi*y;
    return 180.0/kPi*std::atan(std::sinh(n));}

double HeatmapData::tileLeftLongitude(int zoom,int tileX){
    const double tilesCount=static_cast<double>(1<<zoom);
    const double x=static_cast<double>(tileX)/tilesCount;
    return x*360.0-180.0;}

double HeatmapData::tileRightLongitude(int zoom,int tileX){
    const double tilesCount=static_cast<double>(1<<zoom);
    const double x=static_cast<double>(tileX+1)/tilesCount;
    return x*360.0-180.0;}

void HeatmapData::setPixel(std::vector<uint8_t>& rgbaData,int width,int x,int y,
                           uint8_t r,uint8_t g,uint8_t b,uint8_t a){
    const int index=(y*width+x)*4;
    rgbaData[index+0]=r;
    rgbaData[index+1]=g;
    rgbaData[index+2]=b;
    rgbaData[index+3]=a;}

void HeatmapData::colorize(double ratio,float opacity,uint8_t& r,uint8_t& g,uint8_t& b,uint8_t& a){
    struct ColorStop{
        double ratio;
        double r;
        double g;
        double b;};

    static const ColorStop stops[]={
        {0.0,10.0,20.0,120.0},
        {0.35,0.0,120.0,255.0},
        {0.7,255.0,200.0,0.0},
        {1.0,255.0,0.0,0.0}};

    const double normalizedRatio=clamp01(ratio);
    const ColorStop* left=&stops[0];
    const ColorStop* right=&stops[3];
    for (int i=1;i<4;++i){
        if (normalizedRatio<=stops[i].ratio){
            left=&stops[i-1];
            right=&stops[i];
            break;}}

    const double segment=right->ratio-left->ratio;
    const double localRatio=segment>0.0?(normalizedRatio-left->ratio)/segment:0.0;
    r=static_cast<uint8_t>(left->r+(right->r-left->r)*localRatio);
    g=static_cast<uint8_t>(left->g+(right->g-left->g)*localRatio);
    b=static_cast<uint8_t>(left->b+(right->b-left->b)*localRatio);
    a=static_cast<uint8_t>(255.0f*opacity);}
