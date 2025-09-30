# Usage: awk -f flip-orientation.awk model.obj > flipped.obj
/f / {
    line = "f"
    for (i = NF; i > 1; i--)
        line = line " " $i
    print line
    next
}
{ print }
