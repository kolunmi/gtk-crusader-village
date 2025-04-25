# GTK Crusader Village

![Image](https://github.com/user-attachments/assets/148a9cfd-46df-4bec-9023-41fd27077d5b)

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

## This is a WIP

Most functionality is finished, however.

## Acknowledgments

`src/gtk-crusader-village-util-bin.c` and the theme portal client in `src/gtk-crusader-village-application.c` were taken from [libadwaita](https://gitlab.gnome.org/GNOME/libadwaita).

All assets or ideas originating from the original Stronghold Crusader game in this repo are indicated as such and belong to [Firefly Studios](https://fireflyworlds.com/). See `src/shc-data/`. Thanks for creating such an amazing game!

---

## Getting Started (Windows)

Choose one of the following methods to run the application on Windows.

### Option 1: Run Pre-built Version (Easiest)

No building or MSYS2 installation required by the application itself.

1.  **Download & Extract:** Go to the [Actions tab](https://github.com/kolunmi/gtk-crusader-village/actions), select the latest "Build Windows Release" run, download the `gtk-crusader-village-windows-x64.zip` artifact, and extract the zip file to your computer.
2.  **Run:** Open the extracted folder, go into the `bin` subfolder, and double-click `gtk-crusader-village.exe`.
3.  **Set up Python Dependency:** For full functionality, follow the steps in the **"Runtime Dependency: Python + `sourcehold-maps` Library (Manual Setup using venv)"** section below.

### Option 2: Build using Automated Script

Builds the project from source with minimal manual steps after installing prerequisites.

1.  **Install Prerequisites:** Install [Git for Windows](https://git-scm.com/download/win) and [MSYS2](https://www.msys2.org/) (use default path `C:\msys64`).
2.  **Clone Repository:** Open Git Bash (or Cmd/PowerShell), clone the repository, and enter its directory:
    ```
    git clone https://github.com/kolunmi/gtk-crusader-village.git
    cd gtk-crusader-village
    ```
3.  **Run Build Script:** Run `setup_and_build.bat` from Cmd/PowerShell while in the repository root. Wait for the MSYS2 window to complete all automated steps (MSYS2 updates, pacman dependencies, `sourcehold-maps` clone & pip install, meson configure, ninja compile & install).
    ```
    setup_and_build.bat
    ```
4.  **Run Application:** Once the script finishes, the MSYS2 window stays open. Run the application from that MSYS2 shell:
    ```
    gtk-crusader-village.exe
    ```
5.  **Configure Python Path:** The helper script installed `sourcehold-maps` into the MSYS2 Python. Launch the app, go to Preferences/Settings, and set the **Python executable path** to the MSYS2 MinGW Python:
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
4.  **Install Dependencies:** Open **MSYS2 MinGW x64** shell, `cd` to project dir. Run:
    ```
    pacman -S --needed --noconfirm git mingw-w64-x86_64-toolchain mingw-w64-x86_64-meson mingw-w64-x86_64-ninja mingw-w64-x86_64-python mingw-w64-x86_64-pkgconf mingw-w64-x86_64-gtk4 mingw-w64-x86_64-libadwaita mingw-w64-x86_64-json-glib mingw-w64-x86_64-gettext mingw-w64-x86_64-desktop-file-utils mingw-w64-x86_64-appstream mingw-w64-x86_64-python-pip
    ```
5.  **Configure Build:** In the MSYS2 MinGW x64 shell:
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
    *(Ignore potential errors about `update-desktop-database`.)*
8.  **Set up Python Dependency:** Follow the steps in the **"Runtime Dependency: Python + `sourcehold-maps` Library (Manual Setup using venv)"** section below.
9.  **Run Application:** In the MSYS2 MinGW x64 shell:
    ```
    gtk-crusader-village.exe
    ```
10. **Configure Python Path:** Launch app, go to Preferences/Settings, set Python path to the `python.exe` inside the venv created in step 8.

---

## Runtime Dependency: Python + `sourcehold-maps` Library (Manual Setup using venv)

Full functionality (parsing `.map`, `.aiv` files) requires Python 3 and the `sourcehold-maps` library. This setup uses a Python **virtual environment (`venv`)** to keep dependencies isolated and is necessary if you used **Option 1 (Pre-built)** or **Option 3 (Manual Build)**.

**Requirements:**

1.  **A Python 3 Installation:** Install from [python.org](https://www.python.org/) if needed. Ensure `python` and `pip` are accessible from Command Prompt or PowerShell.

**Setup Steps:**

1.  **Navigate to Project/Extraction Directory:** Open Windows Command Prompt or PowerShell. Change directory (`cd`) to where you extracted the pre-built application (Option 1) or cloned the repository (Option 3). For example:
    ```
    cd C:\path\to\gtk-crusader-village-extracted-or-cloned
    ```

2.  **Create Virtual Environment:** Create a venv named `.gtk_village_venv` here:
    ```
    python -m venv .gtk_village_venv
    ```

3.  **Activate Virtual Environment:** Run the activation script:
    ```
    .\.gtk_village_venv\Scripts\activate
    ```
    *(Your command prompt should now show `(.gtk_village_venv)` at the beginning)*

4.  **Install `sourcehold-maps` Library (inside active venv):** Choose Method A or B:
    * **Method A (Wheel File):** Download the `.whl` file from [sourcehold-maps GitHub Actions artifacts](https://github.com/sourcehold/sourcehold-maps/actions). Install it:
        ```
        pip install C:\path\to\downloaded\sourcehold_maps-XXXX.whl
        ```
        *(Use the correct path to the downloaded wheel file)*
    * **Method B (Source Code):** Ensure Git is installed. Clone and install:
        ```
        git clone https://github.com/sourcehold/sourcehold-maps.git sourcehold-maps-src
        cd sourcehold-maps-src
        pip install .
        cd ..
        ```

5.  **Deactivate Virtual Environment (Optional):**
    ```
    deactivate
    ```

6.  **Configure Path in App:**
    * Launch `GTK Crusader Village`.
    * Open **Preferences/Settings**.
    * Find the **Python executable path** setting.
    * Set it to the **full, absolute path** of the `python.exe` inside the virtual environment you created. Example:
        ```
        C:\path\to\gtk-crusader-village-extracted-or-cloned\.gtk_village_venv\Scripts\python.exe
        ```
        *(Replace `C:\path\to...` with the actual absolute path)*
    * Save settings. Restart the application if required.
