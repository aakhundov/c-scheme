; 2.2.2 Hierarchical Structures

(define x (cons (list 1 2) (list 3 4)))

(define (count-leaves x)
    (cond ((null? x) 0)
          ((not (pair? x)) 1)
          (else (+ (count-leaves (car x))
                   (count-leaves (cdr x))))))

(assert-equals '(length x) 3)
(assert-equals '(count-leaves x) 4)

(assert-equals '(list x x) '(((1 2) 3 4) ((1 2) 3 4)))

(assert-equals '(length (list x x)) 2)
(assert-equals '(count-leaves (list x x)) 8)

(define (scale-tree1 tree factor)
    (cond ((null? tree) nil)
          ((not (pair? tree)) (* tree factor))
          (else (cons (scale-tree1 (car tree) factor)
                      (scale-tree1 (cdr tree) factor)))))

(assert-equals '
    (scale-tree1 (list 1 (list 2 (list 3 4) 5) (list 6 7)) 10)
    '(10 (20 (30 40) 50) (60 70)))

(define (scale-tree2 tree factor)
    (map (lambda (sub-tree)
             (if (pair? sub-tree)
                 (scale-tree2 sub-tree factor)
                 (* sub-tree factor)))
         tree))

(assert-equals '
    (scale-tree2 (list 1 (list 2 (list 3 4) 5) (list 6 7)) 10)
    '(10 (20 (30 40) 50) (60 70)))
