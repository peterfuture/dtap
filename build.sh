#!/bin/bash

usage()
{
    echo "usage: `basename $0` linux | android | all | clean"
}

OPT=$1

if [ $# -ne 1 ]; then
    usage
    exit 1
fi

DEST=`pwd`/build/

if [ -d build ]; then
rm -rf build
fi

mkdir -p build

case $OPT in
     linux|Linux) echo "build for Linux platform.."
         cd lvm
         make -f makefile clean
         make -f makefile
         cd ..
         mkdir -p build/linux
         cp lvm/*.a   build/linux
         make -f makefile clean
         make -f makefile
         cp *.so  build/linux
         cp *.a   build/linux
         cp *.exe build/linux
         ;;
     android|Android) echo "build for Android platform.."
         cd lvm
         make -f makefile-android clean
         make -f makefile-android
         cd ..
         mkdir -p build/android
         cp lvm/*.a   build/android
         make -f makefile-android clean
         make -f makefile-android
         cp *.so  build/android
         ;;
     all|All) echo "build for Linux & Android platform.."
         cd lvm
         make -f makefile clean
         make -f makefile
         cd ..
         mkdir -p build/linux
         cp lvm/*.a   build/linux
         make -f makefile clean
         make -f makefile
         cp *.so  build/linux
         cp *.a   build/linux
         cp *.exe build/linux
         
         cd lvm
         make -f makefile-android clean
         make -f makefile-android
         cd ..
         mkdir -p build/android
         cp lvm/*.a   build/android
         make -f makefile-android clean
         make -f makefile-android
         cp *.so  build/android
         ;;
     clean|CLEAN) echo "enter clean.."
         cd lvm
         make -f makefile clean
         cd ..
         make -f makefile clean
         ;;    
     *)usage
         ;;
 esac
