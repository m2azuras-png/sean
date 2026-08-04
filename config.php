<?php
session_start();

$host = 'localhost';
$user = 'u114571555_sean';
$pass = 'Susjol123';
$db   = 'u114571555_sean';

$koneksi = new mysqli($host, $user, $pass, $db);
if ($koneksi->connect_error) {
    die('Koneksi database gagal: ' . $koneksi->connect_error);
}
$koneksi->set_charset('utf8mb4');

define('ESP32_API_KEY', 'Tmbgn-Pertamina-XYZ998');

function hitung_usia($tanggal_lahir) {
    $lahir = new DateTime($tanggal_lahir);
    $sekarang = new DateTime();
    return $lahir->diff($sekarang)->y;
}

function hitung_status_gizi($usia, $bmi) {
    // Penentuan Status Gizi berbasis Indeks BMI (Body Mass Index / IMT)
    // Jika usia anak berada di rentang 6-12 tahun:
    if ($usia !== null && $usia >= 6 && $usia <= 12) {
        $ambang_anak = [
            6  => ['kurus' => 14.0, 'normal' => 17.0, 'gemuk' => 18.5],
            7  => ['kurus' => 14.0, 'normal' => 17.5, 'gemuk' => 19.0],
            8  => ['kurus' => 14.5, 'normal' => 18.0, 'gemuk' => 19.5],
            9  => ['kurus' => 14.5, 'normal' => 18.5, 'gemuk' => 20.5],
            10 => ['kurus' => 15.0, 'normal' => 19.0, 'gemuk' => 21.5],
            11 => ['kurus' => 15.5, 'normal' => 19.5, 'gemuk' => 22.5],
            12 => ['kurus' => 16.0, 'normal' => 20.5, 'gemuk' => 24.0],
        ];
        $t = $ambang_anak[(int)$usia];
        if ($bmi < $t['kurus'])    return 'Kurus';
        if ($bmi <= $t['normal'])  return 'Normal';
        if ($bmi <= $t['gemuk'])   return 'Gemuk';
        return 'Obesitas';
    }

    // Klasifikasi standar Indeks BMI / IMT (Kemenkes RI / WHO)
    if ($bmi < 17.0) {
        return 'Sangat Kurus';
    } elseif ($bmi < 18.5) {
        return 'Kurus';
    } elseif ($bmi <= 25.0) {
        return 'Normal';
    } elseif ($bmi <= 27.0) {
        return 'Gemuk';
    } else {
        return 'Obesitas';
    }
}

function wajib_login() {
    if (!isset($_SESSION['petugas_id'])) {
        header('Location: login.php');
        exit;
    }
}
