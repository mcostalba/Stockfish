#!/bin/sh

export PATH=.:/mingw64/bin:/usr/local/bin:/mingw/bin:/bin
export
mkdir "$APPVEYOR_REPO_BRANCH"

cd src

make clean
make profile-build COMP=mingw ARCH=x86-64
strip stockfish.exe
mv stockfish.exe "../$APPVEYOR_REPO_BRANCH/stockfish-windows-x86_64.exe"

make clean
make profile-build COMP=mingw ARCH=x86-64-modern
strip stockfish.exe
mv stockfish.exe "../$APPVEYOR_REPO_BRANCH/stockfish-windows-x86_64-modern.exe"

make clean
make profile-build COMP=mingw ARCH=x86-64-bmi2
strip stockfish.exe
mv stockfish.exe "../$APPVEYOR_REPO_BRANCH/stockfish-windows-x86_64-bmi2.exe"

cd ..
