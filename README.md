# Dinothawr
Dinothawr is a block pushing puzzle game on slippery surfaces.
Our hero is a dinosaur whose friends are trapped in ice.
Through puzzles it is your task to free the dinos from their ice prison.

### Credits
   - Agnes Heyer (art, level design, some code)
   - Hans-Kristian Arntzen (programming, music, some level design)

## Downloads (Android)
Android APK ([link](http://themaister.net/dinothawr/Dinothawr.apk), [QR](http://themaister.net/dinothawr/qr.png))

## Downloads (Bundled Windows 32-bit/64-bit)
   - Dinothawr v1.0 (Windows 32-bit) [here](http://themaister.net/dinothawr/dinothawr-win32-v1.0.zip)
   - Dinothawr v1.0 (Windows 64-bit) [here](http://themaister.net/dinothawr/dinothawr-win64-v1.0.zip)

### Downloads (Other)

   - Game files ([link](http://themaister.net/dinothawr/dinothawr-data.zip))
   - Libretro cores (Win32, Win64, Linux 64-bit) ([link](http://themaister.net/dinothawr/libretro-cores.zip))

#### Playing the game in RetroArch
The game itself is a shared library and needs a libretro frontend (e.g. RetroArch) to run.
To play Dinothawr, use the right libretro core, and dinothawr.game as a game ROM.
An example command line would be:

    retroarch -L dinothawr_libretro_linux_x86_64.so dinothawr/dinothawr.game

### Controls (gamepad)
Dinothawrs gamepad control are mapped as shown [here](http://themaister.net/dinothawr/shield.png).

### Controls (keyboard)
On PC build of RetroArch, the default keyboard binds are:

   - Z: Push
   - X: Toggle menu
   - S: Reset level
   - Arrow keys: Move around
   - Escape: Exit game

### Platforms

Dinothawr supports a large number of platforms. We only provide bundled builds for Android.
After release, we expect bundled builds to show up.

### libretro/RetroArch
Dinothawr implements the [libretro API](http://libretro.com), and uses e.g. [RetroArch](https://github.com/libretro/RetroArch) as a frontend. On Android, RetroArch is bundled, and is transparent to the user.

### Building (Android)
Make sure latest SDKs and NDKs (r9) are installed.

#### Clone repo
    git clone git://github.com/Themaister/Dinothawr.git
    cd Dinothawr
    export DINOTHAWR_TOP_FOLDER="$(pwd)"

#### Build native libretro library
    cd android/eclipse/jni
    ndk-build -j4

#### Build RetroArch native activity
    git clone git://github.com/libretro/RetroArch.git
    cd RetroArch
    cd android/native/jni
    ndk-build -j4

#### Copy RetroArch libraries to dinothawr
    cd ../libs
    cp -r armeabi-v7a x86 mips "$DINOTHAWR_TOP_FOLDER/android/eclipse/libs/"

#### Copy Dinothawr assets
    cd "$DINOTHAWR_TOP_FOLDER"
    mkdir -p android/eclipse/assets
    cp -r dinothawr/* android/eclipse/assets/

#### Build Java frontend
Open Eclipse and import project from `android/eclipse`. You should see Dinothawr assets in assets/ folder, and various libraries in libs/.
Try running the project on your device, and you should see Dinothawr.apk in android/eclipse/bin/.

### Building (Linux, OSX, Windows)

#### Clone repo
    git clone git://github.com/Themaister/Dinothawr.git
    cd Dinothawr

#### Build libretro core
    make -j4   # (on OSX, you might need make CC=clang CXX="clang++ -stdlib=libc++")

#### Run Dinothawr in RetroArch
    retroarch -L dinothawr_libretro.so dinothawr/dinothawr.game

### Customizing / Hacking 
Dinothawr is fairly hackable. dinothawr.game is the game file itself. It is a simple XML file which points to all assets used by the game.
Levels are organized in chapters. Levels themselves are created using the [Tiled](http://www.mapeditor.org/) editor.
If you want to try making your own levels, make sure you use the "plain XML" format for .tmx files and not the default zlib base64.

