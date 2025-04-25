# GTK Crusader Village

![Image](https://github.com/user-attachments/assets/148a9cfd-46df-4bec-9023-41fd27077d5b)

## What is this

In [Stronghold Crusader](https://en.wikipedia.org/wiki/Stronghold:_Crusader), `.aiv` (AI Village) files detail the way enemy lords construct their castles in skirmish mode. The [official AIV editor](https://stronghold.heavengames.com/downloads/showfile.php?fileid=7534) serves its purpose, but I think it could be improved. This application aims to be an ergonomic, cross-platform `.aiv` editor GUI.

---

## Getting Started (Windows)

There are several ways to get the application running on Windows.

### Option 1: Download Pre-built Version (Easiest)

The easiest way to run the application without building it yourself is to download a pre-built package from the automated builds.

1.  **Go to Actions:** Navigate to the [Actions tab](https://github.com/kolunmi/gtk-crusader-village/actions) of this repository on GitHub.
2.  **Select Latest Run:** Find the latest successful run of the "Build Windows Release" workflow (usually listed at the top). Click on it.
3.  **Download Artifact:** Scroll down to the **Artifacts** section on the workflow summary page. Download the `gtk-crusader-village-windows-x64.zip` file.
4.  **Extract:** Unzip the downloaded file to a location of your choice on your computer.
5.  **Run:** Inside the extracted folder, you should find the necessary files. You might need to navigate into a `bin` subdirectory to find `gtk-crusader-village.exe`. Double-click the executable to run it.
    * **Note:** This package includes the necessary MSYS2/MinGW runtime DLLs, so you should *not* need to install MSYS2 separately just to run this pre-built version.
6.  **Configure Python:** Even with the pre-built version, you still need to set up the Python runtime dependency for full functionality (see the **Runtime Dependency: Python + `sourcehold-maps` Library** section below). You'll need to install Python and the `sourcehold-maps` library manually and configure the path in the application's preferences.

### Option 2: Build from Source (Automated Helper Script)

This option is for users who want to build the project themselves but prefer an automated setup process after installing the basic tools.

1.  **Install Prerequisites:**
    * **Git:** Install Git for Windows from [https://git-scm.com/download/win](https://git-scm.com/download/win).
    * **MSYS2:** Download and install MSYS2 from [https://www.msys2.org/](https://www.msys2.org/). Use the default installation path (`C:\msys64`).

2.  **Clone the Repository:**
    * Open **Git Bash** (installed with Git) or Windows **Command Prompt** / **PowerShell**.
    * Clone this repository:
        ```
        git clone https://github.com/kolunmi/gtk-crusader-village.git
        cd gtk-crusader-village
        ```

3.  **Run the Setup & Build Script:**
    * Locate the `setup_and_build.bat` script in the repository root directory you just cloned.
    * Double-click `setup_and_build.bat` or run it from your Command Prompt / PowerShell:
        ```
        setup_and_build.bat
        ```
    * A new MSYS2 window will open. This script automates the following:
        * Updating MSYS2 packages (`pacman`).
        * Installing all required C/GTK build dependencies (`pacman`).
        * Cloning the `sourcehold-maps` repository (next to this project's directory).
        * Installing `sourcehold-maps` from source into the MSYS2 Python environment (`pip`).
        * Configuring the build using Meson (`meson setup`).
        * Compiling the project using Ninja (`ninja -C build`).
        * Installing the project within MSYS2 using Ninja (`ninja -C build install`).
    * Monitor the MSYS2 window for progress or errors. It may take some time.

4.  **Run the Application:**
    * Once the script finishes, the MSYS2 window should remain open. The application is built and installed.
    * In that same MSYS2 window, run the application:
        ```
        gtk-crusader-village.exe
        ```

5.  **Configure Python Path:**
    * The helper script installed `sourcehold-maps` into the MSYS2 Python environment.
    * Launch the application (if not already running).
    * Go to **Preferences/Settings** and set the **Python executable path** to the MSYS2 MinGW Python: `C:\msys64\mingw64\bin\python.exe`.
    * Save settings and restart the app if needed.

### Option 3: Build from Source (Manual Steps)

This option provides full control over the build process.

1.  **Install MSYS2:**
    * Download and install MSYS2 from [https://www.msys2.org/](https://www.msys2.org/). Use the default installation path (`C:\msys64`).

2.  **Clone the Repository:**
    * Open **Git Bash**, **Command Prompt**, or **PowerShell**.
    * Clone this repository:
        ```
        git clone https://github.com/kolunmi/gtk-crusader-village.git
        cd gtk-crusader-village
        ```

3.  **Update MSYS2 Packages:**
    * Open the **"MSYS2 MSYS"** shell from the Start Menu.
    * Run `pacman -Syu` (follow prompts, may need to close and run again).
        ```
        pacman -Syu
        ```
    * Run `pacman -Su` to ensure everything is updated.
        ```
        pacman -Su
        ```
    * **Close** the "MSYS2 MSYS" shell.

4.  **Install Build Dependencies:**
    * Open the **"MSYS2 MinGW x64"** shell from the Start Menu. Navigate to the project directory: `cd /c/path/to/gtk-crusader-village` (adjust path as needed).
    * Install dependencies:
        ```
        pacman -S --needed --noconfirm git mingw-w64-x86_64-toolchain mingw-w64-x86_64-meson mingw-w64-x86_64-ninja mingw-w64-x86_64-python mingw-w64-x86_64-pkgconf mingw-w64-x86_64-gtk4 mingw-w64-x86_64-libadwaita mingw-w64-x86_64-json-glib mingw-w64-x86_64-gettext mingw-w64-x86_64-desktop-file-utils mingw-w64-x86_64-appstream mingw-w64-x86_64-python-pip
        ```

5.  **Configure the Build:**
    * In the MSYS2 MinGW x64 shell (already in the project directory):
        ```
        meson setup build --prefix=/mingw64 --buildtype=release
        ```

6.  **Compile the Project:**
    * In the same shell:
        ```
        ninja -C build
        ```

7.  **Install the Project:**
    * In the same shell:
        ```
        ninja -C build install
        ```
        *(Ignore potential errors about `update-desktop-database`)*

8.  **Set up Python Dependency:**
    * Follow the steps in the **Runtime Dependency: Python + `sourcehold-maps` Library** section below to install `sourcehold-maps` into your desired Python environment (this could be the MSYS2 one or a separate standard Windows Python install).

9.  **Run the Application:**
    * In the MSYS2 MinGW x64 shell:
        ```
        gtk-crusader-village.exe
        ```

10. **Configure Python Path:**
    * Launch the application.
    * Go to **Preferences/Settings** and set the **Python executable path** to the `python.exe` where you installed `sourcehold-maps`.

---

## Runtime Dependency: Python + `sourcehold-maps` Library

For full functionality (parsing `.map`, `.aiv` files), this application requires Python 3 and the `sourcehold-maps` library. This section details setting it up manually, which is necessary if you downloaded the pre-built version (Option 1) or built manually (Option 3) and didn't rely on the helper script's Python setup.

**Requirements:**

1.  **A Python 3 Installation:** Install from [python.org](https://www.python.org/) if needed. Ensure `pip` is accessible.
2.  **The `sourcehold-maps` Python Library.**

**Manual Setup Steps:**

1.  **Install `sourcehold-maps` Library:** Choose one method:

    * **Method A (Wheel File):**
        1.  Go to [https://github.com/sourcehold/sourcehold-maps](https://github.com/sourcehold/sourcehold-maps) -> Actions tab.
        2.  Find the latest successful run -> Artifacts -> Download `dist`.
        3.  Extract the zip. Find the `.whl` file matching your Python/OS.
        4.  Open Windows Command Prompt or PowerShell (using your chosen Python).
        5.  Install the wheel:
            ```
            pip install path\to\your\downloaded\sourcehold_maps-XXXX.whl
            ```

    * **Method B (Source Code):**
        1.  Ensure Git is installed. Open Command Prompt or PowerShell.
        2.  Clone the repo:
            ```
            git clone https://github.com/sourcehold/sourcehold-maps.git
            cd sourcehold-maps
            ```
        3.  Install with pip (using your chosen Python):
            ```
            pip install .
            ```

2.  **Configure Path in `GTK Crusader Village` Application:**
    * Launch `GTK Crusader Village` (use the MSYS2 MinGW x64 shell if you built it yourself).
    * Open **Preferences/Settings**.
    * Find the **Python executable path** setting.
    * Set it to the **full path** of the `python.exe` where you just installed `sourcehold-maps`.
    * Save settings. Restart the app if needed.

---

## Building (Linux)

These instructions are primarily for Linux distributions.

```
# Configure the build environment
meson setup build --prefix=/usr/local --buildtype=release

# Compile the project
ninja -C build

# Install the project (optional)
sudo ninja -C build install
```

## This is a WIP

Most functionality is finished, however.

## Acknowledgments

`src/gtk-crusader-village-util-bin.c` and the theme portal client in `src/gtk-crusader-village-application.c` were taken from [libadwaita](https://gitlab.gnome.org/GNOME/libadwaita).

All assets or ideas originating from the original Stronghold Crusader game in this repo are indicated as such and belong to [Firefly Studios](https://fireflyworlds.com/). See `src/shc-data/`. Thanks for creating such an amazing game!
