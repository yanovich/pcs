#/bin/sh
SELF=`basename $0`
./pcs -tf t/$SELF.conf
