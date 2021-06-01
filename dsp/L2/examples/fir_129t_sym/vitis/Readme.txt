OVERVIEW:
The AIE DSP library example vitis project. 
Project includes an example design of a 129 tap single rate symmetric FIR. It's purpose is to ilustrate a DSPLIB element instantiation in user's graphs, including graph initialization with argument passing.

USAGE: 
These can be imported into the Vitis IDE in a few steps. The following example shows how to import the Fir129Example_system project.

1. Set the environment variable DSPLIB_ROOT. This must be set to the path under your directory where DSPLib is installed. 
    Set DSPLIB_ROOT <your-install-directory/parameterized_functions/dsplib>

2. Launch the Vitis IDE to import the desired project. Select File → Import → <Vitis project exported zip file>.

3. Click Next and browse to examples/fir_129t_sym/vitis and select the ZIP file.

4. Set the correct path to the VCK190 platform file directory.
    a. In the Vitis IDE, expand the Fir129Example [ aiengine ] project and double click the Fir129Example.prj file.
    b. On Platform Invalid prompt, Select Add platform to repository.
    c. Navigate to the installed platform file (.xpfm).

5. Set the Active configuration to Emulation-AIE for the design.
    a. Right-click the Fir129Example [ aiengine ] and select C/C++ Build Settings → Manage Configurations → Select Emulation-AIE → Set Active.
    b. Click OK.
    c. Select Apply and Close.

6. Build the project by right-clicking the Fir129Example [ aiengine ] project and select Build Project.

7. Perform simulation after the project is built.
    Right-click the Fir129Example [ aiengine ] project and select Run As → 1. Launch AIE Emulator

8. After simulation is complete, navigate to Fir129Example [ aiengine ] → Emulation-AIE → aiesimulator_output → data and compare output.txt with the ref_out.txt file in the data folder.