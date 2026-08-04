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

$daftar_anak = $koneksi->query('SELECT id, nama, nis, tanggal_lahir FROM data_anak ORDER BY nama');
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
        &mdash; Status Gizi: <span class="tag tag-<?= strtolower(str_replace(' ', '-', $hasil['status_gizi'])) ?>"><?= $hasil['status_gizi'] ?></span>
      </div>
    <?php endif; ?>
    <!-- Control Switcher Mode Input -->
    <div style="display:flex; align-items:center; justify-content:space-between; margin-bottom:18px; padding:12px 16px; background:#f8fafc; border:1px solid #e2e8f0; border-radius:8px; flex-wrap:wrap; gap:12px;">
      <div>
        <label style="font-weight:600; font-size:13px; color:#334155; display:block; margin-bottom:6px;">Mode Input Data:</label>
        <div style="display:flex; gap:8px;">
          <button type="button" id="btn-mode-auto" onclick="setModeInput('auto')" class="btn" style="padding:7px 14px; font-size:13px; background:#2f7d5b; transition:all 0.2s;">● Auto (Sensor ESP32)</button>
          <button type="button" id="btn-mode-manual" onclick="setModeInput('manual')" class="btn btn-secondary" style="padding:7px 14px; font-size:13px; transition:all 0.2s;">✍ Manual (Ketik)</button>
        </div>
      </div>
      <div id="status-alat" style="font-size:13px; color:#6b7a86;">Menghubungkan ke alat...</div>
    </div>

    <form method="post">
      <div class="form-row">
        <div>
          <label>Nama Anak</label>
          <select name="anak_id" required>
            <option value="">-- Pilih Anak --</option>
            <?php while ($a = $daftar_anak->fetch_assoc()): ?>
            <option value="<?= $a['id'] ?>" data-tgl-lahir="<?= $a['tanggal_lahir'] ?>"><?= htmlspecialchars($a['nama']) ?><?= $a['nis'] ? ' ('.htmlspecialchars($a['nis']).')' : '' ?></option>
            <?php endwhile; ?>
          </select>
        </div>
        <div>
          <label>Tanggal Pengukuran</label>
          <input type="date" name="tanggal_pengukuran" value="<?= date('Y-m-d') ?>" required>
        </div>
        <div>
          <label>Berat Badan (kg)</label>
          <input type="number" step="0.01" name="berat_badan" id="berat_badan" required placeholder="Contoh: 20.5">
        </div>
        <div>
          <label>Tinggi Badan (cm)</label>
          <input type="number" step="0.1" name="tinggi_badan" id="tinggi_badan" required placeholder="Contoh: 115.0">
        </div>
        <div>
          <label>Lingkar Perut (cm)</label>
          <input type="number" step="0.1" name="lingkar_perut" id="lingkar_perut" required placeholder="Contoh: 50.0">
        </div>
      </div>

      <!-- Live Preview Hasil Real-time -->
      <div id="box-preview-hasil" style="display:none; margin: 12px 0 18px; padding:12px 16px; background:#f0fdf4; border:1px solid #bbf7d0; border-radius:8px; align-items:center; justify-content:space-between; flex-wrap:wrap; gap:10px;">
        <div style="display:flex; align-items:center; gap:14px; flex-wrap:wrap;">
          <span style="font-size:13px; color:#166534; font-weight:700;">● HASIL PREVIEW (REAL-TIME):</span>
          <span style="font-size:14px; color:#0f172a; font-weight:600;">BMI: <strong id="preview-bmi-val" style="color:#2f7d5b;">-</strong></span>
          <span style="font-size:14px; color:#0f172a; font-weight:600;">Status Gizi: <span id="preview-status-tag" class="tag tag-normal">-</span></span>
        </div>
      </div>

      <button type="submit">Simpan Pengukuran</button>
    </form>
  </div>
</div>
<script>
let modeInput = 'auto'; // default mode auto (ESP32)

const fields = ['berat_badan', 'tinggi_badan', 'lingkar_perut'];
const beratEl = document.getElementById('berat_badan');
const tinggiEl = document.getElementById('tinggi_badan');
const selectAnakEl = document.querySelector('select[name="anak_id"]');

function setModeInput(mode) {
    modeInput = mode;
    const btnAuto = document.getElementById('btn-mode-auto');
    const btnManual = document.getElementById('btn-mode-manual');
    const statusEl = document.getElementById('status-alat');
    
    if (mode === 'auto') {
        btnAuto.className = 'btn';
        btnAuto.style.background = '#2f7d5b';
        btnManual.className = 'btn btn-secondary';
        btnManual.style.background = '#6b7a86';
        statusEl.innerHTML = '<span style="color:#10b981; font-weight:600;">● Mode Auto Aktif</span> — Mengambil data dari sensor ESP32...';
        ambilBacaanAlat();
    } else {
        btnManual.className = 'btn';
        btnManual.style.background = '#0284c7';
        btnAuto.className = 'btn btn-secondary';
        btnAuto.style.background = '#6b7a86';
        statusEl.innerHTML = '<span style="color:#0284c7; font-weight:600;">✍ Mode Manual Aktif</span> — Otomatisasi sensor dimatikan, silakan ketik manual.';
    }
}

function hitungUsia(tglLahirStr) {
    if (!tglLahirStr) return null;
    const lahir = new Date(tglLahirStr);
    const sekarang = new Date();
    let usia = sekarang.getFullYear() - lahir.getFullYear();
    const m = sekarang.getMonth() - lahir.getMonth();
    if (m < 0 || (m === 0 && sekarang.getDate() < lahir.getDate())) {
        usia--;
    }
    return usia;
}

function hitungStatusGiziJS(usia, bmi) {
    if (usia !== null && usia >= 6 && usia <= 12) {
        const ambang = {
            6:  { kurus: 14.0, normal: 17.0, gemuk: 18.5 },
            7:  { kurus: 14.0, normal: 17.5, gemuk: 19.0 },
            8:  { kurus: 14.5, normal: 18.0, gemuk: 19.5 },
            9:  { kurus: 14.5, normal: 18.5, gemuk: 20.5 },
            10: { kurus: 15.0, normal: 19.0, gemuk: 21.5 },
            11: { kurus: 15.5, normal: 19.5, gemuk: 22.5 },
            12: { kurus: 16.0, normal: 20.5, gemuk: 24.0 }
        };
        const u = Math.min(Math.max(Math.round(usia), 6), 12);
        const t = ambang[u];
        if (bmi < t.kurus)   return 'Kurus';
        if (bmi <= t.normal) return 'Normal';
        if (bmi <= t.gemuk)  return 'Gemuk';
        return 'Obesitas';
    }
    if (bmi < 17.0) return 'Sangat Kurus';
    if (bmi < 18.5) return 'Kurus';
    if (bmi <= 25.0) return 'Normal';
    if (bmi <= 27.0) return 'Gemuk';
    return 'Obesitas';
}

function updatePreviewKalkulasi() {
    const berat = parseFloat(beratEl ? beratEl.value : 0);
    const tinggi = parseFloat(tinggiEl ? tinggiEl.value : 0);
    const box = document.getElementById('box-preview-hasil');
    
    if (berat > 0 && tinggi > 0) {
        const tinggiM = tinggi / 100;
        const bmi = (berat / (tinggiM * tinggiM)).toFixed(2);
        
        let tglLahir = null;
        if (selectAnakEl && selectAnakEl.selectedIndex > 0) {
            tglLahir = selectAnakEl.options[selectAnakEl.selectedIndex].getAttribute('data-tgl-lahir');
        }
        const usia = hitungUsia(tglLahir);
        const statusGizi = hitungStatusGiziJS(usia, parseFloat(bmi));
        
        document.getElementById('preview-bmi-val').innerText = bmi;
        const tag = document.getElementById('preview-status-tag');
        tag.innerText = statusGizi;
        tag.className = 'tag tag-' + statusGizi.toLowerCase().replace(/\s+/g, '-');
        
        box.style.display = 'flex';
    } else {
        box.style.display = 'none';
    }
}

['input', 'change', 'keyup'].forEach(evt => {
    if (beratEl) beratEl.addEventListener(evt, updatePreviewKalkulasi);
    if (tinggiEl) tinggiEl.addEventListener(evt, updatePreviewKalkulasi);
    if (selectAnakEl) selectAnakEl.addEventListener(evt, updatePreviewKalkulasi);
});

function ambilBacaanAlat() {
    if (modeInput !== 'auto') return;

    fetch('baca_sensor.php?_t=' + Date.now(), { cache: 'no-store' })
        .then(r => r.json())
        .then(data => {
            if (modeInput !== 'auto') return;
            fields.forEach(f => {
                const el = document.getElementById(f);
                if (el && document.activeElement !== el) {
                    el.value = data[f];
                }
            });
            updatePreviewKalkulasi();
            const status = document.getElementById('status-alat');
            const detik = data.detik_lalu !== undefined ? data.detik_lalu : 9999;
            
            if (detik <= 15) {
                status.innerHTML = '<span style="color:#10b981; font-weight:600;">● Alat Terhubung (Auto)</span> — Data diperbarui dari ESP32 (terakhir ' + detik + ' dtk lalu)';
            } else {
                status.innerHTML = '<span style="color:#f59e0b; font-weight:600;">○ Alat Siaga (Auto)</span> — Menunggu data dikirim dari tombol ESP32...';
            }
        })
        .catch(() => {
            if (modeInput === 'auto') {
                document.getElementById('status-alat').innerHTML = '<span style="color:#ef4444;">✕ Gagal terhubung ke server.</span>';
            }
        });
}

ambilBacaanAlat();
setInterval(ambilBacaanAlat, 1000);
</script>
</body>
</html>
