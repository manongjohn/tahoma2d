choco install opencv --version=4.11.0
choco install boost-msvc-14.3 --version=1.87.0

mkdir thirdparty\qt

REM Install Qt 6.7
curl -fsSL -o Qt6.7.3_msvc2022_64.zip https://github.com/tahoma2d/qt5/releases/download/v6.7.3/Qt6.7.3_msvc2022_64.zip
7z x Qt6.7.3_msvc2022_64.zip
move 6.7.3 thirdparty\qt
