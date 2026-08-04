-- Script SQL Inject Data Pasien dari Rekam Medis
-- Database Target: timbangan_anak
-- Status Kondisi: Kurus, Normal, Gemuk (berdasarkan Index Massa Tubuh pada Rekam Medis)

USE timbangan_anak;

-- 1. ANIISAH ZAKIYAH (IMT: 16 - Underweight -> Kurus)
INSERT INTO data_anak (nama, jenis_kelamin, tanggal_lahir, nis) 
VALUES ('ANIISAH ZAKIYAH', 'P', '2019-03-19', '000019586');

INSERT INTO pengukuran (anak_id, berat_badan, tinggi_badan, lingkar_perut, bmi, status_gizi, tanggal_pengukuran) 
VALUES (LAST_INSERT_ID(), 20.30, 114.0, 0.0, 15.62, 'Kurus', '2026-08-03');

-- 2. MUHAMMAD RAFKA AL FATIH (IMT: 17 - Underweight -> Kurus)
INSERT INTO data_anak (nama, jenis_kelamin, tanggal_lahir, nis) 
VALUES ('MUHAMMAD RAFKA AL FATIH', 'L', '2018-12-06', '000164912');

INSERT INTO pengukuran (anak_id, berat_badan, tinggi_badan, lingkar_perut, bmi, status_gizi, tanggal_pengukuran) 
VALUES (LAST_INSERT_ID(), 26.80, 125.0, 0.0, 17.15, 'Kurus', '2026-08-03');

-- 3. FITRI SHAKILLA AZZAHRA (IMT: 22 - Ideal -> Normal)
INSERT INTO data_anak (nama, jenis_kelamin, tanggal_lahir, nis) 
VALUES ('FITRI SHAKILLA AZZAHRA', 'P', '2017-08-23', '000119502');

INSERT INTO pengukuran (anak_id, berat_badan, tinggi_badan, lingkar_perut, bmi, status_gizi, tanggal_pengukuran) 
VALUES (LAST_INSERT_ID(), 42.20, 138.5, 0.0, 22.00, 'Normal', '2026-08-03');

-- 4. RAFANDI WAHYU RAMADHAN (IMT: 24 - Overweight -> Gemuk)
INSERT INTO data_anak (nama, jenis_kelamin, tanggal_lahir, nis) 
VALUES ('RAFANDI WAHYU RAMADHAN', 'L', '2017-06-19', '000189102');

INSERT INTO pengukuran (anak_id, berat_badan, tinggi_badan, lingkar_perut, bmi, status_gizi, tanggal_pengukuran) 
VALUES (LAST_INSERT_ID(), 44.50, 137.0, 0.0, 23.71, 'Gemuk', '2026-08-03');
