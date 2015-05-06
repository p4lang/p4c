./test > test.output 2>/dev/null
DIFF=$(diff test.output test.output.ref) 
if [ "$DIFF" != "" ] 
then
    echo "FAIL!"
else
    echo "PASS!"
fi
