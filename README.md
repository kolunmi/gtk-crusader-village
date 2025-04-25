# GTK Crusader Village

![Image](https://github.com/user-attachments/assets/148a9cfd-46df-4bec-9023-41fd27077d5b)

## What is this

In [Stronghold Crusader](https://en.wikipedia.org/wiki/Stronghold:_Crusader), `.aiv` (AI Village) files detail the way enemy lords construct their castles in skirmish mode. The [official AIV editor](https://stronghold.heavengames.com/downloads/showfile.php?fileid=7534) serves its purpose, but I think it could be improved. This application aims to be an ergonomic, cross-platform `.aiv` editor GUI.

## Building

Right now I am developing on linux, but I will test other operating systems shortly.
```sh
meson setup build --prefix=/usr/local --buildtype=release
ninja -C build

# installation
sudo ninja -C build install
```

## This is a WIP

Most functionality is finished, however.

## Acknowledgments

`src/gtk-crusader-village-util-bin.c` and the theme portal client in `src/gtk-crusader-village-application.c` were taken from [libadwaita](https://gitlab.gnome.org/GNOME/libadwaita).

All assets or ideas originating from the original Stronghold Crusader game in this repo are indicated as such and belong to [Firefly Studios](https://fireflyworlds.com/). See `src/shc-data/`. Thanks for creating such an amazing game!

---

## Building and Running on Windows (using MSYS2)

These instructions guide you through building the project natively on Windows using the MSYS2 environment, which provides the necessary Linux-like tools and libraries.

**1. Install MSYS2:**
    - Download and install MSYS2 from [https://www.msys2.org/](https://www.msys2.org/). Follow the installation instructions on their website.
    - It's recommended to use the default installation directory (`C:\msys64`).

**2. Clone the Repository:**
    - Open a standard Windows Command Prompt (cmd.exe) or PowerShell (or Git Bash).
    - Clone this repository to your machine:
    ```
    git clone [https://github.com/kolunmi/gtk-crusader-village.git](https://github.com/kolunmi/gtk-crusader-village.git)
    cd gtk-crusader-village
    ```

**3. Update MSYS2 & Install Build Dependencies:**
    This step ensures your MSYS2 environment is up-to-date and installs all the tools and libraries needed to compile the project. It can optionally also set up the `sourcehold-maps` Python dependency. Choose one option:

* **Option A (Manual Steps):**
    1.  Open the **"MSYS2 MSYS"** shell from the Start Menu.
    2.  Update the core packages (run twice, closing the shell in between if prompted):
        ```
        pacman -Syu
        ```
        ```
        pacman -Su
        ```
    3.  **Close** the "MSYS2 MSYS" shell.
    4.  Open the **"MSYS2 MinGW x64"** shell from the Start Menu. **Use this shell for all build commands.**
    5.  Install the build dependencies using `pacman`:
        ```
        pacman -S --needed --noconfirm git mingw-w64-x86_64-toolchain mingw-w64-x86_64-meson mingw-w64-x86_64-ninja mingw-w64-x86_64-python mingw-w64-x86_64-pkgconf mingw-w64-x86_64-gtk4 mingw-w64-x86_64-libadwaita mingw-w64-x86_64-json-glib mingw-w64-x86_64-gettext mingw-w64-x86_64-desktop-file-utils mingw-w64-x86_64-appstream
        ```
        *(You can remove `--noconfirm` if you prefer to confirm each package group)*

* **Option B (Helper Script):**
    1.  Make sure you have completed Step 1 (Install MSYS2 to `C:\msys64`) and Step 2 (Clone Repository).
    2.  Locate the `setup_msys2_deps.bat` script in the root directory of this repository.
    3.  Double-click the `setup_msys2_deps.bat` script or run it from a standard Windows Command Prompt (cmd.exe) or PowerShell while inside the repository's root directory:
        ```batch
        setup_msys2_deps.bat
        ```
    4.  This script will automatically launch the MSYS2 shell in a new window and attempt to:
        * Run the necessary `pacman` update and C/GTK dependency installation commands (equivalent to Option A).
        * Clone the `sourcehold-maps` repository into the parent directory (next to this project's directory).
        * Install `sourcehold-maps` from source using `pip` into the MSYS2 MinGW Python environment.
    5.  Monitor the MSYS2 window for progress and any potential errors or prompts.
    6.  If the script runs successfully, it **replaces the manual `pacman` commands** (Option A) **AND attempts the source install for `sourcehold-maps`**. The MSYS2 shell window opened by the script can then be used for the subsequent build steps (Step 4 onwards).
    7.  **Note:** If using this script, proceed to Step 4 for building the C project. For the runtime Python configuration (see below), you **must** configure the application to use the MSYS2 MinGW Python path: `C:\msys64\mingw64\bin\python.exe`.

**4. Configure the Build:**
    - Ensure you are in the **"MSYS2 MinGW x64"** shell, inside the project's root directory (`gtk-crusader-village`).
    - Run Meson to configure the build:
    ```
    meson setup build --prefix=/mingw64 --buildtype=release
    ```

**5. Compile the Project:**
    - In the same MSYS2 shell, run Ninja to compile:
    ```
    ninja -C build
    ```

**6. Install the Project (Recommended for Running):**
    - In the same MSYS2 shell, install the compiled files:
    ```
    ninja -C build install
    ```

**7. Run the Application:**
    - **Important:** Always run the application from the **MSYS2 MinGW x64 shell**. This ensures it can find its required DLL files located in `C:\msys64\mingw64\bin`.
    - If you installed it (Step 6), simply run the executable name:
    ```
    gtk-crusader-village.exe
    ```
    - If you skipped installation (Step 6), run the executable from the build directory (while your shell's current directory is the project root):
    ```
    ./build/src/gtk-crusader-village.exe
    ```

---

## Runtime Dependency: Python + `sourcehold-maps` Library

For full functionality, such as parsing original Stronghold Crusader data files (`.map`, `.aiv`), this application requires access to a Python installation with the `sourcehold-maps` Python library installed. This is a **runtime** requirement, separate from building the application itself.

**Requirements:**

1.  **A Python 3 Installation:** You need a standard Python 3 installation on your Windows system. If you don't have one, download it from [python.org](https://www.python.org/). Ensure Python's `pip` installer is accessible from your command line (usually by adding Python to your system PATH during installation). **Note:** This should generally be a separate Python installation from the one inside MSYS2 used for building the C code, although using the MSYS2 one might work if configured correctly in the app's preferences.
2.  **The `sourcehold-maps` Python Library:** This library provides the file format parsing capabilities.

**Setup Steps:**

1.  **Install `sourcehold-maps` Library:**
    * **If you used the `setup_msys2_deps.bat` script (Step 3, Option B above):** The script already attempted to clone the `sourcehold-maps` repository and install it from source using `pip` into the MSYS2 MinGW Python environment. If it reported success, you can likely skip the manual installation steps below. Proceed to Step 2 (Configure Path) and use the MSYS2 Python path. If the script reported an error during the `pip install` phase, you may need to perform manual installation using Method A or B below into your preferred Python environment.
    * **If you did NOT use the helper script OR it failed:** You need to install the library manually into your chosen Python environment (from Requirement 1). Choose one of the following methods:
        * **Method A (Recommended by `sourcehold-maps`): Install from Wheel File**
        1.  Go to the `sourcehold-maps` GitHub repository: [https://github.com/sourcehold/sourcehold-maps](https://github.com/sourcehold/sourcehold-maps)
        2.  Navigate to the **Actions** tab.
        3.  Click on the latest successful workflow run listed under "Workflows" (usually named "CI" or similar).
        4.  Scroll down to the **Artifacts** section for that run. Download the build artifacts zip file (e.g., "dist").
        5.  Extract the downloaded zip file. Inside, you will find `.whl` (Wheel) files. Choose the one matching your Python version and system architecture (e.g., `...cp311...win_amd64.whl` for Python 3.11 on 64-bit Windows).
        6.  Open a standard Windows **Command Prompt** (cmd.exe) or **PowerShell**. Make sure it's using the Python installation from Requirement 1 (you can check with `python --version` and `pip --version`).
        7.  Use `pip` to install the wheel file you extracted (replace the example filename with the actual one):
            ```
            pip install path\to\your\downloaded\sourcehold_maps-1.0.2-cp311-cp311-win_amd64.whl
            ```
            *(Use backslashes `\` in paths for standard Windows cmd/PowerShell)*

    * **Method B (Alternative): Install from Source Code**
        1.  Ensure you have `git` installed and accessible from your standard Windows Command Prompt or PowerShell.
        2.  Clone the repository:
            ```
            git clone [https://github.com/sourcehold/sourcehold-maps.git](https://github.com/sourcehold/sourcehold-maps.git)
            cd sourcehold-maps
            ```
        3.  Use `pip` to install from the cloned source (this might download additional build dependencies mentioned in `requirements.txt`):
            ```
            pip install .
            ```

2.  **Configure Path in `GTK Crusader Village` Application:**
    * Launch the `GTK Crusader Village` editor (remember to use the `gtk-crusader-village.exe` command within the **MSYS2 MinGW x64 shell**).
    * Find and open the application's **Preferences** or **Settings** dialog (look through menus or for a settings button).
    * Locate the setting labelled **Python executable path** (or similar).
    * Set this field to the **full, absolute path** of the `python.exe` executable belonging to the Python installation where the `sourcehold-maps` library is installed.
        * **If you successfully used the `setup_msys2_deps.bat` script**, this path should be your MSYS2 MinGW Python executable: `C:\msys64\mingw64\bin\python.exe`
        * **If you installed manually** into a standard Windows Python installation (Method A or B), use the path to that specific `python.exe` (e.g., `C:\Users\YourName\AppData\Local\Programs\Python\Python311\python.exe`).
    * Save the settings in the dialog. You might need to restart `GTK Crusader Village` for the change to take effect.

After completing these steps, the runtime warning about the Python configuration should disappear, and the application should be able to use the `sourcehold-maps` library.

Sources and related content
