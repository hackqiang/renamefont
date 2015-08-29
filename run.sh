#!/bin/bash

if [ ! -f 1c2.obj ]; then
	sudo chmod +x ./pdf-parser.py
	./pdf-parser.py ../gb1/1c2.pdf > 1c2.obj
fi

cc -g renamefont.c -lz
rm -rf out.pdf stream
mkdir stream
./a.out ../gb1/1c2.pdf out.pdf < 1c2.obj > a.log