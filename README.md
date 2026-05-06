# Android_Development_backend
# Все лабораторные работы по порядковым номерам находятся в ветках, здесь только крайняя лаба...

## Практическая работа №15 "Отрисовка OpenStreetMap карты"

1. Реализовать генерацию тепловой карты на базе метода обратных взвешенных расстояний (IDW).

    - Выполнил вычисление по нескольким критериям: RSRP, RSRQ, RSSI, Altitude (для каждого EARFCN отдельно);

    - Ограничивать радиус вычисления интерполяции (учет экспериментальных значений) в пределах 10 - 40 метров до вычисляемой точки;

    - В ImGui-интерфейсе добавить выбор критерия, по которому будет отображаться тепловая карта.

2. Выполнить вычисление:

    - Либо одной большой картинкой для всех точек;

    - Либо для каждого тайла в отдельности.

    - Рекомендация выполнять вычисления в отдельном потоке.

3. Все вычисленные картинки тепловой карты сохранять в директориях build/zoom/x/y.png (если вычисляете для каждого тайла), било в /build (если одна общая картинка);

4. Обновить github-репозиторий, доработать README.md.

## Критерии по RSRP:
```
Excellent           (>-80 dBm): Strong signal, maximum data speeds.
Good                (-80 to -90 dBm): Reliable data speeds.
Fair/Marginal       (-90 to -100 dBm): Serviceable, but potential for lower throughput or dropouts.
Poor/Weak           (-100 to -110 dBm): Frequent dropped calls or very slow data.
No Signal/Unusable  (<-110 dBm): Connection often fails
```

## Результаты



## Установка зависимостей
```
sudo apt update
sudo apt install -y build-essential cmake pkg-config
sudo apt install -y libsdl2-dev libglew-dev libglu1-mesa-dev
sudo apt install -y libcurl4-openssl-dev
sudo apt install -y libpq-dev postgresql
sudo apt install -y libzmq3-dev
sudo apt install -y libstb-dev
sudo apt install -y nlohmann-json3-dev
```
## Компиляция

```
git clone hhtps://github.com/seg0ga/Android_Development_backend
cd Android_Development_backend
mkdir build
cd build
cmake ..
make -j16
```

## Запуск ПО

```
cd build
sudo ./main
```
