# cleric
Watch a process, respawn it on death. 

Silly little program, originally intended for an embedded system without a full SysVInit system. Turns out Busybox has runsv so this program won't be needed. But I learned a lot in the process.

Still needs some work.

TODO: 
	switch to SIGKILL if SIGTERM isn't getting the job done
	use syslog not printf
	watch multiple party members? Or one cleric per player? 

David Poole
20170320

