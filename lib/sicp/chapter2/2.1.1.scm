; 2.1.1 Example: Arithmetic Operations for Rational Numbers

(define (add-rat x y)
    (make-rat (+ (* (numer x) (denom y))
                 (* (numer y) (denom x)))
              (* (denom x) (denom y))))

(define (sub-rat x y)
    (make-rat (- (* (numer x) (denom y))
                 (* (numer y) (denom x)))
              (* (denom x) (denom y))))

(define (mul-rat x y)
    (make-rat (* (numer x) (numer y))
              (* (denom x) (denom y))))

(define (div-rat x y)
    (make-rat (* (numer x) (denom y))
              (* (denom x) (numer y))))

(define (equal-rat? x y)
    (= (* (numer x) (denom y))
       (* (numer y) (denom x))))

(define x (cons 1 2))

(assert-equals '(car x) 1)
(assert-equals '(cdr x) 2)

(define x (cons 1 2))
(define y (cons 3 4))
(define z (cons x y))

(assert-equals '(car (car z)) 1)
(assert-equals '(car (cdr z)) 3)

(define (make-rat n d) (cons n d))
(define (numer x) (car x))
(define (denom x) (cdr x))

(define (print-rat x)
    (display (numer x))
    (display "/")
    (display (denom x))
    (newline))

(define one-half (make-rat 1 2))

(assert-equals 'one-half (cons 1 2))

(print-rat one-half)

(define one-third (make-rat 1 3))

(assert-equals '(add-rat one-half one-third) (cons 5 6))
(assert-equals '(mul-rat one-half one-third) (cons 1 6))
(assert-equals '(add-rat one-third one-third) (cons 6 9))

(print-rat (add-rat one-half one-third))
(print-rat (mul-rat one-half one-third))
(print-rat (add-rat one-third one-third))

(define (make-rat n d)
    (let ((g (gcd n d)))
        (cons (/ n g) (/ d g))))

(assert-equals '(add-rat one-third one-third) (cons 2 3))

(print-rat (add-rat one-third one-third))
