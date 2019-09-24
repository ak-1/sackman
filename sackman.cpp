/*****************************************************************************
 * Authors:  Remi Flament <remipouak at gmail dot com>
 *           Andre Kuehne <andre dot kuehne dot 77 at gmail dot com>
 *****************************************************************************
 * Copyright (c) 2005 - 2018, Remi Flament
 * Copyright (c) 2019, Andre Kuehne
 *
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0

 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 *
 */

#ifdef linux
/* For pread()/pwrite() */
#define _X_SOURCE 500
#endif

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <sys/statfs.h>
#ifdef HAVE_SETXATTR
#include <sys/xattr.h>
#endif
#include <stdarg.h>
#include <getopt.h>
#include <sys/time.h>
#include <pwd.h>
#include <grp.h>
#include <stdexcept>
#include <iostream>
#include <set>
#include <string>

#define STR(X) #X
#define rAssert(cond)                                     \
    do                                                    \
    {                                                     \
        if ((cond) == false)                              \
        {                                                 \
            cerr << "Assert failed: " << STR(cond) << "\n"; \
            throw std::runtime_error(STR(cond));          \
        }                                                 \
    } while (false)

#define PUSHARG(ARG)                      \
    rAssert(out->fuseArgc < MaxFuseArgs); \
    out->fuseArgv[out->fuseArgc++] = ARG

using namespace std;
static int savefd;
static std::set<std::string> pathSet;
static FILE *outf = NULL;

const int MaxFuseArgs = 32;
struct SackMan_Args
{
    char *mountPoint; // where the users read files
    uint mountPointLen; // length of mountPoint path
    bool isDaemon; // true == spawn in background, log to syslog
    const char *fuseArgv[MaxFuseArgs];
    int fuseArgc;
};

static SackMan_Args *sackmanArgs = new SackMan_Args;

static bool isAbsolutePath(const char *fileName)
{
    if (fileName && fileName[0] != '\0' && fileName[0] == '/')
        return true;
    else
        return false;
}

static char *getAbsolutePath(const char *path)
{
    char *realPath = new char[strlen(path) + strlen(sackmanArgs->mountPoint) + 1];

    strcpy(realPath, sackmanArgs->mountPoint);
    if (realPath[strlen(realPath) - 1] == '/')
        realPath[strlen(realPath) - 1] = '\0';
    strcat(realPath, path);
    return realPath;
}

static char *getRelativePath(const char *path)
{
    if (path[0] == '/')
    {
        if (strlen(path) == 1)
        {
            return strdup(".");
        }
        const char *substr = &path[1];
        return strdup(substr);
    }

    return strdup(path);
}

static void sackman_log(const char *path)
{
    if (pathSet.count(path)) {;
        return;
    }
    pathSet.insert(path);
    fprintf(outf, "%s\n", path + sackmanArgs->mountPointLen);
    fflush(outf);
}

static void *sackman_init(struct fuse_conn_info *info)
{
    fchdir(savefd);
    close(savefd);
    return NULL;
}

static int sackman_getattr(const char *orig_path, struct stat *stbuf)
{
    int res;

    char *aPath = getAbsolutePath(orig_path);
    char *path = getRelativePath(orig_path);
    res = lstat(path, stbuf);
    sackman_log(aPath);
    delete[] aPath;
    free(path);
    if (res == -1)
        return -errno;

    return 0;
}

static int sackman_access(const char *orig_path, int mask)
{
    int res;

    char *aPath = getAbsolutePath(orig_path);
    char *path = getRelativePath(orig_path);
    res = access(path, mask);
    sackman_log(aPath);
    delete[] aPath;
    free(path);
    if (res == -1)
        return -errno;

    return 0;
}

static int sackman_readlink(const char *orig_path, char *buf, size_t size)
{
    int res;

    char *aPath = getAbsolutePath(orig_path);
    char *path = getRelativePath(orig_path);
    res = readlink(path, buf, size - 1);
    sackman_log(aPath);
    delete[] aPath;
    free(path);
    if (res == -1)
        return -errno;

    buf[res] = '\0';

    return 0;
}

static int sackman_readdir(const char *orig_path, void *buf, fuse_fill_dir_t filler,
                            off_t offset, struct fuse_file_info *fi)
{
    DIR *dp;
    struct dirent *de;
    int res;

    (void)offset;
    (void)fi;

    char *aPath = getAbsolutePath(orig_path);
    char *path = getRelativePath(orig_path);

    sackman_log(aPath);
    delete[] aPath;
    dp = opendir(path);
    free(path);
    if (dp == NULL)
    {
        res = -errno;
        return res;
    }

    while ((de = readdir(dp)) != NULL)
    {
        struct stat st;
        memset(&st, 0, sizeof(st));
        st.st_ino = de->d_ino;
        st.st_mode = de->d_type << 12;
        if (filler(buf, de->d_name, &st, 0))
            break;
    }

    closedir(dp);

    return 0;
}

static int sackman_mknod(const char *orig_path, mode_t mode, dev_t rdev)
{
    int res;
    char *aPath = getAbsolutePath(orig_path);
    char *path = getRelativePath(orig_path);

    sackman_log(aPath);
    delete[] aPath;
    if (S_ISREG(mode))
    {
        res = open(path, O_CREAT | O_EXCL | O_WRONLY, mode);
        if (res >= 0)
            res = close(res);
    }
    else if (S_ISFIFO(mode))
    {
        res = mkfifo(path, mode);
    }
    else
    {
        res = mknod(path, mode, rdev);
    }


    if (res == -1)
    {
        free(path);
        return -errno;
    }
    else
        lchown(path, fuse_get_context()->uid, fuse_get_context()->gid);

    free(path);

    return 0;
}

static int sackman_mkdir(const char *orig_path, mode_t mode)
{
    int res;
    char *aPath = getAbsolutePath(orig_path);
    char *path = getRelativePath(orig_path);
    res = mkdir(path, mode);
    sackman_log(path);
    delete[] aPath;

    if (res == -1)
    {
        free(path);
        return -errno;
    }
    else
        lchown(path, fuse_get_context()->uid, fuse_get_context()->gid);

    free(path);
    return 0;
}

static int sackman_unlink(const char *orig_path)
{
    int res;
    char *aPath = getAbsolutePath(orig_path);
    char *path = getRelativePath(orig_path);
    res = unlink(path);
    sackman_log(aPath);
    delete[] aPath;
    free(path);

    if (res == -1)
        return -errno;

    return 0;
}

static int sackman_rmdir(const char *orig_path)
{
    int res;
    char *aPath = getAbsolutePath(orig_path);
    char *path = getRelativePath(orig_path);
    res = rmdir(path);
    sackman_log(aPath);
    delete[] aPath;
    free(path);
    if (res == -1)
        return -errno;
    return 0;
}

static int sackman_symlink(const char *from, const char *orig_to)
{
    int res;

    char *aTo = getAbsolutePath(orig_to);
    char *to = getRelativePath(orig_to);

    res = symlink(from, to);

    sackman_log(aTo);
    delete[] aTo;

    if (res == -1)
    {
        free(to);
        return -errno;
    }
    else
        lchown(to, fuse_get_context()->uid, fuse_get_context()->gid);

    free(to);
    return 0;
}

static int sackman_rename(const char *orig_from, const char *orig_to)
{
    int res;
    char *aFrom = getAbsolutePath(orig_from);
    char *aTo = getAbsolutePath(orig_to);
    char *from = getRelativePath(orig_from);
    char *to = getRelativePath(orig_to);
    res = rename(from, to);
    sackman_log(aFrom);
    delete[] aFrom;
    delete[] aTo;
    free(from);
    free(to);

    if (res == -1)
        return -errno;

    return 0;
}

static int sackman_link(const char *orig_from, const char *orig_to)
{
    int res;

    char *aFrom = getAbsolutePath(orig_from);
    char *aTo = getAbsolutePath(orig_to);
    char *from = getRelativePath(orig_from);
    char *to = getRelativePath(orig_to);

    res = link(from, to);
    sackman_log(aTo);
    delete[] aFrom;
    delete[] aTo;
    free(from);

    if (res == -1)
    {
        delete[] to;
        return -errno;
    }
    else
        lchown(to, fuse_get_context()->uid, fuse_get_context()->gid);

    free(to);

    return 0;
}

static int sackman_chmod(const char *orig_path, mode_t mode)
{
    int res;
    char *aPath = getAbsolutePath(orig_path);
    char *path = getRelativePath(orig_path);
    res = chmod(path, mode);
    sackman_log(aPath);
    delete[] aPath;
    free(path);

    if (res == -1)
        return -errno;

    return 0;
}

static int sackman_chown(const char *orig_path, uid_t uid, gid_t gid)
{
    int res;
    char *aPath = getAbsolutePath(orig_path);
    char *path = getRelativePath(orig_path);
    res = lchown(path, uid, gid);

    sackman_log(aPath);
    delete[] aPath;
    free(path);

    if (res == -1)
        return -errno;

    return 0;
}

static int sackman_truncate(const char *orig_path, off_t size)
{
    int res;

    char *aPath = getAbsolutePath(orig_path);
    char *path = getRelativePath(orig_path);
    res = truncate(path, size);
    sackman_log(aPath);
    delete[] aPath;
    free(path);

    if (res == -1)
        return -errno;

    return 0;
}

#if (FUSE_USE_VERSION == 25)
static int sackman_utime(const char *orig_path, struct utimbuf *buf)
{
    int res;
    char *aPath = getAbsolutePath(orig_path);
    char *path = getRelativePath(orig_path);
    res = utime(path, buf);
    sackman_log(aPath);
    delete[] aPath;
    free(path);

    if (res == -1)
        return -errno;

    return 0;
}

#else

static int sackman_utimens(const char *orig_path, const struct timespec ts[2])
{
    int res;

    char *aPath = getAbsolutePath(orig_path);
    char *path = getRelativePath(orig_path);

    res = utimensat(AT_FDCWD, path, ts, AT_SYMLINK_NOFOLLOW);

    sackman_log(aPath);
    delete[] aPath;
    free(path);

    if (res == -1)
        return -errno;

    return 0;
}

#endif

static int sackman_open(const char *orig_path, struct fuse_file_info *fi)
{
    int res;
    char *aPath = getAbsolutePath(orig_path);
    char *path = getRelativePath(orig_path);
    res = open(path, fi->flags);

    sackman_log(aPath);

    delete[] aPath;
    free(path);

    if (res == -1)
        return -errno;

    fi->fh = res;
    return 0;
}

static int sackman_read(const char *orig_path, char *buf, size_t size, off_t offset,
                         struct fuse_file_info *fi)
{
    char *aPath = getAbsolutePath(orig_path);
    int res;

    sackman_log(aPath);
    res = pread(fi->fh, buf, size, offset);
    if (res == -1)
    {
        res = -errno;
    }
    delete[] aPath;
    return res;
}

static int sackman_write(const char *orig_path, const char *buf, size_t size,
                          off_t offset, struct fuse_file_info *fi)
{
    int fd;
    int res;
    char *aPath = getAbsolutePath(orig_path);
    char *path = getRelativePath(orig_path);
    (void)fi;

    sackman_log(aPath);
    fd = open(path, O_WRONLY);
    if (fd == -1)
    {
        res = -errno;
        delete[] aPath;
        free(path);
        return res;
    }

    res = pwrite(fd, buf, size, offset);

    if (res == -1)
        res = -errno;

    close(fd);
    delete[] aPath;
    free(path);

    return res;
}

static int sackman_statfs(const char *orig_path, struct statvfs *stbuf)
{
    int res;
    char *aPath = getAbsolutePath(orig_path);
    char *path = getRelativePath(orig_path);
    res = statvfs(path, stbuf);
    sackman_log(aPath);
    delete[] aPath;
    free(path);
    if (res == -1)
        return -errno;

    return 0;
}

static int sackman_release(const char *orig_path, struct fuse_file_info *fi)
{
    char *aPath = getAbsolutePath(orig_path);
    (void)orig_path;

    sackman_log(aPath);
    delete[] aPath;

    close(fi->fh);
    return 0;
}

static int sackman_fsync(const char *orig_path, int isdatasync,
                          struct fuse_file_info *fi)
{
    char *aPath = getAbsolutePath(orig_path);
    (void)orig_path;
    (void)isdatasync;
    (void)fi;
    sackman_log(aPath);
    delete[] aPath;
    return 0;
}

#ifdef HAVE_SETXATTR
/* xattr operations are optional and can safely be left unimplemented */
static int sackman_setxattr(const char *orig_path, const char *name, const char *value,
                             size_t size, int flags)
{
    int res = lsetxattr(orig_path, name, value, size, flags);
    if (res == -1)
        return -errno;
    return 0;
}

static int sackman_getxattr(const char *orig_path, const char *name, char *value,
                             size_t size)
{
    int res = lgetxattr(orig_path, name, value, size);
    if (res == -1)
        return -errno;
    return res;
}

static int sackman_listxattr(const char *orig_path, char *list, size_t size)
{
    int res = llistxattr(orig_path, list, size);
    if (res == -1)
        return -errno;
    return res;
}

static int sackman_removexattr(const char *orig_path, const char *name)
{
    int res = lremovexattr(orig_path, name);
    if (res == -1)
        return -errno;
    return 0;
}
#endif /* HAVE_SETXATTR */

static void usage(char *name)
{
    fprintf(stderr, "Usage:\n");
    fprintf(stderr, "%s [-h] | [-o out-file] [-f] [-p] [-e] /directory-mountpoint\n", name);
    fprintf(stderr, "Type 'man sackman' for more details\n");
    return;
}

static bool processArgs(int argc, char *argv[], SackMan_Args *out)
{
    // set defaults
    out->isDaemon = true;

    out->fuseArgc = 0;

    // pass executable name through
    out->fuseArgv[0] = argv[0];
    ++out->fuseArgc;

    // leave a space for mount point, as FUSE expects the mount point before
    // any flags
    out->fuseArgv[1] = NULL;
    ++out->fuseArgc;
    opterr = 0;

    int res;

    bool got_p = false;

    // We need the "nonempty" option to mount the directory in recent FUSE's
    // because it's non empty and contains the files that will be logged.
    //
    // We need "use_ino" so the files will use their original inode numbers,
    // instead of all getting 0xFFFFFFFF . For example, this is required for
    // logging the ~/.kde/share/config directory, in which hard links for lock
    // files are verified by their inode equivalency.

#define COMMON_OPTS "nonempty,use_ino,attr_timeout=0,entry_timeout=0,negative_timeout=0"

    while ((res = getopt(argc, argv, "hpfeo:")) != -1)
    {
        switch (res)
        {
        case 'h':
            usage(argv[0]);
            return false;
        case 'f':
            out->isDaemon = false;
            // this option was added in fuse 2.x
            PUSHARG("-f");
            fprintf(stderr, "SackMan not running as a daemon\n");
            break;
        case 'p':
            PUSHARG("-o");
            PUSHARG("allow_other,default_permissions," COMMON_OPTS);
            got_p = true;
            fprintf(stderr, "SackMan running as a public filesystem\n");
            break;
        case 'e':
            PUSHARG("-o");
            PUSHARG("nonempty");
            fprintf(stderr, "Using existing directory\n");
            break;
        case 'o':
        {
            fprintf(stderr, "SackMan out file: %s\n", optarg);
            outf = fopen(optarg, "w");
            break;
        }
        default:
            break;
        }
    }

    if (outf == NULL) {
        char* outfile = secure_getenv("SACKMAN_OUTFILE");
        if (outfile != NULL) {
            fprintf(stderr, "SackMan out file [from env]: %s\n", outfile);
            outf = fopen(outfile, "w");
        } else {
            outf = stdout;
        }
    }

    if (!got_p)
    {
        PUSHARG("-o");
        PUSHARG(COMMON_OPTS);
    }
#undef COMMON_OPTS

    if (optind + 1 <= argc)
    {
        out->mountPoint = argv[optind++];
        out->mountPointLen = strlen(out->mountPoint);
        while (out->mountPointLen > 0 && \
               out->mountPoint[out->mountPointLen-1] == '/') {
            out->mountPointLen--;
        }
        out->fuseArgv[1] = out->mountPoint;
    }
    else
    {
        fprintf(stderr, "Missing mountpoint\n");
        usage(argv[0]);
        return false;
    }

    // If there are still extra unparsed arguments, pass them onto FUSE..
    if (optind < argc)
    {
        rAssert(out->fuseArgc < MaxFuseArgs);

        while (optind < argc)
        {
            rAssert(out->fuseArgc < MaxFuseArgs);
            out->fuseArgv[out->fuseArgc++] = argv[optind];
            ++optind;
        }
    }

    if (!isAbsolutePath(out->mountPoint))
    {
        fprintf(stderr, "You must use absolute paths "
                        "(beginning with '/') for %s\n",
                out->mountPoint);
        return false;
    }
    return true;
}

int main(int argc, char *argv[])
{
    char *input = new char[2048]; // 2ko MAX input for configuration

    umask(0);
    fuse_operations sackman_oper;
    // in case this code is compiled against a newer FUSE library and new
    // members have been added to fuse_operations, make sure they get set to
    // 0..
    memset(&sackman_oper, 0, sizeof(fuse_operations));
    sackman_oper.init = sackman_init;
    sackman_oper.getattr = sackman_getattr;
    sackman_oper.access = sackman_access;
    sackman_oper.readlink = sackman_readlink;
    sackman_oper.readdir = sackman_readdir;
    sackman_oper.mknod = sackman_mknod;
    sackman_oper.mkdir = sackman_mkdir;
    sackman_oper.symlink = sackman_symlink;
    sackman_oper.unlink = sackman_unlink;
    sackman_oper.rmdir = sackman_rmdir;
    sackman_oper.rename = sackman_rename;
    sackman_oper.link = sackman_link;
    sackman_oper.chmod = sackman_chmod;
    sackman_oper.chown = sackman_chown;
    sackman_oper.truncate = sackman_truncate;
#if (FUSE_USE_VERSION == 25)
    sackman_oper.utime = sackman_utime;
#else
    sackman_oper.utimens = sackman_utimens;
    sackman_oper.flag_utime_omit_ok = 1;
#endif
    sackman_oper.open = sackman_open;
    sackman_oper.read = sackman_read;
    sackman_oper.write = sackman_write;
    sackman_oper.statfs = sackman_statfs;
    sackman_oper.release = sackman_release;
    sackman_oper.fsync = sackman_fsync;
#ifdef HAVE_SETXATTR
    sackman_oper.setxattr = sackman_setxattr;
    sackman_oper.getxattr = sackman_getxattr;
    sackman_oper.listxattr = sackman_listxattr;
    sackman_oper.removexattr = sackman_removexattr;
#endif

    for (int i = 0; i < MaxFuseArgs; ++i)
        sackmanArgs->fuseArgv[i] = NULL; // libfuse expects null args..

    if (processArgs(argc, argv, sackmanArgs))
    {

        // if (sackmanArgs->isDaemon)
        //     dispatchAction = el::base::DispatchAction::SysLog;

        fprintf(stderr, "SackMan starting at %s.\n", sackmanArgs->mountPoint);

        delete[] input;
        fprintf(stderr, "chdir to %s\n", sackmanArgs->mountPoint);
        chdir(sackmanArgs->mountPoint);
        savefd = open(".", 0);

#if (FUSE_USE_VERSION == 25)
        fuse_main(sackmanArgs->fuseArgc,
                  const_cast<char **>(sackmanArgs->fuseArgv), &sackman_oper);
#else
        fuse_main(sackmanArgs->fuseArgc,
                  const_cast<char **>(sackmanArgs->fuseArgv), &sackman_oper, NULL);
#endif

        fprintf(stderr, "SackMan closing.\n");
    }
}
