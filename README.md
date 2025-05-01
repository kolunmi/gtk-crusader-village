# GTK Crusader Village

![Image](https://github.com/user-attachments/assets/456d48e5-9611-454a-92a9-f98dd68e3b63)

## What is this

In [Stronghold Crusader](https://en.wikipedia.org/wiki/Stronghold:_Crusader), `.aiv` (AI Village) files detail the way enemy lords construct their castles in skirmish mode. The [official AIV editor](https://stronghold.heavengames.com/downloads/showfile.php?fileid=7534) serves its purpose, but I think it could be improved. This application aims to be an ergonomic, cross-platform `.aiv` editor GUI.

---

## Building (Linux)

Instructions for Linux distributions.

```
meson setup build --prefix=/usr/local --buildtype=release
ninja -C build
sudo ninja -C build install # Optional installation
```

## Windows

Download a release, or execute the above commands in an msys2 shell.

## Acknowledgments

`src/gtk-crusader-village-util-bin.c` and the theme portal client in `src/gtk-crusader-village-application.c` were taken from [libadwaita](https://gitlab.gnome.org/GNOME/libadwaita).

All assets or ideas originating from the original Stronghold Crusader game in this repo are indicated as such and belong to [Firefly Studios](https://fireflyworlds.com/). See `src/shc-data/`. Thanks for creating such an amazing game!

