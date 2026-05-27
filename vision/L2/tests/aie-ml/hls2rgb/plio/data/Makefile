all:
	grep -v 'T' ./../aiesimulator_output/data/output.txt > output_Y1.txt	
	g++ chk_output.cpp -I ./../../../../../../L1/include/aie/ -o chk_output.exe 
	./chk_output.exe
	rm ./chk_output.exe
