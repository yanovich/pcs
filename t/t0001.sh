#/bin/sh
SELF=`basename $0`
coproc ./pcs -f t/$SELF.conf 2>/tmp/$SELF.log &&
sleep 0.15 &&
kill $COPROC_PID &&
test 2 -eq `grep -e "mark1:" /tmp/$SELF.log | wc -l` &&
test 2 -eq `cat /tmp/$SELF.log | wc -l`
