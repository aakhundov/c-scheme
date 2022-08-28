; Exercise 1.36

(define tolerance 0.00001)
(define (fixed-point f first-guess)
    (define (close-enough? v1 v2)
        (< (abs (- v1 v2))
           tolerance))
    (define (try guess)
        (let ((next (f guess)))
            (display guess)
            (newline)
            (if (close-enough? guess next)
                next
                (try next))))
    (try first-guess))

(define log-1000 (log 1000))

; without average damping
(assert-equal '
    (fixed-point (lambda (x) (/ log-1000 (log x)))
                 4.0)
    4.55555)

; with average damping
(assert-equal '
    (fixed-point (lambda (x) (average x (/ log-1000 (log x))))
                 4.0)
    4.55555)
