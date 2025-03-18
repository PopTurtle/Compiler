#!/bin/bash

codes_dir="./codes/"
compiler_path="../compiler/parsing"
compiled_asipro_path="./test_compiled.asipro"
compiled_sipro_path="./test_compiled.sipro"

#  test_cmd [command] [expected_result] : explicite
function test_cmd {
        result=$($1)
        if [ $? != 0 ]; then
                echo ""
                echo "An error occured during file test"
                echo "Command: $1"
                exit 1
        fi
        if [ $result != $2 ]; then
                echo "Error on command: $1"
                echo "Got: $result"
                echo "Expected: $2"
                exit -1
        fi
}

# compile [file_path] : Compile le fichier algo au chemin [file_path] en fichier
# asipro et sipro (chemins: $compiled_asipro_path et $compiled_sipro_path)
function compile {
    $compiler_path < $1 > $compiled_asipro_path
    asipro $compiled_asipro_path $compiled_sipro_path 2> /dev/null
}

#  test [file_name] [expected_result] : compile et execute le fichier
#    se trouvant au chemin $codes_dir[file_name].algo, et vérifie que le résultat
#    renvoyé par l'execution est [expected_result].
function test {
    echo "Compiling $1.algo"
    compile "$codes_dir$1.algo"
    echo "Testing $1.algo"
    test_cmd "sipro $compiled_sipro_path" $2
    echo "File $1.algo passed"
    echo ""
}

# all_tests : Lance les tests unitaires
function all_tests {
    test simple 5
    test assignement 18
    test if_and_recursive 13
    test recursion 81
    test recursion_opt 10

    # fibonacci (10)
    # mutual recursion
}


all_tests
rm $compiled_asipro_path
rm $compiled_sipro_path
rm gmon.out
