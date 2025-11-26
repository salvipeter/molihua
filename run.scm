(define filename "models/elretoltkocka.obj")

;;; Shape parameters
(define fullness 0.7)
(define edge-based-offsets? #f)
(define shrink-inwards? #t)
(define shrink-outwards? #t)
(define tangent-scale 1)
(define midvector-scale 1)

;;; Direction blend options
; Possible values:
; - none
; - cubic, cubic-simple, cubic-no-alpha, cubic-tomi-simple, cubic-peti-simple,
;   cubic-linear, cubic-linear-c0
; - quartic-simple, quartic-no-alpha, quartic-tomi-simple, quartic-tomi-no-alpha
; - quintic, quintic-tomi
(define direction-blend-type 'cubic-tomi-simple)
(define cubic-cross-degree? #t)

;;; Output
(define only-one-patch #f)
(define resolution 200)
(define generate-model? #f)

(load "molihua.scm")
(load-model filename)

(write-offsets "/tmp/offsets.obj")
(write-chamfers "/tmp/chamfers.obj")
(write-controls "/tmp/controls.obj")
(write-polyhedron "/tmp/polyhedron.obj")
(write-patches-mgbs "/tmp/patch-")



(define (shell cmd)
  (cond-expand
    (guile (system cmd))
    (gambit (shell-command cmd))))

(when generate-model?
  (let ((indices (if only-one-patch
                     (list only-one-patch)
                     (do ((i 1 (+ i 1))
                          (lst '() (cons i lst)))
                         ((> i (vector-length faces)) (reverse lst)))))
        (triangle-size (do ((i 0 (+ i 1))
                            (min-point (vector-ref vertices 0)
                                       (map min min-point (vector-ref vertices i)))
                            (max-point (vector-ref vertices 0)
                                       (map max max-point (vector-ref vertices i))))
                           ((= i (vector-length vertices))
                            (/ (point-distance max-point min-point) resolution)))))
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
