#!/bin/tcsh

rsync -a --copy-links --delete --progress ../*.c ../*.h ../*.csh ../*.py ../*.tex ../*.pdf ../Makefile* ../README* .
