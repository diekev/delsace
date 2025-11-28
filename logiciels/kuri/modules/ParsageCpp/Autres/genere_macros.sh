kuri génère_valeurs_macros_standard
./génère_valeurs_macros_standard > /tmp/macros_standard.cc
g++-12 /tmp/macros_standard.cc -o a.out
./a.out