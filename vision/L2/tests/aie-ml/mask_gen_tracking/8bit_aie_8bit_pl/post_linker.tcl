global env

if {[info exists ::env(XF_PROJ_ROOT)]} {
    set script $env(XF_PROJ_ROOT)/ext/xf_rtl_utils.tcl 
} else {
    set script [regsub {\/L[123]\/.*$} [pwd] ""]/ext/xf_rtl_utils.tcl
}

source -notrace $script

#Instances of PL kernels need to be declared as in system.cfg
set TilerInstances [list [list "Tiler_top_1" 0] [list "Tiler_top_2" 1] [list "Tiler_top_3" 2]]
set StitcherInstances [list [list "stitcher_top_1" 0] [list "stitcher_top_2" 1]]
#set TilerInstances [list [list "Tiler_top_1" 0] [list "Tiler_top_2" 1]]
#set StitcherInstances [list [list "stitcher_top_1" 0]]

# ConfigDM <HostPixelType> <AIEPixelType> <DDRDataWidth> <AIEshimDataWidth> <TilerInst> <StitcherInst>
# Default     <XF_8UC1>      <XF_8UC1>        <128>          <128>             <"">         <"">
# If No TilerInst/StitcherInst is used then assign it with "" 
# Please set DDRDataWidth and AIEDataWidth depending on PixelTypes to make NPPC Transfer as same.
# Ex: DDRWidth = 64 and HostPixel = 8 => NPPC = 64/8 = 8
# So, AIEWidth = 128 and AIEPixel = 16 => NPPC = 128/16 = 8


# Tiler Instances Configuration 
#foreach inst_info $TilerInstances {
#    configTiler \
#        TilerInstInfo $inst_info
#    }

foreach inst_info $TilerInstances {
    configTiler \
        TilerInstInfo $inst_info
}

# Stitcher Instances Configuration 
foreach inst_info $StitcherInstances {
    configStitcher \
        StitcherInstInfo $inst_info
}
        #StitcherInstInfo [lindex $StitcherInstances 0]

    #configStitcher \
    #    StitcherInstInfo [lindex $StitcherInstances 1]
