#/bin/sh
SELF=`basename $0`
./pcs -tf t/$SELF.conf &&
coproc ./pcs -Df t/$SELF.conf 2>/tmp/$SELF.log &&
sleep 0.14 &&
kill $COPROC_PID &&
test 2 -eq `grep -e "mark1:1 mark2:1" /tmp/$SELF.log | wc -l` &&
test 3 -eq `grep -e "mark3:1" /tmp/$SELF.log | wc -l` &&
test 5 -eq `cat /tmp/$SELF.log | wc -l`
