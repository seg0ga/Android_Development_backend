CREATE TABLE based (
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

CREATE TABLE cells (
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

CREATE INDEX idx_based_time ON based(time);
CREATE INDEX idx_based_location ON based(latitude, longitude);
CREATE INDEX idx_cells_measurement ON cells(measurement_id);
CREATE INDEX idx_cells_mcc_mnc ON cells(mcc, mnc);
CREATE INDEX idx_cells_type ON cells(type);
