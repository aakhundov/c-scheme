; Exercise 2.2

(define (make-point x y) (cons x y))
(define (x-point p) (car p))
(define (y-point p) (cdr p))

(define p (make-point 1 2))

(assert-equals 'p (cons 1 2))
(assert-equals '(x-point p) 1)
(assert-equals '(y-point p) 2)

(define (make-segment p1 p2) (cons p1 p2))
(define (start-segment s) (car s))
(define (end-segment s) (cdr s))

(define s (make-segment (make-point 1 2) (make-point 3 4)))

(assert-equals 's (cons (cons 1 2) (cons 3 4)))
(assert-equals '(start-segment s) (cons 1 2))
(assert-equals '(end-segment s) (cons 3 4))

(define (midpoint-segment s)
    (make-point (average (x-point (start-segment s))
                         (x-point (end-segment s)))
                (average (y-point (start-segment s))
                         (y-point (end-segment s)))))

(assert-equals '(midpoint-segment s) (cons 2 3))

(define (print-point p)
    (display "(")
    (display (x-point p))
    (display ",")
    (display (y-point p))
    (display ")")
    (newline))

(define (print-segment s)
    (display "[(")
    (display (x-point (start-segment s)))
    (display ",")
    (display (y-point (start-segment s)))
    (display "), ")
    (display "(")
    (display (x-point (end-segment s)))
    (display ",")
    (display (y-point (end-segment s)))
    (display ")]")
    (newline))

(display "point p: ")
(print-point p)
(display "segment s: ")
(print-segment s)
(display "midpoint of s: ")
(print-point (midpoint-segment s))
