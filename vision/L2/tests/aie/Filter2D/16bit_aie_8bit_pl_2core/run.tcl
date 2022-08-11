########## Pblocks for BRAMs ##########

set T1 [get_cells -hierarchical -filter { PRIMITIVE_TYPE =~ BLOCKRAM.*.* && NAME =~  "*Tiler_top_1*" } ] 
create_pblock T1_1
resize_pblock T1_1 -add {RAMB36_X5Y48:RAMB36_X5Y64}
add_cells_to_pblock T1_1 [get_cells $T1]

set T2 [get_cells -hierarchical -filter { PRIMITIVE_TYPE =~ BLOCKRAM.*.* && NAME =~  "*Tiler_top_2*" } ] 
create_pblock T2_1
resize_pblock T2_1 -add {RAMB36_X3Y48:RAMB36_X3Y64}
add_cells_to_pblock T2_1 [get_cells $T2]


set T3 [get_cells -hierarchical -filter { PRIMITIVE_TYPE =~ BLOCKRAM.*.* && NAME =~  "*Tiler_top_3*" } ] 
create_pblock T3_1
resize_pblock T3_1 -add {RAMB36_X7Y48:RAMB36_X7Y64}
add_cells_to_pblock T3_1 [get_cells $T3]


set T4 [get_cells -hierarchical -filter { PRIMITIVE_TYPE =~ BLOCKRAM.*.* && NAME =~  "*Tiler_top_4*" } ] 
create_pblock T4_1
resize_pblock T4_1 -add {RAMB36_X6Y48:RAMB36_X6Y64}
add_cells_to_pblock T4_1 [get_cells $T4]


set T5 [get_cells -hierarchical -filter { PRIMITIVE_TYPE =~ BLOCKRAM.*.* && NAME =~  "*Tiler_top_5*" } ]  
create_pblock T5_1
resize_pblock T5_1 -add {RAMB36_X4Y48:RAMB36_X4Y64}
add_cells_to_pblock T5_1 [get_cells $T5]

set T6 [get_cells -hierarchical -filter { PRIMITIVE_TYPE =~ BLOCKRAM.*.* && NAME =~  "*Tiler_top_6*" } ] 
create_pblock T6_1
resize_pblock T6_1 -add {RAMB36_X5Y66:RAMB36_X5Y82}
add_cells_to_pblock T6_1 [get_cells $T6]

set T7 [get_cells -hierarchical -filter { PRIMITIVE_TYPE =~ BLOCKRAM.*.* && NAME =~  "*Tiler_top_7*" } ] 
create_pblock T7_1
resize_pblock T7_1 -add {RAMB36_X6Y66:RAMB36_X6Y82}
add_cells_to_pblock T7_1 [get_cells $T7]


set T8 [get_cells -hierarchical -filter { PRIMITIVE_TYPE =~ BLOCKRAM.*.* && NAME =~  "*Tiler_top_8*" } ] 
create_pblock T8_1
resize_pblock T8_1 -add {RAMB36_X2Y48:RAMB36_X2Y64}
add_cells_to_pblock T8_1 [get_cells $T8]


set T9 [get_cells -hierarchical -filter { PRIMITIVE_TYPE =~ BLOCKRAM.*.* && NAME =~  "*Tiler_top_9*" } ]
create_pblock T9_1
resize_pblock T9_1 -add {RAMB36_X7Y66:RAMB36_X7Y82}
add_cells_to_pblock T9_1 [get_cells $T9]

set T10 [get_cells -hierarchical -filter { PRIMITIVE_TYPE =~ BLOCKRAM.*.* && NAME =~  "*Tiler_top_10*" } ]
create_pblock T10_1
resize_pblock T10_1 -add {RAMB36_X8Y66:RAMB36_X8Y82}
add_cells_to_pblock T10_1 [get_cells $T10]

set T11 [get_cells -hierarchical -filter { PRIMITIVE_TYPE =~ BLOCKRAM.*.* && NAME =~  "*Tiler_top_11*" } ]
create_pblock T11_1
resize_pblock T11_1 -add {RAMB36_X4Y66:RAMB36_X4Y82}
add_cells_to_pblock T11_1 [get_cells $T11]


set T12 [get_cells -hierarchical -filter { PRIMITIVE_TYPE =~ BLOCKRAM.*.* && NAME =~  "*Tiler_top_12*" } ]
create_pblock T12_1
resize_pblock T12_1 -add {RAMB36_X1Y48:RAMB36_X1Y64}
add_cells_to_pblock T12_1 [get_cells $T12]

set T13 [get_cells -hierarchical -filter { PRIMITIVE_TYPE =~ BLOCKRAM.*.* && NAME =~  "*Tiler_top_13*" } ]
create_pblock T13_1
resize_pblock T13_1 -add {RAMB36_X4Y30:RAMB36_X4Y46}
add_cells_to_pblock T13_1 [get_cells $T13]

set T14 [get_cells -hierarchical -filter { PRIMITIVE_TYPE =~ BLOCKRAM.*.* && NAME =~  "*Tiler_top_14*" } ]
create_pblock T14_1
resize_pblock T14_1 -add {RAMB36_X2Y66:RAMB36_X2Y82}
add_cells_to_pblock T14_1 [get_cells $T14]


set T15 [get_cells -hierarchical -filter { PRIMITIVE_TYPE =~ BLOCKRAM.*.* && NAME =~  "*Tiler_top_15*" } ]
create_pblock T15_1
resize_pblock T15_1 -add {RAMB36_X5Y30:RAMB36_X5Y46}
add_cells_to_pblock T15_1 [get_cells $T15]

set T16 [get_cells -hierarchical -filter { PRIMITIVE_TYPE =~ BLOCKRAM.*.* && NAME =~  "*Tiler_top_16*" } ]
create_pblock T16_1
resize_pblock T16_1 -add {RAMB36_X2Y30:RAMB36_X2Y46}
add_cells_to_pblock T16_1 [get_cells $T16]


set_property IS_SOFT 0 [get_pblocks *]

##################################################
