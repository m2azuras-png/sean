<?php
require 'config.php';
wajib_login();

$error = '';
$sukses = '';
$edit_data = null;

if ($_SERVER['REQUEST_METHOD'] === 'POST') {
    $nama = trim($_POST['nama'] ?? '');
    $jk = $_POST['jenis_kelamin'] ?? '';
    $tgl_lahir = $_POST['tanggal_lahir'] ?? '';
    $nis = trim($_POST['nis'] ?? '');
    $id = $_POST['id'] ?? '';

    if ($nama === '' || $jk === '' || $tgl_lahir === '') {
        $error = 'Nama, jenis kelamin, dan tanggal lahir wajib diisi.';
    } elseif ($id !== '') {
        $stmt = $koneksi->prepare('UPDATE data_anak SET nama=?, jenis_kelamin=?, tanggal_lahir=?, nis=? WHERE id=?');
        $stmt->bind_param('ssssi', $nama, $jk, $tgl_lahir, $nis, $id);
        $stmt->execute();
        $sukses = 'Data anak berhasil diperbarui.';
    } else {
        $stmt = $koneksi->prepare('INSERT INTO data_anak (nama, jenis_kelamin, tanggal_lahir, nis) VALUES (?, ?, ?, ?)');
        $stmt->bind_param('ssss', $nama, $jk, $tgl_lahir, $nis);
        $stmt->execute();
        $sukses = 'Data anak berhasil ditambahkan.';
    }
}

if (isset($_GET['hapus'])) {
    $stmt = $koneksi->prepare('DELETE FROM data_anak WHERE id=?');
    $stmt->bind_param('i', $_GET['hapus']);
    $stmt->execute();
    $sukses = 'Data anak berhasil dihapus.';
}

if (isset($_GET['edit'])) {
    $stmt = $koneksi->prepare('SELECT * FROM data_anak WHERE id=?');
    $stmt->bind_param('i', $_GET['edit']);
    $stmt->execute();
    $edit_data = $stmt->get_result()->fetch_assoc();
}

$daftar = $koneksi->query('SELECT * FROM data_anak ORDER BY nama');
?>
<!DOCTYPE html>
<html lang="id">
<head>
<meta charset="UTF-8">
<title>Data Anak - Pemantauan Pertumbuhan Anak</title>
<link rel="stylesheet" href="style.css">
</head>
<body>
<?php include 'navbar.php'; ?>
<div class="container">
  <div class="card">
    <h3><?= $edit_data ? 'Edit Data Anak' : 'Tambah Data Anak' ?></h3>
    <?php if ($error): ?><div class="msg-err"><?= htmlspecialchars($error) ?></div><?php endif; ?>
    <?php if ($sukses): ?><div class="msg-ok"><?= htmlspecialchars($sukses) ?></div><?php endif; ?>
    <form method="post">
      <input type="hidden" name="id" value="<?= $edit_data['id'] ?? '' ?>">
      <div class="form-row">
        <div>
          <label>Nama Anak</label>
          <input type="text" name="nama" value="<?= htmlspecialchars($edit_data['nama'] ?? '') ?>" required>
        </div>
        <div>
          <label>NIS (opsional)</label>
          <input type="text" name="nis" value="<?= htmlspecialchars($edit_data['nis'] ?? '') ?>">
        </div>
        <div>
          <label>Jenis Kelamin</label>
          <select name="jenis_kelamin" required>
            <option value="">-- Pilih --</option>
            <option value="L" <?= (($edit_data['jenis_kelamin'] ?? '') === 'L') ? 'selected' : '' ?>>Laki-laki</option>
            <option value="P" <?= (($edit_data['jenis_kelamin'] ?? '') === 'P') ? 'selected' : '' ?>>Perempuan</option>
          </select>
        </div>
        <div>
          <label>Tanggal Lahir</label>
          <input type="date" name="tanggal_lahir" value="<?= htmlspecialchars($edit_data['tanggal_lahir'] ?? '') ?>" required>
        </div>
      </div>
      <button type="submit"><?= $edit_data ? 'Simpan Perubahan' : 'Tambah Anak' ?></button>
      <?php if ($edit_data): ?><a href="data_anak.php" class="btn btn-secondary">Batal</a><?php endif; ?>
    </form>
  </div>

  <div class="card">
    <h3>Daftar Anak</h3>
    <table>
      <tr><th>Nama</th><th>NIS</th><th>JK</th><th>Tanggal Lahir</th><th>Usia</th><th class="no-print">Aksi</th></tr>
      <?php while ($row = $daftar->fetch_assoc()): ?>
      <tr>
        <td><?= htmlspecialchars($row['nama']) ?></td>
        <td><?= htmlspecialchars($row['nis'] ?: '-') ?></td>
        <td><?= $row['jenis_kelamin'] ?></td>
        <td><?= date('d-m-Y', strtotime($row['tanggal_lahir'])) ?></td>
        <td><?= hitung_usia($row['tanggal_lahir']) ?> tahun</td>
        <td class="actions no-print">
          <a href="data_anak.php?edit=<?= $row['id'] ?>">Edit</a>
          <a href="data_anak.php?hapus=<?= $row['id'] ?>" onclick="return confirm('Hapus data anak ini beserta riwayat pengukurannya?')">Hapus</a>
        </td>
      </tr>
      <?php endwhile; ?>
    </table>
  </div>
</div>
</body>
</html>
