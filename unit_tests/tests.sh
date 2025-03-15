#!/bin/bash

codes_dir="./codes/"
compiler_path="../compiler/compiler"
compiled_asipro_path="./test_compiled.asipro"
compiled_sipro_path="./test_compiled.sipro"

#  test_cmd [command] [expected_result] : explicite
function test_cmd {
        result=$($1)
        if [ $result != $2 ]; then
                echo "Error on command: $1"
                echo "Got: $result"
                echo "Expected: $2"
                exit -1
        fi
}

# compile [file_path] : Compile le fichier algo au chemin [file_path] en fichier
# asipro et sipro (chemins: compiled_asipro_path et compiled_sipro_path)
function compile {
    $compiler_path < $1 > $compiled_asipro_path
    asipro $compiled_asipro_path $compiled_sipro_path
}

#  test [file_name] [expected_result] : compile et execute le fichier
#    se trouvant au chemin ./codes/[file_name], et vérifie que le résultat
#    renvoyé par l'execution est [expected_result].
function test {
    compile "$codes_dir$1"
    test_cmd "sipro $compiled_sipro_path" $2
    echo "File $1 passed"
}

# all_tests : Lance les tests unitaires
function all_tests {
    test simple.algo 5
    #test assignement.algo 18
    #test if_and_recursive.algo 13
}


all_tests;
rm $compiled_asipro_path
rm $compiled_sipro_path
