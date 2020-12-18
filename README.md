# nasa-injector

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
be pointing at other processes PFNs. This project also doesnt not spoof return addresses so everything the CPU executes the internal module it is leaving
return addresses on the stack. 