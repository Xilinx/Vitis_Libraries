global env

if {[info exists ::env(XF_PROJ_ROOT)]} {
    set script $env(XF_PROJ_ROOT)/ext/xf_rtl_utils.tcl 
} else {
    set script [regsub {\/L[123]\/.*$} [pwd] ""]/ext/xf_rtl_utils.tcl
}

source -notrace $script

#Instances of PL kernels need to be declared as in system.cfg
set TilerInstances [list [list "Tiler_top_1" 0]]
set StitcherInstances [list [list "stitcher_top_1" 0] [list "stitcher_top_2" 1]]

# ConfigDM <HostPixelType> <AIEPixelType> <DDRDataWidth> <AIEDataWidth> <TilerInst> <StitcherInst>
# Default     <XF_8UC1>      <XF_8UC1>        <128>          <128>          <"">         <"">
# If No TilerInst/StitcherInst is used then assign it with "" 
# Please set DDRDataWidth and AIEDataWidth depending on PixelTypes to make NPPC Transfer as same.
# Ex: DDRWidth = 64 and HostPixel = 8 => NPPC = 64/8 = 8
# So, AIEWidth = 128 and AIEPixel = 16 => NPPC = 128/16 = 8


# Tiler Instances Configuration 
#foreach inst_info $TilerInstances {
#    configTiler \
#        TilerInstInfo $inst_info
#    }

    configTiler \
        AIEPixelType $PixelType("XF_8UC4") \
        HostPixelType $PixelType("XF_8UC4") \
        TilerInstInfo [lindex $TilerInstances 0]


# Stitcher Instances Configuration 

    configStitcher \
        StitcherInstInfo [lindex $StitcherInstances 0]

    configStitcher \
	AIEPixelType $PixelType("XF_8UC2") \
        HostPixelType $PixelType("XF_8UC2") \
        StitcherInstInfo [lindex $StitcherInstances 1]

