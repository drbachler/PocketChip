#!/usr/bin/env bash

#------------------------------------ DEBUG -----------------------------------#
# Force the script to end when any of the following conditions are met:
#   1.- (-e) A command is executed and it miserably fails.
#   2.- (-o) A command is executed succesfully but with novelties.
#   3.- (-u) A command tries to make use of a variable not defined beforehand.
#   4.- (-x) Show the instruction that is currently being executed.

set -eou pipefail

TMP_INSTALL_DIR=$HOME/install_tmp

# Function declaration.
function local_sources() {
    APT_REPO="1W77d5naX4qQfYYg6HnF_lT2z53Q5w8AJ"
    GD_CODE=`wget -nv --save-cookies cookies.txt --keep-session-cookies --no-check-certificate "https://docs.google.com/uc?export=download&id=$APT_REPO" -O- | sed -rn 's/.*confirm=([0-9A-Za-z_]+).*/Code: \1\n/p' | tail -c 5`

    sudo apt install -y proftpd-basic
    sudo chown -R chip /srv/ftp
    cd /srv/ftp
    wget --load-cookies cookies.txt "https://docs.google.com/uc?export=download&confirm=$GD_CODE&id=$APT_REPO" -O repo.tar.gz
    tar -xzvf repo.tar.gz
    rm repo.tar.gz cookies.txt
    echo "deb ftp://127.0.0.1/opensource.nextthing.co/chip/debian/repo jessie main" | sudo tee -a /etc/apt/sources.list
    echo "deb ftp://127.0.0.1/opensource.nextthing.co/chip/debian/pocketchip jessie main" | sudo tee -a /etc/apt/sources.list
    sudo echo "deb ftp://127.0.0.1/opensource.nextthing.co/chip/debian/repo jessie main" >> /etc/apt/sources.list
    sudo echo -e "<Anonymous ~ftp>\n\tUser ftp\n\tGroup nogroup\n\tUserAlias anonymous ftp\n\tRequireValidShell off\n\t<Directory *>\n\t\t<Limit WRITE>\n\t\t\tDenyAll\n\t\t</Limit>\n\t</Directory>\n</Anonymous>" >> /etc/proftpd/conf.d/anonymous.conf 
    sudo service proftpd restart
    cd $HOME
}

function remote_sources() {
    echo "deb http://chip.jfpossibilities.com/chip/debian/repo jessie main" | sudo tee -a /etc/apt/sources.list
    echo "deb http://chip.jfpossibilities.com/chip/debian/pocketchip jessie main" | sudo tee -a /etc/apt/sources.list
}

function kill_x() {
    echo "Say bye to graphical mode!"

    sudo systemctl set-default multi-user.target
    mkdir $HOME/.local/share/keymaps
    echo -e "keymaps 0-2,4-5,8,12\naltgr keycode  2 = F1\naltgr keycode  3 = F2\naltgr keycode  4 = F3\naltgr keycode  5 = F4\naltgr keycode  6 = F5\naltgr keycode  7 = F6\naltgr keycode  8 = F7\naltgr keycode  9 = F8\naltgr keycode 10 = F9\naltgr keycode 11 = F10\naltgr keycode 74 = F11\nshift keycode 74 = underscore\naltgr keycode 13 = F12\naltgr keycode 21 = braceleft\naltgr keycode 22 = braceright\naltgr keycode 23 = bracketleft\naltgr keycode 24 = bracketright\naltgr keycode 25 = bar\naltgr keycode 35 = less\naltgr keycode 36 = greater\naltgr keycode 37 = apostrophe\naltgr keycode 38 = quotedbl\naltgr keycode 48 = grave\naltgr keycode 49 = asciitilde\naltgr keycode 50 = colon\naltgr keycode 52 = semicolon\nshift keycode 52 = comma\naltgr keycode 53 = backslash\n" > $HOME/.local/share/keymaps/key_map
    sudo loadkeys $HOME/.local/share/keymaps/key_map
}

function build_retroarch() {
    echo "Installin Retro Arch"

    git clone git://github.com/libretro/RetroArch.git
    cd RetroArch/

    ./configure --enable-sdl --disable-sdl2 --enable-floathard --enable-neon --disable-opengl --disable-vg --disable-egl --disable-pulse --disable-oss --disable-x11 --disable-wayland --disable-ffmpeg --disable-7zip --disable-libxml2 --disable-freetype
    
    make
    sudo make install
}

function install_retroarch() {
    cd TMP_INSTALL_DIR
    cp retroarch /usr/local/bin
    cp cg2glsl.py /usr/local/bin/retroarch-cg2glsl
    chmod 755 /usr/local/bin/retroarch
    chmod 755 /usr/local/bin/retroarch-cg2glsl
    mkdir $HOME/.config/retroarch
    mv cfg/* $HOME/.config/retroarch
    mv ROM $HOME
    cd $HOME
}

echo "Deleting default dirs"

cd $HOME
rm -rf Desktop  Documents  Downloads  Music  Pictures  Public  Templates  Videos

echo "Remove power saving on Wi-Fi"

sudo iw dev wlan0 set power_save off
echo "iw dev wlan0 set power_save off" > wlan_pwr
sudo mv wlan_pwr /etc/network/if-up.d/

echo "Replacing Next Thing Co. repos in source lists"

sudo sed -i '/deb http:\/\/opensource.nextthing.co\/chip\/debian\/pocketchip jessie main/d' /etc/apt/sources.list
sudo sed -i '/deb http:\/\/opensource.nextthing.co\/chip\/debian\/repo jessie main/d' /etc/apt/sources.list

while true; do
    read -p "Do you want to store Next Thing Co. repos locally?[y/n]" answer
    case $answer in
        [Yy]* ) local_sources; break;;
        [Nn]* ) remote_sources; break;;
        * ) echo "Please answer yes or no.";
    esac
done

echo "Updating the thing!!"

sudo apt update
sudo apt -y dist-upgrade

echo "Installing all the stuff!!"

sudo apt install -y git build-essential libasound2-dev libx11-dev libxrandr-dev \
libxcursor-dev libxft-dev libxinerama-dev network-manager-dev iw bash-completion \
libi2c-dev libssl-dev xinput-calibrator ssh libsdl1.2-dev vim locales kbd \
libsdl2-dev libboost-system-dev libboost-filesystem-dev libboost-date-time-dev \
libfreeimage-dev libfreetype6-dev libeigen3-dev libcurl4-openssl-dev \
libgl1-mesa-dev cmake pkg-config rsync minicom pulseaudio-module-bluetooth \
blueman libxext-dev

echo "General config"

sudo passwd -l root    # lock the root account from direct login
sudo sed -i.old /etc/ssh/sshd_config -e'/PermitRootLogin/s/yes/no/'    # configure sshd to not allow root
sudo service ssh restart
sudo usermod -a -G dialout $USER
sudo chown -R chip /etc/minicom
sudo sed -i 's/exit 0/sudo systemctl stop serial-getty@ttyS0\nexit 0/g' /etc/rc.local

echo "Reconfiguring locales"

sudo dpkg-reconfigure locales           # if in the USA, select "en_US" locales.
sudo dpkg-reconfigure tzdata            # select your timezone

echo "Installing new terminal"

git clone git://git.suckless.org/st
cd st
sed -i 's/Liberation Mono/Monospace/g' config.def.h
sed -i 's/pixelsize=12/pixelsize=10/g' config.def.h 
sed -i 's/\/bin\/sh/\/bin\/bash/g' config.def.h
sed -i 's/cols = 80/cols = 100/g' config.def.h
sed -i 's/rows = 24/rows = 30/g' config.def.h
sudo make clean install
cd $HOME

echo "Downloading all the stuff!"

mkdir $TMP_INSTALL_DIR
cd $TMP_INSTALL_DIR
git init
git remote add -f origin https://github.com/igrigar/PocketChip.git
git config core.sparseCheckout true

echo "pocket-home_0.0.8.9-1_armhf.deb" >> .git/info/sparse-checkout
echo "Cfg" >> .git/info/sparse-checkout
echo "RetroArch" >> .git/info/sparse-checkout
echo "Icons" >> .git/info/sparse-checkout

git pull origin master
cd $HOME

echo "Installing PocketHome"

mkdir -p $HOME/.pocket-home/
echo "0.8.9-1" > $HOME/.pocket-home/.version
sudo dpkg -i $TMP_INSTALL_DIR/pocket-home_0.0.8.9-1_armhf.deb

echo "Setting configurations"

cp $TMP_INSTALL_DIR/Cfg/bash_aliases $HOME/.bash_aliases
cp $TMP_INSTALL_DIR/Cfg/bashrc $HOME/.bashrc
cp $TMP_INSTALL_DIR/Cfg/vimrc $HOME/.vimrc
cp -a $TMP_INSTALL_DIR/Cfg/vim $HOME/.vim

mkdir -p $HOME/.pocket-home/icons
cp $TMP_INSTALL_DIR/Cfg/config.json $HOME/.pocket-home
cp $TMP_INSTALL_DIR/Icons/* $HOME/.pocket-home/icons

while true; do
    read -p "Do you want to boot diirectly in text mode?[y/n]" answer
    case $answer in
        [Yy]* ) kill_x; break;;
        [Nn]* ) break;;
        * ) echo "Please answer yes or no.";
    esac
done

while true; do
    read -p "Do you want to build retroarch from source?[y/n]" answer
    case $answer in
        [Yy]* ) build_retroarch; break;;
        [Nn]* ) break;;
        * ) echo "Please answer yes or no.";
    esac
done

rm -rf $TMP_INSTALL_DIR
