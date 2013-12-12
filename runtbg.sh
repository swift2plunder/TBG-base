#!/bin/bash
HOSTNAME=asciiking.com
export HOSTNAME
TBG=/home/tbg/work
export TBG

# Remove old battle reports to eliminate name conflicts
rm /home/tbg/work/WWW/results/1/battle*

# Run TBG
/home/tbg/work/bin/tbg > /home/tbg/work/tbglog

# Back up TBG data
cat /home/tbg/work/tbg/players > /home/tbg/work/data/players
cat /home/tbg/work/tbg/g1 > /home/tbg/work/data/g1
cat /home/tbg/work/tbg/data`head -1 /home/tbg/work/tbg/g1` > /home/tbg/work/data/data
mysqldump main_site --single-transaction > /home/tbg/work/data/main_site
mysqldump tbg_site > /home/tbg/work/data/tbg_site

# Copy Times to archive folder
cp /home/tbg/work/WWW/results/1/times* /home/tbg/work/WWW/news/

# Cleanup old order files and broken symlinks
find /home/tbg/work/WWW/orders/1/ -mtime +14 -exec rm {} \;
find -L /home/tbg/work/WWW/ -type l -delete 2> /dev/null

