# LLVM sample of using
Based on official tutorial: https://llvm.org/docs/tutorial/

## Implementing a simplest language with LLVM
Example of code:
```
# Compute the x'th fibonacci number.
def fib(x)
  if x < 3 then
    1
  else
    fib(x-1)+fib(x-2)

# This expression will compute the 40th number.
fib(40)
```