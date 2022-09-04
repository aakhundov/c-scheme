; Exercise 2.1

(define (numer x) (car x))
(define (denom x) (cdr x))

(define (print-rat x)
    (display (numer x))
    (display "/")
    (display (denom x))
    (newline))

(define (make-rat n d)
    (cond ((= d 0) (error "division by zero"))
          ((= n 0) (cons 0 1))
           (else (let ((g (gcd (abs n) (abs d)))
                       (s (if (or (and (< n 0) (< d 0))
                                    (and (>= n 0) (>= d 0)))
                               1
                               -1)))
                     (cons (/ (* s (abs n)) g) (/ (abs d) g))))))

(assert-equal '(make-rat 2 4) (cons 1 2))
(assert-equal '(make-rat -2 4) (cons -1 2))
(assert-equal '(make-rat 2 -4) (cons -1 2))
(assert-equal '(make-rat -2 -4) (cons 1 2))
(assert-equal '(make-rat 0 4) (cons 0 1))
(assert-equal '(make-rat 0 -4) (cons 0 1))

; this returns an error
(print-rat (make-rat 4 0))
