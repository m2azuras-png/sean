CREATE DATABASE IF NOT EXISTS timbangan_anak;
USE timbangan_anak;

CREATE TABLE petugas (
    id INT AUTO_INCREMENT PRIMARY KEY,
    nama VARCHAR(100) NOT NULL,
    email VARCHAR(100) NOT NULL UNIQUE,
    password VARCHAR(255) NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE data_anak (
    id INT AUTO_INCREMENT PRIMARY KEY,
    nama VARCHAR(100) NOT NULL,
    jenis_kelamin ENUM('L','P') NOT NULL,
    tanggal_lahir DATE NOT NULL,
    nis VARCHAR(30) DEFAULT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE pengukuran (
    id INT AUTO_INCREMENT PRIMARY KEY,
    anak_id INT NOT NULL,
    berat_badan DECIMAL(5,2) NOT NULL,
    tinggi_badan DECIMAL(5,1) NOT NULL,
    lingkar_perut DECIMAL(5,1) NOT NULL,
    bmi DECIMAL(5,2) NOT NULL,
    status_gizi VARCHAR(20) NOT NULL,
    tanggal_pengukuran DATE NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (anak_id) REFERENCES data_anak(id) ON DELETE CASCADE
);

CREATE TABLE bacaan_sensor (
    id INT PRIMARY KEY,
    berat_badan DECIMAL(5,2) NOT NULL,
    tinggi_badan DECIMAL(5,1) NOT NULL,
    lingkar_perut DECIMAL(5,1) NOT NULL,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP
);
INSERT INTO bacaan_sensor (id, berat_badan, tinggi_badan, lingkar_perut) VALUES (1, 0, 0, 0);
