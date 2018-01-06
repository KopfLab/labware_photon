# Photon labware

Base classes for particle photons tied into the lab device and data infrastructure.

## Usage

This repository could at some point be turned into a particle photon library but is too lab specific at the moment.
To include as a submodule in labware projects:

- `cd` to the other labware project, there `git submodule add https://github.com/KopfLab/labware_photon device`
- to update (or when someone else is checking out the project for) `git submodule update --init --recursive`

Then reference to the `labware_photon` classes via:

- `#include "device/Device???.h"`
