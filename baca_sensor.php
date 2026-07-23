<?php
require 'config.php';
wajib_login();
header('Content-Type: application/json');

$data = $koneksi->query('SELECT berat_badan, tinggi_badan, lingkar_perut, updated_at FROM bacaan_sensor WHERE id=1')->fetch_assoc();
echo json_encode($data);
