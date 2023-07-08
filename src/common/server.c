#include "server.h"

char *globalRootPath = NULL;
volatile int serverRun = 1;

RoutineArray globalRoutineArray;

SOCKET serverMaxSocket;
SOCKET serverListenSocket;
fd_set serverReadSockets;
fd_set serverWriteSockets;

#pragma clang diagnostic push
#pragma ide diagnostic ignored "UnusedParameter"

void noAction(int signal) {}

#pragma clang diagnostic pop

char handleDir(SOCKET clientSocket, char *webPath, char *absolutePath, char type, PlatformFileStat *st) {
    char buf[BUFSIZ];
    SocketBuffer socketBuffer = socketBufferNew(clientSocket, 0);
    DIR *dir = platformDirOpen(absolutePath);

    LINEDBG;

    if (dir == NULL)
        return httpHeaderHandleError(&socketBuffer, webPath, httpGet, 404);

    /* Headers */
    httpHeaderWriteResponse(&socketBuffer, 200);
    httpHeaderWriteDate(&socketBuffer);
    httpHeaderWriteLastModified(&socketBuffer, st);
    httpHeaderWriteContentType(&socketBuffer, "text/html", "");
    httpHeaderWriteChunkedEncoding(&socketBuffer);
    httpHeaderWriteEnd(&socketBuffer);
    eventHttpRespondInvoke(&socketBuffer.clientSocket, webPath, type, 200);

    if (type == httpHead) {
        if (socketBufferFlush(&socketBuffer))
            goto handleDirAbort;
    }

    htmlHeaderWrite(buf, webPath[0] == '\0' ? "/" : webPath);

    if (httpBodyWriteChunk(&socketBuffer, buf) || socketBufferFlush(&socketBuffer))
        goto handleDirAbort;

    buf[0] = '\0';
    htmlBreadCrumbWrite(buf, webPath[0] == '\0' ? "/" : webPath);

    if (httpBodyWriteChunk(&socketBuffer, buf))
        goto handleDirAbort;

    buf[0] = '\0';
    htmlListStart(buf);

    if (httpBodyWriteChunk(&socketBuffer, buf) || socketBufferFlush(&socketBuffer))
        goto handleDirAbort;

    DirectoryRoutineArrayAdd(&globalRoutineArray, DirectoryRoutineNew(clientSocket, dir, webPath, globalRootPath));

    return 0;

    handleDirAbort:
    if (dir)
        platformDirClose(dir);

    return 1;
}

char handleFile(SOCKET clientSocket, const char *header, char *webPath, char *absolutePath, char httpType,
                PlatformFileStat *st) {
    SocketBuffer socketBuffer = socketBufferNew(clientSocket, 0);
    PlatformFile fp = platformFileOpen(absolutePath, "rb");
    PlatformFileOffset start, finish;
    char e;

    LINEDBG;

    if (fp == NULL) {
        LINEDBG;
        return 1;
    }

    start = finish = 0;
    e = httpHeaderReadRange(header, &start, &finish, (const PlatformFileOffset *) &st->st_size);

    /* Headers */
    httpHeaderWriteResponse(&socketBuffer, (short) (e ? 200 : 206));
    httpHeaderWriteDate(&socketBuffer);
    httpHeaderWriteFileName(&socketBuffer, webPath);
    httpHeaderWriteLastModified(&socketBuffer, st);
    httpHeaderWriteAcceptRanges(&socketBuffer);

    if (e)
        httpHeaderWriteContentLengthSt(&socketBuffer, st);
    else {
        /* Handle the end of the byte range being out of range or unspecified */
        if (finish >= st->st_size || finish == 0)
            finish = st->st_size;

        httpHeaderWriteContentLength(&socketBuffer, finish - start);
        httpHeaderWriteRange(&socketBuffer, start, finish, st->st_size);
    }

    httpHeaderWriteEnd(&socketBuffer);
    socketBufferFlush(&socketBuffer);
    eventHttpRespondInvoke(&socketBuffer.clientSocket, webPath, httpType, (short) (e ? 200 : 206));

    if (httpType == httpHead)
        return 0;

    /* Body */
    if (st->st_size < BUFSIZ) {
        if (httpBodyWriteFile(&socketBuffer, fp, start, e ? st->st_size : finish))
            goto handleFileAbort;
        platformFileClose(fp);
        eventHttpFinishInvoke(&socketBuffer.clientSocket, webPath, httpType, 0);
        return 0;
    } else
        FileRoutineArrayAdd(&globalRoutineArray,
                            FileRoutineNew(clientSocket, fp, start, e ? st->st_size : finish, webPath));
    return 0;

    handleFileAbort:
    if (fp)
        platformFileClose(fp);
    return 1;
}

char handlePath(SOCKET clientSocket, const char *header, char *webPath) {
    PlatformFileStat st;
    PlatformTimeStruct tm;
    char absolutePath[FILENAME_MAX], e = 0;

    LINEDBG;

    if (platformPathWebToSystem(globalRootPath, webPath, absolutePath) || platformFileStat(absolutePath, &st)) {
        LINEDBG;
        goto handlePathNotFound;
    }

    e = httpClientReadType(header);

    if (!httpHeaderReadIfModifiedSince(header, &tm)) {
        PlatformTimeStruct mt;
        if (!platformGetTimeStruct(&st.st_mtime, &mt)) {
            if (platformTimeStructEquals(&tm, &mt)) {
                SocketBuffer socketBuffer = socketBufferNew(clientSocket, 0);

                httpHeaderWriteResponse(&socketBuffer, 304);
                httpHeaderWriteDate(&socketBuffer);
                httpHeaderWriteEnd(&socketBuffer);
                eventHttpRespondInvoke(&socketBuffer.clientSocket, webPath, e, 304);

                if (socketBufferFlush(&socketBuffer))
                    return 1;

                return 0;
            }
        }
    }

    if (platformFileStatIsDirectory(&st))
        e = handleDir(clientSocket, webPath, absolutePath, e, &st);
    else if (platformFileStatIsFile(&st))
        e = handleFile(clientSocket, header, webPath, absolutePath, e, &st);

    return e;

    handlePathNotFound:
    {
        SocketBuffer socketBuffer = socketBufferNew(clientSocket, 0);
        return httpHeaderHandleError(&socketBuffer, webPath, e, 404);
    }
}

char handleConnection(SOCKET clientSocket) {
    char r = 0;
    char buffer[BUFSIZ];
    size_t bytesRead, messageSize = 0;
    char *uriPath;

    LINEDBG;

    while ((bytesRead = recv(clientSocket, buffer + messageSize, (int) (sizeof(buffer) - messageSize - 1), 0))) {
        if (bytesRead == -1)
            return 1;

        messageSize += bytesRead;
        if (messageSize >= BUFSIZ - 1) {
            SocketBuffer socketBuffer = socketBufferNew(clientSocket, 0);

            httpHeaderWriteResponse(&socketBuffer, 431);
            httpHeaderWriteDate(&socketBuffer);
            httpHeaderWriteContentLength(&socketBuffer, 0);
            httpHeaderWriteEnd(&socketBuffer);
            socketBufferFlush(&socketBuffer);
            eventHttpRespondInvoke(&socketBuffer.clientSocket, "", httpUnknown, 431);

            return 1;
        }

        if (buffer[messageSize - 1] == '\n')
            break;
    }

    if (messageSize == 0)
        return 1;

    buffer[messageSize - 1] = 0;

    uriPath = httpClientReadUri(buffer);
    if (uriPath) {
        if (uriPath[0] != '/')
            r = 1;
        else
            r = handlePath(clientSocket, buffer, uriPath);

        free(uriPath);
    }

    return r;
}

unsigned short getPort(const SOCKET *listenSocket) {
    struct sockaddr_storage sock;
    socklen_t sockLen = sizeof(sock);
    if (getsockname(*listenSocket, (struct sockaddr *) &sock, &sockLen) == -1) {
        LINEDBG;
        return 0;
    }

    if (sock.ss_family == AF_INET) {
        struct sockaddr_in *addr = (struct sockaddr_in *) &sock;
        return ntohs(addr->sin_port);
    } else if (sock.ss_family == AF_INET6) {
        struct SOCKIN6 *addr = (struct SOCKIN6 *) &sock;
        return ntohs(addr->sin6_port);
    }
    return 0;
}

void serverTick(void) {
    SOCKET i;
    fd_set readyToReadSockets, readyToWriteSockets/*, errorSockets*/;
    struct timeval globalSelectSleep;

    while (serverRun) {
        Routine *routineArray = (Routine *) globalRoutineArray.array;

        for (i = 0; i < globalRoutineArray.size; ++i) {
            Routine *routine = &routineArray[i];
            switch (routine->state) {
                case STATE_CONTINUE | TYPE_FILE_ROUTINE:
                    if (FileRoutineContinue(routine))
                        break;
                    /* Fall through */
                case STATE_FINISH | TYPE_FILE_ROUTINE:
                case STATE_FAIL | TYPE_FILE_ROUTINE:
                    FileRoutineArrayDel(&globalRoutineArray, routine);
                    break;
                case STATE_CONTINUE | TYPE_DIR_ROUTINE:
                    if (DirectoryRoutineContinue(routine))
                        break;
                    /* Fall through */
                case STATE_FINISH | TYPE_DIR_ROUTINE:
                case STATE_FAIL | TYPE_DIR_ROUTINE:
                    DirectoryRoutineArrayDel(&globalRoutineArray, routine);
                    break;
            }
        }

        globalSelectSleep.tv_usec = 0;
        globalSelectSleep.tv_sec = globalRoutineArray.size ? 0 : 60;

        readyToReadSockets = serverReadSockets;
        readyToWriteSockets = serverWriteSockets;

        if (select((int) serverMaxSocket + 1, &readyToReadSockets, &readyToWriteSockets, NULL, &globalSelectSleep) <
            0) {
            LINEDBG;
            exit(1);
        }

        for (i = 0; i <= serverMaxSocket; ++i) {
            if (FD_ISSET(i, &readyToReadSockets)) {
                if (i == serverListenSocket) {
                    SOCKET clientSocket = platformAcceptConnection(serverListenSocket);
                    if (clientSocket > serverMaxSocket)
                        serverMaxSocket = clientSocket;

                    FD_SET(clientSocket, &serverReadSockets);
                } else {
                    if (handleConnection(i)) {
                        eventSocketCloseInvoke(&i);
                        CLOSE_SOCKET(i);
                        FD_CLR(i, &serverReadSockets);
                    }
                }
            }
        }
    }
}

void serverPoke(void) {
    SOCKET sock;
    struct sockaddr_in ipv4;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        LINEDBG;
        return;
    }

    ipv4.sin_addr.s_addr = inet_addr("127.0.0.1"), ipv4.sin_family = AF_INET, ipv4.sin_port = htons(
            getPort(&serverListenSocket));

    if (connect(sock, (SA *) &ipv4, sizeof(ipv4)) == 0)
        CLOSE_SOCKET(sock);
}