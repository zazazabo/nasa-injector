# Background

This project was created 6-7 months ago (summer of 2020), and was created before PTM and VDM. The code is not very clean, but it does the job. This
project does not map any drivers into the kernel, it does however use a vulnerable driver (any that exposes physical read/write). The project
is designed to inject a dll into a dxd11 process without actually allocating any memory in the process or in the kernel, it simply maps
already allocated memory into the kernel and thus no VAD entries are created. This means NtQueryVirtualMemory will not show the dll's memory.

# Key Features

- 0 bytes allocated in the kernel.
- 0 bytes allocated in the process.
- 0 VAD entries created inside of the game.

<img src="https://githacks.org/nasa-tech/nasa-injector/raw/c5694cf86f627b55196e9badb1fbb1357e9c89b5/unknown.png"/>

This project is old (about 6 months old or 6/xx/2020) and not well structured. It's been rendered obsolete since my new projects have been put together and extended this. I hope this project gives you some ideas for your own projects and you're able to make it far cleaner than this ever will be.

# Rendering Hook

This injector is intended to inject a module which has no imports, the injector is designed for dxd11 games only and will not work on any other type of
process. If you take a look at dxgi.dll present you can see that there is a EtwEventWrite call. nasa injector IAT hooks EtwEventWrite in dxgi.dll
to point to the internal.dll entry point.

<img src="https://imgur.com/n6BJhJj.png"/>

This makes it so when you stream the game in discord or OBS the rendering is stream proof because we draw after discord and OBS captures the screen.

<img src="https://imgur.com/dWgargZ.png"/>

# Detection

This project can easily be detected by checking for dxgi.dll IAT hooks on EtwEventWrite and stack walking of threads that execute EtwEventWrite. Inserting
a pml4e into a pml4 is also detected as the PFN database contains all of the PFNs for a specific process and if a new PML4E is inserted it will
be pointing at other processes page tables and thus other processes PFNs. This project also does not spoof return addresses so everytime the CPU executes the internal module it is leaving
return addresses on the stack which do not land inside of legit modules text sections. Simply APC's will catch this and BattlEye already does this. 

You can also construct a view of executable memory given the page tables of a process. If there is executable pages outside of a loaded modules .text sections
they can become apparent easily. Since the CPU is constantly executing the cheat it is unlikely that it will be paged to disk. 

### Credit
* [ChaosLeader](https://www.unknowncheats.me/forum/members/168188.html) - no_gui is a cleaned up version of this renderer
* [JustasMasiulis](https://github.com/JustasMasiulis/lazy_importer) - no_import.h is lazy importer