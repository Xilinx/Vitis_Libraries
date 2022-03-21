@Library('pipeline-library')_

VitisLibPipeline (branch: 'master', libname: 'xf_dsp', TARGETS: 'hls_csim:hls_csynth:hls_cosim:vitis_sw_emu:vitis_hw_emu:vitis_hw_build:vitis_hw_run:vitis_aie_sim:vitis_aie_x86sim',
                  upstream_dependencies: 'xf_utils_hw,master,../utils; dsplib_internal_scripts,main,../dsplib_internal_scripts',
                  devtest: 'RunDeploy.sh', TOOLVERSION: '2021.2_released',
                  post_launch: '../dsplib_internal_scripts/scripts/create_html_report.sh')

