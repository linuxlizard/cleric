/*  Watch a subprocess, respawn it upon its death.
 *
 * Originally created to monitor a service in a small embedded Linux system
 * without a SysVInit and where /etc/inittab wouldn't have worked for
 * complicated reasons.
 *
 * /usr/bin/cleric /sbin/importantdaemond   # if importantdaemond crashes, restart it.
 * 
 * XXX still needs some work.
 *
 * davep 20170320;  
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>

static sig_atomic_t quit;

/* TODO switch to syslog */
#define info printf
#define err printf

struct party_member {
	pid_t pid;
	const char *path;
	char ** argv;

	uintmax_t raise_count;
	time_t birth; /* time of (re)birth */
};

static void signal_term( int signum )
{
	info("%s caught signal signum=%d\n", __func__, signum);
	quit = 1;
}

void init_signals( void )
{
	int retcode;
	struct sigaction sig;

	memset( &sig, 0, sizeof(struct sigaction) );
	sig.sa_handler = signal_term;
	retcode = sigaction( SIGTERM, &sig, NULL );
	if (retcode) {
		perror( "sigaction(SIGTERM)" );
		exit(1);
	}

	memset( &sig, 0, sizeof(struct sigaction) );
	sig.sa_handler = signal_term;
	retcode = sigaction( SIGINT, &sig, NULL );
	if (retcode) {
		perror( "sigaction(SIGINT)" );
		exit(1);
	}

}

/* http://www.d20srd.org/srd/spells/raiseDead.htm
 *
 * Material Component
 *    Diamonds worth a total of least 5,000 gp. 
 *
 *
 * Return pid_t of the party member raised from the dead.
 */

pid_t raise_dead(struct party_member *p)
{
	int retcode;

	/* TODO check for respawning too fast (costs too many diamonds!) */

	info("%s path=%s\n", __func__, p->path);

	/* arise! */
	pid_t child_pid = fork();
	if (child_pid == 0) {
		/* child */
		retcode = execv(p->path, p->argv);
		if (!retcode) {
			perror("execv");
			return -1;
		}
		/* never get here */
	}
	else if (child_pid < 0) {
		perror("fork");
		return -1;
	}

	return child_pid;
}

int fratricide(struct party_member *p)
{
	int status;
	int retcode;

	/* kill off a party member */
	while (1) {
		/* TODO after a few attempts using SIGTERM with no results, bring down
		 * the SIGKILL hammer 
		 */
		retcode = kill(p->pid, SIGTERM);
		if (retcode < 0) {
			perror("kill");
			return retcode;
		}
		/* stand over the corpse and verify death (if Obi Wan had done
		 * this, it would have saved a lot of people a lot of trouble) 
		 */
		retcode = waitpid(p->pid, &status, WNOHANG);
		if (retcode < 0){
			perror("waitpid");
			return retcode;
		}
		printf("waiting for %s pid=%d to die\n", p->path, p->pid);
		sleep(1);
	}

}

int watch_loop(struct party_member *p)
{
	int status;
	pid_t retpid;
	
	while (1) {
		retpid = waitpid(p->pid, &status, 0);
		if (retpid < 0) {
			if (errno == EINTR) {
				/* interrupted system call */
				if (quit) {
					fratricide(p);
					return 0;
				}
				/* more? */
			}

			perror("waitpid");
			return -1;
		}
		printf("child=%d finished status=%#x\n", p->pid, status);

		time_t death = time(NULL);
		if (difftime(death, p->birth) < 60 && p->raise_count > 6) {
			printf("Player dying too often. Giving up. LTP, noob.\n");
			return -1;
		}

		/* from the man page */
		if (WIFEXITED(status)) {
			printf("exited, status=%d\n", WEXITSTATUS(status));
		} else if (WIFSIGNALED(status)) {
			printf("killed by signal %d\n", WTERMSIG(status));
		} else if (WIFSTOPPED(status)) {
			printf("stopped by signal %d\n", WSTOPSIG(status));
		} else if (WIFCONTINUED(status)) {
			printf("continued\n");
		}

		/* a party member has died! raise dead */
		retpid = raise_dead(p);
		if (retpid < 0) {
			/* failed to raise! */
			return -1;
		}
		else {
			/* New soul? Let's leave that to the theologians. Wait. I'm a cleric. 
			 * I am a theologian.
			 */
			p->pid = retpid;
			p->birth = time(NULL);
			p->raise_count++;
			printf("%s raised as pid=%d\n", p->path, p->pid);
		}

	}

}

int main(int argc, char *argv[])
{
	pid_t child_pid;
	int retcode;

	if (argc < 2) {
		/* TODO usage */
		return EXIT_FAILURE;
	}

	init_signals();

	child_pid = fork();
	if (child_pid == 0) {
		/* child */
		retcode = execv(argv[1], &argv[1]);
		if (!retcode) {
			perror("execv");
			return EXIT_FAILURE;
		}
		/* should never get here */
	}
	else if (child_pid < 0) {
		perror("fork");
		return EXIT_FAILURE;
	}
	else {
		/* parent */
		printf("child pid=%d\n", child_pid);

		struct party_member member;
		memset( &member, 0, sizeof(struct party_member));
		member.path = argv[1];
		member.argv = &argv[1];
		member.pid = child_pid;

		/* keep an eye on the meat shield, will you? */
		watch_loop(&member);
	}

	return EXIT_SUCCESS;
}

