#include "uri.h"

UriDetails UriDetailsNewFrom(const char *uri) {
    UriDetails details;
    char *p, *q;

    details.scheme = details.host = details.port = details.path = NULL;

    /* Get Scheme */
    p = strstr(uri, "://");
    if (p) {
        size_t len = p - uri;
        details.scheme = malloc(len + 1);

        if (details.scheme)
            strncpy(details.scheme, uri, len), details.scheme[len] = '\0';

        q = p + 3;
    } else
        q = (char *) uri;

    /* Get Host */
    p = strchr(q, '/');
    if (p) {
        size_t len;
        char *i = strchr(q, ':');
        if (i && i < p) {
            len = i - q;
            details.host = malloc(len + 1);

            if (details.host)
                strncpy(details.host, q, len), details.host[len + 1] = '\0';

            /* Optional Port */
            len = p - i;
            details.port = malloc(len);

            if (details.port)
                strncpy(details.port, i + 1, len - 1), details.port[len] = '\0';

        } else {
            len = p - q;
            details.host = malloc(len + 1);

            if (details.host)
                strncpy(details.host, q, len), details.host[len + 1] = '\0';

        }

        len = strlen(p);
        details.path = malloc(len + 1);

        if (details.path)
            strcpy(details.path, p);

    } else {
        details.host = malloc(strlen(q) + 1);

        if (details.host)
            strcpy(details.host, q);

    }

    return details;
}

void UriDetailsFree(UriDetails *details) {
    if (details->scheme) free(details->scheme), details->scheme = NULL;
    if (details->host) free(details->host), details->host = NULL;
    if (details->port) free(details->port), details->port = NULL;
    if (details->path) free(details->path), details->path = NULL;
}
