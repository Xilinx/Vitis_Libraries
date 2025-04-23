xrtPath=/opt/xilinx/xrt
xrmPath=/opt/xilinx/xrm
target=hw_emu

usage(){
  echo "
Usage:
  -r, --xrm    path of xrm
  -m, --xrt    path of xrt
  -t, --target    target of build hw_emu/hw
  -h, --help    display this help and exit

  example1: ./build_so.sh -x /opt/xilinx/xrt -m /opt/xilinx/xrm -t hw_emu
  example2: ./build_so.sh -t hw_emu
"
}

while true
do
case "$1" in
    -r|--xrt)
    xrtPath="$2"
    echo "$xrtPath"
    shift
    ;;
    -m|--xrm)
    xrmPath="$2"
    echo "$xrmPath"
    shit
    ;;
    -t|--target)
    target="$2"
    echo "$target"
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

source $xrtPath/setup.sh

source $xrmPath/setup.sh

make clean

if [ $target == 'hw_emu' ];
then
    echo "build libgraphL3.so for hw_emu"
    make libgraphL3 TARGET=hw_emu
else
    echo "build libgraphL3.so for testing on board"
    make libgraphL3 TARGET=hw
fi
