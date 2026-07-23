<?php
require 'config.php';
wajib_login();

$error = '';
$sukses = '';

$stmt = $koneksi->prepare('SELECT * FROM petugas WHERE id=?');
$stmt->bind_param('i', $_SESSION['petugas_id']);
$stmt->execute();
$petugas = $stmt->get_result()->fetch_assoc();

if ($_SERVER['REQUEST_METHOD'] === 'POST') {
    $email_baru = trim($_POST['email'] ?? '');
    $password_lama = $_POST['password_lama'] ?? '';
    $password_baru = $_POST['password_baru'] ?? '';
    $konfirmasi = $_POST['konfirmasi'] ?? '';

    if ($email_baru === '' || $password_lama === '') {
        $error = 'Email dan password saat ini wajib diisi.';
    } elseif (!password_verify($password_lama, $petugas['password'])) {
        $error = 'Password saat ini salah.';
    } elseif ($password_baru !== '' && strlen($password_baru) < 6) {
        $error = 'Password baru minimal 6 karakter.';
    } elseif ($password_baru !== '' && $password_baru !== $konfirmasi) {
        $error = 'Konfirmasi password baru tidak sesuai.';
    } else {
        $stmt = $koneksi->prepare('SELECT id FROM petugas WHERE email = ? AND id != ?');
        $stmt->bind_param('si', $email_baru, $_SESSION['petugas_id']);
        $stmt->execute();
        if ($stmt->get_result()->fetch_assoc()) {
            $error = 'Email sudah digunakan petugas lain.';
        } else {
            if ($password_baru !== '') {
                $hash = password_hash($password_baru, PASSWORD_DEFAULT);
                $stmt = $koneksi->prepare('UPDATE petugas SET email=?, password=? WHERE id=?');
                $stmt->bind_param('ssi', $email_baru, $hash, $_SESSION['petugas_id']);
            } else {
                $stmt = $koneksi->prepare('UPDATE petugas SET email=? WHERE id=?');
                $stmt->bind_param('si', $email_baru, $_SESSION['petugas_id']);
            }
            $stmt->execute();
            $petugas['email'] = $email_baru;
            $sukses = 'Perubahan berhasil disimpan.';
        }
    }
}
?>
<!DOCTYPE html>
<html lang="id">
<head>
<meta charset="UTF-8">
<title>Pengaturan Akun - Pemantauan Pertumbuhan Anak</title>
<link rel="stylesheet" href="style.css">
</head>
<body>
<?php include 'navbar.php'; ?>
<div class="container">
  <div class="card" style="max-width:420px;">
    <h3>Pengaturan Akun</h3>
    <?php if ($error): ?><div class="msg-err"><?= htmlspecialchars($error) ?></div><?php endif; ?>
    <?php if ($sukses): ?><div class="msg-ok"><?= htmlspecialchars($sukses) ?></div><?php endif; ?>
    <form method="post">
      <label>Nama</label>
      <input type="text" value="<?= htmlspecialchars($petugas['nama']) ?>" disabled>

      <label>Email</label>
      <input type="email" name="email" value="<?= htmlspecialchars($petugas['email']) ?>" required>

      <label>Password Baru (kosongkan jika tidak ingin ganti)</label>
      <input type="password" name="password_baru">

      <label>Konfirmasi Password Baru</label>
      <input type="password" name="konfirmasi">

      <label>Password Saat Ini (wajib untuk konfirmasi perubahan)</label>
      <input type="password" name="password_lama" required>

      <button type="submit" class="full">Simpan Perubahan</button>
    </form>
  </div>
</div>
</body>
</html>
