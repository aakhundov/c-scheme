start
    (assign continue (label end))
fact-loop
    (branch (label base-case) (op =) (reg n) (const 1))
    (save continue)
    (save n)
    (assign n (op -) (reg n) (const 1))
    (assign continue (label after-fact))
    (goto (label fact-loop))
after-fact
    (restore n)
    (restore continue)
    (assign val (op *) (reg n) (reg val))
    (goto (reg continue))
base-case
    (assign val (const 1))
    (goto (reg continue))
end
