<?php
require 'config.php';
wajib_login();

$error = '';
$hasil = null;

if ($_SERVER['REQUEST_METHOD'] === 'POST') {
    $anak_id = $_POST['anak_id'] ?? '';
    $berat = $_POST['berat_badan'] ?? '';
    $tinggi = $_POST['tinggi_badan'] ?? '';
    $lingkar = $_POST['lingkar_perut'] ?? '';
    $tanggal = $_POST['tanggal_pengukuran'] ?? date('Y-m-d');

    if ($anak_id === '' || $berat === '' || $tinggi === '' || $lingkar === '') {
        $error = 'Semua kolom wajib diisi.';
    } else {
        $stmt = $koneksi->prepare('SELECT * FROM data_anak WHERE id=?');
        $stmt->bind_param('i', $anak_id);
        $stmt->execute();
        $anak = $stmt->get_result()->fetch_assoc();

        if (!$anak) {
            $error = 'Data anak tidak ditemukan.';
        } else {
            $tinggi_m = $tinggi / 100;
            $bmi = round($berat / ($tinggi_m * $tinggi_m), 2);
            $usia = hitung_usia($anak['tanggal_lahir']);
            $status_gizi = hitung_status_gizi($usia, $bmi);

            $stmt = $koneksi->prepare('INSERT INTO pengukuran (anak_id, berat_badan, tinggi_badan, lingkar_perut, bmi, status_gizi, tanggal_pengukuran) VALUES (?, ?, ?, ?, ?, ?, ?)');
            $stmt->bind_param('idddsss', $anak_id, $berat, $tinggi, $lingkar, $bmi, $status_gizi, $tanggal);
            $stmt->execute();

            $hasil = [
                'nama' => $anak['nama'],
                'bmi' => $bmi,
                'status_gizi' => $status_gizi,
            ];
        }
    }
}

$daftar_anak = $koneksi->query('SELECT id, nama, nis FROM data_anak ORDER BY nama');
?>
<!DOCTYPE html>
<html lang="id">
<head>
<meta charset="UTF-8">
<title>Input Pengukuran - Pemantauan Pertumbuhan Anak</title>
<link rel="stylesheet" href="style.css">
</head>
<body>
<?php include 'navbar.php'; ?>
<div class="container">
  <div class="card">
    <h3>Input Hasil Pengukuran</h3>
    <?php if ($error): ?><div class="msg-err"><?= htmlspecialchars($error) ?></div><?php endif; ?>
    <?php if ($hasil): ?>
      <div class="msg-ok">
        Pengukuran <?= htmlspecialchars($hasil['nama']) ?> tersimpan. BMI: <?= $hasil['bmi'] ?>
        &mdash; Status Gizi: <span class="tag tag-<?= strtolower($hasil['status_gizi']) ?>"><?= $hasil['status_gizi'] ?></span>
      </div>
    <?php endif; ?>
    <p id="status-alat" style="font-size:13px; color:#6b7a86;">Menghubungkan ke alat...</p>
    <form method="post">
      <div class="form-row">
        <div>
          <label>Nama Anak</label>
          <select name="anak_id" required>
            <option value="">-- Pilih Anak --</option>
            <?php while ($a = $daftar_anak->fetch_assoc()): ?>
            <option value="<?= $a['id'] ?>"><?= htmlspecialchars($a['nama']) ?><?= $a['nis'] ? ' ('.htmlspecialchars($a['nis']).')' : '' ?></option>
            <?php endwhile; ?>
          </select>
        </div>
        <div>
          <label>Tanggal Pengukuran</label>
          <input type="date" name="tanggal_pengukuran" value="<?= date('Y-m-d') ?>" required>
        </div>
        <div>
          <label>Berat Badan (kg)</label>
          <input type="number" step="0.01" name="berat_badan" id="berat_badan" required>
        </div>
        <div>
          <label>Tinggi Badan (cm)</label>
          <input type="number" step="0.1" name="tinggi_badan" id="tinggi_badan" required>
        </div>
        <div>
          <label>Lingkar Perut (cm)</label>
          <input type="number" step="0.1" name="lingkar_perut" id="lingkar_perut" required>
        </div>
      </div>
      <button type="submit">Simpan Pengukuran</button>
    </form>
  </div>
</div>
<script>
const fields = ['berat_badan', 'tinggi_badan', 'lingkar_perut'];

function ambilBacaanAlat() {
    fetch('baca_sensor.php?_t=' + Date.now(), { cache: 'no-store' })
        .then(r => r.json())
        .then(data => {
            fields.forEach(f => {
                const el = document.getElementById(f);
                if (el && document.activeElement !== el) {
                    el.value = data[f];
                }
            });
            const status = document.getElementById('status-alat');
            const detik = data.detik_lalu !== undefined ? data.detik_lalu : 9999;
            
            if (detik <= 15) {
                status.innerHTML = '<span style="color:#10b981; font-weight:600;">● Alat Terhubung (Real-time)</span> — Data diperbarui dari ESP32 (terakhir ' + detik + ' dtk lalu)';
            } else {
                status.innerHTML = '<span style="color:#f59e0b; font-weight:600;">○ Alat Siaga</span> — Menunggu data dikirim dari tombol ESP32...';
            }
        })
        .catch(() => {
            document.getElementById('status-alat').innerHTML = '<span style="color:#ef4444;">✕ Gagal terhubung ke server.</span>';
        });
}

ambilBacaanAlat();
setInterval(ambilBacaanAlat, 1000);
</script>
</body>
</html>
