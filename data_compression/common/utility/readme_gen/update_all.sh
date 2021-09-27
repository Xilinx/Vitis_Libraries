BASEDIR=$(pwd)

dir_list=( $(git ls-files | grep 'description.json' | sed -r 's|/[^/]+$||' | sort | uniq ))

echo ${dir_list[@]}
echo $BASEDIR

for i in "${dir_list[@]}"
do
    cd $i
    echo "Updating README for = $i"
    rm README.rst
    python ../module_readme.py description.json
    cd $BASEDIR
done
