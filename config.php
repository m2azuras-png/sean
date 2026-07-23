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

define('ESP32_API_KEY', 'ubah-kunci-ini');

function hitung_usia($tanggal_lahir) {
    $lahir = new DateTime($tanggal_lahir);
    $sekarang = new DateTime();
    return $lahir->diff($sekarang)->y;
}

function hitung_status_gizi($usia, $bmi) {
    $ambang = [
        6  => [17.0, 18.5],
        7  => [17.5, 19.0],
        8  => [18.0, 19.5],
        9  => [18.5, 20.5],
        10 => [19.0, 21.5],
        11 => [19.5, 22.5],
        12 => [20.5, 24.0],
    ];
    if ($usia < 6) $usia = 6;
    if ($usia > 12) $usia = 12;
    [$batas_normal, $batas_gemuk] = $ambang[$usia];

    if ($bmi <= $batas_normal) return 'Normal';
    if ($bmi <= $batas_gemuk) return 'Gemuk';
    return 'Obesitas';
}

function wajib_login() {
    if (!isset($_SESSION['petugas_id'])) {
        header('Location: login.php');
        exit;
    }
}
