#!/bin/bash
HOSTNAME=asciiking.com
export HOSTNAME
TBG=/home/tbg/work
export TBG
rm /home/tbg/work/WWW/results/1/battle*
/home/tbg/work/bin/tbg -v > /home/tbg/work/tbglog
find /home/tbg/work/WWW/orders/1/ -mtime +7 -exec rm {} \;
cat /home/tbg/work/tbg/players > /home/tbg/work/data/players
cat /home/tbg/work/tbg/g1 > /home/tbg/work/data/g1
cat /home/tbg/work/tbg/data`head -1 g1` > /home/tbg/work/data/data
mysqldump main_site --single-transaction > /home/tbg/work/data/main_site
mysqldump tbg_site > /home/tbg/work/data/tbg_site

