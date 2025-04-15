(define filename "models/cube1.obj")
(define fullness 0.7)
(define tangent-scale 1.33)
(define only-one-patch #f)              ; #f or patch index

(load "molihua.scm")
(load-model filename)

(write-offsets "/tmp/offsets.obj")
(write-chamfers "/tmp/chamfers.obj")
(write-controls "/tmp/controls.obj")
;(write-patches-mp "/tmp/patch-")
(write-patches-cgb "/tmp/patch-")
