#!/bin/bash
appDir=$(dirname $(dirname $(dirname $(readlink -f $0))))

echo "-----------------------"
echo "--  UPDATING MAKEFILES --"
echo "-----------------------"

update_file() {
	ignore=0

	for i in $IGNORE; do
		if [[ $1 =~ ^description.json ]]; then 
			ignore=1
		fi
	done

	if [[ $VERBOSE == "true" ]]; then
		echo -n "Checking $1 ... "
	fi
	if [[ $ignore == 1 ]]; then
		if [[ $VERBOSE == "true" ]]; then
			echo "SKIP"
		fi
	else
		pushd . > /dev/null 
		cd $(dirname $1)
		$appDir/utility/makefile_gen/makegen.py description.json #> /dev/null 2>&1
		popd >/dev/null
	fi
}


VCS_FILES=$(git ls-files)

for f in $VCS_FILES; do
	if [[ ($f == */description.json) ]]; then
		if grep -q '"match_makefile": "false"' $f; then
			echo $f
			echo "Makefile Manually Edited:: AutoMakefile Generator Failed"			
		else
			echo $f
			update_file $(readlink -f $f)
		fi
	fi
done
