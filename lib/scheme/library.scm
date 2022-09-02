; library functions

(compile '(define (caar x) (car (car x))))
(compile '(define (cadr x) (car (cdr x))))
(compile '(define (cdar x) (cdr (car x))))
(compile '(define (cddr x) (cdr (cdr x))))

(compile '(define (caaar x) (car (car (car x)))))
(compile '(define (caadr x) (car (car (cdr x)))))
(compile '(define (cadar x) (car (cdr (car x)))))
(compile '(define (caddr x) (car (cdr (cdr x)))))
(compile '(define (cdaar x) (cdr (car (car x)))))
(compile '(define (cdadr x) (cdr (car (cdr x)))))
(compile '(define (cddar x) (cdr (cdr (car x)))))
(compile '(define (cdddr x) (cdr (cdr (cdr x)))))

(compile '(define (caaaar x) (car (car (car (car x))))))
(compile '(define (caaadr x) (car (car (car (cdr x))))))
(compile '(define (caadar x) (car (car (cdr (car x))))))
(compile '(define (caaddr x) (car (car (cdr (cdr x))))))
(compile '(define (cadaar x) (car (cdr (car (car x))))))
(compile '(define (cadadr x) (car (cdr (car (cdr x))))))
(compile '(define (caddar x) (car (cdr (cdr (car x))))))
(compile '(define (cadddr x) (car (cdr (cdr (cdr x))))))
(compile '(define (cdaaar x) (cdr (car (car (car x))))))
(compile '(define (cdaadr x) (cdr (car (car (cdr x))))))
(compile '(define (cdadar x) (cdr (car (cdr (car x))))))
(compile '(define (cdaddr x) (cdr (car (cdr (cdr x))))))
(compile '(define (cddaar x) (cdr (cdr (car (car x))))))
(compile '(define (cddadr x) (cdr (cdr (car (cdr x))))))
(compile '(define (cdddar x) (cdr (cdr (cdr (car x))))))
(compile '(define (cddddr x) (cdr (cdr (cdr (cdr x))))))

(compile '(define (first x) (car x)))
(compile '(define (second x) (car (cdr x))))
(compile '(define (third x) (car (cdr (cdr x)))))
(compile '(define (fourth x) (car (cdr (cdr (cdr x))))))
(compile '(define (fifth x) (car (cdr (cdr (cdr (cdr x)))))))
(compile '(define (sixth x) (car (cdr (cdr (cdr (cdr (cdr x))))))))
(compile '(define (seventh x) (car (cdr (cdr (cdr (cdr (cdr (cdr x)))))))))
(compile '(define (eighth x) (car (cdr (cdr (cdr (cdr (cdr (cdr (cdr x))))))))))
(compile '(define (ninth x) (car (cdr (cdr (cdr (cdr (cdr (cdr (cdr (cdr x)))))))))))
(compile '(define (tenth x) (car (cdr (cdr (cdr (cdr (cdr (cdr (cdr (cdr (cdr x))))))))))))

(compile '(define (square x) (* x x)))

(compile '(define (sqrt x) (expt x 0.5)))

(compile '(define (cube x) (* x x x)))

(compile '(define (inc n) (+ 1 n)))

(compile '(define (identity x) x))

(compile '(define (average x y) (/ (+ x y) 2)))

(compile '(define (positive? x) (> x 0)))

(compile '(define (negative? x) (< x 0)))

(compile '
    (define (gcd a b)
        (if (= b 0)
            a
            (gcd b (remainder a b))))
)

(compile '
    (define (list-ref n lst)
        (cond ((null? lst) '())
            ((= n 0) (car lst))
            (else (list-ref (- n 1) (cdr lst)))))
)

(compile '
    (define (map f lst)
        (if (null? lst)
            '()
            (cons (f (car lst))
                (map f (cdr lst)))))
)

; diagnostics tools

(define (assert-equal expression expected)
    (let ((result (eval expression)))
        (cond ((equal? result expected)
               (info "%s == %s" expression expected))  ; exactly equals
              ((and (number? expected)
                    (number? result)
                    (< (abs (- result expected)) 1e-4))
               (info "%s ~= %s" expression expected))  ; approximately equals
              (else (error "%s == %s, but %s was expected" expression result expected)))))

(define (measure expression)
    (let ((start (time)))
        (begin (eval expression)
               (info "%s took %s seconds" expression (- (time) start)))))
