#!/bin/bash

echo "Building arttools by SanyaWaffles"

if [ ! -d ./release ]; then
	mkdir ./release
fi

cd ./release

gcc ../src/art2png.c -I/opt/local/include -L/opt/local/lib -lfreeimage -arch x86_64 -arch i386 -o ./art2png
gcc ../src/png2art.c -I/opt/local/include -L/opt/local/lib -lfreeimage -arch x86_64 -arch i386 -o ./png2art
gcc ../src/palgen.c -I/opt/local/include -L/opt/local/lib -arch x86_64 -arch i386 -o ./palgen


echo "Copying to MacPorts directory"

if [ -f /opt/local/bin/art2png ]; then
	rm /opt/local/bin/art2png
fi

if [ -f /opt/local/bin/png2art ]; then
	rm /opt/local/bin/png2art
fi

if [ -f /opt/local/bin/palgen ] ; then
	rm /opt/local/bin/palgen
fi

cp -f art2png /opt/local/bin
cp -f png2art /opt/local/bin
cp -f palgen /opt/local/bin
