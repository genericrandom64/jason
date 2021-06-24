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

`util/j65-monitor.out` will let you test binaries with j65. it will load them with entry point 0x00, so you will have to jump out of zeropage yourself. it provides basic machine code debugging tools, like register state and steps. it does not have a disassembler, machine code viewer, or breakpoints. *yet.*

there are some tests in `tests/` but they do not cover the majority of opcodes yet.

## website?

these links require tor, but if you're here you probably already use some tor client.

[stat page](http://7u5uyjitajhhbzlhgxz5p2h5lj3mvfb6bralcuzkcgqgkrnlah37yiad.onion/computer/opcodes)
[future project site will be here](http://7u5uyjitajhhbzlhgxz5p2h5lj3mvfb6bralcuzkcgqgkrnlah37yiad.onion/computer/j65)

## references

[6502 Reference](http://www.obelisk.me.uk/6502/reference.html)

[Visual 6502](http://www.visual6502.org/JSSim/expert.html)

[6502 Instruction Set](https://www.masswerk.at/6502/6502_instruction_set.html)

[6502 Disassembler](https://www.masswerk.at/6502/disassembler.html)

## license?

The entire emulator is public domain under the Unlicense.
