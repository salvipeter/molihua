(define filename "cube.obj")
(define fullness 0.5)
(define tangent-scale 1.4)
(define write-offsets #t)
(define write-chamfers #t)
(define write-controls #t)
(define write-patches #t)


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


;;; Utilities

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

(define (offset-face verts)
  (let ((center (v* (apply v+ verts)
                    (/ (length verts)))))
    (map (lambda (p)
           (v+ center
               (v* (v- p center)
                   fullness)))
         verts)))

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


;;; Main code

;;; Read input cage
(define vertices #f)
(define faces #f)
(let ((model (read-obj filename)))
  (set! vertices (car model))
  (set! faces (cdr model)))

;;; Generate topology information
(define vertex-faces
  (do ((result (make-vector (vector-length vertices) '()))
       (i 0 (+ i 1)))
      ((= i (vector-length faces)) result)
    (for-each (lambda (v)
                (vector-set! result v
                             (cons i (vector-ref result v))))
              (vector-ref faces i))))

;;; Create offset faces
(define offset-vertices #f)
(define offset-faces #f)
(let ((offsets (generate-offsets vertices faces)))
  (set! offset-vertices (car offsets))
  (set! offset-faces (cdr offsets)))


;;; Generate output

(when write-offsets
  (with-output-to-file "/tmp/offsets.obj"
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
      (vector-for-each
       (lambda (face)
         (display "f")
         (for-each (lambda (v)
                     (display " ")
                     (display (+ v 1)))
                   face)
         (newline))
       offset-faces))))

;;; Returns the chamfer associated with the v-th vertex.
;;; The result is a list of indices to offset-vertices.
(define (chamfer v)
  (map (lambda (f)
         (list-ref (vector-ref offset-faces f)
                   (find-index v (vector-ref faces f))))
       (vector-ref vertex-faces v)))

(when write-chamfers
  (with-output-to-file "/tmp/chamfers.obj"
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
      (do ((i 0 (+ i 1)))
          ((= i (vector-length vertices)))
        (display "f")
        (for-each (lambda (v)
                    (display " ")
                    (display (+ v 1)))
                  (chamfer i))
        (newline)))))

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

(when write-controls
  (with-output-to-file "/tmp/controls.obj"
    (lambda ()
      (do ((index 0)
           (i 0 (+ i 1)))
          ((= i (vector-length faces)))
        (do ((j 0 (+ j 1)))
            ((= j (length (vector-ref faces i))))
          (let ((ribbon (generate-ribbon i j)))
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
              (set! index (+ index 8)))))))))

(when write-patches
  (do ((index 0)
       (i 0 (+ i 1)))
      ((= i (vector-length faces)))
    (with-output-to-file (string-append "/tmp/patch-" (number->string (+ i 1)) ".mp")
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
          (do ((j 0 (+ j 1)))
              ((= j n))
            (let ((ribbon (generate-ribbon i j)))
              (write-curve (car ribbon))
              (write-curve (cdr ribbon)))))))))
