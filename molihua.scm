;;; Global variables
(define vertices #f)
(define faces #f)
(define offset-vertices #f)
(define offset-faces #f)
(define vertex-faces #f)
(define ribbons #f)


;;; R7RS compatibility
(cond-expand
  (guile
   (use-modules (ice-9 rdelim))         ; read-line
   (define (vector-map f v)
     (let ((len (vector-length v)))
       (do ((result (make-vector len))
            (i 0 (+ i 1)))
           ((= i len) result)
         (vector-set! result i (f (vector-ref v i))))))
   (define (vector-for-each f v)
     (let ((len (vector-length v)))
       (do ((i 0 (+ i 1)))
           ((= i len))
         (f (vector-ref v i))))))
  (gambit))


;;; General utilities

;;; Brings the last element to the beginning
(define (rotate lst)
  (let loop ((lst lst) (acc '()))
    (if (null? (cdr lst))
        (append lst (reverse acc))
        (loop (cdr lst) (cons (car lst) acc)))))

;;; Split by spaces; multiple spaces count as one.
;;; Newlines are also count as spaces; starting/ending spaces are discounted.
;;; Trim chunks starting with '/' characters.
(define (split-string s)
  (define (stringify-car! lst)
    (set-car! lst (list->string (reverse (car lst)))))
  (do ((result (list (list)))
       (skip #f)
       (lst (string->list s) (cdr lst)))
      ((null? lst)
       (if (null? (car result))
           (set! result (cdr result))
           (stringify-car! result))
       (reverse result))
    (case (car lst)
      ((#\space #\return #\newline)
       (unless (null? (car result))
         (stringify-car! result)
         (set! result (cons (list) result))
         (set! skip #f)))
      ((#\/)
       (set! skip #t))
      (else (unless skip
              (set-car! result
                        (cons (car lst) (car result))))))))

;;; List of integers (from .. end-1)
(define (range from end)
  (let loop ((from from))
    (if (= from end)
        '()
        (cons from (loop (+ from 1))))))

(define (find-index elt lst)
  (let loop ((i 0) (lst lst))
    (cond ((null? lst) #f)
          ((eq? (car lst) elt) i)
          (else (loop (+ i 1) (cdr lst))))))

(define (common-element a b)
  (let loop ((a a))
    (cond ((null? a) #f)
          ((memq (car a) b) (car a))
          (else (loop (cdr a))))))

(define (common-elements a b)
  (let loop ((a a))
    (cond ((null? a) '())
          ((memq (car a) b)
           (cons (car a) (loop (cdr a))))
          (else (loop (cdr a))))))

(define (disjoint? a b)
  (eq? (common-element a b) #f))

;;; Finds a list in lst (a list of lists) that has
;;; common elements with both a and b (both lists).
(define (find-connection a b lst)
  (let loop ((lst lst))
    (cond ((null? lst) #f)
          ((or (disjoint? (car lst) a)
               (disjoint? (car lst) b))
           (loop (cdr lst)))
          (else (car lst)))))


;;; I/O routines

(define (read-obj filename)
  (with-input-from-file filename
    (lambda ()
      (do ((v '())
           (f '())
           (line (read-line) (read-line)))
          ((eof-object? line)
           (cons (list->vector (reverse v))
                 (list->vector (reverse f))))
        (let ((type (substring line 0 2)))
          (cond ((string-ci=? type "v ")
                 (let ((vertex (map string->number (cdr (split-string line)))))
                   (set! v (cons vertex v))))
                ((string-ci=? type "f ")
                 (let ((face (map (lambda (s)
                                    (- (string->number s) 1))
                                  (cdr (split-string line)))))
                   (set! f (cons face f))))))))))

(define (write-offsets filename)
  (with-output-to-file filename
    (lambda ()
      (vector-for-each
       (lambda (v)
         (display "v")
         (for-each (lambda (xyz)
                     (display " ")
                     (display xyz))
                   v)
         (newline))
       offset-vertices)
      (let ((write-face (lambda (face)
                          (display "f")
                          (for-each (lambda (v)
                                      (display " ")
                                      (display (+ v 1)))
                                    face)
                          (newline))))
        (if only-one-patch
            (write-face (vector-ref offset-faces only-one-patch))
            (vector-for-each write-face offset-faces))))))

(define (write-chamfers filename)
  (with-output-to-file filename
    (lambda ()
      (vector-for-each
       (lambda (v)
         (display "v")
         (for-each (lambda (xyz)
                     (display " ")
                     (display xyz))
                   v)
         (newline))
       offset-vertices)
      (let ((write-chamfer (lambda (i)
                             (display "f")
                             (for-each (lambda (v)
                                         (display " ")
                                         (display (+ v 1)))
                                       (chamfer i))
                             (newline))))
        (if only-one-patch
            (for-each write-chamfer (vector-ref faces only-one-patch))
            (do ((i 0 (+ i 1)))
                ((= i (vector-length vertices)))
              (write-chamfer i)))))))

(define (write-controls filename)
  (with-output-to-file filename
    (lambda ()
      (do ((index 0)
           (i 0 (+ i 1)))
          ((= i (vector-length faces)))
        (when (or (not only-one-patch)
                  (= only-one-patch i))
          (for-each (lambda (ribbon)
                      (for-each (lambda (vertex)
                                  (display "v")
                                  (for-each (lambda (xyz)
                                              (display " ")
                                              (display xyz))
                                            vertex)
                                  (newline))
                                (append (car ribbon) (cdr ribbon)))
                      (let ((line (lambda (indices)
                                    (display "l")
                                    (for-each (lambda (v)
                                                (display " ")
                                                (display (+ index v)))
                                              indices)
                                    (newline))))
                        (line '(1 2 3 4))
                        (line '(5 6 7 8))
                        (line '(1 5))
                        (line '(2 6))
                        (line '(3 7))
                        (line '(4 8))
                        (set! index (+ index 8))))
                    (vector-ref ribbons i)))))))

(define (write-patches filename-prefix)
  (do ((index 0)
       (i 0 (+ i 1)))
      ((= i (vector-length faces)))
    (when (or (not only-one-patch)
              (= only-one-patch i))
      (with-output-to-file
          (string-append filename-prefix (number->string (+ i 1)) ".mp")
        (lambda ()
          (let ((n (length (vector-ref faces i)))
                (write-curve (lambda (cpts)
                               (display "3 4")
                               (newline)
                               (display "0 0 0 0 1 1 1 1")
                               (newline)
                               (for-each (lambda (vertex)
                                           (for-each (lambda (xyz)
                                                       (display xyz)
                                                       (display " "))
                                                     vertex)
                                           (newline))
                                         cpts))))
            (display n)
            (newline)
            (for-each (lambda (ribbon)
                        (write-curve (car ribbon))
                        (write-curve (cdr ribbon)))
                      (vector-ref ribbons i))))))))


;;; Geometry procedures

(define (v+ . args)
  (apply map + args))

(define (v- . args)
  (apply map - args))

(define (v* u . args)
  (map (lambda (x) (apply * x args)) u))

(define (scalar-product u v)
  (apply + (map * u v)))

(define (vlength u)
  (sqrt (scalar-product u u)))

(define (vnormalize u)
  (v* u (/ (vlength u))))

(define (point-distance p q)
  (vlength (v- q p)))

;;; Line is given as (point . point)
(define (line-point-distance line p)
  (let* ((q1 (car line))
         (q2 (cdr line))
         (v (v- p q1))
         (d (v- q2 q1)))
    (vlength (v- v (v* d (/ (scalar-product v d)
                            (scalar-product d d)))))))

;;; Pushes the line towards the point by `offset` part of their distance
(define (push-line line point offset)
  (let ((d (v* (v- point (car line)) offset)))
    (cons (v+ (car line) d)
          (v+ (cdr line) d))))

(define (line-line-intersection l1 l2)
  (let* ((ap (car l1))
         (ad (v- (cdr l1) (car l1)))
         (bp (car l2))
         (bd (v- (cdr l2) (car l2)))
         (a (scalar-product ad ad))
         (b (scalar-product ad bd))
         (c (scalar-product bd bd))
         (d (scalar-product ad (v- ap bp)))
         (e (scalar-product bd (v- ap bp))))
    (let ((denom (- (* a c) (* b b))))
      (if (< denom 1e-7)
          (error "parallel lines")
          (let ((s (/ (- (* b e) (* c d)) denom))
                (t (/ (- (* a e) (* b d)) denom)))
            (v* (v+ ap (v* ad s) bp (v* bd t)) 0.5))))))

;;; Simple heuristic version (not used)
(define (offset-face-central verts)
  (let ((center (v* (apply v+ verts)
                    (/ (length verts)))))
    (map (lambda (p)
           (v+ center
               (v* (v- p center)
                   fullness)))
         verts)))

(define (closest-vertex line verts)
  (let loop ((best #f) (dist #f) (lst verts))
    (cond ((null? lst) best)
          ((or (equal? (car line) (car lst))
               (equal? (cdr line) (car lst)))
           (loop best dist (cdr lst)))
          (else (let ((d (line-point-distance line (car lst))))
                  (if (or (not dist) (< d dist))
                      (loop (car lst) d (cdr lst))
                      (loop best dist (cdr lst))))))))

(define (offset-face verts)
  (let* ((lines (map cons verts (append (cdr verts) (list (car verts)))))
         (points (map (lambda (l) (closest-vertex l verts)) lines))
         (offsets (map (lambda (l p) (push-line l p (/ fullness 2))) lines points)))
    (map line-line-intersection (rotate offsets) offsets)))

;;; Returns (verts* . faces*)
(define (generate-offsets vertices faces)
  (do ((rfaces (make-vector (vector-length faces)))
       (rvertices '())
       (index 0)
       (i 0 (+ i 1)))
      ((= i (vector-length faces))
       (cons (list->vector (reverse rvertices)) rfaces))
    (let ((off (offset-face
                (map (lambda (v)
                       (vector-ref vertices v))
                     (vector-ref faces i))))
          (n (length (vector-ref faces i))))
      (set! rvertices (append (reverse off) rvertices))
      (vector-set! rfaces i (range index (+ index n)))
      (set! index (+ index n)))))

(define (update-topology)
  (set! vertex-faces
    (do ((result (make-vector (vector-length vertices) '()))
         (i 0 (+ i 1)))
        ((= i (vector-length faces))
         (vector-map fix-order result))
      (for-each (lambda (v)
                  (vector-set! result v
                               (cons i (vector-ref result v))))
                (vector-ref faces i)))))

(define (update-offsets)
  (let ((offsets (generate-offsets vertices faces)))
    (set! offset-vertices (car offsets))
    (set! offset-faces (cdr offsets))))

;;; Moves the adjacent face to the beginning
(define (select-adjacent face lst)
  (let loop ((lst lst) (acc '()))
    (cond ((null? lst)
           (error "cannot find adjacent face"))
          ((= (length (common-elements (vector-ref faces face)
                                       (vector-ref faces (car lst))))
              2)
           (cons (car lst) (append acc (cdr lst))))
          (else
           (loop (cdr lst) (cons (car lst) acc))))))

;;; Permute the list of faces to be cyclic
(define (fix-order faces)
  (let loop ((lst faces))
    (if (null? (cdr lst))
        lst
        (cons (car lst)
              (loop (select-adjacent (car lst) (cdr lst)))))))

;;; Returns the chamfer associated with the v-th vertex.
;;; The result is a list of indices to offset-vertices.
(define (chamfer v)
  (map (lambda (f)
         (list-ref (vector-ref offset-faces f)
                   (find-index v (vector-ref faces f))))
       (vector-ref vertex-faces v)))

;;; At vertex j with chamfer c0, find the face connecting to chamfer c1,
;;; which is the opposite of face i (i.e., not face i).
(define (find-opposite i j c0 c1)
  (let ((candidates
         (let loop ((lst (vector-ref vertex-faces j)))
           (cond ((null? lst) '())
                 ((= (car lst) i) (loop (cdr lst)))
                 (else (cons (vector-ref offset-faces (car lst))
                             (loop (cdr lst))))))))
    (find-connection c0 c1 candidates)))

;;; Face i side j
;;; m: chamfer midpoint
;;; o: chamfer vertex in the current face
;;; e: chamfer midpoint between the current and opposite faces
;;; f: chamfer midpoint between the current and adjacent-opposite faces
(define (generate-ribbon i j)
  (let* ((face (vector-ref faces i))
         (offset-face (vector-ref offset-faces i))
         (n (length face))
         (j-1 (modulo (- j 1) n))
         (j+1 (modulo (+ j 1) n))
         (j+2 (modulo (+ j 2) n))
         (c0 (chamfer (list-ref face j)))
         (c1 (chamfer (list-ref face j+1)))
         (m0 (v* (apply v+ (map (lambda (v)
                                  (vector-ref offset-vertices v))
                                c0))
                 (/ (length c0))))
         (m1 (v* (apply v+ (map (lambda (v)
                                  (vector-ref offset-vertices v))
                                c1))
                 (/ (length c1))))
         (scaled-offset (lambda (c i)
                          (v+ c (v* (v- (vector-ref offset-vertices i) c)
                                    tangent-scale))))
         (o0 (scaled-offset m0 (list-ref offset-face j)))
         (o1 (scaled-offset m1 (list-ref offset-face j+1)))
         ;; the opposite face includes a vertex index from both c0 and c1
         (opp (find-opposite i (list-ref face j) c0 c1))
         (opp-1 (find-opposite i (list-ref face j) c0 (chamfer (list-ref face j-1))))
         (opp+1 (find-opposite i (list-ref face j+1) c1 (chamfer (list-ref face j+2))))
         (e0 (v* (v+ o0 (scaled-offset m0 (common-element c0 opp))) 1/2))
         (e1 (v* (v+ o1 (scaled-offset m1 (common-element c1 opp))) 1/2))
         (f0 (v* (v+ o0 (scaled-offset m0 (common-element c0 opp-1))) 1/2))
         (f1 (v* (v+ o1 (scaled-offset m1 (common-element c1 opp+1))) 1/2)))
    (cons (list m0 e0 e1 m1)
          (list f0 o0 o1 f1))))

(define (fix-continuity ribbons)
  ribbons)                              ; TODO

(define (update-ribbons)
  (set! ribbons
    (do ((result (make-vector (vector-length faces)))
         (i 0 (+ i 1)))
        ((= i (vector-length faces))
         (fix-continuity result))
      (vector-set! result i
                   (map (lambda (j)
                          (generate-ribbon i j))
                        (range 0 (length (vector-ref faces i))))))))


;;; Main function

(define (load-model filename)
  (let ((model (read-obj filename)))
    (set! vertices (car model))
    (set! faces (cdr model)))
  (update-topology)
  (update-offsets)
  (update-ribbons))
