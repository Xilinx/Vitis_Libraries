#!/bin/bash
gadmin stop -fy
gadmin start -fy
time gsql schema_xgraph.gsql
time gsql load_xgraph.gsql
time gsql base.gsql
time gsql query.gsql
time gsql -g xgraph "set query_timeout=240000000 run query cosinesim_ss_tg()"
time gsql -g xgraph "set query_timeout=240000000 run query cosinesim_ss_fpga()"
time gsql -g xgraph "set query_timeout=240000000 run query close_fpga()"
