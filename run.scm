(define filename "cube1.obj")
(define fullness 0.5)
(define tangent-scale 1.4)
(define only-one-patch #f)
(define write-offsets? #t)
(define write-chamfers? #t)
(define write-controls? #t)
(define write-patches? #t)

(load "molihua.scm")                      ; reload functions (only for debugging)
(load-model filename)

(when write-offsets?
  (write-offsets "/tmp/offsets.obj"))
(when write-chamfers?
  (write-chamfers "/tmp/chamfers.obj"))
(when write-controls?
  (write-controls "/tmp/controls.obj"))
(when write-patches?
  (write-patches "/tmp/patch-"))
