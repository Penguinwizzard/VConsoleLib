A PoC library for working with the VConsole network protocol.

Basically, it's a stream of chunks. Every chunk has a 12-byte header; 4 characters for
the chunk type identifier, 4 bytes for 0x00d20000 (dunno what that's for), 2 bytes for
the chunk length (including the header), and 2 bytes of padding. The rest of the chunk
contents are specific to the type of chunk; look in the header file if you want to get
a more detailed picture of what's going on.

test.c is a sample file that uses the library; dump.c is a simple dumping utility that
can be used to figure out what the cause of some issues is when debugging the library.
