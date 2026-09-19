# <img src="docs/brand/logo.svg" width="40" height="40" align="top" alt=""> Tray Grid

[![CI](https://github.com/YakupEmreYerli/tray-grid/actions/workflows/ci.yml/badge.svg)](https://github.com/YakupEmreYerli/tray-grid/actions/workflows/ci.yml) [![Lisans: GPL-2.0-or-later](https://img.shields.io/badge/lisans-GPL--2.0--or--later-05182F)](LICENSE) [![KDE Plasma 6](https://img.shields.io/badge/KDE%20Plasma-6-ADD5FF?logo=kde&logoColor=white)](https://kde.org/plasma-desktop/)

Plasma'nın tepsisi arka plandaki uygulamaları bir okun arkasına saklıyor, açınca da her satırında yazı olan uzun bir liste gösteriyor. Tray Grid, bunları Windows'taki gibi gösteren bir panel parçası: küçük bir ok ve üstünde açılan, yalnız simgelerden oluşan kompakt bir ızgara.

> English: [README.md](README.md)

![Panelin üstünde açık Tray Grid: sekiz tepsi uygulaması simge ızgarasında](docs/screenshots/popup.png)

Tepsi uygulamalarıyla doğrudan D-Bus üzerinden, Plasma'nın kendi tepsisinin kullandığı StatusNotifierItem protokolüyle konuşur. Sistem tepsisinde görünen her uygulama burada da görünür: Electron, Qt, GTK ve libappindicator uygulamaları.

## Özellikler

- **Yalnız simge.** 1–8 sütunluk ızgara, yazı yok; üstüne gelince uygulamanın kendi ipucu.
- **Tıklamalar tepsideki gibi.** Sol tık uygulamayı açar (açacak bir şeyi yoksa menüsünü), orta tık ikincil eylem, tekerlek kaydırma, sağ tık uygulamanın kendi menüsü: alt menüler, onay kutuları, seçenek düğmeleri dahil.
- **Canlı.** Açılan, kapanan ya da simgesi değişen uygulama ızgarada hemen güncellenir.
- **Seçim sende.** Ayar sayfası çalışan tepsi uygulamalarını onay kutusuyla listeler; ızgarada istemediğini kaldırırsın. Sütun sayısı, simge boyutu ve boşta ("pasif") uygulamaların görünüp görünmeyeceği de ayarlanır.
- **Yerli.** Plasma renk şemanı ve simge temanı izler; ok, panel ekranın hangi kenarındaysa ters yöne bakar.

## Kurulum

Kaynaktan derlenir, kullanıcıya kurulur, yönetici yetkisi gerekmez. Plasma 6, KDE Frameworks 6.10+ ve Qt 6.8+ ister.

```bash
# Arch: sudo pacman -S --needed cmake ninja extra-cmake-modules qt6-declarative kwindowsystem kiconthemes kstatusnotifieritem libplasma
git clone https://github.com/YakupEmreYerli/tray-grid.git && cd tray-grid
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=$HOME/.local
cmake --build build && ctest --test-dir build
cmake --install build
```

Sonra panele sağ tık → **Pencere Öğesi Ekle** → **Tray Grid**, sistem tepsisinin yanına sürükle. Paneline dokunmadan denemek için: `plasmawindowed com.github.yakupemreyerli.traygrid`.

Güncelledikten sonra Plasma'yı bir kez yeniden başlat (`systemctl --user restart plasma-plasmashell`): parçanın kütüphanesi oturum başına bir kez yüklenir.

Kaldırmak için: `xargs rm < build/install_manifest.txt`.

## Plasma tepsisiyle birlikte

Plasma'nın sistem tepsisi aynı uygulamaları listelemeye devam eder; gizli bölümünde bir şey olduğu sürece kendi oku da görünür. Tek ok kalsın diye sistem tepsisinin ayarları → **Girdiler** sayfasında:

1. Her uygulama girdisini **Devre dışı** yap. Plasma onları hiçbir yerde göstermez, Tray Grid göstermeye devam eder.
2. Kalan her girdiyi ya **Her zaman göster** ya **Devre dışı** yap. "İlgili olduğunda göster"deki bir girdi gizli bölüme düşüp Plasma'nın okunu geri getirebilir.
3. **Tüm girdileri her zaman göster** kapalı kalsın; "Devre dışı"yı ezer.

Yeni kurulan bir uygulama ilk açıldığında Plasma'nın tepsisinde de görünür; orada bir kez devre dışı bırakman gerekir.

## Bilinen sınırlar

- libayatana üstüne kurulu uygulamalar (örneğin LocalSend) her açılışta rastgele yeni bir tepsi kimliği alır. Tray Grid onları adıyla hatırlar ama Plasma'nın "Devre dışı" ayarı onlara tutunamaz.
- Ham piksel olarak gönderilen bindirme simgeleri ve hareketli dikkat simgeleri henüz çizilmiyor; adla gönderilen bindirme simgeleri çiziliyor.
- Tek beyaz tepsi simgesi gönderen uygulama (Docker Desktop) açık renk şemasında zor görünür; Plasma'nın tepsisinde de öyle.

## Nasıl çalışır

Mimari tablo, geliştirme komutları ve testin kapsamı İngilizce [README](README.md#how-it-works)'de. Marka: [marka kiti](docs/brand/README.md).

## Lisans

KDE'nin çoğu gibi GPL-2.0-or-later. Bkz. [LICENSE](LICENSE).
