/*****
 *
 * Description: Difftree Functions
 *
 * Copyright (c) 2010-2022, Ron Dilley
 * All rights reserved.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 ****/

/****
 *
 * includes
 *
 ****/

#include "dt.h"
#include "mem.h"

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

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

int main(int argc, char *argv[])
{
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
  char **tmpExclPtr = NULL;

#ifdef LINUX
  struct rlimit rlim;

  getrlimit(RLIMIT_CORE, &rlim);
#ifdef DEBUG
  rlim.rlim_cur = rlim.rlim_max;
  printf("DEBUG - RLIMIT_CORE: %ld\n", rlim.rlim_cur);
#else
  rlim.rlim_cur = 0;
#endif
  setrlimit(RLIMIT_CORE, &rlim);
#endif

  /* setup config */
  config = (Config_t *)XMALLOC(sizeof(Config_t));
  XMEMSET(config, 0, sizeof(Config_t));

  /* get real uid and gid, we may want to drop privs */
  config->gid = getgid();
  config->uid = getuid();

  while (1)
  {
    int this_option_optind = optind ? optind : 1;
#ifdef HAVE_GETOPT_LONG
    int option_index = 0;
    static struct option long_options[] = {
        {"atime", no_argument, 0, 'a'},
        {"count", no_argument, 0, 'c'},
        {"debug", required_argument, 0, 'd'},
        {"exdir", required_argument, 0, 'e'},
        {"exfile", required_argument, 0, 'E'},
        {"help", no_argument, 0, 'h'},
        {"md5", no_argument, 0, 'm'},
        {"preserve", no_argument, 0, 'p'},
        {"quick", no_argument, 0, 'q'},
        {"sha256", no_argument, 0, 's'},
        {"version", no_argument, 0, 'v'},
        {"write", required_argument, 0, 'w'},
        {0, no_argument, 0, 0}};
    c = getopt_long(argc, argv, "acd:e:E:hmpqsvw:", long_options, &option_index);
#else
    c = getopt(argc, argv, "acd:e:E:hmpqsvw:");
#endif

    if (c EQ - 1)
      break;

    switch (c)
    {
    case 'a':
      /* enable atime change reporting */
      config->show_atime = TRUE;

    case 'p':
      if (config->uid != 0)
      {
        fprintf(stderr,
                "ERR - Insufficient priviledges to preserve ATIME, aborting\n");
        print_help();
        return (EXIT_FAILURE);
      }
      config->preserve_atime = TRUE;

      break;

    case 'c':
      /* count lines and bytes */
      config->count = TRUE;
      config->hash = FALSE;
      config->md5_hash = FALSE;
      config->sha256_hash = FALSE;
      break;

    case 'd':
      /* show debig info */
      config->debug = atoi(optarg);
      config->mode = MODE_INTERACTIVE;
      break;

    case 'e':
      /* exclude a specific directory from the diff */
      if (config->exclusions EQ NULL)
      {
        if ((config->exclusions = (char **)XMALLOC(sizeof(char *) * 2)) EQ NULL)
        {
          fprintf(stderr, "ERR - Unable to allocate memory for exclusion list\n");
          return (EXIT_FAILURE);
        }
        XMEMSET(config->exclusions, 0, sizeof(char *) * 2);
      }

      for (i = 0; config->exclusions[i] != NULL; i++)
      {
#ifdef DEBUG
        if (config->debug >= 5)
          printf("DEBUG - Exclusion [%s]\n", config->exclusions[i]);
#endif
      }

      if ((config->exclusions[i] = XMALLOC(MAXPATHLEN + 1)) EQ NULL)
      {
        fprintf(stderr, "ERR - Unable to allocate memory for exclusion entry\n");
        while (--i != 0)
          XFREE(config->exclusions[i]);
        XFREE(config->exclusions);
        return (EXIT_FAILURE);
      }
      if ((tmpExclPtr = XREALLOC(config->exclusions, sizeof(char *) * (i + 2))) EQ NULL)
      {
        fprintf(stderr, "ERR - Unable to allocate memory for exclusion list\n");
        while (--i != 0)
          XFREE(config->exclusions[i]);
        XFREE(config->exclusions);
        return (EXIT_FAILURE);
      }
      config->exclusions = tmpExclPtr;
      XSTRNCPY(config->exclusions[i], optarg, MAXPATHLEN);
      config->exclusions[i + 1] = NULL;
      break;

    case 'E':
      /* exclude a list of directories in the specific file */
      if (loadExclusions(optarg) != TRUE)
        return (EXIT_FAILURE);
      break;

    case 'h':
      /* show help info */
      print_help();
      return (EXIT_SUCCESS);

    case 'm':
      /* md5 hash files */
      config->hash = TRUE;
      config->md5_hash = TRUE;
      config->digest_size = 16;
      config->sha256_hash = FALSE;
      break;

    case 'q':
      /* do quick checks only */
      config->quick = TRUE;
      break;

    case 's':
      /* sha256 hash files */
      config->hash = TRUE;
      config->sha256_hash = TRUE;
      config->digest_size = 32;
      config->md5_hash = FALSE;
      break;

    case 'v':
      /* show the version */
      print_version();
      return (EXIT_SUCCESS);

    case 'w':
      /* define the dir to store logs in */
      if ((config->outfile = (char *)XMALLOC(MAXPATHLEN + 1)) EQ NULL)
      {
        /* XXX problem */
      }
      XMEMSET(config->outfile, 0, MAXPATHLEN + 1);
      strncpy(config->outfile, optarg, MAXPATHLEN);
      break;

    default:
      fprintf(stderr, "Unknown option code [0%o]\n", c);
    }
  }

  /* turn off quick mode if hash mode is enabled */
  if (config->hash)
    config->quick = FALSE;

  /* check dirs and files for danger */

  if (time(&config->current_time) EQ - 1)
  {
    fprintf(stderr, "ERR - Unable to get current time\n");

    /* cleanup buffers */
    cleanup();
    return EXIT_FAILURE;
  }

  /* initialize program wide config options */
  config->hostname = (char *)XMALLOC(MAXHOSTNAMELEN + 1);

  /* get processor hostname */
  if (gethostname(config->hostname, MAXHOSTNAMELEN) != 0)
  {
    fprintf(stderr, "Unable to get hostname\n");
    strncpy(config->hostname, "unknown", MAXHOSTNAMELEN);
  }

  /* setup gracefull shutdown */
  signal(SIGINT, sigint_handler);
  signal(SIGTERM, sigterm_handler);
  signal(SIGFPE, sigfpe_handler);
  signal(SIGILL, sigill_handler);
  signal(SIGSEGV, sigsegv_handler);
#ifndef MINGW
  signal(SIGHUP, sighup_handler);
  signal(SIGBUS, sigbus_handler);
#endif

  /****
   *
   * lets get this party started
   *
   ****/

#ifdef DEBUG
  show_info();
#endif

  if ((baseDir = (char *)XMALLOC(PATH_MAX)) EQ NULL)
  {
    fprintf(stderr, "ERR - Unable to allocate memory for baseDir string\n");
    cleanup();
    return (EXIT_FAILURE);
  }
  if ((compDir = (char *)XMALLOC(PATH_MAX)) EQ NULL)
  {
    fprintf(stderr, "ERR - Unable to allocate memory for compDir string\n");
    cleanup();
    return (EXIT_FAILURE);
  }

  compDirHash = initHash(52);

  while (optind < argc)
  {
    if ((compDirLen = strlen(argv[optind])) >= PATH_MAX)
    {
      fprintf(stderr, "ERR - Argument too long\n");
      if (baseDirHash != NULL)
        freeHash(baseDirHash);
      freeHash(compDirHash);
      cleanup();
      return (EXIT_FAILURE);
    }
    else
    {
      strncpy(compDir, argv[optind++], PATH_MAX - 1);
      /* process directory tree */
      if (processDir(compDir) EQ FAILED)
      {
        if (baseDirHash != NULL)
          freeHash(baseDirHash);
        freeHash(compDirHash);
        cleanup();
        return (EXIT_FAILURE);
      }

      if (baseDirHash != NULL)
      {
        /* compare the old tree to the new tree to find missing files */
        if (traverseHash(baseDirHash, findMissingFiles) != TRUE)
        {
          freeHash(baseDirHash);
          freeHash(compDirHash);
          cleanup();
          return (EXIT_FAILURE);
        }
      }

      /* Prep for next dir to compare */
      if (baseDirHash != NULL)
        freeHash(baseDirHash);
      baseDirHash = compDirHash;
      compDirHash = initHash(getHashSize(baseDirHash));
      baseDirLen = compDirLen;
      strncpy(baseDir, compDir, compDirLen);
    }
  }

  if (baseDirHash != NULL)
    freeHash(baseDirHash);
  if (compDirHash != NULL)
    freeHash(compDirHash);

  /****
   *
   * we are done
   *
   ****/

  cleanup();

  return (EXIT_SUCCESS);
}

/****
 *
 * display prog info
 *
 ****/

void show_info(void)
{
  fprintf(stderr, "%s v%s [%s - %s]\n", PROGNAME, VERSION, __DATE__, __TIME__);
  fprintf(stderr, "By: Ron Dilley\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "%s comes with ABSOLUTELY NO WARRANTY.\n", PROGNAME);
  fprintf(stderr, "This is free software, and you are welcome\n");
  fprintf(stderr, "to redistribute it under certain conditions;\n");
  fprintf(stderr, "See the GNU General Public License for details.\n");
  fprintf(stderr, "\n");
}

/*****
 *
 * display version info
 *
 *****/

PRIVATE void print_version(void)
{
  printf("%s v%s [%s - %s]\n", PROGNAME, VERSION, __DATE__, __TIME__);
}

/*****
 *
 * print help info
 *
 *****/

PRIVATE void print_help(void)
{
  print_version();

  fprintf(stderr, "\n");
  fprintf(stderr, "syntax: %s [options] {dir}|{file} [{dir} ...]\n", PACKAGE);

#ifdef HAVE_GETOPT_LONG
  fprintf(stderr, " -a|--atime           show last access time changes (enables -p|--preserve)\n");
  fprintf(stderr, " -c|--count           count file lines and bytes (disables hash modes)\n");
  fprintf(stderr, " -d|--debug (0-9)     enable debugging info\n");
  fprintf(stderr, " -e|--exdir {dir}     exclude {dir}\n");
  fprintf(stderr, " -E|--exfile {file}   exclude directories listed in {file}\n");
  fprintf(stderr, " -h|--help            this info\n");
  fprintf(stderr, " -m|--md5             MD5 hash files and compare (disables -q|--quick and -s|--sha256 modes)\n");
  fprintf(stderr, " -p|--preserve        preserve ATIME when hashing files (must have appropriate priviledges)\n");
  fprintf(stderr, " -q|--quick           do quick comparisons only\n");
  fprintf(stderr, " -s|--sha256          SHA256 hash files and compare (disables -q|--quick and -m|--md5 modes)\n");
  fprintf(stderr, " -v|--version         display version information\n");
  fprintf(stderr, " -w|--write {file}    write directory tree to file\n");
#else
  fprintf(stderr, " -a         show last access time changes (enables -p)\n");
  fprintf(stderr, " -c         count file lines and bytes (disables hash modes)\n");
  fprintf(stderr, " -d {lvl}   enable debugging info\n");
  fprintf(stderr, " -e {dir}   exclude {dir}\n");
  fprintf(stderr, " -E {file}  exclude directories listed in {file}\n");
  fprintf(stderr, " -h         this info\n");
  fprintf(stderr, " -m         MD5 hash files and compare (disables -q and -s modes)\n");
  fprintf(stderr, " -p         preserve ATIME when hashing files (must have appropriate priviledges)\n");
  fprintf(stderr, " -q         do quick comparisons only\n");
  fprintf(stderr, " -s         SHA256 hash files and compare (disables -q and -m modes)\n");
  fprintf(stderr, " -v         display version information\n");
  fprintf(stderr, " -w {file}  write directory tree to file\n");
#endif

  fprintf(stderr, "\n");
}

/****
 *
 * cleanup
 *
 ****/

PRIVATE void cleanup(void)
{
  int i;

  /* variables from main */
  if (config->exclusions != NULL)
  {
    for (i = 0; config->exclusions[i] != NULL; i++)
      XFREE(config->exclusions[i]);
    XFREE(config->exclusions);
  }
  if (baseDir != NULL)
    XFREE(baseDir);
  if (compDir != NULL)
    XFREE(compDir);
  if (config->hostname != NULL)
    XFREE(config->hostname);
  if (config->home_dir != NULL)
    XFREE(config->home_dir);
  if (config->outfile != NULL)
    XFREE(config->outfile);

  /* config */
  if (config != NULL)
    XFREE(config);
#ifdef MEM_DEBUG
  XFREE_ALL();
#endif
}

/****
 *
 * SIGINT handler
 *
 ****/

void sigint_handler(int signo)
{
  signal(signo, SIG_IGN);

  /* do a calm shutdown as time and pcap_loop permit */
  quit = TRUE;
  signal(signo, sigint_handler);
}

/****
 *
 * SIGTERM handler
 *
 ****/

void sigterm_handler(int signo)
{
  signal(signo, SIG_IGN);

  /* do a calm shutdown as time and pcap_loop permit */
  quit = TRUE;
  signal(signo, sigterm_handler);
}

/****
 *
 * SIGHUP handler
 *
 ****/

#ifndef MINGW
void sighup_handler(int signo)
{
  signal(signo, SIG_IGN);

  /* time to rotate logs and check the config */
  reload = TRUE;
  signal(SIGHUP, sighup_handler);
}
#endif

/****
 *
 * SIGSEGV handler
 *
 ****/

void sigsegv_handler(int signo)
{
  signal(signo, SIG_IGN);

  fprintf(stderr, "ERR - Caught a sig%d, shutting down fast\n", signo);

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

void sigbus_handler(int signo)
{
  signal(signo, SIG_IGN);

  fprintf(stderr, "ERR - Caught a sig%d, shutting down fast\n", signo);

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

void sigill_handler(int signo)
{
  signal(signo, SIG_IGN);

  fprintf(stderr, "ERR - Caught a sig%d, shutting down fast\n", signo);

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

void sigfpe_handler(int signo)
{
  signal(signo, SIG_IGN);

  fprintf(stderr, "ERR - Caught a sig%d, shutting down fast\n", signo);

  cleanup();
#ifdef MEM_DEBUG
  XFREE_ALL();
#endif
  /* core out */
  abort();
}
