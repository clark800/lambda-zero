#!/bin/sh

if ! vim --version | grep -q -- +conceal; then
    echo "Warning: vim not compiled with support for concealing"
fi

if [ -d ~/.config/vim/runtime ]; then
    cp -r vim/* ~/.config/vim/runtime
elif [ -d ~/.vim ]; then
    cp -r vim/* ~/.vim
else
    echo "Error: could not locate vim runtime directory"
fi
