# Arena Ready Markers

This is a module for [AzerothCore](http://www.azerothcore.org/) that adds ready markers for arenas, allowing the start of matches to be expedited when all players mark themselves as ready.

## How to Install

The [guide found here](https://www.azerothcore.org/wiki/installing-a-module) can be referred to for installation.

The required SQL queries are placed in a directory that will be picked up by the worldserver's auto-updater.

### (Optional) Edit Module Configuration

You are also able to edit the [mod_arena-ready-marker.conf](https://github.com/heyitsbench/mod-arena-ready-marker/blob/master/conf/mod_arena-ready-marker.conf.dist) to your liking. At the time of writing, the only option is a simple enable/disable switch, but this may be expanded in the future.

## Usage

Ready marker objects are placed within the starting safe-zone of an arena map for each team. Players click the markers and are told how many are ready as they are clicked. When all the markers are activated, the match countdown jumps to fifteen seconds until match start, and the markers are removed.

## How to Uninstall

To uninstall the module, you must remove the module from the `modules` directory of your AC source, rerun CMake steps depending on your OS, and recompile. You will also need to remove the offending rows from the `gameobject_template` table and associated tables related to the ready marker object.
