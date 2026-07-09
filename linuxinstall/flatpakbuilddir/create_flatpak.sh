#!/bin/bash

cd "$(dirname "$0")"
flatpak-builder --force-clean --user --install-deps-from=flathub --repo=repo builddir ../com.infinipaint.infinipaint.yml
flatpak build-bundle repo infinipaint.flatpak com.infinipaint.infinipaint
