# Usage: awk -f obj2stl.awk model1.obj ... modelN.obj > model.stl
BEGIN { print "solid model" }
BEGINFILE { n = 0 }
/^v/ { x[++n] = $2; y[n] = $3; z[n] = $4 }
/^f/ { printFacet($2, $3, $4) }
END { print "endsolid model" }

function printFacet(a, b, c) {
    dx1 = x[b] - x[a]; dy1 = y[b] - y[a]; dz1 = z[b] - z[a]
    dx2 = x[c] - x[a]; dy2 = y[c] - y[a]; dz2 = z[c] - z[a]
    nx = dy1 * dz2 - dy2 * dz1
    ny = dx2 * dz1 - dx1 * dz2
    nz = dx1 * dy2 - dx2 * dy1
    len = sqrt(nx * nx + ny * ny + nz * nz)
    if (len > 0) {
        nx /= len; ny /= len; nz /= len
    }
    print "facet normal", nx, ny, nz
    print "  outer loop"
    print "    vertex", x[a], y[a], z[a]
    print "    vertex", x[b], y[b], z[b]
    print "    vertex", x[c], y[c], z[c]
    print "  endloop"
    print "endfacet"
}
