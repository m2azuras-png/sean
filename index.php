<?php
require 'config.php';
wajib_login();

$total_anak = $koneksi->query('SELECT COUNT(*) c FROM data_anak')->fetch_assoc()['c'];
$total_pengukuran = $koneksi->query('SELECT COUNT(*) c FROM pengukuran')->fetch_assoc()['c'];
$total_gemuk = $koneksi->query("SELECT COUNT(*) c FROM pengukuran WHERE status_gizi = 'Gemuk'")->fetch_assoc()['c'];
$total_obesitas = $koneksi->query("SELECT COUNT(*) c FROM pengukuran WHERE status_gizi = 'Obesitas'")->fetch_assoc()['c'];

$terbaru = $koneksi->query('
    SELECT p.*, a.nama, a.jenis_kelamin
    FROM pengukuran p
    JOIN data_anak a ON a.id = p.anak_id
    ORDER BY p.created_at DESC
    LIMIT 8
');
?>
<!DOCTYPE html>
<html lang="id">
<head>
<meta charset="UTF-8">
<title>Dashboard - Pemantauan Pertumbuhan Anak</title>
<link rel="stylesheet" href="style.css">
</head>
<body>
<?php include 'navbar.php'; ?>
<div class="container">
  <div class="cards-row">
    <div class="stat-card"><div class="num"><?= $total_anak ?></div><div class="label">Total Anak</div></div>
    <div class="stat-card"><div class="num"><?= $total_pengukuran ?></div><div class="label">Total Pengukuran</div></div>
    <div class="stat-card"><div class="num"><?= $total_gemuk ?></div><div class="label">Status Gemuk</div></div>
    <div class="stat-card"><div class="num"><?= $total_obesitas ?></div><div class="label">Status Obesitas</div></div>
  </div>

  <div class="card">
    <h3>Pengukuran Terbaru</h3>
    <table>
      <tr>
        <th>Nama</th><th>JK</th><th>BB (kg)</th><th>TB (cm)</th><th>Lingkar Perut (cm)</th><th>BMI</th><th>Status Gizi</th><th>Tanggal</th>
      </tr>
      <?php while ($row = $terbaru->fetch_assoc()): ?>
      <tr>
        <td><?= htmlspecialchars($row['nama']) ?></td>
        <td><?= $row['jenis_kelamin'] ?></td>
        <td><?= $row['berat_badan'] ?></td>
        <td><?= $row['tinggi_badan'] ?></td>
        <td><?= $row['lingkar_perut'] ?></td>
        <td><?= $row['bmi'] ?></td>
        <td><span class="tag tag-<?= strtolower($row['status_gizi']) ?>"><?= $row['status_gizi'] ?></span></td>
        <td><?= date('d-m-Y', strtotime($row['tanggal_pengukuran'])) ?></td>
      </tr>
      <?php endwhile; ?>
      <?php if ($total_pengukuran == 0): ?>
      <tr><td colspan="8">Belum ada data pengukuran.</td></tr>
      <?php endif; ?>
    </table>
  </div>
</div>
</body>
</html>
