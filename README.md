# GTK Crusader Village

![Image](https://github.com/user-attachments/assets/148a9cfd-46df-4bec-9023-41fd27077d5b)

## What is this

In [Stronghold Crusader](https://en.wikipedia.org/wiki/Stronghold:_Crusader), `.aiv` (AI Village) files detail the way enemy lords construct their castles in skirmish mode. The [official AIV editor](https://stronghold.heavengames.com/downloads/showfile.php?fileid=7534) serves its purpose, but I think it could be improved. This application aims to be an ergonomic, cross-platform `.aiv` editor GUI.

---

## Getting Started (Windows)

Choose one method to run the application on Windows.

### Option 1: Run Pre-built Version (Easiest)

No building or MSYS2 installation required.

1.  **Download & Extract:** Go to the [Actions tab](https://github.com/kolunmi/gtk-crusader-village/actions), select the latest "Build Windows Release" run, download the `gtk-crusader-village-windows-x64.zip` artifact from the "Artifacts" section, and extract the zip file.
2.  **Run:** Open the extracted folder, go into the `bin` subfolder, and double-click `gtk-crusader-village.exe`.
3.  **Set up Python Dependency:** For full functionality, follow the steps in the **"Runtime Dependency: Python + `sourcehold-maps` Library (Manual Setup)"** section below.

### Option 2: Build using Automated Script

Builds the project from source with minimal manual steps after initial setup.

1.  **Install Prerequisites:** Install [Git for Windows](https://git-scm.com/download/win) and [MSYS2](https://www.msys2.org/) (use default path `C:\msys64`).
2.  **Clone Repository:** Open Git Bash (or Cmd/PowerShell), clone the repository, and enter its directory:
    ```
    git clone https://github.com/kolunmi/gtk-crusader-village.git
    cd gtk-crusader-village
    ```
3.  **Run Build Script:** Double-click `setup_and_build.bat` or run it from Cmd/PowerShell. Wait for the MSYS2 window to complete all automated steps (updates, dependency install, `sourcehold-maps` install, configure, compile, install).
    ```
    setup_and_build.bat
    ```
4.  **Run Application:** Once the script finishes, the MSYS2 window stays open. Run the application from that MSYS2 shell:
    ```
    gtk-crusader-village.exe
    ```
5.  **Configure Python Path:** Launch the app, go to Preferences/Settings, and set the Python executable path to the MSYS2 Python installation:
    ```
    C:\msys64\mingw64\bin\python.exe
    ```

### Option 3: Build using Manual Steps

Provides full control over the build process.

1.  **Install MSYS2:** Install from [https://www.msys2.org/](https://www.msys2.org/) to `C:\msys64`.
2.  **Clone Repository:** Use Git to clone this repository and `cd` into it.
    ```
    git clone https://github.com/kolunmi/gtk-crusader-village.git
    cd gtk-crusader-village
    ```
3.  **Update MSYS2:** Open **MSYS2 MSYS** shell, run `pacman -Syu`, then `pacman -Su`. Close shell.
    ```
    pacman -Syu
    pacman -Su
    ```
4.  **Install Dependencies:** Open **MSYS2 MinGW x64** shell, `cd` to project dir (e.g., `cd /c/path/to/gtk-crusader-village`). Run:
    ```
    pacman -S --needed --noconfirm git mingw-w64-x86_64-toolchain mingw-w64-x86_64-meson mingw-w64-x86_64-ninja mingw-w64-x86_64-python mingw-w64-x86_64-pkgconf mingw-w64-x86_64-gtk4 mingw-w64-x86_64-libadwaita mingw-w64-x86_64-json-glib mingw-w64-x86_64-gettext mingw-w64-x86_64-desktop-file-utils mingw-w64-x86_64-appstream mingw-w64-x86_64-python-pip
    ```
5.  **Configure:** In the same MSYS2 shell:
    ```
    meson setup build --prefix=/mingw64 --buildtype=release
    ```
6.  **Compile:** In the same shell:
    ```
    ninja -C build
    ```
7.  **Install:** In the same shell:
    ```
    ninja -C build install
    ```
8.  **Set up Python Dependency:** Follow the steps in the **"Runtime Dependency: Python + `sourcehold-maps` Library (Manual Setup)"** section below.
9.  **Run Application:** In the MSYS2 MinGW x64 shell:
    ```
    gtk-crusader-village.exe
    ```
10. **Configure Python Path:** Launch app, go to Preferences/Settings, set Python path to the `python.exe` where `sourcehold-maps` was installed.

---

## Runtime Dependency: Python + `sourcehold-maps` Library (Manual Setup)

Full functionality requires Python 3 and the `sourcehold-maps` library. This setup is needed for **Option 1 (Pre-built)** and **Option 3 (Manual Build)**.

**Requirements:**

1.  Python 3 installation (from [python.org](https://www.python.org/)). Ensure `pip` is accessible from Command Prompt/PowerShell.
2.  `sourcehold-maps` library installed in that Python environment.

**Installation Steps:**

1.  **Install `sourcehold-maps` Library:** Choose Method A or B:
    * **Method A (Wheel File):** Download `.whl` file from [sourcehold-maps GitHub Actions artifacts](https://github.com/sourcehold/sourcehold-maps/actions). Then, in Command Prompt/PowerShell:
        ```
        pip install C:\path\to\downloaded\sourcehold_maps-XXXX.whl
        ```
    * **Method B (Source Code):** In Command Prompt/PowerShell:
        ```
        git clone https://github.com/sourcehold/sourcehold-maps.git
        cd sourcehold-maps
        pip install .
        ```
2.  **Configure Path in App:** Launch `GTK Crusader Village`. Go to Preferences/Settings. Set **Python executable path** to the full path of the `python.exe` used in Step 1 (e.g., `C:\Path\To\Your\Python\python.exe`). Save settings.

---

## Building (Linux)

Instructions for Linux distributions.

```
meson setup build --prefix=/usr/local --buildtype=release
ninja -C build
sudo ninja -C build install # Optional installation
```

## This is a WIP

Most functionality is finished, however.

## Acknowledgments

`src/gtk-crusader-village-util-bin.c` and the theme portal client in `src/gtk-crusader-village-application.c` were taken from [libadwaita](https://gitlab.gnome.org/GNOME/libadwaita).

All assets or ideas originating from the original Stronghold Crusader game in this repo are indicated as such and belong to [Firefly Studios](https://fireflyworlds.com/). See `src/shc-data/`. Thanks for creating such an amazing game!
