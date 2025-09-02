#include "assets.h"

#if defined(__linux__)
    #include <dirent.h>
    #include <errno.h>
#elif defined(__APPLE__) && defined (__MACH__)
#include <CoreFoundation/CoreFoundation.h>
#endif

#define LINUX_ASSET_DIR "/usr/local/share/jackdaw" /* Must be created at install time */


#if defined(__APPLE__) && defined(__MACH__)
/* a chatbot did most of this for me */
char *asset_get_abs_path_macos(const char *relative_path)
{
    CFBundleRef mainBundle = CFBundleGetMainBundle();
    if (!mainBundle) {
	return NULL;
    }

    CFStringRef relPathStr = CFStringCreateWithCString(kCFAllocatorDefault, relative_path, kCFStringEncodingUTF8);
    CFURLRef resourceURL = CFBundleCopyResourceURL(mainBundle, relPathStr, NULL, NULL);
    CFRelease(relPathStr);
    
    if (!resourceURL) {
	return NULL;
    }
    
    char absolutePath[PATH_MAX];
    if (!CFURLGetFileSystemRepresentation(resourceURL, true, (UInt8*)absolutePath, PATH_MAX)) {
        CFRelease(resourceURL);
        return NULL;
    }
    
    CFRelease(resourceURL);
    return strdup(absolutePath);    
}
#endif

#if defined(__linux__)
char *asset_get_abs_path_linux(const char *relative_path)
{
    int rootlen = strlen(LINUX_ASSET_DIR);
    int relpathlen = strlen(relative_path);
    char *abs_path = malloc(rootlen + relpathlen + 1);
    snprintf(abs_path, relpathlen + rootlen + 1, "%s%s", LINUX_ASSET_DIR, relative_path);
    return abs_path;
}
#endif


/* Return the absolute path of a resource, depending on OS.
 The returned string must be freed */

static char *asset_get_abs_path(const char *relative_path)
{
    char *ret;
    #ifdef TESTBUILD
    goto dev_build;
    #elif defined(__APPLE__) && defined(__MACH__)
    ret = asset_get_abs_path_macos(relative_path);
    if (!ret) goto dev_build;
    else return ret;
    #elif defined (__linux__)
    DIR* dir = opendir(LINUX_ASSET_DIR);
    if (!dir) goto dev_build;
    return asset_get_abs_path_linux(relative_path);
    else return ret;
    #else
    fprintf(stderr, "Error: unsupported os!\n");
    return NULL;
    #endif

dev_build:
    (void)0;
    const char *head = "./assets/";
    int len = strlen(relative_path) + strlen(head) + 1;
    char path[len];
    snprintf(path, len, "%s%s", head, relative_path);
    return strdup(path);
}

FILE *open_asset(const char *relative_path, char *mode_str)
{
    char *path = asset_get_abs_path(relative_path);
    if (!path) {
	fprintf(stderr, "Error retrieving absolute path for asset at \"%s\"\n", relative_path);
	return NULL;
    }
    fprintf(stderr, "opening asset at %s\n", path);
    FILE *f = fopen(path, "r");
    if (!f) fprintf(stderr, "Error opening asset at \"%s\" (abs path \"%s\"): \n", relative_path, path);
    return f;
}
