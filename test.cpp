#include <sstream>
#include <iostream>
#include <chrono>
#include <vector>
#include <future>
#include <memory>
#include <math.h>
#include <numbers>
#include <algorithm>

#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <stb_image.h>
#include <curl/curl.h>

#include "backends/imgui_impl_opengl3.h"
#include "backends/imgui_impl_sdl2.h"
#include "imgui.h"
#include "implot.h"



// struct FutureData {
//     std::shared_ptr<ITile> tile;
// };

static const ImPlotAxisFlags _xFlags{
    ImPlotAxisFlags_NoLabel | ImPlotAxisFlags_NoGridLines |
    ImPlotAxisFlags_NoTickMarks | ImPlotAxisFlags_NoTickLabels |
    ImPlotAxisFlags_NoInitialFit | ImPlotAxisFlags_NoMenus |
    ImPlotAxisFlags_NoMenus | ImPlotAxisFlags_NoHighlight};

static const ImPlotAxisFlags _yFlags{_xFlags |
                                                ImPlotAxisFlags_Invert};

double PI{std::numbers::pi_v<double>};
double PI2{std::numbers::pi_v<double> * 2.0};
double RAD{std::numbers::pi_v<double> / 180.0};
double DEG{180.0 / std::numbers::pi_v<double>};

int POW2[]{
    (1 << 0),  (1 << 1),  (1 << 2),  (1 << 3),  (1 << 4),  (1 << 5),  (1 << 6),
    (1 << 7),  (1 << 8),  (1 << 9),  (1 << 10), (1 << 11), (1 << 12), (1 << 13),
    (1 << 14), (1 << 15), (1 << 16), (1 << 17), (1 << 18)};


double lon2x(const double lon, int z = 0) {
  return (lon + 180.0) / 360.0 * double(POW2[z]);
}

double lat2y(const double lat, int z = 0) {
  return (1.0 - asinh(tan(lat * RAD)) / PI) / 2.0 * double(POW2[z]);
}

double x2lon(const double x, int z = 0) {
  return x / double(POW2[z]) * 360.0 - 180.0;
}

double y2lat(const double y, const int z = 0) {
  const double n{PI - PI2 * y / double(POW2[z])};
  return DEG * atan(0.5 * (exp(n) - exp(-n)));
}


double _minLat{-85.0};
double _maxLat{+85.0};
double _minLon{-179.9};
double _maxLon{+179.9};
int MinZoom{0};
int MaxZoom{18};


double _minX = lon2x(_minLon, 0);
double _maxX = lon2x(_maxLon, 0);
double _minY = lat2y(_minLat, 0);
double _maxY = lat2y(_maxLat, 0);

ImPlotPoint _mousePos{};
ImPlotRect _plotLims{};
ImVec2 _plotSize{};

int _width{256}, _height{256}, _channels{};
std::vector<unsigned char> _rawBlob;
unsigned char *data;
GLuint _id{0};

bool loaded = false;

void glLoad(){
  glGenTextures(1, &_id);
  glBindTexture(GL_TEXTURE_2D, _id);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, _width, _height, 0, GL_RGBA,
                GL_UNSIGNED_BYTE, data);
}

void stbLoad() {
  data = stbi_load_from_memory(_rawBlob.data(), _rawBlob.size(), &_width, &_height, &_channels, STBI_rgb_alpha);
}
//https://tiles.api-maps.yandex.ru/v1/tiles/?scale=2&x=9902&y=513714&lang=ru_RU&l=map&apikey=cc1e5c4a-192f-49ac-9f64-683d0c86fa47

std::string makeUrl(int z, int x, int y) {
  std::ostringstream urlmaker;
  urlmaker << "https://tiles.api-maps.yandex.ru/v1/tiles/?scale=2&x=";
  urlmaker << x << '&y=' << y << "&lang=ru_RU&l=map&apikey=cc1e5c4a-192f-49ac-9f64-683d0c86fa47";
  return urlmaker.str();
}

size_t onPullResponse(void *data, size_t size, size_t nmemb,
                                     void *userp) {
  size_t realsize{size * nmemb};
  auto &blob{*static_cast<std::vector<unsigned char> *>(userp)};
  auto const *const dataptr{static_cast<unsigned char *>(data)};
  blob.insert(blob.cend(), dataptr, dataptr + realsize);
  std::cout << "tile size = " << realsize << std::endl;
  return realsize;
}

bool receiveTile(int z, int x, int y,
                                std::vector<unsigned char> &blob) {
  CURL *curl{curl_easy_init()};
  curl_easy_setopt(curl, CURLOPT_URL, makeUrl(z, x, y).c_str());
  curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
  // curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
  curl_easy_setopt(curl, CURLOPT_USERAGENT, "curl");
  curl_easy_setopt(curl, CURLOPT_TIMEOUT, 1);
  curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 1);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&blob);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, onPullResponse);
  const bool ok{curl_easy_perform(curl) == CURLE_OK};
  curl_easy_cleanup(curl);
  loaded = true;
  return ok;
}

std::vector<unsigned char> tileRequest(int z, int x, int y) {
  std::vector<unsigned char> blob;
  if (receiveTile(z, x, y, blob)) {
      std::cout << "tile is received" << std::endl;
      // тут в blob лежат байтики после получения тайлов
  } else {
    std::cout << "tile is not received" << std::endl;
    // тут Dummy байтики
  }
   return blob;
}

int main(){

      SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER);
    SDL_Window* window = SDL_CreateWindow(
        "Backend start", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        1024, 768, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    SDL_GLContext gl_context = SDL_GL_CreateContext(window);

    ImGui::CreateContext();
    ImPlot::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Включить Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Включить Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Включить Docking
    // io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;       // Включить Multi-Viewport / Platform Windows. Позволяет работать "окнам" вне основного окна.

    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init("#version 330");

    bool running = true;
    // auto last_frame_time = std::chrono::steady_clock::now();
    while (running) {

        // Обработка event'ов (inputs, window resize, mouse moving, etc.)
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT) {
                running = false;
            }
        }

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();
        ImGui::DockSpaceOverViewport(0, nullptr, ImGuiDockNodeFlags_None);




        ImPlot::BeginPlot("##ImOsmMapPlot");

        if(!loaded){
          std::cout << "min max X = " << _minX << " " << _maxX << std::endl;
          std::cout << "min max y = " << _minY << " " << _maxY << std::endl;
        }
        // Top-left of the texture
        // Bottom-right of the texture
        ImVec2 _uv0{0, 1}, _uv1{1, 0};
        ImVec4 _tint{1, 1, 1, 1};
        ImVec2 bmin{0, 0};
        ImVec2 bmax{256, 256};
        if(!loaded){

          std::cout << "min max X = " << _minX << " " << _maxX << std::endl;
          std::cout << "min max y = " << _minY << " " << _maxY << std::endl;
          _rawBlob = tileRequest(16, 47867, 20726);

          stbLoad();
          glLoad();
        }
        if (loaded)
        {
          ImPlot::PlotImage("##", _id, bmin, bmax, _uv0, _uv1, _tint);
        }

        ImPlot::EndPlot();

        ImGui::Render();
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        SDL_GL_SwapWindow(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();
    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();
  return 0;
}