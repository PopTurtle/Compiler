#!/bin/bash

codes_dir="./codes/"
compiler_path="../compiler/algosipro"
compiled_asipro_path="./test_compiled.asipro"
compiled_sipro_path="./test_compiled.sipro"

RESET='\033[0m'
RED='\033[0;31m'
GREEN='\033[0;32m'
BOLD='\033[1m'

#  test_cmd [command] [expected_result] : explicite
function test_cmd {
        result=$($1)
        if [ $? != 0 ]; then
                echo ""
                printf "\n${RED}${BOLD}An error occurred during file test${RESET}\n"
                echo "Command: $1"
                exit 1
        fi
        if [ $result != $2 ]; then
                printf "\n${RED}${BOLD}Error on command: $1${RESET}\n"
                echo "Got: $result"
                echo "Expected: $2"
                exit -1
        fi
}

# compile [file_path] : Compile le fichier algo au chemin [file_path] en fichier
# asipro et sipro (chemins: $compiled_asipro_path et $compiled_sipro_path)
function compile {
    $compiler_path < $1 > $compiled_asipro_path
    if [ $? != 0 ]; then
            echo ""
            printf "\n${RED}${BOLD}An error occurred during file compilation${RESET}\n"
            printf "${RED}File: ${1}${RESET}\n"
            exit 1
    fi
    asipro $compiled_asipro_path $compiled_sipro_path 2> /dev/null
}

#  test [file_name] [expected_result] : compile et execute le fichier
#    se trouvant au chemin $codes_dir[file_name].algo, et vérifie que le résultat
#    renvoyé par l'execution est [expected_result].
function test {
    echo ""
    echo "Compiling $1.algo"
    compile "$codes_dir$1.algo"
    echo "Testing $1.algo"
    test_cmd "sipro $compiled_sipro_path" $2
    printf "${GREEN}${BOLD}>>>\t${RESET}${GREEN}File $1.algo passed${RESET}\n"
}

# all_tests : Lance les tests unitaires
function all_tests {
    printf "\n${GREEN}${BOLD}Starting tests here${RESET}\n"

    test simple 5
    test assignement 18
    test not 30
    test if_and_recursive 13
    test decr_incr 1115
    test egyptian_multiplication 1598
    test recursion 81
    test recursion_opt 10
    test fibonacci 55
    test mutual_recursion 5460
    test expr_opt 0

    printf "\n${GREEN}${BOLD}No error occured during tests${RESET}\n"
}


all_tests
rm -f $compiled_asipro_path
rm -f $compiled_sipro_path
rm -f gmon.out
