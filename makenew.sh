#!/bin/bash

cd `dirname $0`
DIR="${HOME}/src/$1"

mkdir -p "$DIR"
cp *.ino "${DIR}/${1}.ino"
cp mpub Makefile "$DIR"
cd "$DIR"
git init
git add *
git commit -m "Initial commit"

