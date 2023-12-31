/* vi: set sw=4 ts=4: */
/* Copyright 2005 Rob Landley <rob@landley.net>
 *
 * Switch from rootfs to another filesystem as the root of the mount tree.
 *
 * Licensed under GPLv2, see file LICENSE in this source tree.
 */

//usage:#define switch_root_trivial_usage
//usage:       "[-c /dev/console] NEW_ROOT NEW_INIT [ARGS]"
//usage:#define switch_root_full_usage "\n\n"
//usage:       "Free initramfs and switch to another root fs:\n"
//usage:       "chroot to NEW_ROOT, delete all in /, move NEW_ROOT to /,\n"
//usage:       "execute NEW_INIT. PID must be 1. NEW_ROOT must be a mountpoint.\n"
//usage:     "\n    -c DEV  Reopen stdio to DEV after switch"
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/vfs.h>
#include <sys/mount.h>

// Make up for header deficiencies
#ifndef RAMFS_MAGIC
# define RAMFS_MAGIC ((unsigned)0x858458f6)
#endif
#ifndef TMPFS_MAGIC
# define TMPFS_MAGIC ((unsigned)0x01021994)
#endif
#ifndef MS_MOVE
# define MS_MOVE     8192
#endif

#define DOT_OR_DOTDOT(s) ((s)[0] == '.' && (!(s)[1] || ((s)[1] == '.' && !(s)[2])))

static char *last_char_is(const char *s, int c)
{
    if (s && *s) {
        size_t sz = strlen(s) - 1;
        s += sz;
        if ( (unsigned char)*s == c)
            return (char*)s;
    }
    return NULL;
}

static char *concat_path_file(const char *path, const char *filename)
{
    char *lc;
    char *string;

    if (!path)
        path = "";
    lc = last_char_is(path, '/');
    while (*filename == '/')
        filename++;
    if (asprintf(&string, "%s%s%s", path, (lc==NULL ? "/" : ""), filename) < 0)
        return NULL;
    return string;
}

// Recursively delete contents of rootfs
static int delete_contents(const char *directory, dev_t rootdev)
{
    int ret = 0;
    DIR *dir;
    struct dirent *d;
    struct stat st;

    // Don't descend into other filesystems
    if (lstat(directory, &st) || st.st_dev != rootdev)
        return 0;

    // Recursively delete the contents of directories
    if (S_ISDIR(st.st_mode)) {
        dir = opendir(directory);
        if (dir) {
            while ((d = readdir(dir))) {
                char *newdir = d->d_name;

                // Skip . and ..
                if (DOT_OR_DOTDOT(newdir))
                    continue;

                // Recurse to delete contents
                char *concatdir = concat_path_file(directory, newdir);
                if (!concatdir) {
                    fprintf(stderr, "failed to concatenate path '%s' and '%s'\n", directory, newdir);
                    ret = -1;
                    break;
                }

                ret = delete_contents(concatdir, rootdev);
                free(concatdir);
                if (ret)
                    break;
            }
            closedir(dir);

            // Directory should now be empty, zap it
            rmdir(directory);
        }
    } else {
        // It wasn't a directory, zap it
        unlink(directory);
    }

    return ret;
}

int switch_root(const char *newroot, const char *init, char *const argv[], char *const envp[])
{
    //char *console = NULL;
    struct stat st;
    struct statfs stfs;
    dev_t rootdev;

    // Change to new root directory and verify it's a different fs
    if (chdir(newroot)) {
        fprintf(stderr, "failed to chdir to '%s': %s\n", newroot, strerror(errno));
        return -1;
    }
    if (stat("/", &st)) {
        fprintf(stderr, "failed to stat root directory: %s\n", strerror(errno));
        return -1;
    }
    rootdev = st.st_dev;
    if (stat(".", &st)) {
        fprintf(stderr, "failed to stat current directory: %s\n", strerror(errno));
        return -1;
    }
    if (st.st_dev == rootdev || getpid() != 1) {
        fprintf(stderr, "new root must be a mountpoint and we must be pid 1\n");
        return -1;
    }

    // Additional sanity checks: we're about to rm -rf /, so be REALLY SURE
    // we mean it. I could make this a CONFIG option, but I would get email
    // from all the people who WILL destroy their filesystems.
    if (stat("/init", &st) != 0 || !S_ISREG(st.st_mode)) {
        fprintf(stderr, "/init is not a regular file\n");
        return -1;
    }
    statfs("/", &stfs); // this never fails
    if ((unsigned)stfs.f_type != RAMFS_MAGIC
     && (unsigned)stfs.f_type != TMPFS_MAGIC
    ) {
        fprintf(stderr, "root filesystem is not a ramfs/tmpfs\n");
        return -1;
    }

    // Zap everything out of rootdev
    delete_contents("/", rootdev);

    // Overmount / with newdir and chroot into it
    if (mount(".", "/", NULL, MS_MOVE, NULL)) {
        // For example, fails when newroot is not a mountpoint
        fprintf(stderr, "failed to move root: %s\n", strerror(errno));
        return -1;
    }
    if (chroot(".")) {
        fprintf(stderr, "failed to chroot to new root: %s\n", strerror(errno));
        return -1;
    }
    if (chdir("/")) {
        fprintf(stderr, "failed to chdir to new root: %s\n", strerror(errno));
        return -1;
    }

    // If a new console specified, redirect stdin/stdout/stderr to it
    //if (console) {
    //    close(0);
    //    xopen(console, O_RDWR);
    //    xdup2(0, 1);
    //    xdup2(0, 2);
    //}

    // Exec real init
    execve(init, argv, envp);

    fprintf(stderr, "failed to execv '%s': %s\n", init, strerror(errno));
    return -1;
}

/*
From: Rob Landley <rob@landley.net>
Date: Tue, Jun 16, 2009 at 7:47 PM
Subject: Re: switch_root...

...
...
...

If you're _not_ running out of init_ramfs (if for example you're using initrd
instead), you probably shouldn't use switch_root because it's the wrong tool.

Basically what the sucker does is something like the following shell script:

 find / -xdev | xargs rm -rf
 cd "$1"
 shift
 mount --move . /
 exec chroot . "$@"

There are a couple reasons that won't work as a shell script:

1) If you delete the commands out of your $PATH, your shell scripts can't run
more commands, but you can't start using dynamically linked _new_ commands
until after you do the chroot because the path to the dynamic linker is wrong.
So there's a step that needs to be sort of atomic but can't be as a shell
script.  (You can work around this with static linking or very carefully laid
out paths and sequencing, but it's brittle, ugly, and non-obvious.)

2) The "find | rm" bit will acually delete everything because the mount points
still show up (even if their contents don't), and rm -rf will then happily zap
that.  So the first line is an oversimplification of what you need to do _not_
to descend into other filesystems and delete their contents.

The reason we do this is to free up memory, by the way.  Since initramfs is a
ramfs, deleting its contents frees up the memory it uses.  (We leave it with
one remaining dentry for the new mount point, but that's ok.)

Note that you cannot ever umount rootfs, for approximately the same reason you
can't kill PID 1.  The kernel tracks mount points as a doubly linked list, and
the pointer to the start/end of that list always points to an entry that's
known to be there (rootfs), so it never has to worry about moving that pointer
and it never has to worry about the list being empty.  (Back around 2.6.13
there _was_ a bug that let you umount rootfs, and the system locked hard the
instant you did so endlessly looping to find the end of the mount list and
never stopping.  They fixed it.)

Oh, and the reason we mount --move _and_ do the chroot is due to the way "/"
works.  Each process has two special symlinks, ".", and "/".  Each of them
points to the dentry of a directory, and give you a location paths can start
from.  (Historically ".." was also special, because you could enter a
directory via a symlink so backing out to the directory you came from doesn't
necessarily mean the one physically above where "." points to.  These days I
think it's just handed off to the filesystem.)

Anyway, path resolution starts with "." or "/" (although the "./" at the start
of the path may be implicit), meaning it's relative to one of those two
directories.  Your current directory, and your current root directory.  The
chdir() syscall changes where "." points to, and the chroot() syscall changes
where "/" points to.  (Again, both are per-process which is why chroot only
affects your current process and its child processes.)

Note that chroot() does _not_ change where "." points to, and back before they
put crazy security checks into the kernel your current directory could be
somewhere you could no longer access after the chroot.  (The command line
chroot does a cd as well, the chroot _syscall_ is what I'm talking about.)

The reason mounting something new over / has no obvious effect is the same
reason mounting something over your current directory has no obvious effect:
the . and / links aren't recalculated after a mount, so they still point to
the same dentry they did before, even if that dentry is no longer accessible
by other means.  Note that "cd ." is a NOP, and "chroot /" is a nop; both look
up the cached dentry and set it right back.  They don't re-parse any paths,
because they're what all paths your process uses would be relative to.

That's why the careful sequencing above: we cd into the new mount point before
we do the mount --move.  Moving the mount point would otherwise make it
totally inaccessible to is because cd-ing to the old path wouldn't give it to
us anymore, and cd "/" just gives us the cached dentry from when the process
was created (in this case the old initramfs one).  But the "." symlink gives
us the dentry of the filesystem we just moved, so we can then "chroot ." to
copy that dentry to "/" and get the new filesystem.  If we _didn't_ save that
dentry in "." we couldn't get it back after the mount --move.

(Yes, this is all screwy and I had to email questions to Linus Torvalds to get
it straight myself.  I keep meaning to write up a "how mount actually works"
document someday...)
*/
