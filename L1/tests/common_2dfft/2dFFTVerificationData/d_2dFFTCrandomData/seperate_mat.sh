base_folder_name=$1
sep="_"
cwd=$(pwd)

for i in {0..5} ; do
    cd $base_folder_name$sep$i
    pwd
    
    if [ ! -d "mat" ]; then
        mkdir "mat"
    fi
    
    mv *.mat mat/
    echo $i
cd $cwd
done
