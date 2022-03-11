# Directory Tree Differ (DT)

by Ron Dilley <ron.dilley@uberadmin.com>

For the latest information on dt, please see:
http://www.uberadmin.com/Projects/difftree/

## What is dt?

dt is short for difftree and it is a fast directory
comparison tool.

## Why use it?

I build dt during a security incident to compare directory
snapshots on a large SAN.  I attemped to use both tripwire
and osiris and neither could complete the comparison of
directories in a reasonable amount of time.  This tool
sacrifices absolute comparisons using hashing and databases
for speed and a minimal set of comparisons of data available
from fstat.

Not to mention, it's fast.  The following runs are against
four copies of my project directories with 2,994 files
totalling 2.8Gb of data.

As a baseline, here is how long it takes find to process the
first of the four copies of the  directory tree:

```
% time find ~/cvs > /dev/null

real    0m0.011s
user    0m0.003s
sys     0m0.007s
```

A quick scan of all my project directories with comparisons
across four versions:

```
% time ./src/dt -q ~/cvs ~/c1 ~/c2 ~/c3 > /dev/null

real    0m0.058s
user    0m0.031s
sys     0m0.019s
```

A standard scan of all my project directories with comparisons
across four versions:

```
% time ./src/dt ~/cvs ~/c1 ~/c2 ~/c3 > /dev/null

real    0m0.085s
user    0m0.035s
sys     0m0.039s
```

A full scan with md5 hashing of all my project directories with
comparisons across four versions:

```
% time ./src/dt -m ~/cvs ~/c1 ~/c2 ~/c3 > /dev/null

real    1m28.804s
user    0m5.043s
sys     0m34.487s
```

## Implimentation

The output from dt describes all the detected differences found:

### Interpreting difftree output

Any changes are noted as {type}[{old}->{new}] and a file can have multiple
changes.

The change types are as follows:

| symbol | meaning |
| ------:| ------- |
|+ | New file |
|- | Missing file |
|s | Size changed |
|u | UID changed |
|g | GID changed |
|p | Permissions changed |
|mt | Modify time changed |
|at | Access time changed |
|ct | Create time changed (disabled) |
|md5 | File hash has changed |
|sha256 | File hash has changed |

The second column is the file type:
-----------------------------------

| symbol | meaning |
| ------:| ------- |
|f | File |
|d | Directory |
|sl | Soft Link |
|blk | Block device |
|fifo | FIFO |
|chr | Character device |
|sok | Socket |

The third column is the fully qualified filename.

### difftree examples

Here is an example of running dt against a set of directories.
Each directory passed to dt will be compared to the previous
argument.  This allows a quick comparison between each directory
to get a summary of the changes over time.

```
% ./dt ~/cvs ~/c1 ~/c2 ~/c3

Processing dir [/home/rdilley/cvs]
Processing dir [/home/rdilley/c1]
mt[2011/07/11@00:32:31->2011/07/10@20:10:05] d [/home/rdilley/c1/difftree]
mt[2011/07/10@23:52:12->2011/07/10@15:06:10] d [/home/rdilley/c1/difftree/CVS]
s[457->435] f [/home/rdilley/c1/difftree/CVS/Entries]
+ f [/difftree/CVS/Entries.Log]
mt[2011/07/11@01:07:39->2011/07/10@21:53:23] d [/home/rdilley/c1/difftree/src]
md5[e00bcf99c5b7116dc1cdcc275cfd02e8->5fee2a5fbdacebd80ff7b048cf4b2806] f [/home/rdilley/c1/difftree/version.sh]
g[100>1000] d [/home/rdilley/c1/pdnsd]
p[rwxr-xr-x->rwxr-x---] d [/home/rdilley/c1/imspy]
- f [/bc.tar.gz]
- f [/difftree/configure.scan]
- f [/difftree/autoscan.log]
Processing dir [/home/rdilley/c2]
s[686328114->0] f [/home/rdilley/c2/dictionary.txt]
g[1000>100] f [/home/rdilley/c2/difftree/src/hash.c]
g[1000>100] d [/home/rdilley/c2/pdnsd]
g[100>1000] p[rwxr-xr-x->rwxr-sr-x] d [/home/rdilley/c2/quickparser]
p[rwxr-x---->rwxr-xr-x] d [/home/rdilley/c2/imspy]
+ f [/bc.tar.gz]
mt[2007/08/22@22:34:40->2011/07/11@01:08:34] f [/home/rdilley/c2/wsd-0.1.config]
Processing dir [/home/rdilley/c3]
g[1000>100] p[rwxr-sr-x->rwxr-xr-x] d [/home/rdilley/c3/quickparser]
+ f [/psmd.config.orig]
s[654->16] t[f->sl] p[rwxr-xr-x->rwxrwxrwx] sl [/home/rdilley/c3/psmd.config]
mt[2011/07/11@01:08:34->2007/08/22@22:34:40] f [/home/rdilley/c3/wsd-0.1.config]
- f [/dictionary.txt]
```

Here is an example of running dt against a directory tree and saving the data:

```
% ./dt -m -w cvs_dir.txt ~/cvs

Processing dir [/home/rdilley/cvs]
```

You can then use the file as one of the directory arguments to compare an existing
directory to the one previously saves with the '-w' option.

```
% ./dt -m cvs_dir.txt ~/cvs

Processing file [cvs_dir.txt]
Read [3006] and loaded [3005] lines from file [/home/rdilley/cvs] dated [2011/07/24@18:51:16]
Processing dir [/home/rdilley/cvs]
mt[2011/07/24@18:40:38->2011/07/24@18:51:45] d [/home/rdilley/cvs/difftree]
s[20480->24576] mt[2011/07/24@18:50:07->2011/07/24@18:52:02] f [/home/rdilley/cvs/difftree/.README.swp]
+ f [/difftree/cvs_dir.txt]
```

You can combine comparing a file to the current directory and writing a new file
into a single run:

```
% ./dt -m -w cvs_dir_new.txt cvs_dir.txt ~/cvs

Processing file [cvs_dir.txt]
Read [3006] and loaded [3005] lines from file [/home/rdilley/cvs] dated [2011/07/24@18:51:16]
Processing dir [/home/rdilley/cvs]
mt[2011/07/24@18:40:38->2011/07/24@18:51:45] d [/home/rdilley/cvs/difftree]
s[20480->24576] mt[2011/07/24@18:50:07->2011/07/24@18:53:05] f [/home/rdilley/cvs/difftree/.README.swp]
+ f [/difftree/cvs_dir.txt]
```

You can then compare just the two files:

```
% ./dt -m cvs_dir.txt cvs_dir_new.txt
 
Processing file [cvs_dir.txt]
Read [3006] and loaded [3005] lines from file [/home/rdilley/cvs] dated [2011/07/24@18:51:16]
Processing file [cvs_dir_new.txt]
+ f [/difftree/cvs_dir.txt]
mt[2011/07/24@18:40:38->2011/07/24@18:51:45] d [/home/rdilley/cvs/difftree]
s[20480->24576] mt[2011/07/24@18:50:07->2011/07/24@18:53:05] f [/home/rdilley/cvs/difftree/.README.swp]
Read [3007] and loaded [3006] lines from file [/home/rdilley/cvs] dated [2011/07/24@18:53:36]
```

To monitor a directory tree over time, you can run this tool once a week/day/hour
and store the files so that you can compare an arbitrary point in the in the past
to the current directory tree, or any other point that you have stored.

```
% ./dt -m -w cvs_dir.`date '+%Y%m%d%H%M%S'`.txt ~/cvs
```

If you want a quick way to monitor changes to your UNIX system over time, you can
run the following script from cron once per day:

```
#!/bin/sh
#
# desc: use to keep track of filesystem changes
#
#####

TIME=`date '+%Y%m%d%H%M%S'`
DT_ARGS="-m -p" # md5 hash files and preserve access time
DT_EXCLUSIONS="-e /proc -e /run -e /sys -e /snap -e /home -e /var/log"
DT_DIR="/root/dt"
DT_COMMAND="/usr/local/bin/dt"
MAILTO="root"

if [ -f "${DT_DIR}/root_dir.current" ]; then
 # compare to last run
 ${DT_COMMAND} ${DT_ARGS} ${DT_EXCLUSIONS} -w ${DT_DIR}/root_dir.${TIME}.dt ${DT_DIR}/root_dir.current / | mailx -s DT_Delta ${MAILTO}
 # create link
 ln -f -s ${DT_DIR}/root_dir.${TIME}.dt ${DT_DIR}/root_dir.current
else
 # first run, no compare this time
 ${DT_COMMAND} ${DT_ARGS} ${DT_EXCLUSIONS} -w ${DT_DIR}/root_dir.${TIME}.dt /
 # create link
 ln -s ${DT_DIR}/root_dir.${TIME}.dt ${DT_DIR}/root_dir.current
fi

# done
exit
```

The output sent to the root user's e-mail address can look something like this:

```
Processing file [/root/dt/root_dir.current]
Read [301847] and loaded [301847] lines from file about [/] dated [2022/03/11@10:10:32]
Processing dir [/]
mt[2022/03/11@10:10:32->2022/03/11@10:14:23] d [/tmp]
md5[55bab7730bbefcdd39c5cbdc2d0d2ef3->088617c72b9ad708c2112aa886950864] f [/swapfile]
mt[2022/03/11@00:36:27->2022/03/11@10:14:20] d [/usr/local/bin]
s[412424->372528] mt[2022/03/11@00:36:27->2022/03/11@10:14:20] f [/usr/local/bin/dt]
mt[2022/03/11@00:36:27->2022/03/11@10:14:20] d [/usr/local/share/man/man1]
mt[2022/03/11@00:36:27->2022/03/11@10:14:20] f [/usr/local/share/man/man1/dt.1]
md5[7c922ffc3587b5683e9d2093ba8fa7a9->e02bcdaac5a73fa7235180cae7362adf] mt[2022/03/11@10:10:32->2022/03/11@10:14:23] f [/var/lib/fail2ban/fail2ban.sqlite3]
mt[2022/03/11@10:07:50->2022/03/11@10:12:50] d [/var/lib/NetworkManager]
md5[44c64a176f02f53aada6312aec6e08d1->75e80cb0b88aa8ec115a3b0d4af64175] mt[2022/03/11@10:07:50->2022/03/11@10:12:50] f [/var/lib/NetworkManager/timestamps]
mt[2022/03/11@07:35:48->2022/03/11@10:13:15] d [/var/spool/postfix/active]
mt[2022/03/11@07:35:47->2022/03/11@10:13:15] d [/var/spool/postfix/maildrop]
mt[2022/03/11@07:35:47->2022/03/11@10:13:15] d [/var/spool/postfix/incoming]
mt[2022/03/11@07:35:48->2022/03/11@10:13:15] d [/var/mail]
s[2422->7676] mt[2022/03/11@07:35:48->2022/03/11@10:13:15] f [/var/mail/rdilley]
mt[2022/03/11@10:12:16->2022/03/11@10:13:53] d [/dev/shm]
mt[2022/03/11@10:12:08->2022/03/11@10:15:46] chr [/dev/pts/2]
mt[2022/03/11@10:10:32->2022/03/11@10:14:16] chr [/dev/pts/0]
mt[2022/03/11@10:10:32->2022/03/11@10:14:16] chr [/dev/ptmx]
mt[2022/03/11@01:16:39->2022/03/11@10:13:15] d [/root/dt]
mt[2022/03/11@01:16:39->2022/03/11@10:13:15] sl [/root/dt/root_dir.current]
+ f [root/dt/root_dir.20220311101032.dt]
```

dt comes with a minimal set of options as follows:

```
% dt --help
dt v1.0.1 [Mar 10 2022 - 18:12:06]

syntax: difftree [options] {dir}|{file} [{dir} ...]
 -a|--atime           show last access time changes (enables -p|--preserve)
 -c|--count           count file lines and bytes (disables hash modes)
 -d|--debug (0-9)     enable debugging info
 -e|--exdir {dir}     exclude {dir}
 -E|--exfile {file}   exclude directories listed in {file}
 -h|--help            this info
 -m|--md5             MD5 hash files and compare (disables -q|--quick and -s|--sha256 modes)
 -p|--preserve        preserve ATIME when hashing files (must have appropriate priviledges)
 -q|--quick           do quick comparisons only
 -s|--sha256          SHA256 hash files and compare (disables -q|--quick and -m|--md5 modes)
 -v|--version         display version information
 -w|--write {file}    write directory tree to file
```

The -a|--atime option reports on any change to the last access times. This option enables the -p|--preserve option.

The -e|--exdir and -E|--exfile options allow you to exclude directories.

The exclusion file is a <CR> delimited list of directories:

```
% cat etc/exclusions.txt 
/proc
/run
```

The -p|--preserve option attempts to preserve the last access time when files are hashed. This only works if you have sufficient privileges to change atime and mtime.

The -q option detects new and missing files and changes in file size. This disables -m|--md5 and -s|--sha256 mode.

The -m|--md5 and -s|--sha256 options enable file hashing, these are mutually exclusive and either option disables -q|--quick mode.

The -d option is only useful if dt is compiled with --enable-debug.

## Compatibility

I have built and tested dt on the following operating systems:

* Linux (openSUSE v13.2 [32/64bit])
* Linux (CentOS v7.0 [32/64bit])
* Linux (Ubuntu v18.04.x/20.04.x [64bit])
* openBSD (v5.7 [i386])
* FreeBSD (v10.2 [64bit])
* Mac OS/X (Mavericks/Yosemite)
* Cygwin

I have confirmed that dt does not compile or run on the following:

* MinGW [missing nftw() and bzero()]

## Security Implications

Assume that there are errors in the dt source that would
allow an attacker to gain unauthorized access to your
computer.  Don't trust this software and install and use
it at your own risk.

## Bugs

I am not a programmer by any strech of the imagination.  I
have attempted to remove the obvious bugs and other
programmer related errors but please keep in mind the first
sentence.  If you find an issue with code, please send me
an e-mail with details and I will be happy to look into
it.

Ron Dilley
ron.dilley@uberadmin.com


