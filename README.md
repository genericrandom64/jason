# jason

WIP NES emulator and general-purpose 6502 emulator.

## name?

![lol](img/name.png)

## could i potentially use this in my own project?

j65, probably; it's pretty simple to interface with, even without a libc. jason, probably not unless you're making something NES related.

expect documentation and a tutorial of making a j65 client soon.

## how build? very exciting to see the colors and numbers in all of place this year the soap mario and other all vincent game

no

## how compile? see funny number with letters in it over and over until go completely insane

building j65 more or less consists of just these 2 commands.

```
make depend
make -j1
```

j65 is primarily tested on:
- arch linux (amd64, gcc 11.1.0)
- openbsd (macppc, clang 10.0.1)

it *should,* however, work on any 32 bit operating system with a working c compiler.

there are some tests in `tests/` but they do not cover the majority of opcodes yet.

## website?

[stat page (youll need tor for this)](http://7u5uyjitajhhbzlhgxz5p2h5lj3mvfb6bralcuzkcgqgkrnlah37yiad.onion/computer/opcodes)

## references

[6502 Reference](http://www.obelisk.me.uk/6502/reference.html)

[Visual 6502](http://www.visual6502.org/JSSim/expert.html)

[6502 Instruction Set](https://www.masswerk.at/6502/6502_instruction_set.html)

[6502 Disassembler](https://www.masswerk.at/6502/disassembler.html)

## license?

The entire emulator is public domain under the Unlicense.
