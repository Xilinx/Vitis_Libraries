To test l1 primitives, please follow the steps below:
	1. navigate to l1_primitive/tests
	2. source ./set_env.sh
	3. launch vivado_hls to create HLS project and run testbench. For example:
			vivado_hls -f dimv_hls/build/run.tcl "runCsim 1 runRTLsynth 0 runRTLsim 0 part vu9p dataType double size 8192 numDiag 3 entriesInParallel 4 runArgs 'absolute_path_to_diagonal_matrix_file/diag_3.csv 8192'"
			vivado_hls -f dimv_hls/build/run.tcl "runCsim 1 runRTLsynth 0 runRTLsim 0 part vu9p dataType double size 8192 numDiag 5 entriesInParallel 4 runArgs 'absolute_path_to_diagonal_matrix_file/diag_5.csv 8192'"
