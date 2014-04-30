#/bin/sh
SELF=`basename $0`
coproc ./pcs -tf t/$SELF.conf 2>/tmp/$SELF.log
