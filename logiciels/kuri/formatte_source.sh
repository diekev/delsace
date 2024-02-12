#!/bin/bash

FICHIERS=`find . -type f \( -name "*.cc" -or -name "*.h" -or -name "*.hh" \)`

for fichier in ${FICHIERS[@]}; do
    if [[ `git check-ignore $fichier` ]]; then
        continue
    fi

    echo "Formattage de :" $fichier
    clang-format-18 -i $fichier
done
