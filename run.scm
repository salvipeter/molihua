(define filename "models/L-shape.obj")
(define fullness 0.5)
(define tangent-scale 1)
(define only-one-patch 7)
(define triangle-size 0.05)
(define generate-model #f)

(load "molihua.scm")
(load-model filename)

(write-offsets "/tmp/offsets.obj")
(write-chamfers "/tmp/chamfers.obj")
(write-controls "/tmp/controls.obj")
;(write-patches-mp "/tmp/patch-")
;(write-patches-cgb "/tmp/patch-")
;(write-patches-mgbs "/tmp/patch-")



(define (shell cmd)
  (cond-expand
    (guile (system cmd))
    (gambit (shell-command cmd))))

(when generate-model?
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
                                      (number->string triangle-size)
                                      " > /dev/null")))
              indices)
    (shell (apply string-append
                  `("awk -f obj2stl.awk"
                    ,@(map (lambda (i)
                             (string-append " /tmp/surf-"
                                            (number->string i)
                                            ".obj"))
                           indices)
                    " > /tmp/model.stl")))))
