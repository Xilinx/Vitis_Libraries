#!/bin/bash

echo "Checking TigerGraph installation directory"
if [ ! -f "$HOME/.gsql/gsql.cfg" ]; then
    echo "Please run this install script as the user for the TigerGraph installation."
fi
tg_root_dir_kv=$(grep tigergraph.root.dir $HOME/.gsql/gsql.cfg)
tg_root_dir=$(cut -d' ' -f2 <<< $tg_root_dir_kv)
echo "Found TigerGraph installation in $tg_root_dir"

rm -rf tigergraph/QueryUdf/tgFunctions.hpp
rm -rf tigergraph/QueryUdf/ExprFunctions.hpp
rm -rf tigergraph/QueryUdf/ExprUtil.hpp
rm -rf tigergraph/QueryUdf/graph.hpp

# save a copy of the original UDF Files
if [ ! -d "$tg_root_dir/dev/gdk/gsql/src/QueryUdf.orig" ]; then
    cp -r $tg_root_dir/dev/gdk/gsql/src/QueryUdf $tg_root_dir/dev/gdk/gsql/src/QueryUdf.orig
    echo "Saved a copy of the original QueryUdf files in $tg_root_dir/gdk/gsql/src/QueryUdf.orig"
fi

cp $tg_root_dir/dev/gdk/gsql/src/QueryUdf.orig/ExprFunctions.hpp tigergraph/QueryUdf/tgFunctions.hpp
cp $tg_root_dir/dev/gdk/gsql/src/QueryUdf.orig/ExprUtil.hpp tigergraph/QueryUdf
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

#make clean
make TigerGraphPath=$tg_root_dir libgraphL3wrapper

#rm -rf $tg_install_dir/tigergraph/dev/gdk/gsql/src/QueryUdf
cp -rf tigergraph/QueryUdf/* $tg_root_dir/dev/gdk/gsql/src/QueryUdf
cp -rf tigergraph/MakeUdf $tg_root_dir/dev/gdk/

# update files with tg_root_dir
for f in $tg_root_dir/dev/gdk/gsql/src/QueryUdf/*.json; do
    # use | as the demiliter since tg_root_dir has / in it
    sed -i "s|TG_ROOT_DIR|$tg_root_dir|" $f 
done
sed -i "s|TG_ROOT_DIR|$tg_root_dir|" $tg_root_dir/dev/gdk/MakeUdf 

# Back up .bash_tigergraph
if [ -f "$HOME/.bash_tigergraph" ]; then
    backup_file=".bash_tigergraph-$(date "+%Y%m%d-%H%M%S")"
    cp $HOME/.bash_tigergraph $HOME/$backup_file
    #cp -r $tg_root_dir/dev/gdk/gsql/src/QueryUdf $tg_root_dir/dev/gdk/gsql/src/QueryUdf.orig
    echo "Saved the original $HOME/.bash_tigergraph as $HOME/$backup_file"
fi
cp tigergraph/bash_tigergraph $HOME/.bash_tigergraph

echo "Xilinx FPGA acceleration plugin for Tigergraph has been installed."

# Copy xclbins to TG root directory 
echo "Xilinx FPGA binary files for accelerated graph functions need to be dowloaded"
echo "from Xilinx DBA lounge and then installed by following instructions in the package."
#mkdir -p $tg_root_dir/dev/gdk/gsql/src/QueryUdf/xclbin
#cp xclbin/xilinx_u50_gen3x16_xdma_201920_3/*.xclbin $tg_root_dir/dev/gdk/gsql/src/QueryUdf/xclbin 
