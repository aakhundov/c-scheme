; 2.2.1 Representing Sequences

(assert-equals '
    (cons 1
        (cons 2
            (cons 3
                (cons 4 nil))))
    '(1 2 3 4))

(define one-through-four (list 1 2 3 4))

(assert-equals 'one-through-four '(1 2 3 4))

(assert-equals '(car one-through-four) 1)
(assert-equals '(cdr one-through-four) '(2 3 4))
(assert-equals '(car (cdr one-through-four)) 2)
(assert-equals '(cons 10 one-through-four) '(10 1 2 3 4))
(assert-equals '(cons 5 one-through-four) '(5 1 2 3 4))

(define (list-ref-iter items n)
    (if (= n 0)
        (car items)
        (list-ref-iter (cdr items) (- n 1))))

(define squares (list 1 4 9 16 25))

(assert-equals '(list-ref-iter squares 3) 16)

(define (length-rec items)
    (if (null? items)
        0
        (+ 1 (length-rec (cdr items)))))

(define odds (list 1 3 5 7))

(assert-equals '(length-rec odds) 4)

(define (length-iter items)
    (define (iter a count)
        (if (null? a)
            count
            (iter (cdr a) (+ 1 count))))
    (iter items 0))

(assert-equals '(length-iter odds) 4)

(define (append-rec list1 list2)
    (if (null? list1)
        list2
        (cons (car list1) (append-rec (cdr list1) list2))))

(assert-equals '(append-rec squares odds) '(1 4 9 16 25 1 3 5 7))
(assert-equals '(append-rec odds squares) '(1 3 5 7 1 4 9 16 25))

(define (scale-list items factor)
    (if (null? items)
        nil
        (cons (* (car items) factor)
              (scale-list (cdr items)
                          factor))))

(assert-equals '(scale-list (list 1 2 3 4 5) 10) '(10 20 30 40 50))

(define (map-rec proc items)
    (if (null? items)
        nil
        (cons (proc (car items))
              (map-rec proc (cdr items)))))

(assert-equals '(map-rec abs (list -10 2.5 -11.6 17)) '(10 2.5 11.6 17))
(assert-equals '(map-rec (lambda (x) (* x x)) (list 1 2 3 4)) '(1 4 9 16))

(define (scale-list items factor)
    (map (lambda (x) (* x factor))
         items))

(assert-equals '(scale-list (list 1 2 3 4 5) 10) '(10 20 30 40 50))
