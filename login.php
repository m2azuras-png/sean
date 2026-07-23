<?php
require 'config.php';

if (isset($_SESSION['petugas_id'])) {
    header('Location: index.php');
    exit;
}

$error = '';

if ($_SERVER['REQUEST_METHOD'] === 'POST') {
    $email = trim($_POST['email'] ?? '');
    $password = $_POST['password'] ?? '';

    if ($email === '' || $password === '') {
        $error = 'Email dan password wajib diisi.';
    } else {
        $stmt = $koneksi->prepare('SELECT id, nama, password FROM petugas WHERE email = ?');
        $stmt->bind_param('s', $email);
        $stmt->execute();
        $result = $stmt->get_result();
        $petugas = $result->fetch_assoc();

        if ($petugas && password_verify($password, $petugas['password'])) {
            $_SESSION['petugas_id'] = $petugas['id'];
            $_SESSION['petugas_nama'] = $petugas['nama'];
            header('Location: index.php');
            exit;
        } else {
            $error = 'Email atau password salah.';
        }
    }
}
?>
<!DOCTYPE html>
<html lang="id">
<head>
<meta charset="UTF-8">
<title>Login - Pemantauan Pertumbuhan Anak</title>
<link rel="stylesheet" href="style.css">
</head>
<body>
<div class="auth-wrap">
  <div class="auth-box">
    <h1>Masuk</h1>
    <p class="sub">Sistem Pemantauan Pertumbuhan Anak</p>
    <?php if ($error): ?><div class="msg-err"><?= htmlspecialchars($error) ?></div><?php endif; ?>
    <form method="post">
      <label for="email">Email</label>
      <input type="email" id="email" name="email" required>
      <label for="password">Password</label>
      <input type="password" id="password" name="password" required>
      <button type="submit" class="full">Masuk</button>
    </form>
    <div class="switch">Belum punya akun? <a href="daftar.php">Daftar</a></div>
  </div>
</div>
</body>
</html>
