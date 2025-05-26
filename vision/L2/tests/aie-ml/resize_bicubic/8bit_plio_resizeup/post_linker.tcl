global env

if {[info exists ::env(XF_PROJ_ROOT)]} {
    set script $env(XF_PROJ_ROOT)/ext/xf_rtl_utils.tcl 
} else {
    set script [regsub {\/L[123]\/.*$} [pwd] ""]/ext/xf_rtl_utils.tcl
}

source -notrace $script

#Instances of PL kernels need to be declared as in system.cfg
set TilerInstances [list [list "Tiler_top_1" 0] [list "Tiler_top_2" 1] [list "Tiler_top_3" 2] [list "Tiler_top_4" 3] \
                         [list "Tiler_top_5" 4] [list "Tiler_top_6" 5] [list "Tiler_top_7" 6] [list "Tiler_top_8" 7]]
#                        [list "Tiler_top_9" 8] [list "Tiler_top_10" 9]
#                         [list "Tiler_top_11" 10] [list "Tiler_top_12" 11] [list "Tiler_top_13" 12] [list "Tiler_top_14" 13]
#                          [list "Tiler_top_15" 14]
#                         [list "Tiler_top_16" 15] [list "Tiler_top_17" 16] [list "Tiler_top_18" 17] [list "Tiler_top_19" 18] [list "Tiler_top_20" 19]]
set StitcherInstances [list [list "stitcher_top_1" 0] [list "stitcher_top_2" 1] [list "stitcher_top_3" 2] [list "stitcher_top_4" 3] \
                             [list "stitcher_top_5" 4] [list "stitcher_top_6" 5] [list "stitcher_top_7" 6] [list "stitcher_top_8" 7]]
# [list "stitcher_top_9" 8] [list "stitcher_top_10" 9] 
#                            [list "stitcher_top_11" 10] [list "stitcher_top_12" 11] [list "stitcher_top_13" 12] [list "stitcher_top_14" 13]
#                             [list "stitcher_top_15" 14]
#                            [list "stitcher_top_16" 15] [list "stitcher_top_17" 16] [list "stitcher_top_18" 17] [list "stitcher_top_19" 18] [list "stitcher_top_20" 19]]

# ConfigDM <HostPixelType> <AIEPixelType> <DDRDataWidth> <AIEDataWidth> <TilerInst> <StitcherInst>
# Default     <XF_8UC1>      <XF_8UC1>        <128>          <128>          <"">         <"">
# If No TilerInst/StitcherInst is used then assign it with "" 
# Please set DDRDataWidth and AIEDataWidth depending on PixelTypes to make NPPC Transfer as same.
# Ex: DDRWidth = 64 and HostPixel = 8 => NPPC = 64/8 = 8
# So, AIEWidth = 128 and AIEPixel = 16 => NPPC = 128/16 = 8


# Tiler Instances Configuration 
foreach inst_info $TilerInstances {
    configTiler \
        HostPixelType $PixelType("XF_8UC4") \
        AIEPixelType $PixelType("XF_8UC4") \
        TilerInstInfo [lindex $TilerInstances 0]
    configTiler \
        HostPixelType $PixelType("XF_8UC4") \
        AIEPixelType $PixelType("XF_8UC4") \
        TilerInstInfo [lindex $TilerInstances 1]
    configTiler \
        HostPixelType $PixelType("XF_8UC4") \
        AIEPixelType $PixelType("XF_8UC4") \
        TilerInstInfo [lindex $TilerInstances 2]
    configTiler \
        HostPixelType $PixelType("XF_8UC4") \
        AIEPixelType $PixelType("XF_8UC4") \
        TilerInstInfo [lindex $TilerInstances 3]
    configTiler \
        HostPixelType $PixelType("XF_8UC4") \
        AIEPixelType $PixelType("XF_8UC4") \
        TilerInstInfo [lindex $TilerInstances 4]
    configTiler \
        HostPixelType $PixelType("XF_8UC4") \
        AIEPixelType $PixelType("XF_8UC4") \
        TilerInstInfo [lindex $TilerInstances 5]
    configTiler \
        HostPixelType $PixelType("XF_8UC4") \
        AIEPixelType $PixelType("XF_8UC4") \
        TilerInstInfo [lindex $TilerInstances 6]
    configTiler \
        HostPixelType $PixelType("XF_8UC4") \
        AIEPixelType $PixelType("XF_8UC4") \
        TilerInstInfo [lindex $TilerInstances 7]
    configTiler \
        HostPixelType $PixelType("XF_8UC4") \
        AIEPixelType $PixelType("XF_8UC4") \
        TilerInstInfo [lindex $TilerInstances 8]
    configTiler \
        HostPixelType $PixelType("XF_8UC4") \
        AIEPixelType $PixelType("XF_8UC4") \
        TilerInstInfo [lindex $TilerInstances 9]
    configTiler \
        HostPixelType $PixelType("XF_8UC4") \
        AIEPixelType $PixelType("XF_8UC4") \
        TilerInstInfo [lindex $TilerInstances 10]
    configTiler \
        HostPixelType $PixelType("XF_8UC4") \
        AIEPixelType $PixelType("XF_8UC4") \
        TilerInstInfo [lindex $TilerInstances 11]
    configTiler \
        HostPixelType $PixelType("XF_8UC4") \
        AIEPixelType $PixelType("XF_8UC4") \
        TilerInstInfo [lindex $TilerInstances 12]
    configTiler \
        HostPixelType $PixelType("XF_8UC4") \
        AIEPixelType $PixelType("XF_8UC4") \
        TilerInstInfo [lindex $TilerInstances 13]
    configTiler \
        HostPixelType $PixelType("XF_8UC4") \
        AIEPixelType $PixelType("XF_8UC4") \
        TilerInstInfo [lindex $TilerInstances 14]
    configTiler \
        HostPixelType $PixelType("XF_8UC4") \
        AIEPixelType $PixelType("XF_8UC4") \
        TilerInstInfo [lindex $TilerInstances 15]
    configTiler \
        HostPixelType $PixelType("XF_8UC4") \
        AIEPixelType $PixelType("XF_8UC4") \
        TilerInstInfo [lindex $TilerInstances 16]
    configTiler \
        HostPixelType $PixelType("XF_8UC4") \
        AIEPixelType $PixelType("XF_8UC4") \
        TilerInstInfo [lindex $TilerInstances 17]
    configTiler \
        HostPixelType $PixelType("XF_8UC4") \
        AIEPixelType $PixelType("XF_8UC4") \
        TilerInstInfo [lindex $TilerInstances 18]
    configTiler \
        HostPixelType $PixelType("XF_8UC4") \
        AIEPixelType $PixelType("XF_8UC4") \
        TilerInstInfo [lindex $TilerInstances 19]

    }

# Stitcher Instances Configuration 
foreach inst_info $StitcherInstances { 
    configStitcher \
        HostPixelType $PixelType("XF_8UC4") \
        AIEPixelType $PixelType("XF_8UC4") \
        StitcherInstInfo [lindex $StitcherInstances 0]
    configStitcher \
        AIEPixelType $PixelType("XF_8UC4") \
        HostPixelType $PixelType("XF_8UC4") \
        StitcherInstInfo [lindex $StitcherInstances 1]
    configStitcher \
        AIEPixelType $PixelType("XF_8UC4") \
        HostPixelType $PixelType("XF_8UC4") \
        StitcherInstInfo [lindex $StitcherInstances 2]
    configStitcher \
        AIEPixelType $PixelType("XF_8UC4") \
        HostPixelType $PixelType("XF_8UC4") \
        StitcherInstInfo [lindex $StitcherInstances 3]
    configStitcher \
        AIEPixelType $PixelType("XF_8UC4") \
        HostPixelType $PixelType("XF_8UC4") \
        StitcherInstInfo [lindex $StitcherInstances 4]
            configStitcher \
        HostPixelType $PixelType("XF_8UC4") \
        AIEPixelType $PixelType("XF_8UC4") \
        StitcherInstInfo [lindex $StitcherInstances 5]
    configStitcher \
        AIEPixelType $PixelType("XF_8UC4") \
        HostPixelType $PixelType("XF_8UC4") \
        StitcherInstInfo [lindex $StitcherInstances 6]
    configStitcher \
        AIEPixelType $PixelType("XF_8UC4") \
        HostPixelType $PixelType("XF_8UC4") \
        StitcherInstInfo [lindex $StitcherInstances 7]
    configStitcher \
        AIEPixelType $PixelType("XF_8UC4") \
        HostPixelType $PixelType("XF_8UC4") \
        StitcherInstInfo [lindex $StitcherInstances 8]
    configStitcher \
        AIEPixelType $PixelType("XF_8UC4") \
        HostPixelType $PixelType("XF_8UC4") \
        StitcherInstInfo [lindex $StitcherInstances 9]
            configStitcher \
        HostPixelType $PixelType("XF_8UC4") \
        AIEPixelType $PixelType("XF_8UC4") \
        StitcherInstInfo [lindex $StitcherInstances 10]
    configStitcher \
        AIEPixelType $PixelType("XF_8UC4") \
        HostPixelType $PixelType("XF_8UC4") \
        StitcherInstInfo [lindex $StitcherInstances 11]
    configStitcher \
        AIEPixelType $PixelType("XF_8UC4") \
        HostPixelType $PixelType("XF_8UC4") \
        StitcherInstInfo [lindex $StitcherInstances 12]
    configStitcher \
        AIEPixelType $PixelType("XF_8UC4") \
        HostPixelType $PixelType("XF_8UC4") \
        StitcherInstInfo [lindex $StitcherInstances 13]
    configStitcher \
        AIEPixelType $PixelType("XF_8UC4") \
        HostPixelType $PixelType("XF_8UC4") \
        StitcherInstInfo [lindex $StitcherInstances 14]
            configStitcher \
        HostPixelType $PixelType("XF_8UC4") \
        AIEPixelType $PixelType("XF_8UC4") \
        StitcherInstInfo [lindex $StitcherInstances 15]
    configStitcher \
        AIEPixelType $PixelType("XF_8UC4") \
        HostPixelType $PixelType("XF_8UC4") \
        StitcherInstInfo [lindex $StitcherInstances 16]
    configStitcher \
        AIEPixelType $PixelType("XF_8UC4") \
        HostPixelType $PixelType("XF_8UC4") \
        StitcherInstInfo [lindex $StitcherInstances 17]
    configStitcher \
        AIEPixelType $PixelType("XF_8UC4") \
        HostPixelType $PixelType("XF_8UC4") \
        StitcherInstInfo [lindex $StitcherInstances 18]
    configStitcher \
        AIEPixelType $PixelType("XF_8UC4") \
        HostPixelType $PixelType("XF_8UC4") \
        StitcherInstInfo [lindex $StitcherInstances 19]

}


    configTiler \
        HostPixelType $PixelType("XF_8UC4") \
        AIEPixelType $PixelType("XF_8UC4") \
        TilerInstInfo [lindex $TilerInstances 4]
    configTiler \
        HostPixelType $PixelType("XF_8UC4") \
        AIEPixelType $PixelType("XF_8UC4") \
        TilerInstInfo [lindex $TilerInstances 5]
    configTiler \
        HostPixelType $PixelType("XF_8UC4") \
        AIEPixelType $PixelType("XF_8UC4") \
        TilerInstInfo [lindex $TilerInstances 6]
    configTiler \
        HostPixelType $PixelType("XF_8UC4") \
        AIEPixelType $PixelType("XF_8UC4") \
        TilerInstInfo [lindex $TilerInstances 7]
		
		
		
		    configStitcher \
        AIEPixelType $PixelType("XF_8UC4") \
        HostPixelType $PixelType("XF_8UC4") \
        StitcherInstInfo [lindex $StitcherInstances 4]
            configStitcher \
        HostPixelType $PixelType("XF_8UC4") \
        AIEPixelType $PixelType("XF_8UC4") \
        StitcherInstInfo [lindex $StitcherInstances 5]
    configStitcher \
        AIEPixelType $PixelType("XF_8UC4") \
        HostPixelType $PixelType("XF_8UC4") \
        StitcherInstInfo [lindex $StitcherInstances 6]
    configStitcher \
        AIEPixelType $PixelType("XF_8UC4") \
        HostPixelType $PixelType("XF_8UC4") \
        StitcherInstInfo [lindex $StitcherInstances 7]
