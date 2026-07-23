<?php
require 'config.php';
wajib_login();

$anak_id = $_GET['anak_id'] ?? '';

$sql = '
    SELECT p.*, a.nama, a.jenis_kelamin
    FROM pengukuran p
    JOIN data_anak a ON a.id = p.anak_id
';
if ($anak_id !== '') {
    $sql .= ' WHERE p.anak_id = ?';
}
$sql .= ' ORDER BY p.tanggal_pengukuran DESC, p.created_at DESC';

$stmt = $koneksi->prepare($sql);
if ($anak_id !== '') {
    $stmt->bind_param('i', $anak_id);
}
$stmt->execute();
$riwayat = $stmt->get_result();

$daftar_anak = $koneksi->query('SELECT id, nama FROM data_anak ORDER BY nama');
?>
<!DOCTYPE html>
<html lang="id">
<head>
<meta charset="UTF-8">
<title>Riwayat Pengukuran - Pemantauan Pertumbuhan Anak</title>
<link rel="stylesheet" href="style.css">
</head>
<body>
<?php include 'navbar.php'; ?>
<div class="container">
  <div class="card">
    <div class="toolbar">
      <h3 style="margin:0">Riwayat Pengukuran</h3>
      <div class="no-print">
        <form method="get" style="display:flex; gap:8px; align-items:center;">
          <select name="anak_id" onchange="this.form.submit()">
            <option value="">-- Semua Anak --</option>
            <?php while ($a = $daftar_anak->fetch_assoc()): ?>
            <option value="<?= $a['id'] ?>" <?= ($anak_id == $a['id']) ? 'selected' : '' ?>><?= htmlspecialchars($a['nama']) ?></option>
            <?php endwhile; ?>
          </select>
          <a href="riwayat.php<?= $anak_id !== '' ? '?anak_id='.$anak_id : '' ?>" class="btn btn-secondary">Refresh</a>
          <button type="button" onclick="window.print()">Cetak</button>
        </form>
      </div>
    </div>
    <table>
      <tr>
        <th>Nama</th><th>JK</th><th>BB (kg)</th><th>TB (cm)</th><th>Lingkar Perut (cm)</th><th>BMI</th><th>Status Gizi</th><th>Tanggal</th>
      </tr>
      <?php while ($row = $riwayat->fetch_assoc()): ?>
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
    </table>
  </div>
</div>
</body>
</html>
