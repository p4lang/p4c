force=0
while [[ $# -gt 0 ]]; do
key="$1"
case $key in
    -f|--force)
    force=1
    ;;
    *)
            # unknown option
    ;;
esac
shift # past argument or value
done

function quit {
    if [ $force == 0 ]; then
        echo "skipping installation, you can force installation with '-f'"
        exit 0
    fi
}

function check_lib {
    ldconfig -p | grep $2 &> /dev/null
    if [ $? == 0 ]; then
        echo "$2 found"
        quit
    fi
    ldconfig -p | grep $1 &> /dev/null
    if [ $? == 0 ]; then
        echo "a version of $1 was found, but not $2"
        echo "you may experience issues when using a different version"
        quit
    fi
}
