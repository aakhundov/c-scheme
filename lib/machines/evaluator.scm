start
    (assign continue (label end))

eval-dispatch
    (test (op self-evaluating?) (reg exp))
    (branch (label ev-self-eval))
    (test (op variable?) (reg exp))
    (branch (label ev-variable))
    (test (op quoted?) (reg exp))
    (branch (label ev-quoted))
    (test (op assignment?) (reg exp))
    (branch (label ev-assignment))
    (test (op definition?) (reg exp))
    (branch (label ev-definition))
    (test (op if?) (reg exp))
    (branch (label ev-if))
    (test (op lambda?) (reg exp))
    (branch (label ev-lambda))
    (test (op let?) (reg exp))
    (branch (label ev-let))
    (test (op begin?) (reg exp))
    (branch (label ev-begin))
    (test (op cond?) (reg exp))
    (branch (label ev-cond))
    (test (op eval?) (reg exp))
    (branch (label ev-eval))
    (test (op apply?) (reg exp))
    (branch (label ev-apply))
    (test (op application?) (reg exp))
    (branch (label ev-application))
    (perform (op signal-error) (const "can't evaluate %s") (reg exp))

ev-self-eval
    (assign val (reg exp))
    (goto (reg continue))

ev-variable
    (assign val (op lookup-variable-value) (reg exp) (reg env))
    (goto (reg continue))

ev-quoted
    (assign val (op text-of-quotation) (reg exp))
    (goto (reg continue))

ev-assignment
    (assign unev (op assignment-variable) (reg exp))
    (save unev)
    (assign exp (op assignment-value) (reg exp))
    (save env)
    (save continue)
    (assign continue (label ev-assignment-did-value))
    (goto (label eval-dispatch))
ev-assignment-did-value
    (restore continue)
    (restore env)
    (restore unev)
    (assign val (op set-variable-value!) (reg unev) (reg val) (reg env))
    (goto (reg continue))

ev-definition
    (assign unev (op definition-variable) (reg exp))
    (save unev)
    (assign exp (op definition-value) (reg exp))
    (save env)
    (save continue)
    (assign continue (label ev-definition-did-value))
    (goto (label eval-dispatch))
ev-definition-did-value
    (restore continue)
    (restore env)
    (restore unev)
    (assign val (op define-variable!) (reg unev) (reg val) (reg env))
    (goto (reg continue))

ev-if
    (save exp)
    (save env)
    (save continue)
    (assign continue (label ev-if-decide))
    (assign exp (op if-predicate) (reg exp))
    (goto (label eval-dispatch))
ev-if-decide
    (restore continue)
    (restore env)
    (restore exp)
    (test (op true?) (reg val))
    (branch (label ev-if-consequent))
ev-if-alternative
    (assign exp (op if-alternative) (reg exp))
    (goto (label eval-dispatch))
ev-if-consequent
    (assign exp (op if-consequent) (reg exp))
    (goto (label eval-dispatch))

ev-lambda
    (assign unev (op lambda-parameters) (reg exp))
    (assign exp (op lambda-body) (reg exp))
    (assign val (op make-compound-procedure) (reg unev) (reg exp) (reg env))
    (goto (reg continue))

ev-let
    (assign exp (op transform-let) (reg exp))
    (goto (label eval-dispatch))

ev-begin
    (assign unev (op begin-actions) (reg exp))
    (save continue)
    (goto (label ev-sequence))

ev-cond
    (assign exp (op transform-cond) (reg exp))
    (goto (label eval-dispatch))

ev-eval
    (save continue)
    (assign exp (op eval-expression) (reg exp))
    (assign continue (label ev-eval-did-once))
    (goto (label eval-dispatch))
ev-eval-did-once
    (restore continue)
    (assign exp (reg val))
    (goto (label eval-dispatch))

ev-apply
    (save continue)
    (save env)
    (assign unev (op apply-arguments) (reg exp))
    (save unev)
    (assign exp (op apply-operator) (reg exp))
    (assign continue (label ev-apply-did-operator))
    (goto (label eval-dispatch))
ev-apply-did-operator
    (restore unev)
    (restore env)
    (assign proc (reg val))
    (save proc)
    (assign exp (reg unev))
    (assign continue (label ev-apply-did-arguments))
    (goto (label eval-dispatch))
ev-apply-did-arguments
    (assign argl (reg val))
    (restore proc)
    (perform (op apply-verify) (reg argl))
    (goto (label apply-dispatch))

ev-application
    (save continue)
    (save env)
    (assign unev (op operands) (reg exp))
    (save unev)
    (assign exp (op operator) (reg exp))
    (assign continue (label ev-application-did-operator))
    (goto (label eval-dispatch))
ev-application-did-operator
    (restore unev)
    (restore env)
    (assign argl (op make-empty-arglist))
    (assign proc (reg val))
    (test (op no-operands?) (reg unev))
    (branch (label apply-dispatch))
    (save proc)
ev-application-operand-loop
    (save argl)
    (assign exp (op first-operand) (reg unev))
    (test (op last-operand?) (reg unev))
    (branch (label ev-application-last-arg))
    (save env)
    (save unev)
    (assign continue (label ev-application-accumulate-arg))
    (goto (label eval-dispatch))
ev-application-accumulate-arg
    (restore unev)
    (restore env)
    (restore argl)
    (assign argl (op adjoin-arg) (reg val) (reg argl))
    (assign unev (op rest-operands) (reg unev))
    (goto (label ev-application-operand-loop))
ev-application-last-arg
    (assign continue (label ev-application-accumulate-last-arg))
    (goto (label eval-dispatch))
ev-application-accumulate-last-arg
    (restore argl)
    (assign argl (op adjoin-arg) (reg val) (reg argl))
    (restore proc)
    (goto (label apply-dispatch))

apply-dispatch
    (test (op primitive-procedure?) (reg proc))
    (branch (label primitive-apply))
    (test (op compound-procedure?) (reg proc))
    (branch (label compound-apply))
    (perform (op signal-error) (const "can't apply %s") (reg proc))
primitive-apply
    (assign val (op apply-primitive-procedure) (reg proc) (reg argl))
    (restore continue)
    (goto (reg continue))
compound-apply
    (assign unev (op procedure-parameters) (reg proc))
    (assign env (op procedure-environment) (reg proc))
    (assign env (op extend-environment) (reg unev) (reg argl) (reg env))
    (assign unev (op procedure-body) (reg proc))
    (goto (label ev-sequence))

ev-sequence
    (assign exp (op first-exp) (reg unev))
    (test (op last-exp?) (reg unev))
    (branch (label ev-sequence-last-exp))
    (save unev)
    (save env)
    (assign continue (label ev-sequence-continue))
    (goto (label eval-dispatch))
ev-sequence-continue
    (restore env)
    (restore unev)
    (assign unev (op rest-exps) (reg unev))
    (goto (label ev-sequence))
ev-sequence-last-exp
    (restore continue)
    (goto (label eval-dispatch))

end
