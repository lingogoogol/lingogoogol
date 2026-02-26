global    _start
extern GetStdHandle

          section   .text
_start:   
          jmp       [GetStdHandle]