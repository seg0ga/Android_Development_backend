#!/bin/bash


set -e

echo "  Сборка Android_Development_backend"

echo ""
echo "[1/4] Проверка зависимостей..."

REQUIRED_PACKAGES=(
    "cmake"
    "g++"
    "libpq-dev"
    "libzmq3-dev"
    "libsdl2-dev"
    "libgl1-mesa-dev"
    "libglew-dev"
    "nlohmann-json3-dev"
    "pkg-config"
)

MISSING_PACKAGES=()
for pkg in "${REQUIRED_PACKAGES[@]}"; do
    if ! dpkg -l | grep -q "^ii  $pkg "; then
        MISSING_PACKAGES+=("$pkg")
    fi
done

if [ ${#MISSING_PACKAGES[@]} -ne 0 ]; then
    echo "  Отсутствуют пакеты: ${MISSING_PACKAGES[*]}"
    echo " Установка через apt..."
    sudo apt update
    sudo apt install -y "${MISSING_PACKAGES[@]}"
else
    echo "✅ Все зависимости установлены"
fi

echo ""
echo "[2/4] Создание директории сборки..."
mkdir -p build
cd build

echo ""
echo "[3/4] Конфигурация CMake..."
cmake ..

echo ""
echo "[4/4] Компиляция..."
make -j$(nproc)

echo ""
echo "  ✅ Сборка завершена успешно!"
echo ""
echo "Запуск: ./build/main"
