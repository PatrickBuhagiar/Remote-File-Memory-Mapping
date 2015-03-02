The aim of this assignment is to implement an API which provides a number of calls allowing processes to memory map sections of a file located on a remote machine. Processes can then read and write to this memory area using the provided calls, with changes being propagated to the remote ﬁle by the library. Multiple processes from multiple client machines should be able to access and modify the same ﬁle on a remote machine.