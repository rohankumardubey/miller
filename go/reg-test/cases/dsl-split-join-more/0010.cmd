mlr --from reg-test/input/abixy-het put '$* = splitkv("a=1,b=2,c", IPS, IFS); print ">>".IRS."<<"; for (k, v in $*) {print k.":".typeof(k)." ".v.":".typeof(v)}'
