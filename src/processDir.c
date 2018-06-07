/*****
 *
 * Description: Directory Processing Functions
 *
 * Copyright (c) 2009-2018, Ron Dilley
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
 *.
 ****/

#include "processDir.h"

/****
 *
 * local variables
 *
 ****/

FILE *out;

/****
 *
 * global variables
 *
 ****/

/****
 *
 * external variables
 *
 ****/

extern int errno;
extern char **environ;
extern struct hash_s *baseDirHash;
extern struct hash_s *compDirHash;
extern int quit;
extern int baseDirLen;
extern int compDirLen;
extern char *baseDir;
extern char *compDir;
extern Config_t *config;

/****
 *
 * functions
 *
 ****/

/****
 *
 * process FTW() record
 *
 * NOTE: This is a callback function
 *
 ****/

#define FTW_RECORD 0
#define FILE_RECORD 1

static int processFtwRecord(const char *fpath, const struct stat *sb, int tflag,
                            struct FTW *ftwbuf) {
#ifdef DEBUG
  if (config->debug > 6)
    if (ftwbuf != NULL)
      printf("DEBUG - ftw: base=%d level=%d\n", ftwbuf->base, ftwbuf->level);
#endif
  return processRecord(fpath, sb, FTW_RECORD, NULL);
}

/****
 *
 * process record
 *
 ****/

int processRecord(const char *fpath, const struct stat *sb, char mode,
                  unsigned char *digestPtr) {
  struct hashRec_s *tmpRec;
  struct stat *tmpSb;
  metaData_t *tmpMD;
  char tmpBuf[1024];
  char diffBuf[4096];
  char *foundPtr;
  struct tm *tmPtr;
  MD5_CTX md5_ctx;
  sha256_context sha256_ctx;
  FILE *inFile;
  gzFile gzInFile;
  size_t rCount, lCount, tCount;
  unsigned char rBuf[16384];
  unsigned char digest[32];
  int i, tflag = sb->st_mode & S_IFMT;
  struct utimbuf tmp_utimbuf;

  /* check the directory exclusion list */
  if (config->exclusions != NULL) {
#ifndef FTW_SKIP_SUBTREE
    for (i = 0; config->exclusions[i] != NULL; i++) {
#ifdef DEBUG
      if (config->debug >= 5)
        printf("DEBUG - Exclusion [%s] Dir [%s]\n", config->exclusions[i],
               fpath);
#endif
      if ((foundPtr = strstr(fpath, config->exclusions[i])) != NULL) {
        if (tflag EQ S_IFDIR) {
          if ((foundPtr[strlen(config->exclusions[i])] EQ '/') ||
              (foundPtr[strlen(config->exclusions[i])] EQ 0)) {
#ifdef DEBUG
            if (config->debug >= 1)
              printf("DEBUG - Excluding directory [%s]\n", fpath);
#endif
            return (FTW_CONTINUE);
          }
        } else {
          if (foundPtr[strlen(config->exclusions[i])] EQ '/') {
#ifdef DEBUG
            if (config->debug >= 1)
              printf("DEBUG - Excluding child [%s]\n", fpath);
#endif
            return (FTW_CONTINUE);
          }
        }
      }
    }
#else
    if (tflag EQ S_IFDIR) {
      for (i = 0; config->exclusions[i] != NULL; i++) {
#ifdef DEBUG
        if (config->debug >= 5)
          printf("DEBUG - Exclusion [%s] Dir [%s]\n", config->exclusions[i],
                 fpath);
#endif
        if ((foundPtr = strstr(fpath, config->exclusions[i])) != NULL) {
          if (foundPtr[strlen(config->exclusions[i])] EQ 0) {
#ifdef DEBUG
            if (config->debug >= 1)
              printf("DEBUG - Excluding [%s]\n", fpath);
#endif
            return (FTW_SKIP_SUBTREE);
          }
        }
      }
    }
#endif
  }

  /* zero buffers */
  XMEMSET(&md5_ctx, 0, sizeof(md5_ctx));
  XMEMSET(&sha256_ctx, 0, sizeof(sha256_ctx));
  XMEMSET(digest, 0, sizeof(digest));
  XMEMSET(rBuf, 0, sizeof(rBuf));
  XMEMSET(tmpBuf, 0, sizeof(tmpBuf));
  XMEMSET(diffBuf, 0, sizeof(diffBuf));

  if (strlen(fpath) <= compDirLen) {
    /* ignore the root */
    return (FTW_CONTINUE);
  }

  if (quit) {
    /* graceful shutdown */
    return (FTW_STOP);
  }

#ifdef DEBUG
  if (config->debug >= 1)
    printf("DEBUG - Processing File [%s]\n", fpath + compDirLen);
#endif

  if (baseDirHash != NULL) {

    /*
     * now we need to compare
     */

#ifdef DEBUG
    if (config->debug >= 2)
      printf("DEBUG - Compairing [%s]\n", fpath + compDirLen);
#endif

    if (tflag EQ S_IFREG) {
      if (config->count) { /* count regular files */
        if (mode EQ FTW_RECORD) {
          if ((((foundPtr = strrchr(fpath + compDirLen, '.')) != NULL)) &&
              (strncmp(foundPtr, ".gz", 3) EQ 0)) {
            /* gzip compressed */
            if ((gzInFile = gzopen(fpath, "rb"))EQ NULL) {
              fprintf(stderr, "ERR - Unable to open file [%s]\n",
                      fpath + compDirLen);
            } else {
              lCount = tCount = 0;
              while ((rCount = gzread(gzInFile, rBuf, sizeof(rBuf))) > 0) {
#ifdef DEBUG
                if (config->debug >= 6)
                  printf("DEBUG - Read [%ld] bytes from [%s]\n",
                         (long int)rCount, fpath + compDirLen);
#endif
                tCount += rCount;
                for (i = 0; i < rCount; i++) {
                  if (rBuf[i] EQ '\n')
                    lCount++;
                }
              }
              gzclose(gzInFile);
#ifdef DEBUG
			  if ( config->debug >= 3 )
				  printf("Size: %ld Lines: %ld\n", tCount, lCount);
#endif
			  
			  
              /* preserve atime */
              if (config->preserve_atime) {
                tmp_utimbuf.actime = sb->st_atime;
                tmp_utimbuf.modtime = sb->st_mtime;
                if (utime(fpath, &tmp_utimbuf) != 0)
                  sprintf("ERR - Unable to reset ATIME for [%s] %d (%s)\n",
                          fpath, errno, strerror(errno));
              }
            }
          } else {
            if ((inFile = fopen(fpath, "r"))EQ NULL) {
              fprintf(stderr, "ERR - Unable to open file [%s]\n",
                      fpath + compDirLen);
            } else {
              lCount = tCount = 0;
              while ((rCount = fread(rBuf, 1, sizeof(rBuf), inFile)) > 0) {
#ifdef DEBUG
                if (config->debug >= 6)
                  printf("DEBUG - Read [%ld] bytes from [%s]\n",
                         (long int)rCount, fpath + compDirLen);
#endif
                tCount += rCount;
                for (i = 0; i < rCount; i++) {
                  if (rBuf[i] EQ '\n')
                    lCount++;
                }
              }
              fclose(inFile);

#ifdef DEBUG
			  if (config->debug >= 3)
				  printf("Size: %ld Lines: %ld\n", tCount, lCount);
#endif
			  
              /* preserve atime */
              if (config->preserve_atime) {
                tmp_utimbuf.actime = sb->st_atime;
                tmp_utimbuf.modtime = sb->st_mtime;
                if (utime(fpath, &tmp_utimbuf) != 0)
                  sprintf("ERR - Unable to reset ATIME for [%s] %d (%s)\n",
                          fpath, errno, strerror(errno));
              }
            }
          }
        } else if (mode EQ FILE_RECORD) {
        }
      } else if (config->hash) { /* hash regular files */
        if (mode EQ FTW_RECORD) {
#ifdef DEBUG
          if (config->debug >= 3)
            printf("DEBUG - Generating hash of file [%s]\n",
                   fpath + compDirLen);
#endif
          if (config->sha256_hash)
            sha256_starts(&sha256_ctx);
          else
            MD5_Init(&md5_ctx);

          if ((inFile = fopen(fpath, "r"))EQ NULL) {
            fprintf(stderr, "ERR - Unable to open file [%s]\n",
                    fpath + compDirLen);
          } else {
            while ((rCount = fread(rBuf, 1, sizeof(rBuf), inFile)) > 0) {
#ifdef DEBUG
              if (config->debug >= 6)
                printf("DEBUG - Read [%ld] bytes from [%s]\n", (long int)rCount,
                       fpath + compDirLen);
#endif
              if (config->sha256_hash)
                sha256_update(&sha256_ctx, rBuf, rCount);
              else
                MD5_Update(&md5_ctx, rBuf, rCount);
            }
            fclose(inFile);

            /* preserve atime */
            if (config->preserve_atime) {
              tmp_utimbuf.actime = sb->st_atime;
              tmp_utimbuf.modtime = sb->st_mtime;
              if (utime(fpath, &tmp_utimbuf) != 0)
                sprintf("ERR - Unable to reset ATIME for [%s] %d (%s)\n", fpath,
                        errno, strerror(errno));
            }

            /* complete hash */
            if (config->sha256_hash)
              sha256_finish(&sha256_ctx, digest);
            else
              MD5_Final(digest, &md5_ctx);
          }
        } else if (mode EQ FILE_RECORD) {
          if (digestPtr != NULL) {
#ifdef DEBUG
            if (config->debug >= 3)
              printf("DEBUG - Loading previously generated hash of file [%s]\n",
                     fpath + compDirLen);
#endif
            XMEMCPY(digest, digestPtr, config->digest_size);
          }
        } else {
          /* unknown record type */
        }
      }
    }

    if ((tmpRec = getHashRecord(baseDirHash, fpath + compDirLen))EQ NULL) {
      printf("+ %s [%s]\n",
             (tflag == S_IFDIR)
                 ? "d"
                 : (tflag == S_IFCHR)
                       ? "chr"
                       : (tflag == S_IFBLK)
                             ? "blk"
                             : (tflag == S_IFREG)
                                   ? "f"
                                   :
#ifndef MINGW
                                   (tflag == S_IFSOCK)
                                       ? "sok"
                                       : (tflag == S_IFLNK) ? "sl" :
#endif
                                                            (tflag == S_IFIFO)
                                                                ? "fifo"
                                                                : "???",
             fpath + compDirLen);
    } else {
#ifdef DEBUG
      if (config->debug >= 3)
        printf("DEBUG - Found match, comparing metadata\n");
#endif
      tmpMD = (metaData_t *)tmpRec->data;
      tmpSb = (struct stat *)&tmpMD->sb;

      if (sb->st_size != tmpSb->st_size) {
#ifdef HAVE_SNPRINTF
        snprintf(tmpBuf, sizeof(tmpBuf), "s[%ld->%ld] ",
                 (long int)tmpSb->st_size, (long int)sb->st_size);
#else
        sprintf(tmpBuf, "s[%ld->%ld] ", (long int)tmpSb->st_size,
                (long int)sb->st_size);
#endif

#ifdef HAVE_STRNCAT
        strncat(diffBuf, tmpBuf, sizeof(diffBuf) - 1);
#else
        strlcat(diffBuf, tmpBuf, sizeof(diffBuf) - 1);
#endif
      } else {
        if (config->hash && (tflag EQ S_IFREG)) {
          if (memcmp(tmpMD->digest, digest, config->digest_size) != 0) {
            /* hash does not match */
            if (config->sha256_hash) {
#ifdef HAVE_SNPRINTF
              snprintf(
                  tmpBuf, sizeof(tmpBuf), "sha256[%s->",
                  hash2hex(tmpMD->digest, (char *)rBuf, config->digest_size));
#else
              sprintf(tmpBuf, "sha256[%s->",
                      hash2hex(tmpMD->digest, rBuf, config->digest_size));
#endif
            } else {
#ifdef HAVE_SNPRINTF
              snprintf(
                  tmpBuf, sizeof(tmpBuf), "md5[%s->",
                  hash2hex(tmpMD->digest, (char *)rBuf, config->digest_size));
#else
              sprintf(tmpBuf, "md5[%s->",
                      hash2hex(tmpMD->digest, rBuf, config->digest_size));
#endif
            }
#ifdef HAVE_STRNCAT
            strncat(diffBuf, tmpBuf, sizeof(diffBuf) - 1);
#else
            strlcat(diffBuf, tmpBuf, sizeof(diffBuf) - 1);
#endif
#ifdef HAVE_SNPRINTF
            snprintf(tmpBuf, sizeof(tmpBuf), "%s] ",
                     hash2hex(digest, (char *)rBuf, config->digest_size));
#else
            sprintf(tmpBuf, "%s] ",
                    hash2hex(digest, rBuf, config->digest_size));
#endif
#ifdef HAVE_STRNCAT
            strncat(diffBuf, tmpBuf, sizeof(diffBuf) - 1);
#else
            strlcat(diffBuf, tmpBuf, sizeof(diffBuf) - 1);
#endif
          }
        }
      }

      if (!config->quick) {
        /* do more testing */

        /* uid/gid */
        if (sb->st_uid != tmpSb->st_uid) {
#ifdef HAVE_SNPRINTF
          snprintf(tmpBuf, sizeof(tmpBuf), "u[%d>%d] ", tmpSb->st_uid,
                   sb->st_uid);
#else
          sprintf(tmpBuf, "u[%d>%d] ", tmpSb->st_uid, sb->st_uid);
#endif
#ifdef HAVE_STRNCAT
          strncat(diffBuf, tmpBuf, sizeof(diffBuf) - 1);
#else
          strlcat(diffBuf, tmpBuf, sizeof(diffBuf) - 1);
#endif
        }
        if (sb->st_gid != tmpSb->st_gid) {
#ifdef HAVE_SNPRINTF
          snprintf(tmpBuf, sizeof(tmpBuf), "g[%d>%d] ", tmpSb->st_gid,
                   sb->st_gid);
#else
          sprintf(tmpBuf, "g[%d>%d] ", tmpSb->st_gid, sb->st_gid);
#endif

#ifdef HAVE_STRNCAT
          strncat(diffBuf, tmpBuf, sizeof(diffBuf) - 1);
#else
          strlcat(diffBuf, tmpBuf, sizeof(diffBuf) - 1);
#endif
        }

        /* file type */
        if ((sb->st_mode & S_IFMT) != (tmpSb->st_mode & S_IFMT)) {
#ifdef HAVE_SNPRINTF
          snprintf(
              tmpBuf, sizeof(tmpBuf), "t[%s->%s] ",
#else
          sprintf(tmpBuf, "t[%s->%s] ",
#endif
              ((tmpSb->st_mode & S_IFMT) == S_IFDIR)
                  ? "d"
                  : ((tmpSb->st_mode & S_IFMT) == S_IFBLK)
                        ? "blk"
                        : ((tmpSb->st_mode & S_IFMT) == S_IFREG)
                              ? "f"
                              : ((tmpSb->st_mode & S_IFMT) == S_IFCHR)
                                    ? "chr"
                                    :
#ifndef MINGW
                                    ((tmpSb->st_mode & S_IFMT) == S_IFSOCK)
                                        ? "sok"
                                        : ((tmpSb->st_mode & S_IFMT) == S_IFLNK)
                                              ? "sl"
                                              :
#endif
                                              ((tmpSb->st_mode & S_IFMT) ==
                                               S_IFIFO)
                                                  ? "fifo"
                                                  : "???",
              ((sb->st_mode & S_IFMT) == S_IFDIR)
                  ? "d"
                  : ((sb->st_mode & S_IFMT) == S_IFBLK)
                        ? "blk"
                        : ((sb->st_mode & S_IFMT) == S_IFREG)
                              ? "f"
                              : ((sb->st_mode & S_IFMT) == S_IFCHR)
                                    ? "chr"
                                    :
#ifndef MINGW
                                    ((sb->st_mode & S_IFMT) == S_IFSOCK)
                                        ? "sok"
                                        : ((sb->st_mode & S_IFMT) == S_IFLNK)
                                              ? "sl"
                                              :
#endif
                                              ((sb->st_mode & S_IFMT) ==
                                               S_IFIFO)
                                                  ? "fifo"
                                                  : "???");

#ifdef HAVE_STRNCAT
          strncat(diffBuf, tmpBuf, sizeof(diffBuf) - 1);
#else
          strlcat(diffBuf, tmpBuf, sizeof(diffBuf) - 1);
#endif
        }

        /* permissions */
        if ((sb->st_mode & 0xffff) != (tmpSb->st_mode & 0xffff)) {
#ifdef HAVE_SNPRINTF
          snprintf(tmpBuf, sizeof(tmpBuf),
                   "p[%c%c%c%c%c%c%c%c%c->%c%c%c%c%c%c%c%c%c] ",
#else
          sprintf(tmpBuf, "p[%c%c%c%c%c%c%c%c%c->%c%c%c%c%c%c%c%c%c] ",
#endif
#ifdef MINGW
                   (tmpSb->st_mode & S_IREAD) ? 'r' : '-',
                   (tmpSb->st_mode & S_IWRITE) ? 'w' : '-',
                   (tmpSb->st_mode & S_IEXEC) ? 'x' : '-', '-', '-', '-', '-',
                   '-', '-',
#else
                  (tmpSb->st_mode & S_IRUSR) ? 'r' : '-',
                  (tmpSb->st_mode & S_IWUSR) ? 'w' : '-',
                  (tmpSb->st_mode & S_ISUID) ? 's' : (tmpSb->st_mode & S_IXUSR)
                                                         ? 'x'
                                                         : '-',
                  (tmpSb->st_mode & S_IRGRP) ? 'r' : '-',
                  (tmpSb->st_mode & S_IWGRP) ? 'w' : '-',
                  (tmpSb->st_mode & S_ISGID) ? 's' : (tmpSb->st_mode & S_IXGRP)
                                                         ? 'x'
                                                         : '-',
                  (tmpSb->st_mode & S_IROTH) ? 'r' : '-',
                  (tmpSb->st_mode & S_IWOTH) ? 'w' : '-',
                  (tmpSb->st_mode & S_ISVTX) ? 's' : (tmpSb->st_mode & S_IXOTH)
                                                         ? 'x'
                                                         : '-',
#endif
#ifdef MINGW
                   (sb->st_mode & S_IREAD) ? 'r' : '-',
                   (sb->st_mode & S_IWRITE) ? 'w' : '-',
                   (sb->st_mode & S_IEXEC) ? 'x' : '-', '-', '-', '-', '-', '-',
                   '-'
#else
                  (sb->st_mode & S_IRUSR) ? 'r' : '-',
                  (sb->st_mode & S_IWUSR) ? 'w' : '-',
                  (sb->st_mode & S_ISUID) ? 's' : (sb->st_mode & S_IXUSR) ? 'x'
                                                                          : '-',
                  (sb->st_mode & S_IRGRP) ? 'r' : '-',
                  (sb->st_mode & S_IWGRP) ? 'w' : '-',
                  (sb->st_mode & S_ISGID) ? 's' : (sb->st_mode & S_IXGRP) ? 'x'
                                                                          : '-',
                  (sb->st_mode & S_IROTH) ? 'r' : '-',
                  (sb->st_mode & S_IWOTH) ? 'w' : '-',
                  (sb->st_mode & S_ISVTX) ? 's' : (sb->st_mode & S_IXOTH) ? 'x'
                                                                          : '-'
#endif
                   );
#ifdef HAVE_STRNCAT
          strncat(diffBuf, tmpBuf, sizeof(diffBuf) - 1);
#else
          strlcat(diffBuf, tmpBuf, sizeof(diffBuf) - 1);
#endif
        }

        /* report if MTIME does not match */
        if (sb->st_mtime != tmpSb->st_mtime) {
          tmPtr = localtime(&tmpSb->st_mtime);
#ifdef HAVE_SNPRINTF
          snprintf(tmpBuf, sizeof(tmpBuf), "mt[%04d/%02d/%02d@%02d:%02d:%02d->",
#else
          sprintf(tmpBuf, "mt[%04d/%02d/%02d@%02d:%02d:%02d->",
#endif
                   tmPtr->tm_year + 1900, tmPtr->tm_mon + 1, tmPtr->tm_mday,
                   tmPtr->tm_hour, tmPtr->tm_min, tmPtr->tm_sec);
#ifdef HAVE_STRNCAT
          strncat(diffBuf, tmpBuf, sizeof(diffBuf) - 1);
#else
          strlcat(diffBuf, tmpBuf, sizeof(diffBuf) - 1);
#endif
          tmPtr = localtime(&sb->st_mtime);
#ifdef HAVE_SNPRINTF
          snprintf(tmpBuf, sizeof(tmpBuf), "%04d/%02d/%02d@%02d:%02d:%02d] ",
#else
          sprintf(tmpBuf, "%04d/%02d/%02d@%02d:%02d:%02d] ",
#endif
                   tmPtr->tm_year + 1900, tmPtr->tm_mon + 1, tmPtr->tm_mday,
                   tmPtr->tm_hour, tmPtr->tm_min, tmPtr->tm_sec);
#ifdef HAVE_STRNCAT
          strncat(diffBuf, tmpBuf, sizeof(diffBuf) - 1);
#else
          strlcat(diffBuf, tmpBuf, sizeof(diffBuf) - 1);
#endif
        }

        if (config->show_atime) {
          /* report if ATIME does not match */
          if (sb->st_atime != tmpSb->st_atime) {
            tmPtr = localtime(&tmpSb->st_atime);
#ifdef HAVE_SNPRINTF
            snprintf(tmpBuf, sizeof(tmpBuf),
                     "at[%04d/%02d/%02d@%02d:%02d:%02d->",
#else
            sprintf(tmpBuf, "at[%04d/%02d/%02d@%02d:%02d:%02d->",
#endif
                     tmPtr->tm_year + 1900, tmPtr->tm_mon + 1, tmPtr->tm_mday,
                     tmPtr->tm_hour, tmPtr->tm_min, tmPtr->tm_sec);
#ifdef HAVE_STRNCAT
            strncat(diffBuf, tmpBuf, sizeof(diffBuf) - 1);
#else
            strlcat(diffBuf, tmpBuf, sizeof(diffBuf) - 1);
#endif
            tmPtr = localtime(&sb->st_atime);
#ifdef HAVE_SNPRINTF
            snprintf(tmpBuf, sizeof(tmpBuf), "%04d/%02d/%02d@%02d:%02d:%02d] ",
#else
            sprintf(tmpBuf, "%04d/%02d/%02d@%02d:%02d:%02d] ",
#endif
                     tmPtr->tm_year + 1900, tmPtr->tm_mon + 1, tmPtr->tm_mday,
                     tmPtr->tm_hour, tmPtr->tm_min, tmPtr->tm_sec);
#ifdef HAVE_STRNCAT
            strncat(diffBuf, tmpBuf, sizeof(diffBuf) - 1);
#else
            strlcat(diffBuf, tmpBuf, sizeof(diffBuf) - 1);
#endif
          }
        }
      }

      if (strlen(diffBuf)) {
        printf("%s%s [%s]\n", diffBuf,
               (tflag == S_IFDIR)
                   ? "d"
                   : (tflag == S_IFBLK)
                         ? "blk"
                         : (tflag == S_IFREG)
                               ? "f"
                               : (tflag == S_IFCHR)
                                     ? "chr"
                                     :
#ifndef MINGW
                                     (tflag == S_IFSOCK)
                                         ? "sok"
                                         : (tflag == S_IFLNK) ? "sl" :
#endif
                                                              (tflag == S_IFIFO)
                                                                  ? "fifo"
                                                                  : "???",
               fpath);
      }
#ifdef DEBUG
      else {
        if (config->debug > 4)
          printf("DEBUG - Record matches\n");
      }
#endif
    }

    /* store file metadata */
    tmpMD = (metaData_t *)XMALLOC(sizeof(metaData_t));
    XMEMSET(tmpMD, 0, sizeof(metaData_t));
    /* save the hash for regular files */
    if (config->hash && (tflag EQ S_IFREG)) {
#ifdef DEBUG
      if (config->debug >= 3)
        printf("DEBUG - Adding hash [%s] for file [%s]\n",
               hash2hex(digest, (char *)rBuf, sizeof(digest)),
               fpath + compDirLen);
#endif
      XMEMCPY(tmpMD->digest, digest, sizeof(digest));
    }
    XMEMCPY((void *)&tmpMD->sb, (void *)sb, sizeof(struct stat));
    addUniqueHashRec(compDirHash, fpath + compDirLen,
                     strlen(fpath + compDirLen) + 1, tmpMD);
    /* check to see if the hash should be grown */
    compDirHash = dyGrowHash(compDirHash);
  } else {

    /*
     * first time through a directory
     */

#ifdef DEBUG
    if (config->debug >= 2)
      printf("DEBUG - Scanning [%s]\n", fpath + compDirLen);
#endif

    tmpMD = (metaData_t *)XMALLOC(sizeof(metaData_t));
    XMEMSET(tmpMD, 0, sizeof(metaData_t));

    if (tflag EQ S_IFREG) {
      if (config->count) { /* count regular files */
        if (mode EQ FTW_RECORD) {
          if ((((foundPtr = strrchr(fpath + compDirLen, '.')) != NULL)) &&
              (strncmp(foundPtr, ".gz", 3) EQ 0)) {
            /* gzip compressed */

            if ((gzInFile = gzopen(fpath, "rb"))EQ NULL) {
              fprintf(stderr, "ERR - Unable to open file [%s]\n",
                      fpath + compDirLen);
            } else {
              lCount = tCount = 0;
              while ((rCount = gzread(gzInFile, rBuf, sizeof(rBuf))) > 0) {
#ifdef DEBUG
                if (config->debug >= 6)
                  printf("DEBUG - Read [%ld] bytes from [%s]\n",
                         (long int)rCount, fpath + compDirLen);
#endif
                tCount += rCount;
                for (i = 0; i < rCount; i++) {
                  if (rBuf[i] EQ '\n')
                    lCount++;
                }
              }
              gzclose(gzInFile);

			  tmpMD->byteCount = tCount;
			  tmpMD->lineCount = lCount;
			  
#ifdef DEBUG
			  if (config->debug >= 3)
				  printf("Size: %ld Lines: %ld\n", tCount, lCount);
#endif
			  
              /* preserve atime */
              if (config->preserve_atime) {
                tmp_utimbuf.actime = sb->st_atime;
                tmp_utimbuf.modtime = sb->st_mtime;
                if (utime(fpath, &tmp_utimbuf) != 0)
                  sprintf("ERR - Unable to reset ATIME for [%s] %d (%s)\n",
                          fpath, errno, strerror(errno));
              }
            }
          } else {
            if ((inFile = fopen(fpath, "r"))EQ NULL) {
              fprintf(stderr, "ERR - Unable to open file [%s]\n",
                      fpath + compDirLen);
            } else {
              lCount = tCount = 0;
              while ((rCount = fread(rBuf, 1, sizeof(rBuf), inFile)) > 0) {
#ifdef DEBUG
                if (config->debug >= 6)
                  printf("DEBUG - Read [%ld] bytes from [%s]\n",
                         (long int)rCount, fpath + compDirLen);
#endif
                tCount += rCount;
                for (i = 0; i < rCount; i++) {
                  if (rBuf[i] EQ '\n')
                    lCount++;
                }
              }
              fclose(inFile);
			  
			  tmpMD->byteCount = tCount;
			  tmpMD->lineCount = lCount;
			  
#ifdef DEBUG
			  if ( config->debug >= 3 )
				  printf("Name: %s Size: %lu Lines: %lu\n", fpath, tCount, lCount);
#endif
			  
              /* preserve atime */
              if (config->preserve_atime) {
                tmp_utimbuf.actime = sb->st_atime;
                tmp_utimbuf.modtime = sb->st_mtime;
                if (utime(fpath, &tmp_utimbuf) != 0)
                  sprintf("ERR - Unable to reset ATIME for [%s] %d (%s)\n",
                          fpath, errno, strerror(errno));
              }
            }
          }
        } else if (mode EQ FILE_RECORD) {
        }
      } else if (config->hash) { /* hash regular files */
        if (mode EQ FTW_RECORD) {
#ifdef DEBUG
          if (config->debug >= 3)
            printf("DEBUG - Generating hash of file [%s]\n",
                   fpath + compDirLen);
#endif
          if (config->sha256_hash)
            sha256_starts(&sha256_ctx);
          else
            MD5_Init(&md5_ctx);

          if ((inFile = fopen(fpath, "r"))EQ NULL) {
            fprintf(stderr, "ERR - Unable to open file [%s]\n",
                    fpath + compDirLen);
          } else {
            while ((rCount = fread(rBuf, 1, sizeof(rBuf), inFile)) > 0) {
#ifdef DEBUG
              if (config->debug >= 6)
                printf("DEBUG - Read [%ld] bytes from [%s]\n", (long int)rCount,
                       fpath + compDirLen);
#endif
              if (config->sha256_hash)
                sha256_update(&sha256_ctx, rBuf, rCount);
              else
                MD5_Update(&md5_ctx, rBuf, rCount);
            }
            fclose(inFile);

            /* preserve atime */
            if (config->preserve_atime) {
              tmp_utimbuf.actime = sb->st_atime;
              tmp_utimbuf.modtime = sb->st_mtime;
              if (utime(fpath, &tmp_utimbuf) != 0)
                sprintf("ERR - Unable to reset ATIME for [%s] %d (%s)\n", fpath,
                        errno, strerror(errno));
            }

            /* finalize hash */
            if (config->sha256_hash)
              sha256_finish(&sha256_ctx, digest);
            else
              MD5_Final(digest, &md5_ctx);
          }
        } else if (mode EQ FILE_RECORD) {
          if (digestPtr != NULL) {
#ifdef DEBUG
            if (config->debug >= 3)
              printf("DEBUG - Loading previously generated MD5 of file [%s]\n",
                     fpath + compDirLen);
#endif
            XMEMCPY(digest, digestPtr, sizeof(digest));
          }
        }
        XMEMCPY(tmpMD->digest, digest, sizeof(digest));
      }
    }

    XMEMCPY((void *)&tmpMD->sb, (void *)sb, sizeof(struct stat));
#ifdef DEBUG
    if (config->debug >= 5)
      printf("DEBUG - Adding RECORD\n");
#endif
    addUniqueHashRec(compDirHash, fpath + compDirLen,
                     strlen(fpath + compDirLen) + 1, tmpMD);
    /* check to see if the hash should be grown */
    compDirHash = dyGrowHash(compDirHash);
  }

  return (FTW_CONTINUE);
}

/****
 *
 * process directory tree
 *
 ****/

PUBLIC int processDir(char *startDirStr) {
  struct stat sb;
  char dirStr[PATH_MAX + 1];
  int tflag, ret, startDirStrLen;

  if (lstat(startDirStr, &sb) EQ 0) {
    /* file exists, make sure it is a file */
    tflag = sb.st_mode & S_IFMT;
    if (tflag EQ S_IFREG) {
      /* process file */
      compDirLen = 0;
      XSTRNCPY(dirStr, startDirStr, PATH_MAX);
      printf("Processing file [%s]\n", dirStr);
      if ((ret = loadFile(dirStr))EQ(-1)) {
        fprintf(stderr, "ERR - Problem while loading file\n");
        return (FAILED);
      } else if (ret EQ FTW_STOP) {
        fprintf(stderr, "ERR - loadFile() was interrupted by a signal\n");
        return (FAILED);
      }
    } else if (tflag EQ S_IFDIR) {
      /* process directory */

      /* add trailing slash if it is missing */
      startDirStrLen = strlen(startDirStr);
      if (startDirStr[startDirStrLen - 1] != '/') {
        snprintf(dirStr, PATH_MAX, "%s/", startDirStr);
        /* need to update the compDir variable used in main() */
        compDir[compDirLen++] = '/';
        compDir[compDirLen] = 0;
      } else
        XSTRNCPY(dirStr, startDirStr, PATH_MAX);

      printf("Processing dir [%s]\n", dirStr);

#ifdef HAVE_NFTW
      if ((ret = nftw(dirStr, processFtwRecord, 20,
                      FTW_PHYS | FTW_ACTIONRETVAL))EQ(-1)) {
#else
      if ((ret = noftw(dirStr, processFtwRecord, 20,
                       FTW_PHYS | FTW_ACTIONRETVAL))EQ(-1)) {
#endif
        fprintf(stderr, "ERR - Unable to open dir [%s]\n", dirStr);
        return (FAILED);
      } else if (ret EQ FTW_STOP) {
        fprintf(stderr, "ERR - nftw() was interrupted by a signal\n");
        return (FAILED);
      }

      /* write data to file */
      if (config->outfile != NULL) {
        writeDirHash2File(compDirHash, compDir, config->outfile);
        /* only write out the first read dir */
        XFREE(config->outfile);
        config->outfile = NULL;
      }

#ifdef DEBUG
      if (config->debug >= 2)
        printf("DEBUG - Finished processing dir [%s]\n", dirStr);
#endif
    } else {
      fprintf(stderr, "ERR - [%s] is not a regular file or a directory\n",
              startDirStr);
      return (FAILED);
    }
  } else {
    fprintf(stderr, "ERR - Unable to stat file [%s]\n", startDirStr);
    return (FAILED);
  }

  return (TRUE);
}

/****
 *
 * check for missing files
 *
 ****/

int findMissingFiles(const struct hashRec_s *hashRec) {
  struct hashRec_s *tmpRec;
  metaData_t *tmpMD;
  struct stat *tmpSb;
  int tflag;

#ifdef DEBUG
  if (config->debug >= 3)
    printf("DEBUG - Searching for [%s]\n", hashRec->keyString);
#endif

  if ((tmpRec = getHashRecord(compDirHash, hashRec->keyString))EQ NULL) {
    tmpMD = (metaData_t *)hashRec->data;
    tmpSb = (struct stat *)&tmpMD->sb;
    tflag = tmpSb->st_mode & S_IFMT;
    printf("- %s [%s]\n",
           (tflag == S_IFDIR)
               ? "d"
               : (tflag == S_IFBLK)
                     ? "blk"
                     : (tflag == S_IFREG)
                           ? "f"
                           : (tflag == S_IFCHR)
                                 ? "chr"
                                 :
#ifndef MINGW
                                 (tflag == S_IFSOCK)
                                     ? "sok"
                                     : (tflag == S_IFLNK)
                                           ? "sl"
                                           :
#endif
                                           (tflag == S_IFIFO) ? "fifo" : "???",
           hashRec->keyString);
  }
#ifdef DEBUG
  else if (config->debug >= 4)
    printf("DEBUG - Record found [%s]\n", hashRec->keyString);
#endif

  /* can use this later to interrupt traversing the hash */
  if (quit)
    return (TRUE);
  return (FALSE);
}

/****
 *
 * convert hash to hex
 *
 ****/

char *hash2hex(const unsigned char *hash, char *hashStr, int hLen) {
  int i;
  char hByte[3];
  bzero(hByte, sizeof(hByte));
  hashStr[0] = 0;

  for (i = 0; i < hLen; i++) {
    snprintf(hByte, sizeof(hByte), "%02x", hash[i] & 0xff);
#ifdef HAVE_STRNCAT
    strncat(hashStr, hByte, hLen * 2);
#else
    strlcat(hashStr, hByte, hLen * 2);
#endif
  }

  return hashStr;
}
