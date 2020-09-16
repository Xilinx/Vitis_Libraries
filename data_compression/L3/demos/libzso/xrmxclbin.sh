rm -rf /tmp/xrm.json
DEVICES=$1
printf "{\n" >> /tmp/xrm.json
printf "\t\"request\": {\n" >> /tmp/xrm.json
printf "\t\t\"name\":\"load\",\n" >> /tmp/xrm.json
printf "\t\t\"requestId\":1,\n" >> /tmp/xrm.json
printf "\t\t\"parameters\":[\n" >> /tmp/xrm.json

for ((i = 0 ; i < $1 ; i++))
do 
printf "\t\t\t{\n" >> /tmp/xrm.json
printf "\t\t\t\t\"device\":%s,\n" "$i" >> /tmp/xrm.json
printf "\t\t\t\t\"xclbin\":\"%s\"\n" "$XILINX_LIBZ_XCLBIN" >> /tmp/xrm.json
printf "\t\t\t}\n" >> /tmp/xrm.json
if [ ${DEVICES} > 1 ] 
then
    VAR=$(expr $i + 1)
    if [ ${DEVICES} != $VAR ] 
    then
        printf "\t\t\t,\n" >> /tmp/xrm.json
    fi
fi
done


printf "\t\t]\n" >> /tmp/xrm.json
printf "\t}\n" >> /tmp/xrm.json
printf "}\n" >> /tmp/xrm.json
chmod 777 /tmp/xrm.json
xrmadm /tmp/xrm.json
