set filename=_rbdoom_global_illumination_data.pk4
7z a -tzip %filename% generated/images/env/maps generated/maps/game/*.blightgrid maps/game/*.lightgrid -x!testmaps
pause
