; Exercise 2.23

(define (for-each f items)
    (if (null? items)
        '()
        (begin (f (car items))
               (for-each f (cdr items)))))

(for-each (lambda (x)
              (display x)
              (newline))
          (list 57 321 88))
