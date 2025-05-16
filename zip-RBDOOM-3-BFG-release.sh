#!/bin/bash

# Get current date in yyyymmdd format
date_string=$(date +"%Y%m%d")

# Get first 7 characters of the git hash
git_hash=$(git rev-parse --short=7 HEAD)

buildversion="1.6.0.22"
filename="RBDOOM-3-BFG-${buildversion}-lite-win64-${date_string}-git-${git_hash}.7z"

# Copy the base file to the new filename
cp "RBDOOM-3-BFG-1.6.0.x-lite-win64-yyyymmdd-git-xxxxxxx.7z" "$filename"

# Copy necessary files
cp build/Release/RBDoom3BFG.exe .
cp build/tools/compilers/release/rbdmap.exe .

# Add files to the archive
7z a "$filename" RBDoom3BFG.exe rbdmap.exe

7z a "$filename" README.md RELEASE-NOTES.md LICENSE.md LICENSE_EXCEPTIONS.md \
    base/*.cfg base/materials/*.mtr \
    base/textures/common base/textures/editor \
    base/maps/zoomaps -x!generated -xr!autosave -xr!*.xcf -xr!*.blend

#7z a "$filename" base/maps/game/*_extra_ents.map
7z a "$filename" base/renderprogs2/
7z a "$filename" base/_tb/fgd/*.fgd
7z a "$filename" base/_tb/ -xr!*.png -xr!*.tga
7z a "$filename" base/_bl/*.json

7z a "$filename" base/_rbdoom_blood.pk4
7z a "$filename" base/_rbdoom_core.pk4

7z a "$filename" tools/trenchbroom -xr!TrenchBroom-nomanual* -xr!TrenchBroom.pdb
7z a "$filename" tools/optick-profiler


# Extend to full version
fullname="RBDOOM-3-BFG-${buildversion}-full-win64-${date_string}-git-${git_hash}.7z"
cp "$filename" "$fullname"

7z a "$fullname" base/_rbdoom_global_illumination_data.pk4

# Pause for user input
#read -p "Press any key to exit..."
