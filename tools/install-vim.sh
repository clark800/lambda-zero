#!/bin/sh
if ! vim --version | grep -q -- +conceal; then
    echo "Warning: vim not compiled with support for concealing"
fi
cp -r vim/* ~/.vim
