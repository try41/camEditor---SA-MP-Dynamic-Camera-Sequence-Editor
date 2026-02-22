# 🎬 camEditor
> Dynamic Camera Sequence Editor for SA-MP / open.mp  
> by **AdityaQ** & **FLUXC**

---

## 📖 Tentang

**camEditor** adalah include Pawn untuk SA-MP / open.mp yang memungkinkan kamu merekam, menyimpan, dan memutar kembali sequence kamera secara dinamis langsung di dalam server — tanpa perlu hardcode koordinat satu per satu.

Versi **1.1.0** sekarang kompatibel dengan **CLEO Free Cam** (client-side). Cukup aktifkan CLEO cam, gerakin kamera ke posisi yang kamu mau, lalu ketik `/came` — titik langsung terekam dari posisi kamera, bukan dari posisi player.

---

## ✨ Fitur

- 📍 Rekam hingga **50 titik** per sequence
- 💾 Simpan & load sequence dari file `.camseq`
- 🎥 Preview sequence langsung di dalam game
- 🔁 Mode loop untuk cinematic yang terus berputar
- ⏱️ Durasi tiap segmen bisa diatur sendiri (100ms – 30000ms)
- 🎞️ Support `CAMERA_MOVE` dan `CAMERA_CUT`
- 🧊 **Player otomatis di-freeze saat rekam** — kompatibel dengan CLEO Free Cam
- 📤 Export kode `InterpolateCameraPos` & `InterpolateCameraLookAt` siap pakai
- 🗂️ Manajemen sequence: putar, loop, hapus

---

## 🔧 Requirements

| Dependency | Keterangan |
|---|---|
| [easyDialog](https://github.com/Awsomedude/easyDialog) | Wajib, include sebelum camEditor |
| [izcmd](https://github.com/YashasSamaga/I-ZCMD) atau [zcmd](https://github.com/Southclaws/ZCMD) | Untuk command `/came` |
| CLEO Free Cam *(opsional)* | Client-side, untuk navigasi kamera saat rekam |

---

## 📦 Instalasi

1. Copy `camEditor.inc` ke folder `pawno/include/`
2. Buat folder `scriptfiles/camseq/` di root server kamu
3. Tambahkan ke gamemode:

```pawn
#include <easyDialog>
#include <camEditor>
```

4. Panggil fungsi setup di callback yang sesuai:

```pawn
public OnGameModeInit() {
    CamEditor_Init();
    return 1;
}

public OnPlayerConnect(playerid) {
    CamEditor_OnConnect(playerid);
    return 1;
}

public OnPlayerDisconnect(playerid, reason) {
    CamEditor_OnDisconnect(playerid);
    return 1;
}
```

---

## 🕹️ Cara Pakai

### Rekam Sequence (dengan CLEO Free Cam)

1. Aktifkan **CLEO Free Cam** di client
2. Ketik `/came` → pilih **Rekam Sequence Baru** → masukkan nama
3. Gerakin kamera CLEO ke posisi pertama (player otomatis freeze)
4. Di menu rekam, pilih **+ Tambah Titik** → gerak ke posisi berikutnya → `/came`
5. Ulangi sampai semua titik terekam
6. Pilih **Selesai dan Simpan**

### Putar Sequence

```pawn
// Dari in-game: /came → Putar Sequence → pilih nama

// Dari script:
CamEditor_Play(playerid, "nama_sequence", false); // sekali putar
CamEditor_Play(playerid, "nama_sequence", true);  // loop
CamEditor_Stop(playerid);                          // stop
CamEditor_IsPlaying(playerid);                     // cek status
```

---

## 🔑 Command

| Command | Fungsi |
|---|---|
| `/came` | Buka menu utama |
| `/came` *(saat CE_REC_WAIT)* | Rekam titik di posisi kamera sekarang |
| `/came` *(saat preview/play)* | Stop playback |

---

## 📁 Struktur File

```
server/
├── pawno/include/
│   └── camEditor.inc
└── scriptfiles/
    └── camseq/
        ├── index.txt
        ├── intro_scene.camseq
        └── cinematic_ls.camseq
```

---

## 📤 Format File `.camseq`

File yang dihasilkan berisi kode siap pakai dan raw data:

```
// segment 1 -> 2  (3000ms)
InterpolateCameraPos(playerid, 100.00, 200.00, 15.00, 110.00, 210.00, 15.00, 3000, CAMERA_MOVE);
InterpolateCameraLookAt(playerid, 105.00, 205.00, 15.00, 115.00, 215.00, 15.00, 3000, CAMERA_MOVE);
```

Raw data di bawahnya dipakai oleh loader internal untuk fitur play/loop.

---

## 📝 Changelog

### v1.1.0
- ✅ **CLEO Free Cam compatible** — titik direkam dari `GetPlayerCameraPos` + `GetPlayerCameraFrontVector`, bukan posisi player
- ✅ **Player freeze otomatis** saat mode rekam aktif, unfreeze saat selesai atau batal
- 🐛 Fix fly mode built-in yang tidak berfungsi (arah kamera tidak ikut rotasi)
- ❌ Hapus built-in fly mode — digantikan CLEO Free Cam

### v1.0.0
- 🎉 Initial release
- Rekam, simpan, putar, loop sequence kamera
- Export kode `InterpolateCameraPos` siap pakai
- Built-in fly mode *(deprecated di v1.1.0)*

---

## 👤 Credits

| Nama | Role |
|---|---|
| **AdityaQ** | Developer |
| **FLUXC** | Developer |

---
