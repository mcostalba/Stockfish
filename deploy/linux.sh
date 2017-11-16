#!/bin/sh -e

cd src

echo
echo "### Linker info"
echo
ldd --version || echo "no ldd"

echo
echo "### CPU capabilities"
echo
grep "^flags" /proc/cpuinfo || echo

echo
echo "### Running profile-build for x86-64 ..."
echo
make clean
make profile-build ARCH=x86-64 EXE=stockfish-x86_64

if [ -f /proc/cpuinfo ]; then
    make clean
    if grep "^flags" /proc/cpuinfo | grep -q popcnt ; then
        echo
        echo "### Running profile-build for x86-64-modern ..."
        echo
        make profile-build ARCH=x86-64-modern EXE=stockfish-x86_64-modern
    else
        echo
        echo "### Running build for x86-64-modern ..."
        echo
        make build ARCH=x86-64-modern EXE=stockfish-x86_64-modern
    fi

    make clean
    if grep "^flags" /proc/cpuinfo | grep popcnt | grep -q bmi2 ; then
        echo
        echo "### Running profile-build for x86-64-bmi2 ..."
        echo
        make profile-build ARCH=x86-64-bmi2 EXE=stockfish-x86_64-bmi2
    else
        echo
        echo "### Running build for x86-64-bmi2 ..."
        echo
        make build ARCH=x86-64-bmi2 EXE=stockfish-x86_64-bmi2
    fi
fi
