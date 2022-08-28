; Exercise 1.8

(define (cube-root-iter guess x)
    (let ((improved (improve guess x)))
        (if (good-enough? guess improved)
            guess
            (cube-root-iter improved x))))

(define (improve guess x)
    (/ (+ (/ x (* guess guess))
          (* 2 guess))
       3))

(define (good-enough? guess improved)
    (< (abs (- guess improved)) 1e-8))

(define (cube-root x)
    (cube-root-iter 1.0 x))

(assert-equal '(cube-root 1e9) 1000.0000)
(assert-equal '(cube-root 1e8) 464.15888)
(assert-equal '(cube-root 1e7) 215.44347)
(assert-equal '(cube-root 1e6) 100.00000)
(assert-equal '(cube-root 1e5) 46.415888)
(assert-equal '(cube-root 1e4) 21.544347)
(assert-equal '(cube-root 1e3) 10.000000)
(assert-equal '(cube-root 1e2) 4.6415888)
(assert-equal '(cube-root 1e1) 2.1544347)

(assert-equal '(cube-root 1e0) 1)

(assert-equal '(cube-root 1e-1) 0.4641589)
(assert-equal '(cube-root 1e-2) 0.2154435)
(assert-equal '(cube-root 1e-3) 0.1000000)
(assert-equal '(cube-root 1e-4) 0.0464159)
(assert-equal '(cube-root 1e-5) 0.0215443)
(assert-equal '(cube-root 1e-6) 0.0100000)
(assert-equal '(cube-root 1e-7) 0.0046416)
(assert-equal '(cube-root 1e-8) 0.0021544)
