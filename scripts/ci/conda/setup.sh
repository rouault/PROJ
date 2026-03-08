#!/bin/bash

conda update -n base -c defaults conda -y
conda install rattler-build ninja compilers yq -y

pwd
ls
git clone https://github.com/conda-forge/proj.4-feedstock.git

cd proj.4-feedstock

yq -y -i '.source = {"path": "../../../PROJ"} | .build.number = 2112' recipe/recipe.yaml

ls recipe
