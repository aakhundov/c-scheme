start
    (assign continue (label end))
fib-loop
    (branch (label immediate-answer) (op <) (reg n) (const 2))
    (save continue)
    (assign continue (label afterfib-n-1))
    (save n)
    (assign n (op -) (reg n) (const 1))
    (goto (label fib-loop))
afterfib-n-1
    (restore n)
    (restore continue)
    (assign n (op -) (reg n) (const 2))
    (save continue)
    (assign continue (label afterfib-n-2))
    (save val)
    (goto (label fib-loop))
afterfib-n-2
    (assign n (reg val))
    (restore val)
    (restore continue)
    (assign val
    (op +) (reg val) (reg n))
    (goto (reg continue))
immediate-answer
    (assign val (reg n))
    (goto (reg continue))
end
