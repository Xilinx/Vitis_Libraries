#!/bin/bash



i="0"

while [ $i -lt 4 ]
do
	make cleanall; make
echo "Build finished at: $(date)"
echo "===================================================="
echo "Sleeping ..."
echo "===================================================="
sleep 5

done
