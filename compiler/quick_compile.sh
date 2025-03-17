*
./parsing < test_code.algosipro > test.asipro
asipro test.asipro test.sipro

echo ""
echo "EXECUTE CODE"
echo ""

sipro test.sipro
