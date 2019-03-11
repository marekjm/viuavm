# Signals

Signals are delievered to processes asynchronously, and force the receiving
process to handle them - if the process can handle the signal a new stack is
created for the handler and the execution switches to it; otherwise the process'
stack is unwound and the process is killed (exactly what would happen on an
unhandled exception).
