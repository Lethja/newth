#ifndef NEW_DL_SITE_H
#define NEW_DL_SITE_H

#include "site/file.h"
#include "site/http.h"
#include "../platform/platform.h"

#include <stddef.h>

enum SiteType {
    SITE_FILE, SITE_HTTP
};

typedef struct Site {
    int type;
    union site {
        FileSite file;
        HttpSite http;
    } site;
} Site;

typedef struct SiteDirectoryEntry {
    char *name;
    char *modifiedDate;
    char isDirectory;
} SiteDirectoryEntry;

/**
 * Return the active site structure
 * @return Pointer to the currently active site
 */
Site *siteArrayGetActive(void);

/**
 * Return the active site number
 * @return Entry number of the active site
 */
long siteArrayGetActiveNth(void);

/**
 * Set the active site by number
 * @param siteNumber The site number to set to active
 * @return NULL on success or a user friendly error message as to why the site couldn't be changed-
 */
char *siteArraySetActiveNth(long siteNumber);

/**
 * Initialize a siteArray system
 * @return Structure with a file:// site pre-mounted
 * @remark Free with siteArrayFree() after use
 */
char *siteArrayInit(void);

/**
 * Free all memory inside a siteArray made with siteArrayInit()
 */
void siteArrayFree(void);

/**
 * Create a new site mount
 * @param type In: The type of site to create
 * @param path In: The current site to mount or NULL for default
 * @return A new site ready to be added to the array
 */
Site siteNew(enum SiteType type, const char *path);

/**
 * Free internal members of a site
 * @param self The site pointer to free internal members of
 */
void siteFree(Site *self);

/**
 * Free internal memory of a SiteDirectoryEntry
 * @param entry The SiteDirectoryEntry to be freed
 */
void siteDirectoryEntryFree(void *entry);

/**
 * Get the current working directory of site
 * @param self In: The site to get the directory of
 * @return The full uri of the sites working directory
 */
char *siteGetWorkingDirectory(Site *self);

/**
 * Set the working directory of a site
 * @param self In/Mod: The site to set the directory of
 * @param path In: The path to set the directory to. Can be relative to current directory, uri, or absolute
 * @return 0 on success, positive number on path error, negative value on unknown errors
 */
int siteSetWorkingDirectory(Site *self, char *path);

/**
 * Site-like implementation of POSIX 'opendir()'
 * @param self In: The site to open the path from
 * @param path In: The path to open the directory of. Can be relative to current directory, uri, or absolute
 * @return Pointer to a listing to be used on siteReadDirectoryListing or NULL on error
 * @remark Returned pointer must be freed with siteCloseDirectoryListing()
 */
void *siteOpenDirectoryListing(Site *self, char *path);

/**
 * Site-like implementation of POSIX 'readdir()'
 * @param self In: The site relative to the listing
 * @param listing In: The listing returned from siteOpenDirectoryListing()
 * @return A SiteDirectoryEntry with the next files information
 * @remark Returned pointer must be freed with siteDirectoryEntryFree()
 */
SiteDirectoryEntry *siteReadDirectoryListing(Site *self, void *listing);

/**
 * Site-like implementation of POSIX 'closedir()'
 * @param self In: The site relative to the listing
 * @param listing In: The listing returned from siteOpenDirectoryListing() to free internal memory from
 */
void siteCloseDirectoryListing(Site *self, void *listing);

#endif /* NEW_DL_SITE_H */
