<div class="topbar no-print">
  <div class="brand">Pemantauan Pertumbuhan Anak</div>
  <nav>
    <a href="index.php" class="<?= basename($_SERVER['PHP_SELF']) === 'index.php' ? 'active' : '' ?>">Dashboard</a>
    <a href="data_anak.php" class="<?= basename($_SERVER['PHP_SELF']) === 'data_anak.php' ? 'active' : '' ?>">Data Anak</a>
    <a href="pengukuran.php" class="<?= basename($_SERVER['PHP_SELF']) === 'pengukuran.php' ? 'active' : '' ?>">Input Pengukuran</a>
    <a href="riwayat.php" class="<?= basename($_SERVER['PHP_SELF']) === 'riwayat.php' ? 'active' : '' ?>">Riwayat</a>
    <a href="profil.php" class="<?= basename($_SERVER['PHP_SELF']) === 'profil.php' ? 'active' : '' ?>">Akun</a>
    <a href="logout.php">Keluar (<?= htmlspecialchars($_SESSION['petugas_nama']) ?>)</a>
  </nav>
</div>
