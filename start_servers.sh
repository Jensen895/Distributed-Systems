ILLINI_ACCOUNT=$1
GROUP_NUMBER=41
for CLUSTER_NUMBER in $(seq -w 1 10)
do
    echo "Executing command for ${CLUSTER_NUMBER} server"
    ssh ${ILLINI_ACCOUNT}@fa23-cs425-${GROUP_NUMBER}${CLUSTER_NUMBER}.cs.illinois.edu "cd mp1; nohup ./server < /dev/null &" &
done