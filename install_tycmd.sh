
ls: An alternative to the teensy_cli uploader
#
# For the requirements see https://koromix.dev/tytools#build-on-linux
# If you are using Ubuntu, Debian, RPM you can also use the packages
# provided there. Otherwise, build from source. As dependencies, you
# need something like
#
#  sudo apt install libudev-dev
#  or pacman -S --needed udev
#
# USAGE is then just
#
#   ./tycmd upload  .pio/build/teensy41/firmware.hex
#
#

set -e

[[ -e rygel ]] || git clone --depth 1 https://github.com/Koromix/rygel
cd rygel
[[ -e felix ]] || ./bootstrap.sh
./felix -pFast tycmd
ln -s ./bin/Fast/tycmd
cd ..
ln -s rygel/tycmd

