# Usage: awk -f scale-obj.awk -v scale=100 model.obj > scaled.obj
/v / { print "v", $2 * scale, $3 * scale, $4 * scale; next }
{ print }
