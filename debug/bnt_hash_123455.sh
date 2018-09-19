mm=~/bnt/app/bnt_access\ -a
md=~/bnt/app/bnt_access
#$mm -w 01 0004
~/bnt/app/bnt_reset -v
$mm -w 03 0002
$mm -w 06 -n 23 0055 E35E 869A 9890 D0F8 0301 9ECA 7431 751C 614F 0B27 4063 B264 287A C0F3 9BEA 8CD7 dfb1 1b27 c8c7 cb4d b393 6a1a
while true;do $md -r 1d; sleep 5;done
