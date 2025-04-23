target=hw_emu

usage(){
  echo "
Usage:
  -t, --target    target of build hw_emu/hw
  -h, --help    display this help and exit

  example: ./pre_launch.sh -t hw_emu
"
}

while true
do
case "$1" in
    -t|--target)
    target="$2"
    shift
    ;;
    -h|--help)
    usage
    exit 1
    ;;
    --)
    shift
    ;;
    *)
    break
    ;;
    esac
done

curDir=$(dirname $0)
File1=$curDir/../lib/libgraphL3.so
File2=$curDir/../lib/libgraphL3.json

if [ ! -f "$File2" ];
then
    if [ $target == 'hw_emu' ];
    then
        echo "build libgraphL3.so for hw_emu"
        make -C $curDir/../lib/ clean
        make -C $curDir/../lib/ libgraphL3 TARGET=hw_emu
    elif [ $target == 'hw' ];
    then
        echo "build libgraphL3.so for testing on board"
        make -C $curDir/../lib/ clean
        make -C $curDir/../lib/ libgraphL3 TARGET=hw
    else
        echo "unknown target"
        exit 1
    fi
else
    mode=$(<$File2)
    if [ $target == 'hw_emu' ] && ([ ! -f "$File1" ] || [ $mode != 'EMULATION_MODE' ]);
    then
        echo "build libgraphL3.so for hw_emu"
        make -C $curDir/../lib/ clean
        make -C $curDir/../lib/ libgraphL3 TARGET=hw_emu
    elif [ $target == 'hw' ] && ([ ! -f "$File1" ] || [ $mode == 'EMULATION_MODE' ]);
    then
        echo "build libgraphL3.so for testing on board"
        make -C $curDir/../lib/ clean
        make -C $curDir/../lib/ libgraphL3 TARGET=hw
    elif [ -f "$File1" ] || [ -f "$File2" ];
    then
        echo "build libgraphL3.so successfully"
    else
        echo "unknown target"
    fi
fi

