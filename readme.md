camellia
========
This is a small experimental kernel focused on some ideas I've had about privilege separation. I haven't written much about them yet, but I'll probably do that soon(ish).

main goals
----------
* Small, understandable, auditable. The kernel shouldn't include anything which isn't absolutely needed to implement those ideas. I'm not focusing on this too much atm, since it's still in early stages, but I'll probably spend a lot of time later slimming it down.
* Stable syscall API, easy to implement by other people. There isn't much needed to implement those ideas, and being able to choose what exact kernel you want to use would be pretty nice.
* Processes can always reduce their access to resources, but can *never* escalate it back. This includes stuff like setuid or whatver.
* Easy to use access control on all scales. It should be just as easy to disallow a program access to /home, as to disallow access to every file containing swear words, as to disallow access to the internet based on some packet filter. All of those would use the same exact API.

I'm bad at explaining stuff, and I know that those look very generic, but I already have most of this planned out.
