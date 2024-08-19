#!/bin/sh

set -e

git clone https://github.com/Koromix/rygel
cd rygel
./bootstrap.sh
./felix -pFast tycmd
ln -s ./bin/Fast/tycmd
cd ..
ln -s rygel/tycmd
