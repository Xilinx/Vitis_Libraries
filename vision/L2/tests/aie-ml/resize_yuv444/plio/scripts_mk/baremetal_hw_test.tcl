connect
targets -set -nocase -filter {name =~ "*Versal*"}
device program ./package.hw/BOOT.BIN
after 2000
exit
