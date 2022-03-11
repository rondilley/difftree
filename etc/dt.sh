#!/bin/sh
#
# desc: use to keep track of filesystem changes
#
#####

TIME=`date '+%Y%m%d%H%M%S'`
DT_ARGS="-m -p" # md5 hash files and preserve access time
DT_EXCLUSIONS="-e /proc -e /run -e /sys -e /snap -e /home -e /var/log"
DT_DIR="/root/dt"
DT_COMMAND="/usr/local/bin/dt"
MAILTO="root"

if [ -f "${DT_DIR}/root_dir.current" ]; then
 # compare to last run
 ${DT_COMMAND} ${DT_ARGS} ${DT_EXCLUSIONS} -w ${DT_DIR}/root_dir.${TIME}.dt ${DT_DIR}/root_dir.current / | mailx -s DT_Delta ${MAILTO}
 # create link
 ln -f -s ${DT_DIR}/root_dir.${TIME}.dt ${DT_DIR}/root_dir.current
else
 # first run, no compare this time
 ${DT_COMMAND} ${DT_ARGS} ${DT_EXCLUSIONS} -w ${DT_DIR}/root_dir.${TIME}.dt /
 # create link
 ln -s ${DT_DIR}/root_dir.${TIME}.dt ${DT_DIR}/root_dir.current
fi

# done
exit

