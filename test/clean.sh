#! /bin/bash 

find ../ -name "*~" -exec rm -fv {} \;

rm -fv ./*.so ./*.exe

