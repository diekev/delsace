#!/bin/bash

if [[ -f a.out ]]; then
    rm a.out
fi

if [[ -f .chaines_ajoutées ]]; then
    rm .chaines_ajoutées
fi

FICHIERS=`find . -maxdepth 1 -type f -name "*.kuri"|sort`

for fichier in ${FICHIERS[@]}; do
    echo "Compilation de :" $fichier
    kuri $fichier 2&> /dev/null

    if [[ ! -f a.out ]]; then
        echo -e "    \e[1m\e[31mÉCHEC\e[0m"
        continue
    fi

    sortie="$(./a.out)"

    if [[ $sortie == "SUCCÈS" ]]; then
        echo -e "    \e[1m\e[32mSUCCÈS\e[0m"
    else
        echo -e "    \e[1m\e[31mÉCHEC\e[0m"
    fi

    rm a.out

    if [[ -f .chaines_ajoutées ]]; then
        rm .chaines_ajoutées
    fi
done
