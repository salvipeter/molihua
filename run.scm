(define filename "models/cube1.obj")
(define fullness 0.7)
(define tangent-scale 1.33)
(define only-one-patch #f)              ; #f or patch index
(define triangle-size 0.2)

(load "molihua.scm")
(load-model filename)

(write-offsets "/tmp/offsets.obj")
(write-chamfers "/tmp/chamfers.obj")
(write-controls "/tmp/controls.obj")
;(write-patches-mp "/tmp/patch-")
;(write-patches-cgb "/tmp/patch-")
(write-patches-mgbs "/tmp/patch-")

(define (shell cmd)
  (cond-expand
    (guile (system cmd))
    (gambit (shell-command cmd))))

(let ((indices (if only-one-patch
                   (list only-one-patch)
                   (do ((i 1 (+ i 1))
                        (lst '() (cons i lst)))
                       ((> i (vector-length faces)) (reverse lst))))))
  (for-each (lambda (i)
              (shell (string-append "./mgbs2obj /tmp/patch-"
                                    (number->string i)
                                    ".mgbs /tmp/surf-"
                                    (number->string i)
                                    ".obj "
                                    (number->string triangle-size))))
            indices)
  (shell (apply string-append
                `("awk -f obj2stl.awk"
                  ,@(map (lambda (i)
                           (string-append " /tmp/surf-"
                                          (number->string i)
                                          ".obj"))
                         indices)
                  " > /tmp/model.stl"))))
