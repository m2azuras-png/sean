<?php
require 'config.php';

if (isset($_SESSION['petugas_id'])) {
    header('Location: index.php');
    exit;
}

$error = '';
$sukses = '';

if ($_SERVER['REQUEST_METHOD'] === 'POST') {
    $nama = trim($_POST['nama'] ?? '');
    $email = trim($_POST['email'] ?? '');
    $password = $_POST['password'] ?? '';
    $konfirmasi = $_POST['konfirmasi'] ?? '';

    if ($nama === '' || $email === '' || $password === '') {
        $error = 'Semua kolom wajib diisi.';
    } elseif ($password !== $konfirmasi) {
        $error = 'Konfirmasi password tidak sesuai.';
    } elseif (strlen($password) < 6) {
        $error = 'Password minimal 6 karakter.';
    } else {
        $stmt = $koneksi->prepare('SELECT id FROM petugas WHERE email = ?');
        $stmt->bind_param('s', $email);
        $stmt->execute();
        if ($stmt->get_result()->fetch_assoc()) {
            $error = 'Email sudah terdaftar.';
        } else {
            $hash = password_hash($password, PASSWORD_DEFAULT);
            $stmt = $koneksi->prepare('INSERT INTO petugas (nama, email, password) VALUES (?, ?, ?)');
            $stmt->bind_param('sss', $nama, $email, $hash);
            $stmt->execute();
            $sukses = 'Akun berhasil dibuat. Silakan masuk.';
        }
    }
}
?>
<!DOCTYPE html>
<html lang="id">
<head>
<meta charset="UTF-8">
<title>Daftar Akun - Pemantauan Pertumbuhan Anak</title>
<link rel="stylesheet" href="style.css">
</head>
<body>
<div class="auth-wrap">
  <div class="auth-box">
    <h1>Daftar Akun</h1>
    <p class="sub">Sistem Pemantauan Pertumbuhan Anak</p>
    <?php if ($error): ?><div class="msg-err"><?= htmlspecialchars($error) ?></div><?php endif; ?>
    <?php if ($sukses): ?><div class="msg-ok"><?= htmlspecialchars($sukses) ?></div><?php endif; ?>
    <form method="post">
      <label for="nama">Nama Petugas</label>
      <input type="text" id="nama" name="nama" required>
      <label for="email">Email</label>
      <input type="email" id="email" name="email" required>
      <label for="password">Password</label>
      <input type="password" id="password" name="password" required>
      <label for="konfirmasi">Konfirmasi Password</label>
      <input type="password" id="konfirmasi" name="konfirmasi" required>
      <button type="submit" class="full">Daftar</button>
    </form>
    <div class="switch">Sudah punya akun? <a href="login.php">Masuk</a></div>
  </div>
</div>
</body>
</html>
