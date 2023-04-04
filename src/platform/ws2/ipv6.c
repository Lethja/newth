#include <winsock2.h>
#include <iphlpapi.h>
#include <iptypes.h>
#include <stdio.h>

#include "ipv6.h"

#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"
AdapterAddressArray *platformGetAdapterInformationIpv6(sa_family_t family,
                                                       void (arrayAdd)(AdapterAddressArray *, char *, sa_family_t,
                                                                       char *), void (nTop)(const void *, char *)) {
    struct AdapterAddressArray *array;
    DWORD rv;
    ULONG size = 15000, it = 0;
    PIP_ADAPTER_ADDRESSES adapter_addresses = NULL, aa;
    PIP_ADAPTER_UNICAST_ADDRESS ua;

    do {
        adapter_addresses = malloc(size);
        rv = GetAdaptersAddresses(AF_UNSPEC, 0, NULL, adapter_addresses, &size);
        if (rv == ERROR_BUFFER_OVERFLOW) {
            free(adapter_addresses);
            adapter_addresses = NULL;
        } else
            break;

        ++it;
    } while (it < 3);


    if (rv != NO_ERROR) {
        LPTSTR errorText;
        FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS, NULL,
                      rv, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR) &errorText, 0, NULL);

        if (errorText) {
            fprintf(stderr, "GetAdaptersAddresses() failed... %s\n", errorText);
            LocalFree(errorText);
        } else {
            fprintf(stderr, "GetAdaptersAddresses() failed...\n");
            return NULL;
        }
        free(adapter_addresses), adapter_addresses = NULL;
        return NULL;
    }

    if (!adapter_addresses) {
        return NULL;
    }


    array = malloc(sizeof(AdapterAddressArray));
    array->size = 0;

    for (aa = adapter_addresses; aa != NULL; aa = aa->Next) {
        char *addrBuf = NULL, *nameBuf = NULL;
        int len, wlen;

        /* Skip loop-back adapters */
        if (!aa || IF_TYPE_SOFTWARE_LOOPBACK == aa->IfType)
            continue;

        nameBuf = malloc(BUFSIZ);
        if (!nameBuf || !aa->FriendlyName)
            continue;

        wlen = (int) wcslen(aa->FriendlyName);
        len = WideCharToMultiByte(CP_ACP, 0, aa->FriendlyName, wlen, nameBuf, BUFSIZ, NULL, NULL);
        nameBuf[len] = '\0';

        for (ua = aa->FirstUnicastAddress; ua != NULL; ua = ua->Next) {
            if (ua->Address.lpSockaddr->sa_family == AF_INET && family != AF_INET6) {
                struct sockaddr_in *sa = (struct sockaddr_in *) ua->Address.lpSockaddr;
                char *ip4 = inet_ntoa(sa->sin_addr);
                addrBuf = malloc(BUFSIZ);
                strcpy(addrBuf, ip4);
            } else if (ua->Address.lpSockaddr->sa_family == AF_INET6 && family != AF_INET) {
                struct sockaddr_in6 *sa = (struct sockaddr_in6 *) ua->Address.lpSockaddr;
                addrBuf = malloc(BUFSIZ);
                nTop(&sa->sin6_addr, addrBuf);
            } else
                continue;

            if (addrBuf) {
                arrayAdd(array, nameBuf, ua->Address.lpSockaddr->sa_family == AF_INET6 ? 1 : 0, addrBuf);
                free(addrBuf), addrBuf = NULL;
            }
        }

        free(nameBuf);
    }

    free(adapter_addresses);

    return array;
}
#pragma clang diagnostic pop
