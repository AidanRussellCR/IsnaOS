# IsnaOS

Now known as IsnaOS, meaning Ice in the Thurlsk language.
I'd like to think it represents the slippery slope of beginning this project to understand kernels better and then becoming much more than that.

---

<img width="330" height="120" alt="IsnaOS_Logo" src="https://github.com/user-attachments/assets/0d99d753-5265-4a0d-a826-7f380b0fc4e7" />

## Current functionality

### Core System
- Basic VGA terminal with cursor
- Multitasking system
- Task switching via context switching
- Janus task/session manager

### Process/Scheduling
- Round-robin scheduler
- Multiprocess execution
- Focus-based input routing with Janus
- Wraith task cleanup system (reclaim memory from terminated processes)

### File System
- Simple virtual file system (VFS)
- File creation, reading, writing, movement, deletion

### Shell
- Interactive command shell
- Script execution system
- Command parsing and dispatch

### Applications

#### Scribe
- Modal text editor
- Cursor movement, editing
- File saving/loading

#### Sigildraw
- Simple bitmap editor
- Color palette (5 colors; black, white, red, green, blue)
- Keyboard-based drawing system

#### Glyph
- Display `.rune` bitmap files to the terminal

---

## Custom Toolchain

### Shape Assembler
- Custom assembler
- Fully documented in `Shape` folder
- Designed with a future compiler in mind

### Golem Executable Format
- Custom executable format `.glm`
- Built for the IsnaOS runtime
- Integrated with Shape assembler output

---

## Graphics/Bitmap System

### Rune Format
- Custom bitmap format `.rune`
- Indexed color (5 colors)
- Designed for terminal rendering with future UI in mind

### Glyphforge
- Bitmap rendering system
- Draws the image directly to terminal

---

## Short Term Goals
- Continue to improve and expand on the Shape Assembler
- Continue to improve and expand on the Golem executable format
- Improve Janus for cleaner focus switching and better task metadata

---

## Roadmap
- Scheduler improvement (Priority, MLQ, focus-aware)
- Processing improvements/additions (Fork, task lifecycle control, IPC)
- Filesystem improvements (file search, better metadata)
- UI/Rendering (UI bitmap integration)

---

## Long Term Goals

### Compiler
- Build a compiler that targets Shape
- Set up basis to port existing compilers on top of Shape

### Graphics
- Expand bitmap support
- Framebuffer support

### Audio
- Basic audio output support
- Hardware driver integration

### Networking
- Ethernet support
- Basic protocols (IP, UDP)
- File transfer between systems

---

## Naming Conventions

IsnaOS takes a lot of inspiration from fantasy and mythology, and thus has a consistent theme internally.

- **Janus** — Task/session management
- **Wraith** — Task cleanup
- **Scribe** — Modal text editor
- **Sigildraw** — Bitmap editor
- **Glyphforge** — Rendering system
- **Rune** — Bitmap format
- **Shape** — Assembler
- **Golem** — Executable format

Other naming conventions include **Magic Spells** (shell scripts), which are *learned* (marked as executable) and then *cast* (executed).

---

Come along to Myrkthrima, and witness the evolution of IsnaOS!
