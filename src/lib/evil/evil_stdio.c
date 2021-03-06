#ifdef HAVE_CONFIG_H
# include "config.h"
#endif /* HAVE_CONFIG_H */

#include "Evil.h"
#include "evil_private.h"

#undef fopen
#undef rename

#ifdef _WIN32_WCE

/*
 * Error related functions
 *
 */

void evil_perror (const char *s EVIL_UNUSED)
{
   fprintf(stderr, "[Windows CE] error\n");
}

/*
 * Stream related functions
 *
 */

FILE *evil_fopen(const char *path, const char *mode)
{
   FILE *f;
   char *filename;

   if (!path || !*path)
     return NULL;

   if ((*path != '\\') && (*path != '/'))
     {
        char buf[PATH_MAX];
        int   l1;
        int   l2;

        if (!evil_getcwd(buf, PATH_MAX))
          return NULL;

        l1 = strlen(buf);
        l2 = strlen(path);
        filename = (char *)malloc(l1 + 1 + l2 + 1);
        if (!filename)
          return NULL;
        memcpy(filename, buf, l1);
        filename[l1] = '\\';
        memcpy(filename + l1 + 1, path, l2);
        filename[l1 + 1 + l2] = '\0';
     }
   else
     filename = _strdup(path);

   EVIL_PATH_SEP_UNIX_TO_WIN32(filename);

   printf ("fopen : %s\n", filename);

   f = fopen(filename, mode);
   free(filename);

   return f;
}

void evil_rewind(FILE *stream)
{
   fseek(stream, 0, SEEK_SET);
}

int evil_remove(const char *path)
{
   struct stat st;

   if (stat(path, &st) < 0) return -1;

   if (S_ISDIR(st.st_mode))
     {
        if (rmdir(path) < 0) return -1;
        return 0;
     }

   if (S_ISREG(st.st_mode))
     {
        if (unlink(path) < 0) return -1;
        return 0;
     }

   return -1;
}


#endif /* _WIN32_WCE */


#ifdef _WIN32_WCE

FILE *evil_fopen_native(const char *path, const char *mode)
{
   HANDLE   handle;
   char    *filename;
   wchar_t *wfilename;
   DWORD    acs = GENERIC_READ;
   DWORD    creation;

   if (!path || !*path || !mode || !*mode)
     return NULL;

   if ((*path != '\\') && (*path != '/'))
     {
        char buf[PATH_MAX];
        int   l1;
        int   l2;

        if (!evil_getcwd(buf, PATH_MAX))
          return NULL;

        l1 = strlen(buf);
        l2 = strlen(path);
        filename = (char *)malloc(l1 + 1 + l2 + 1);
        if (!filename)
          return NULL;
        memcpy(filename, buf, l1);
        filename[l1] = '\\';
        memcpy(filename + l1 + 1, path, l2);
        filename[l1 + 1 + l2] = '\0';
     }
   else
     filename = _strdup(path);

   EVIL_PATH_SEP_UNIX_TO_WIN32(filename);
   printf ("fopen native : %s\n", filename);

   wfilename = evil_char_to_wchar(filename);
   free(filename);

   if (!wfilename)
     return NULL;

   if (*mode == 'r')
     {
        acs = GENERIC_READ;
        creation = OPEN_EXISTING;
     }
   if (*mode == 'w')
     {
        acs = GENERIC_WRITE;
        creation = CREATE_ALWAYS;
     }

   handle = CreateFile(wfilename,
                       acs,
                       0, NULL,
                       creation,
                       FILE_ATTRIBUTE_NORMAL,
                       NULL);
   free(wfilename);

   if (handle == INVALID_HANDLE_VALUE)
     {
        _evil_last_error_display(__FUNCTION__);
        return NULL;
     }

   return (FILE *)handle;
}

size_t evil_fread_native(void* buffer, size_t size, size_t count, FILE* stream)
{
   HANDLE handle;
   DWORD  bytes_read;
   BOOL   res;

   if ((size == 0) || (count == 0))
     return 0;

   handle = (HANDLE)stream;
   res = ReadFile(handle, buffer, size * count, &bytes_read, NULL);
   if (!res)
     {
        _evil_last_error_display(__FUNCTION__);
        return 0;
     }

   return (bytes_read != size * count) ? 0 : bytes_read;
}

int evil_fclose_native(FILE *stream)
{
   if (!CloseHandle((HANDLE)stream))
     {
        _evil_last_error_display(__FUNCTION__);
        return -1;
     }

   return 0;
}

#endif /* _WIN32_WCE */

int 
evil_rename(const char *src, const char* dst)
{
   struct stat st;

   if (stat(dst, &st) < 0)
        return rename(src, dst);

   if (stat(src, &st) < 0)
        return -1;

   if (S_ISDIR(st.st_mode))
     {
        rmdir(dst);
        return rename(src, dst);
     }

   if (S_ISREG(st.st_mode))
     {
        unlink(dst);
        return rename(src, dst);
     }

   return -1;
}

