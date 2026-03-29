#!/bin/bash

set -e

echo "  Настройка PostgreSQL"

DB_NAME="mobile_network_db"
DB_USER="postgres"
DB_PASSWORD="postgres1234"

echo ""
echo "[1/3] Проверка PostgreSQL..."
if ! command -v psql &> /dev/null; then
    echo "⚠️  PostgreSQL не найден. Установка..."
    sudo apt update
    sudo apt install -y postgresql postgresql-contrib
fi

echo "✅ PostgreSQL установлен"

echo ""
echo "[2/3] Запуск PostgreSQL..."
sudo service postgresql start

echo ""
echo "[3/3] Создание БД и таблиц..."

sudo -u postgres psql << EOF
DROP DATABASE IF EXISTS ${DB_NAME};
CREATE DATABASE ${DB_NAME};

\\c ${DB_NAME}

CREATE TABLE IF NOT EXISTS based (
    id SERIAL PRIMARY KEY,
    counter INTEGER NOT NULL,
    "current_time" BIGINT NOT NULL,
    time TEXT NOT NULL,
    latitude REAL NOT NULL,
    longitude REAL NOT NULL,
    accuracy REAL,
    altitude REAL,
    traffic_total BIGINT,
    traffic_total_rx BIGINT,
    traffic_total_tx BIGINT
);

CREATE TABLE IF NOT EXISTS cells (
    id SERIAL PRIMARY KEY,
    measurement_id INTEGER NOT NULL,
    type TEXT,
    mcc INTEGER,
    mnc INTEGER,
    cell_identity BIGINT,
    tac INTEGER,
    earfcn INTEGER,
    band INTEGER,
    pci INTEGER,
    rsrp INTEGER,
    rsrq INTEGER,
    rssi INTEGER,
    rssnr INTEGER,
    asu_level INTEGER,
    cqi INTEGER,
    timing_advance INTEGER,
    FOREIGN KEY (measurement_id) REFERENCES based(id)
);


CREATE INDEX IF NOT EXISTS idx_based_time ON based(time);
CREATE INDEX IF NOT EXISTS idx_based_location ON based(latitude, longitude);
CREATE INDEX IF NOT EXISTS idx_cells_measurement ON cells(measurement_id);
CREATE INDEX IF NOT EXISTS idx_cells_mcc_mnc ON cells(mcc, mnc);
CREATE INDEX IF NOT EXISTS idx_cells_type ON cells(type);

ALTER USER ${DB_USER} WITH PASSWORD '${DB_PASSWORD}';

EOF

echo ""
echo "  ✅ База данных настроена!"
echo ""
echo "Параметры подключения:"
echo "  Host:     localhost"
echo "  Port:     5432"
echo "  DB:       ${DB_NAME}"
echo "  User:     ${DB_USER}"
echo "  Password: ${DB_PASSWORD}"
echo ""
