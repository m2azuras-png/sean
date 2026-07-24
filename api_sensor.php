<?php
require 'config.php';
header('Content-Type: application/json');

if ($_SERVER['REQUEST_METHOD'] !== 'POST') {
    http_response_code(405);
    echo json_encode(['status' => 'error', 'pesan' => 'Method tidak diizinkan, gunakan POST.']);
    exit;
}

$input = json_decode(file_get_contents('php://input'), true);
if (!$input) {
    $input = $_POST;
}

if (($input['api_key'] ?? '') !== ESP32_API_KEY) {
    http_response_code(401);
    echo json_encode(['status' => 'error', 'pesan' => 'API key tidak valid.']);
    exit;
}

$berat = $input['berat_badan'] ?? null;
$tinggi = $input['tinggi_badan'] ?? null;
$lingkar = $input['lingkar_perut'] ?? null;

if ($berat === null || $tinggi === null || $lingkar === null) {
    http_response_code(400);
    echo json_encode(['status' => 'error', 'pesan' => 'Field berat_badan, tinggi_badan, lingkar_perut wajib diisi.']);
    exit;
}

$stmt = $koneksi->prepare('INSERT INTO bacaan_sensor (id, berat_badan, tinggi_badan, lingkar_perut) VALUES (1, ?, ?, ?) ON DUPLICATE KEY UPDATE berat_badan=VALUES(berat_badan), tinggi_badan=VALUES(tinggi_badan), lingkar_perut=VALUES(lingkar_perut)');
$stmt->bind_param('ddd', $berat, $tinggi, $lingkar);
$stmt->execute();

echo json_encode([
    'status' => 'ok',
    'pesan' => 'Data sensor berhasil tersimpan.',
    'data' => [
        'berat_badan' => (float)$berat,
        'tinggi_badan' => (float)$tinggi,
        'lingkar_perut' => (float)$lingkar
    ]
]);
