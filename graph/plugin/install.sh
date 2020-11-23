TigerGraphPath=/home/tigergraph/tigergraph

rm -rf tigergraph/QueryUdf/tgFunctions.hpp

rm -rf tigergraph/QueryUdf/ExprFunctions.hpp

rm -rf tigergraph/QueryUdf/ExprUtil.hpp

rm -rf tigergraph/QueryUdf/graph.hpp

sudo cp -rf $TigerGraphPath/dev/gdk/gsql/src/QueryUdf $TigerGraphPath/dev/gdk/gsql/src/bck_QueryUdf

sudo cp -rf $TigerGraphPath/dev/gdk/MakeUdf $TigerGraphPath/dev/gdk/bck_MakeUdf

sudo cp -rf $TigerGraphPath/../.bash_tigergraph $TigerGraphPath/../.bck_bash_tigergraph

sudo cp $TigerGraphPath/dev/gdk/gsql/src/QueryUdf/ExprFunctions.hpp tigergraph/QueryUdf/tgFunctions.hpp

sudo cp $TigerGraphPath/dev/gdk/gsql/src/QueryUdf/ExprUtil.hpp tigergraph/QueryUdf

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

sudo rm -rf $TigerGraphPath/dev/gdk/gsql/src/QueryUdf

sudo cp -rf tigergraph/QueryUdf $TigerGraphPath/dev/gdk/gsql/src/

sudo cp -rf tigergraph/MakeUdf $TigerGraphPath/dev/gdk/

sudo cp -rf tigergraph/bash_tigergraph $TigerGraphPath/../.bash_tigergraph

sudo chmod 777 -R $TigerGraphPath/dev/gdk/gsql/src/QueryUdf
