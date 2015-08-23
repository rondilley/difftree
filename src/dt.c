/*****
 *
 * Copyright (c) 2010-2014, Ron Dilley
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *   - Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   - Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   - Neither the name of Uberadmin/BaraCUDA/Nightingale nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *****/

/****
 *
 * includes
 *.
 ****/

#include <stdio.h>
#include <stdlib.h>

#include "dt.h"

/****
 *
 * local variables
 *
 ****/

/****
 *
 * global variables
 *
 ****/

PUBLIC int quit = FALSE;
PUBLIC int reload = FALSE;
PUBLIC Config_t *config = NULL;
PUBLIC int baseDirLen;
PUBLIC int compDirLen;
PUBLIC char *baseDir;
PUBLIC char *compDir;

/* hashes */
struct hash_s *baseDirHash = NULL;
struct hash_s *compDirHash = NULL;

/****
 *
 * external variables
 *
 ****/

extern int errno;
extern char **environ;

/****
 *
 * main function
 *
 ****/

int main(int argc, char *argv[]) {
  PRIVATE int pid = 0;
  PRIVATE int c = 0, i = 0, fds = 0, status = 0;
  int digit_optind = 0;
  PRIVATE struct passwd *pwd_ent;
  PRIVATE struct group *grp_ent;
  PRIVATE char **ptr;
  char *tmp_ptr = NULL;
  char *pid_file = NULL;
  char *user = NULL;
  char *group = NULL;
#ifdef LINUX
  struct rlimit rlim;

  getrlimit( RLIMIT_CORE, &rlim );
#ifdef DEBUG
  rlim.rlim_cur = rlim.rlim_max;
  printf( "DEBUG - Core: %ld\n", rlim.rlim_cur );
#else
  rlim.rlim_cur = 0; 
#endif
  setrlimit( RLIMIT_CORE, &rlim );
#endif

  /* setup config */
  config = ( Config_t * )XMALLOC( sizeof( Config_t ) );
  XMEMSET( config, 0, sizeof( Config_t ) );

  while (1) {
    int this_option_optind = optind ? optind : 1;
#ifdef HAVE_GETOPT_LONG
    int option_index = 0;
    static struct option long_options[] = {
      {"debug", required_argument, 0, 'd' },
      {"help", no_argument, 0, 'h' },
      {"logdir", required_argument, 0, 'l' },
      {"md5", no_argument, 0, 'm' },
      {"quick", no_argument, 0, 'q' },
      {"version", no_argument, 0, 'v' },
      {"write", required_argument, 0, 'w' },
      {0, no_argument, 0, 0}
    };
    c = getopt_long(argc, argv, "d:hl:mqvw:", long_options, &option_index);
#else
    c = getopt( argc, argv, "d:hl:mqvw:" );
#endif

    if (c EQ -1)
      break;

    switch (c) {

    case 'v':
      /* show the version */
      print_version();
      return( EXIT_SUCCESS );

    case 'd':
      /* show debig info */
      config->debug = atoi( optarg );
      config->mode = MODE_INTERACTIVE;
      break;

    case 'h':
      /* show help info */
      print_help();
      return( EXIT_SUCCESS );

    case 'l':
      /* define the dir to store logs in */
      config->log_dir = ( char * )XMALLOC( MAXPATHLEN + 1 );
      XMEMSET( config->log_dir, 0, MAXPATHLEN + 1 );
      XSTRNCPY( config->log_dir, optarg, MAXPATHLEN );
      break;

    case 'w':
      /* define the dir to store logs in */
      config->outfile = ( char * )XMALLOC( MAXPATHLEN + 1 );
      XMEMSET( config->outfile, 0, MAXPATHLEN + 1 );
      XSTRNCPY( config->outfile, optarg, MAXPATHLEN );
      break;

    case 'm':
      /* md5 hash files */
      config->hash = TRUE;
      break;

    case 'q':
      /* do quick checks only */
      config->quick = TRUE;
      break;

    default:
      fprintf( stderr, "Unknown option code [0%o]\n", c);
    }
  }

  /* set default options */
  if ( config->log_dir EQ NULL ) {
    config->log_dir = ( char * )XMALLOC( strlen( LOGDIR ) + 1 );
    XSTRNCPY( config->log_dir, LOGDIR, strlen( LOGDIR ) );   
  }

  /* turn off quick mode if hash mode is enabled */
  if ( config->hash )
    config->quick = FALSE;

  /* enable syslog */
#ifdef HAVE_OPENLOG
  openlog( PROGNAME, LOG_CONS & LOG_PID, LOG_LOCAL0 );
#endif

  /* check dirs and files for danger */

  if ( time( &config->current_time ) EQ -1 ) {
    fprintf( stderr, "ERR - Unable to get current time\n" );
#ifdef HAVE_CLOSELOG
    /* cleanup syslog */
    closelog();
#endif
    /* cleanup buffers */
    cleanup();
    return EXIT_FAILURE;
  }

  /* initialize program wide config options */
  config->hostname = (char *)XMALLOC( MAXHOSTNAMELEN+1 );

  /* get processor hostname */
  if ( gethostname( config->hostname, MAXHOSTNAMELEN ) != 0 ) {
    fprintf( stderr, "Unable to get hostname\n" );
    strncpy( config->hostname, "unknown", MAXHOSTNAMELEN );
  }

  /* setup gracefull shutdown */
  signal( SIGINT, sigint_handler );
  signal( SIGTERM, sigterm_handler );
  signal( SIGFPE, sigfpe_handler );
  signal( SIGILL, sigill_handler );
  signal( SIGSEGV, sigsegv_handler );
#ifndef MINGW
  signal( SIGHUP, sighup_handler );
  signal( SIGBUS, sigbus_handler );
#endif  

  /****
   *
   * lets get this party started
   *
   ****/

  show_info();
  if ( ( baseDir = (char *)XMALLOC( PATH_MAX ) ) EQ NULL ) {
    fprintf( stderr, "ERR - Unable to allocate memory for baseDir string\n" );
    cleanup();
    return( EXIT_FAILURE );
  }
  if ( ( compDir = (char *)XMALLOC( PATH_MAX ) ) EQ NULL ) {
    fprintf( stderr, "ERR - Unable to allocate memory for compDir string\n" );
    cleanup();
    return( EXIT_FAILURE );
  }

  compDirHash = initHash( 52 );

  while (optind < argc ) {
    if ( ( compDirLen = strlen( argv[optind] ) ) >= PATH_MAX ) {
      fprintf( stderr, "ERR - Argument too long\n" );
      if ( baseDirHash != NULL )
	freeHash( baseDirHash );
      freeHash( compDirHash );
      cleanup();
      return( EXIT_FAILURE );
    } else {
      strncpy( compDir, argv[optind++], PATH_MAX-1 );
      /* process directory tree */
      if ( processDir( compDir ) EQ FAILED ) {
	if ( baseDirHash != NULL  )
	  freeHash( baseDirHash );
	freeHash( compDirHash );
	cleanup();
	return( EXIT_FAILURE );
      }

      if ( baseDirHash != NULL ) {
	/* compare the old tree to the new tree to find missing files */
	if ( traverseHash( baseDirHash, findMissingFiles ) != TRUE ) {
	  freeHash( baseDirHash );
	  freeHash( compDirHash );
	  cleanup();
	  return( EXIT_FAILURE );
	}
      }

      /* Prep for next dir to compare */
      if ( baseDirHash != NULL )
	freeHash( baseDirHash );
      baseDirHash = compDirHash;
      compDirHash = initHash( getHashSize( baseDirHash ) );
      baseDirLen = compDirLen;
      strncpy( baseDir, compDir, compDirLen );
    }
  }

  if ( baseDirHash != NULL )
    freeHash( baseDirHash );
  if ( compDirHash != NULL )
    freeHash( compDirHash );

  /****
   *
   * we are done
   *
   ****/

  /* cleanup syslog */
  closelog();

  cleanup();

  return( EXIT_SUCCESS );
}

/****
 *
 * display prog info
 *
 ****/

void show_info( void ) {
  fprintf( stderr, "%s v%s [%s - %s]\n", PROGNAME, VERSION, __DATE__, __TIME__ );
  fprintf( stderr, "By: Ron Dilley\n" );
  fprintf( stderr, "\n" );
  fprintf( stderr, "%s comes with ABSOLUTELY NO WARRANTY.\n", PROGNAME );
  fprintf( stderr, "This is free software, and you are welcome\n" );
  fprintf( stderr, "to redistribute it under certain conditions;\n" );
  fprintf( stderr, "See the GNU General Public License for details.\n" );
  fprintf( stderr, "\n" );
}

/*****
 *
 * display version info
 *
 *****/

PRIVATE void print_version( void ) {
  printf( "%s v%s [%s - %s]\n", PROGNAME, VERSION, __DATE__, __TIME__ );
}

/*****
 *
 * print help info
 *
 *****/

PRIVATE void print_help( void ) {
  print_version();

  fprintf( stderr, "\n" );
  fprintf( stderr, "syntax: %s [options] {dir}|{file} [{dir} ...]\n", PACKAGE );

#ifdef HAVE_GETOPT_LONG
  fprintf( stderr, " -d|--debug (0-9)     enable debugging info\n" );
  fprintf( stderr, " -h|--help            this info\n" );
  fprintf( stderr, " -l|--logdir {dir}    directory to create logs in (default: %s)\n", LOGDIR );
  fprintf( stderr, " -m|--md5             hash files and compare (disables -q|--quick mode)\n" );
  fprintf( stderr, " -q|--quick           do quick comparisons only\n" );
  fprintf( stderr, " -v|--version         display version information\n" );
  fprintf( stderr, " -w|--write {file}    write directory tree to file\n" );
#else
  fprintf( stderr, " -d {lvl}   enable debugging info\n" );
  fprintf( stderr, " -h         this info\n" );
  fprintf( stderr, " -l {dir}   directory to create logs in (default: %s)\n", LOGDIR );
  fprintf( stderr, " -m         hash files and compare (disables -q|--quick mode)\n" );
  fprintf( stderr, " -q         do quick comparisons only\n" );
  fprintf( stderr, " -v         display version information\n" );
  fprintf( stderr, " -w {file}  write directory tree to file\n" );
#endif

  fprintf( stderr, "\n" );
}

/****
 *
 * cleanup
 *
 ****/

PRIVATE void cleanup( void ) {
  if ( baseDir != NULL )
    XFREE( baseDir );
  if ( compDir != NULL )
    XFREE( compDir );
  XFREE( config->hostname );
  if ( config->log_dir != NULL )
    XFREE( config->log_dir );
  if ( config->home_dir != NULL )
    XFREE( config->home_dir );
  if ( config->outfile != NULL )
    XFREE( config->outfile );
  XFREE( config );
#ifdef MEM_DEBUG
  XFREE_ALL();
#endif
}

/****
 *
 * SIGINT handler
 *
 ****/

void sigint_handler( int signo ) {
  signal( signo, SIG_IGN );

  /* do a calm shutdown as time and pcap_loop permit */
  quit = TRUE;
  signal( signo, sigint_handler );
}

/****
 *
 * SIGTERM handler
 *
 ****/

void sigterm_handler( int signo ) {
  signal( signo, SIG_IGN );

  /* do a calm shutdown as time and pcap_loop permit */
  quit = TRUE;
  signal( signo, sigterm_handler );
}

/****
 *
 * SIGHUP handler
 *
 ****/

#ifndef MINGW
void sighup_handler( int signo ) {
  signal( signo, SIG_IGN );

  /* time to rotate logs and check the config */
  reload = TRUE;
  signal( SIGHUP, sighup_handler );
}
#endif

/****
 *
 * SIGSEGV handler
 *
 ****/

void sigsegv_handler( int signo ) {
  signal( signo, SIG_IGN );

  fprintf( stderr, "ERR - Caught a sig%d, shutting down fast\n", signo );

  cleanup();
#ifdef MEM_DEBUG
  XFREE_ALL();
#endif
  /* core out */
  abort();
}

/****
 *
 * SIGBUS handler
 *
 ****/

void sigbus_handler( int signo ) {
  signal( signo, SIG_IGN );

  fprintf( stderr, "ERR - Caught a sig%d, shutting down fast\n", signo );

  cleanup();
#ifdef MEM_DEBUG
  XFREE_ALL();
#endif
  /* core out */
  abort();
}

/****
 *
 * SIGILL handler
 *
 ****/

void sigill_handler ( int signo ) {
  signal( signo, SIG_IGN );

  fprintf( stderr, "ERR - Caught a sig%d, shutting down fast\n", signo );

  cleanup();
#ifdef MEM_DEBUG
  XFREE_ALL();
#endif
  /* core out */
  abort();
}

/****
 *
 * SIGFPE handler
 *
 ****/

void sigfpe_handler( int signo ) {
  signal( signo, SIG_IGN );

  fprintf( stderr, "ERR - Caught a sig%d, shutting down fast\n", signo );

  cleanup();
#ifdef MEM_DEBUG
  XFREE_ALL();
#endif
  /* core out */
  abort();
}
