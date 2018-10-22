#number of Input Fifo is 256
#md=~/bnt/app/bnt_access\ -r0
i=0
while
    ((i<256))
do
~/bnt/app/bnt_access -d -c$i
i=$(($i+1))
#read -p "Press key to continue.. " -n1 -s
done;
