j-nasm
=========
A stupid preprocessor to NASM. <br>
It will help you with the crazy control flow of Assembly x86.<br>

## Usage
### How to
1. Download the source of j-nasm.
2. Compile it with GCC: `$ gcc j-nasm.c -o j-nasm`
3. Be sure your source has `_temp: dq 0` declared
4. To preprocess, you should give a file as argument: `$ ./j-nasm path/to/file`
5. Then use your NASM compiler

### Conditions
They should appear with a `#if`, `#elseif`, `#while` or `#dowhile`<br>
Supports true and false constants:
```
; spaces as shown
#while true
; nasm code
#end
```
Supports literals, regiters and memory adresses:
```
; operator has to be separated by spaces
#dowhile eax >= 90
; nasm code
#end
#while dword[ecx] < eax
; nasm code
#end
```
### Supported tags
* `#break`
* `#continue`
* `#dowhile`
* `#else`
* `#elseif`
* `#end`
* `#function`
* `#if`
* `#loop`
* `#>`
* `#return`
* `#while`

### Supported comparison operators
* `>=`
* `>`
* `==`
* `!=`
* `<`
* `<=`

### `#while`, `#dowhile`
Up to 30 nested loops: _( enough to almost anything )_
```
#while true
  #dowhile 3 < eax
  ; code
  #end
#end
```
### `#loop`
Does not support nested loops <br>
Receives only one parameter, which will be moved to the `cx` register <br>
and will be decremented at every iteration until it hits `0`
```
; to put a literal, it is needed to specify the size to dword
#loop dword 50
; nasm code
#end
; for registers it is very simple
#loop eax
#end
```
If you really want to use the `ecx` and still have your loop
```
; just push and pop it
#loop dword 5
push ecx
; nasm code
pop ecx
#end
```
### `#break`, `#continue`
You can put any of these anywhere inside a loop or even a nested loop,
it will change the flow of the closest loop
```
#while true
  #if true
    #break
  #else
    #continue
  #end
#end
```
### `#if`, `#elseif`, `#else`
You can put it anywhere in your code,
but it is needed at least an `#if` to use the others
```
#dowhile 43 < 2
  #if false
    #if true
      ; code
    #end
  #elseif byte[eax] == 2
    ; code
  #else
    ; code
  #end
#end
```
### `#function`, `#return`, `#>`
Cannot nest `#function` and it has to be declared before its use,
so it's highly advised to declare it at the beginning of the code
```
; after the name of it, you tell the number of arguments
#function foo 2: eax, ebx
  add eax, ebx
  #return eax
#end
; later...
foo 32, 2
#> eax ; eax = 34
```
You are perfectly able to make recursion, ~~you just don't have the best stack~~
```
; receives 1 argument and puts it in eax
#function fatorial 1: eax
  #if eax <= 1
    #return dword 1
  #end
  dec eax
  fatorial eax
  #> ebx
  inc eax
  imul eax, ebx
  #return eax
#end
```
