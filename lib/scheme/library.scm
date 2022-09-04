(compile '(begin  ; compile all define expressions below

; library functions

(define (caar x) (car (car x)))
(define (cadr x) (car (cdr x)))
(define (cdar x) (cdr (car x)))
(define (cddr x) (cdr (cdr x)))

(define (caaar x) (car (car (car x))))
(define (caadr x) (car (car (cdr x))))
(define (cadar x) (car (cdr (car x))))
(define (caddr x) (car (cdr (cdr x))))
(define (cdaar x) (cdr (car (car x))))
(define (cdadr x) (cdr (car (cdr x))))
(define (cddar x) (cdr (cdr (car x))))
(define (cdddr x) (cdr (cdr (cdr x))))

(define (caaaar x) (car (car (car (car x)))))
(define (caaadr x) (car (car (car (cdr x)))))
(define (caadar x) (car (car (cdr (car x)))))
(define (caaddr x) (car (car (cdr (cdr x)))))
(define (cadaar x) (car (cdr (car (car x)))))
(define (cadadr x) (car (cdr (car (cdr x)))))
(define (caddar x) (car (cdr (cdr (car x)))))
(define (cadddr x) (car (cdr (cdr (cdr x)))))
(define (cdaaar x) (cdr (car (car (car x)))))
(define (cdaadr x) (cdr (car (car (cdr x)))))
(define (cdadar x) (cdr (car (cdr (car x)))))
(define (cdaddr x) (cdr (car (cdr (cdr x)))))
(define (cddaar x) (cdr (cdr (car (car x)))))
(define (cddadr x) (cdr (cdr (car (cdr x)))))
(define (cdddar x) (cdr (cdr (cdr (car x)))))
(define (cddddr x) (cdr (cdr (cdr (cdr x)))))

(define (first x) (car x))
(define (second x) (car (cdr x)))
(define (third x) (car (cdr (cdr x))))
(define (fourth x) (car (cdr (cdr (cdr x)))))
(define (fifth x) (car (cdr (cdr (cdr (cdr x))))))
(define (sixth x) (car (cdr (cdr (cdr (cdr (cdr x)))))))
(define (seventh x) (car (cdr (cdr (cdr (cdr (cdr (cdr x))))))))
(define (eighth x) (car (cdr (cdr (cdr (cdr (cdr (cdr (cdr x)))))))))
(define (ninth x) (car (cdr (cdr (cdr (cdr (cdr (cdr (cdr (cdr x))))))))))
(define (tenth x) (car (cdr (cdr (cdr (cdr (cdr (cdr (cdr (cdr (cdr x)))))))))))

(define (square x) (* x x))
(define (sqrt x) (expt x 0.5))
(define (cube x) (* x x x))
(define (inc n) (+ 1 n))
(define (identity x) x)
(define (average x y) (/ (+ x y) 2))
(define (positive? x) (> x 0))
(define (negative? x) (< x 0))

(define (gcd a b)
    (if (= b 0)
        a
        (gcd b (remainder a b))))

(define (list-ref items n)
    (cond ((null? items) '())
          ((= n 0) (car items))
          (else (list-ref (cdr items) (- n 1)))))

(define (map f items)
    (define (iter rest tail)
        (if (null? rest)
            false
            (let ((next (list (f (car rest)))))
                (set-cdr! tail next)
                (iter (cdr rest) next))))
    (if (null? items)
        '()
        (let ((head (list (f (car items)))))
            (iter (cdr items) head)
            head)))

(define (length items)
    (define (iter rest result)
        (if (null? rest)
            result
            (iter (cdr rest) (+ 1 result))))
    (iter items 0))

(define (append list1 list2)
    (define (iter rest1 tail)
        (if (null? rest1)
            (set-cdr! tail list2)
            (let ((next (list (car rest1))))
                (set-cdr! tail next)
                (iter (cdr rest1) next))))
    (if (null? list1)
        list2
        (let ((head (list (car list1))))
            (iter (cdr list1) head)
            head)))

(define (reverse items)
    (define (iter rest head)
        (if (null? rest)
            head
            (let ((new-head (list (car rest))))
                (set-cdr! new-head head)
                (iter (cdr rest) new-head))))
    (if (or (null? items) (null? (cdr items)))
        items
        (iter (cdr items)
              (list (car items)))))

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

)) ; closing of the "(compile '(begin" expression
