rm -rf tigergraph/QueryUdf/tgFunctions.hpp

rm -rf tigergraph/QueryUdf/ExprFunctions.hpp

rm -rf tigergraph/QueryUdf/ExprUtil.hpp

rm -rf tigergraph/QueryUdf/graph.hpp

sudo cp /home/tigergraph/tigergraph/dev/gdk/gsql/src/QueryUdf/ExprFunctions.hpp tigergraph/QueryUdf/tgFunctions.hpp

sudo cp /home/tigergraph/tigergraph/dev/gdk/gsql/src/QueryUdf/ExprUtil.hpp tigergraph/QueryUdf

cp tigergraph/QueryUdf/xilinxUdf.hpp tigergraph/QueryUdf/ExprFunctions.hpp

cp -rf ../L3/include/graph.hpp tigergraph/QueryUdf

xrtPath=/opt/xilinx/xrt

xrmPath=/opt/xilinx/xrm

while getopts ":r:m:" opt
do
case $opt in
    r)
    xrtPath=$OPTARG
    echo "$xrtPath"
    ;;
    m)
    xrmPath=$OPTARG
    echo "$xrmPath"
    ;;
    ?)
    echo "unknown"
    exit 1;;
    esac
done

source $xrtPath/setup.sh

source $xrmPath/setup.sh

make clean

make libgraphL3wrapper

sudo chmod 777 -R tigergraph

sudo rm -rf /home/tigergraph/tigergraph/dev/gdk/gsql/src/QueryUdf

sudo cp -rf tigergraph/QueryUdf /home/tigergraph/tigergraph/dev/gdk/gsql/src/

sudo cp -rf tigergraph/MakeUdf /home/tigergraph/tigergraph/dev/gdk/

sudo cp -rf tigergraph/bash_tigergraph /home/tigergraph/.bash_tigergraph

sudo chmod 777 -R /home/tigergraph/tigergraph/dev/gdk/gsql/src/QueryUdf
