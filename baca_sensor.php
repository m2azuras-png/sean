<?php
require 'config.php';
wajib_login();

header('Cache-Control: no-store, no-cache, must-revalidate, max-age=0');
header('Pragma: no-cache');
header('Content-Type: application/json');

$data = $koneksi->query('SELECT berat_badan, tinggi_badan, lingkar_perut, UNIX_TIMESTAMP(updated_at) as last_update, updated_at FROM bacaan_sensor WHERE id=1')->fetch_assoc();
if ($data) {
    $data['server_time'] = time();
    $data['detik_lalu'] = time() - (int)$data['last_update'];
} else {
    $data = [
        'berat_badan' => 0,
        'tinggi_badan' => 0,
        'lingkar_perut' => 0,
        'last_update' => 0,
        'server_time' => time(),
        'detik_lalu' => 9999
    ];
}

echo json_encode($data);
