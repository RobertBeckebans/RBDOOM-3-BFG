#!/usr/bin/bash

# Set game folder paths
rbdoom3bfg_data_dir=${XDG_DATA_HOME}/rbdoom3bfg
rbdoom3bfg_base_dir=${rbdoom3bfg_data_dir}/base

# Check data folder
if [ ! -d ${rbdoom3bfg_data_dir} ]
then
    mkdir -p ${rbdoom3bfg_data_dir}
fi

# Check base folder
if [ ! -d ${rbdoom3bfg_base_dir} ]
then
    zenity --width=400 --error --title="Game data files not found" \
    --no-wrap --text="The /base game folder must be copied to:\n${rbdoom3bfg_base_dir}" \
    --ok-label "Close"
    exit 1
fi

# Check version matching
rbdoom3bfg_setup_commit_file=${rbdoom3bfg_data_dir}/.setup
rbdoom3bfg_setup_commit=$(cat $rbdoom3bfg_setup_commit_file)
rbdoom3bfg_app_commit=$(grep app-commit /.flatpak-info | cut -d "=" -f 2)
if [ "$rbdoom3bfg_setup_commit" != "$rbdoom3bfg_app_commit" ];
then
    echo "New version detected, copying data patches..."
    cp --recursive --force --verbose /app/share/rbdoom3bfg/base/* --target-directory ${rbdoom3bfg_base_dir} | tee \
        >(zenity --progress --pulsate --auto-close --no-cancel --width=300 --title "New version detected" --text "Patching base data files...")
    echo ${rbdoom3bfg_app_commit} > ${rbdoom3bfg_setup_commit_file}
    echo "Data patches copied"
else
    echo "The current version matches: skip data patching"
fi

# Run game engine
exec /app/bin/RBDoom3BFG "$@"
