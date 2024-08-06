#Utility file to generate configurable DM parameters

set PixelType("XF_8UC1")  1
set PixelType("XF_8UC3")  2
set PixelType("XF_8UC4")  3
set PixelType("XF_16UC1") 4
set PixelType("XF_16UC3") 5
set PixelType("XF_16UC4") 6
set PixelType("XF_8UC2")  7
set PixelType("XF_16UC2") 8

# PixelInfo gives a pair (ChannelWidth, NumChannels)
set PixelInfo($PixelType("XF_8UC1"))  {width {8} channel {1}}
set PixelInfo($PixelType("XF_8UC3"))  {width {8} channel {3}}
set PixelInfo($PixelType("XF_8UC4"))  {width {8} channel {4}}
set PixelInfo($PixelType("XF_16UC1")) {width {16} channel {1}}
set PixelInfo($PixelType("XF_16UC3")) {width {16} channel {3}}
set PixelInfo($PixelType("XF_16UC4")) {width {16} channel {4}}
set PixelInfo($PixelType("XF_8UC2"))  {width {8} channel {2}}
set PixelInfo($PixelType("XF_16UC2")) {width {16} channel {2}}

proc configDM {args} {
    global PixelType
    global PixelInfo

    dict with args {}

    if {![info exists HostPixelType]} {
       set HostPixelType $PixelType("XF_8UC1")
    }

    if {![info exists AIEPixelType]} {
        set AIEPixelType $HostPixelType
    }

    if {![info exists DDRDataWidth]} {
        if {[dict get $PixelInfo($AIEPixelType) width] == [dict get $PixelInfo($HostPixelType) width]} {
            set DDRDataWidth 128
        } else {
            set DDRDataWidth 64
        }
    }

    if {![info exists AIEShimDataWidth]} {
        set AIEShimDataWidth 128
    }

    if {![info exists TilerInstInfo]} {
        set TilerInstInfo [list "Tiler_top_1" 0]
    }

    if {![info exists StitcherInstInfo]} {
        set StitcherInstInfo [list "stitcher_top_1" 0]
    }

    # TODO: Check Errors

    # Configure Tiler kernel
    startgroup
    if {$TilerInstInfo != ""} {
        # Deleting IP block which is created for default PL AXIS DW not matching with AIEShim AXIS from xml
        # Later on will match the DataWidth by configuring 
        delete_bd_objs [get_bd_intf_nets dwc_[lindex $TilerInstInfo 0]_OutputStream_M_AXIS] [get_bd_intf_nets [lindex $TilerInstInfo 0]_OutputStream] [get_bd_cells dwc_[lindex $TilerInstInfo 0]_OutputStream]
        #delete_bd_objs [get_bd_intf_nets cdc_[lindex $TilerInstInfo 0]_OutputStream_M_AXIS] [get_bd_intf_nets [lindex $TilerInstInfo 0]_OutputStream] [get_bd_cells cdc_[lindex $TilerInstInfo 0]_OutputStream]

        set_property -dict [list CONFIG.C_M00_AXIS_DATA_WIDTH $AIEShimDataWidth] [get_bd_cells [lindex $TilerInstInfo 0]]
        set_property -dict [list CONFIG.C_M00_AXI_DATA_WIDTH $DDRDataWidth] [get_bd_cells [lindex $TilerInstInfo 0]]
        set_property -dict [list CONFIG.C_AXI_PIXEL_CHANNEL_WIDTH [dict get $PixelInfo($HostPixelType) width]] [get_bd_cells [lindex $TilerInstInfo 0]]
        set_property -dict [list CONFIG.C_AXI_CHANNELS [dict get $PixelInfo($HostPixelType) channel]] [get_bd_cells [lindex $TilerInstInfo 0]]
        set_property -dict [list CONFIG.C_AXIS_PIXEL_CHANNEL_WIDTH [dict get $PixelInfo($AIEPixelType) width]] [get_bd_cells [lindex $TilerInstInfo 0]]
        set_property -dict [list CONFIG.C_AXIS_CHANNELS [dict get $PixelInfo($AIEPixelType) channel]] [get_bd_cells [lindex $TilerInstInfo 0]]

        #Creating a direct connection between PL(tiler/stitcher) AXIS and AIEShim AXIS
        connect_bd_intf_net [get_bd_intf_pins ai_engine_0/S0[lindex $TilerInstInfo 1]\_AXIS] [get_bd_intf_pins [lindex $TilerInstInfo 0]/OutputStream]
    }    

    # Configure Stitcher kernel
    if {$StitcherInstInfo != ""} {
        # Deleting IP block which is created for default PL AXIS DW not matching with AIEShim AXIS from xml
        # Later on will match the DataWidth by configuring 
        delete_bd_objs [get_bd_intf_nets ai_engine_0_M0[lindex $StitcherInstInfo 1]\_AXIS] [get_bd_intf_nets dwc_ai_engine_0_M0[lindex $StitcherInstInfo 1]\_AXIS_M_AXIS] [get_bd_cells dwc_ai_engine_0_M0[lindex $StitcherInstInfo 1]\_AXIS]
        #delete_bd_objs [get_bd_intf_nets dwc_ai_engine_0_M0[lindex $StitcherInstInfo 1]_AXIS_M_AXIS] [get_bd_intf_nets cdc_ai_engine_0_M0[lindex $StitcherInstInfo 1]_AXIS_M_AXIS] [get_bd_cells cdc_ai_engine_0_M0[lindex $StitcherInstInfo 1]_AXIS]

        set_property -dict [list CONFIG.C_S00_AXIS_DATA_WIDTH $AIEShimDataWidth] [get_bd_cells [lindex $StitcherInstInfo 0]]
        set_property -dict [list CONFIG.C_M00_AXI_DATA_WIDTH $DDRDataWidth] [get_bd_cells [lindex $StitcherInstInfo 0]]
        set_property -dict [list CONFIG.C_AXI_PIXEL_CHANNEL_WIDTH [dict get $PixelInfo($HostPixelType) width]] [get_bd_cells [lindex $StitcherInstInfo 0]]
        set_property -dict [list CONFIG.C_AXI_CHANNELS [dict get $PixelInfo($HostPixelType) channel]] [get_bd_cells [lindex $StitcherInstInfo 0]]
        set_property -dict [list CONFIG.C_AXIS_PIXEL_CHANNEL_WIDTH [dict get $PixelInfo($AIEPixelType) width]] [get_bd_cells [lindex $StitcherInstInfo 0]]
        set_property -dict [list CONFIG.C_AXIS_CHANNELS [dict get $PixelInfo($AIEPixelType) channel]] [get_bd_cells [lindex $StitcherInstInfo 0]]

        #Creating a direct connection between PL(tiler/stitcher) AXIS and AIEShim AXIS
        connect_bd_intf_net [get_bd_intf_pins ai_engine_0/M0[lindex $StitcherInstInfo 1]\_AXIS] [get_bd_intf_pins [lindex $StitcherInstInfo 0]/InputStream]
    }
    endgroup
}

proc configTiler {args} {

    eval configDM $args [list StitcherInstInfo ""]
    
}

proc configStitcher {args} {
    
    eval configDM $args [list TilerInstInfo ""]

}
