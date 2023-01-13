#ifndef CX_PLATFORM_LINUX
#define CX_PLATFORM_LINUX 1
#endif

#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>
#include <msquic.h>
#include <msquichelper.h>
#include <quic_var_int.h>
#include "H3QPack/H3QPack.h"
#include "H3Parse/H3Parse.h"
#include "H3Table/H3Table.h"

#define min(x, y)  ((x > y) ? (y) : (x))

struct StreamServer
{
    int32_t  pos;
    int32_t  size;
    HQUIC   *stream;
} streamServer = {
    0,
    0,
    NULL
};

void CxPlatLogAssert(
    _In_z_ const char *File,
    _In_ int Line,
    _In_z_ const char *Expr
)
{
}

void
quic_bugcheck(
    _In_z_ const char* File,
    _In_ int Line,
    _In_z_ const char* Expr
)
{
}

//
// The QUIC API/function table returned from MsQuicOpen2. It contains all the
// functions called by the app to interact with MsQuic.
//
const QUIC_API_TABLE *MsQuic;

const QUIC_REGISTRATION_CONFIG RegConfig = { "h3_test", QUIC_EXECUTION_PROFILE_LOW_LATENCY };

HQUIC Registration;
HQUIC Configuration;

QUIC_TLS_SECRETS TlsSecrets;
HQUIC streamControl = NULL;
HQUIC streamDecoder = NULL;
HQUIC streamEncoder = NULL;
HQUIC streamRequest = NULL;

enum H3StreamType
{
    H3StreamControl = 0,
    H3StreamPush,
    H3StreamEncoder,
    H3StreamDecoder
};

typedef enum
{
    H3FrameData = 0,
    H3FrameHeaders,
    H3FramePriority,
    H3FrameCancelPush,
    H3FrameSettings,
    H3FramePushPromise,
    H3FrameGoaway = 7,
    H3FrameUnknown = 0xFF
} H3FrameType;

typedef struct
{
    H3FrameType  type;
    size_t       length;
    size_t       offset;
    uint8_t     *data;
    uint8_t      buffer[H3_BUFFER_MAX_LEN];
    size_t       bufferLen;
} ReceiveBuffer;

ReceiveBuffer receiveBuffer;
uint32_t      reqInsertCount;
const char   *host_str = NULL;
const char   *port_str = NULL;

static void ReleaseField(H3Field *field)
{
    if (field) {
        if (field->free_name && field->name.data) {
            field->free_name = false;
            free(field->name.data);
            field->name.data = NULL;
        }

        if (field->free_value && field->value.data) {
            field->free_value = false;
            free(field->value.data);
            field->value.data = NULL;
        }
    }
}

void ReceiveResponseHeader(ReceiveBuffer *buffer)
{
    int32_t     rc;
    H3Headers   hdr;
    H3Field    *field;
    uint8_t    *last, *end;

    memset(&hdr, 0, sizeof(H3Headers));
    hdr.length = (uint32_t) buffer->length;

    last = buffer->data;
    end = last + hdr.length;

    for ( ;; ) {
        rc = H3ParseHeaders(&hdr, &H3DynamicTable, buffer->data, &last, end);
        if (rc == H3_OK) {
            field = &hdr.field_rep.field;

            printf("H3 received field line \"%s\": \"%s\"\n", field->name.data, field->value.data);
            ReleaseField(field);
        } else {
            ReleaseField(field);
            break;
        }
    }

	printf("H3 parse headers done\n");
}

void ReceiveResponseData(ReceiveBuffer *buffer)
{
    printf("[strm] ReceiveResponseData: %d\n", (int) buffer->offset);
    printf("%*.*s", (int) buffer->offset, (int) buffer->offset, buffer->data);
}

bool ReceiveResponse(QUIC_STREAM_EVENT *Event)
{
    uint16_t            offset;
    uint32_t            i;
    size_t              copyLength;
    QUIC_VAR_INT        value;
    const QUIC_BUFFER  *Buffer;

    for (i = 0; i < Event->RECEIVE.BufferCount; ++i) {
        Buffer = &Event->RECEIVE.Buffers[i];
        offset = 0;

        do {
            if (receiveBuffer.type == H3FrameUnknown) {
                receiveBuffer.type = (H3FrameType) Buffer->Buffer[offset++];
                value = 0;
                QuicVarIntDecode(Buffer->Length - offset, Buffer->Buffer, &offset, &value);
                receiveBuffer.length = value;
                receiveBuffer.data = receiveBuffer.buffer;
            }

            copyLength = min(Buffer->Length - offset, receiveBuffer.length - receiveBuffer.offset);
            memcpy(receiveBuffer.buffer + receiveBuffer.offset, Buffer->Buffer + offset, copyLength);
            offset += copyLength;
            receiveBuffer.offset += copyLength;

            if (receiveBuffer.offset == receiveBuffer.length) {
                printf("[strm] Receiving frame completed, type: %d\n", receiveBuffer.type);
                if (receiveBuffer.type == H3FrameHeaders) {
                    ReceiveResponseHeader(&receiveBuffer);
                } else if (receiveBuffer.type == H3FrameData) {
                    ReceiveResponseData(&receiveBuffer);
                    return true;
                }

                receiveBuffer.type = H3FrameUnknown;
                receiveBuffer.offset = 0;
                receiveBuffer.length = 0;
            }
        } while (offset < Buffer->Length);
    }

    return false;
}

/* 
    QUIC_STREAM_EVENT_START_COMPLETE            = 0,
    QUIC_STREAM_EVENT_RECEIVE                   = 1,
    QUIC_STREAM_EVENT_SEND_COMPLETE             = 2,
    QUIC_STREAM_EVENT_PEER_SEND_SHUTDOWN        = 3,
    QUIC_STREAM_EVENT_PEER_SEND_ABORTED         = 4,
    QUIC_STREAM_EVENT_PEER_RECEIVE_ABORTED      = 5,
    QUIC_STREAM_EVENT_SEND_SHUTDOWN_COMPLETE    = 6,
    QUIC_STREAM_EVENT_SHUTDOWN_COMPLETE         = 7,
    QUIC_STREAM_EVENT_IDEAL_SEND_BUFFER_SIZE    = 8
*/

_Function_class_(QUIC_STREAM_CALLBACK)
QUIC_STATUS
QUIC_API QuicStreamCallback(
    _In_ HQUIC Stream,
    _In_opt_ void *Context,
    _Inout_ QUIC_STREAM_EVENT *Event
)
{
    int32_t  i;

    switch (Event->Type) {
    case QUIC_STREAM_EVENT_RECEIVE:
        printf("[strm][%p] Received\n", Stream);
        if (Stream == streamRequest) {
            printf("[strm][%p] Request stream received\n", Stream);
            if (ReceiveResponse(Event)) { // finished to receive data
                MsQuic->StreamClose(streamRequest);
                MsQuic->StreamClose(streamControl);
                MsQuic->StreamClose(streamDecoder);
                MsQuic->StreamClose(streamEncoder);
                for (i = 0; i < streamServer.size; i++) {
                    MsQuic->StreamClose(streamServer.stream[i]);
                };
            }
        }
        break;
    default:
        printf("[strm][%p] Received stream event: %d\n", Stream, Event->Type);
        break;
    }

    return QUIC_STATUS_SUCCESS;
}

HQUIC CreateStream(HQUIC Connection, bool BiDir)
{
    HQUIC        Stream = NULL;
    QUIC_STATUS  Status;

    if (QUIC_FAILED(Status = MsQuic->StreamOpen(Connection, BiDir ? QUIC_STREAM_OPEN_FLAG_NONE : QUIC_STREAM_OPEN_FLAG_UNIDIRECTIONAL,
        QuicStreamCallback, NULL, &Stream))) {
        return NULL;
    }

    if (QUIC_FAILED(Status = MsQuic->StreamStart(Stream, QUIC_STREAM_START_FLAG_NONE))) {
        return NULL;
    }

    return Stream;
}

bool StreamSend(HQUIC stream, uint8_t *buffer, size_t bufLen, bool last)
{
    QUIC_STATUS  Status;

    // Allocates and builds the buffer to send over the stream.
    //
    QUIC_BUFFER *sendBuffer = (QUIC_BUFFER *) malloc(sizeof(QUIC_BUFFER) + bufLen);
    if (sendBuffer == NULL) {
        return false;
    }

    sendBuffer->Length = (uint32_t) bufLen;
    sendBuffer->Buffer = (uint8_t *) sendBuffer + sizeof(QUIC_BUFFER);
    memcpy(sendBuffer->Buffer, buffer, bufLen);

    if (QUIC_FAILED(Status = MsQuic->StreamSend(stream, sendBuffer, 1, last ? QUIC_SEND_FLAG_FIN : QUIC_SEND_FLAG_NONE, (void *) sendBuffer))) {
        free(sendBuffer);
        return false;
    }

    return true;
}

H3Header Headers[] = {
    { H3StrInit(":method"),     H3StrInit("GET") },
    { H3StrInit(":path"),       H3StrInit("/index.html") },
    { H3StrInit(":scheme"),     H3StrInit("https") },
    { H3StrInit(":authority"),  H3StrInit("") },
    { H3StrInit("user-agent"),  H3StrInit("h3_over_msquic") },
    { H3StrInit("accept"),      H3StrInit("*/*") }
};

bool HttpSendRequest(_In_ HQUIC hRequest, _In_ HQUIC hEncoder)
{
    uint8_t    settingsHdr[8], buffer[H3_BUFFER_MAX_LEN];
    size_t     i, j;
    int32_t    encInsertCount, insertCount, base;
    uint8_t   *p, *last;
    H3Header  *h, *s;

    reqInsertCount = 0;
    insertCount = 0;
    encInsertCount = 0;

    memset(buffer, 0, H3_BUFFER_MAX_LEN);
    p = buffer;
    s = H3StaticTable;

    // encoder instructions
    for (i = 0; i < sizeof(Headers) / sizeof(Headers[0]); i++) {
        h = &Headers[i];

        if (memcmp(s[15].Name.data, h->Name.data, h->Name.len) == 0) {
            for (j = 15; j < 22; j++) {
                if (memcmp(s[j].Value.data, s->Value.data, s->Value.len) == 0) {
                    break;
                }
            }

            if (j == 22) {
                printf("H3 invalid field line, name: %s, value: %s\n", h->Name.data, h->Value.data);
                return false;
            }
        } else if (memcmp(s[1].Name.data, h->Name.data, h->Name.len) == 0) {
            if (h->Value.data[0] != '/' || h->Value.len != 1) {
                p = H3EncodeInsertNR(p, 1, h->Value.data, h->Value.len, NULL);
                reqInsertCount++;
            }
        } else if (memcmp(s[22].Name.data, h->Name.data, h->Name.len) == 0) {
            for (j = 22; j < 24; j++) {
                if (memcmp(s[j].Value.data, h->Value.data, h->Value.len) == 0) {
                    break;
                }
            }

            if (j == 24) {
                printf("H3 unsupported scheme: %s\n", h->Value.data);
                return false;
            }
        } else if (memcmp(s[0].Name.data, h->Name.data, h->Name.len) == 0) {
            h->Value.data = (uint8_t *) host_str;
            h->Value.len = strlen(host_str);
            p = H3EncodeInsertNR(p, 0, h->Value.data, h->Value.len, NULL);
            reqInsertCount++;
        } else if (memcmp(s[95].Name.data, h->Name.data, h->Name.len) == 0) {
            p = H3EncodeInsertNR(p, 95, h->Value.data, h->Value.len, NULL);
            reqInsertCount++;
        } else if (memcmp(s[29].Name.data, h->Name.data, h->Name.len) == 0) {
            for (j = 29; j < 31; j++) {
                if (memcmp(s[j].Value.data, h->Value.data, h->Value.len) == 0) {
                    break;
                }
            }

            if (j == 31) {
                p = H3EncodeInsertNR(p, 31, h->Value.data, h->Value.len, NULL);
                reqInsertCount++;
            }
        }
        // TODO: other insertions with name reference or insertions with literal name
    }

    if (p - buffer > 0) {
        StreamSend(hEncoder, buffer, p - buffer, false);
    }

    memset(settingsHdr, 0, 8);
    memset(buffer, 0, H3_BUFFER_MAX_LEN);

    settingsHdr[0] = H3FrameHeaders;
    p = buffer;

    if (reqInsertCount > 0) {
        encInsertCount = (reqInsertCount % ((4096 / 32) << 1)) + 1;
    }

    p = H3EncodeFieldSectionPrefix(p, encInsertCount, false, 0, NULL);

    base = reqInsertCount - 1;

    // TODO: optimize to improve performance
    for (i = 0; i < sizeof(Headers) / sizeof(Headers[0]); i++) {
        h = &Headers[i];

        if (memcmp(s[15].Name.data, h->Name.data, h->Name.len) == 0) {
            for (j = 15; j < 22; j++) {
                if (memcmp(s[j].Value.data, h->Value.data, h->Value.len) == 0) {
                    p = H3EncodeFieldIFL(p, false, j, NULL);
                    break;
                }
            }

            if (j == 22) {
                printf("H3 invalid field line, name: %s, value: %s\n", h->Name.data, h->Value.data);
                return false;
            }
        } else if (memcmp(s[1].Name.data, h->Name.data, h->Name.len) == 0) {
            p = H3EncodeFieldLFLNR(p, true, base - insertCount, h->Value.data, h->Value.len, NULL);
            if (H3RefInsert(&H3DynamicTable, false, 1, &h->Value) != H3_OK) {
                printf("H3 failed insert literal field line with name reference, index: 1\n");
                return false;
            }
        } else if (memcmp(s[22].Name.data, h->Name.data, h->Name.len) == 0) {
            for (j = 22; j < 24; j++) {
                if (memcmp(s[j].Value.data, h->Value.data, h->Value.len) == 0) {
                    if (j == 22) {
                        printf("H3 http not supported yet\n");
                        return false;
                    }

                    p = H3EncodeFieldIFL(p, false, j, NULL);
                    break;
                }
            }

            if (j == 24) {
                printf("H3 unsupported scheme: %s\n", h->Value.data);
                return false;
            }
        } else if (memcmp(s[0].Name.data, h->Name.data, h->Name.len) == 0) {
            insertCount++;
            h->Value.data = (uint8_t *) host_str;
            h->Value.len = strlen(host_str);
            p = H3EncodeFieldLFLNR(p, true, base - insertCount, h->Value.data, h->Value.len, NULL);
            if (H3RefInsert(&H3DynamicTable, false, 0, &h->Value) != H3_OK) {
                printf("H3 failed insert literal field line with name reference, index: 0\n");
                return false;
            }
        } else if (memcmp(s[95].Name.data, h->Name.data, h->Name.len) == 0) {
            insertCount++;
            p = H3EncodeFieldLFLNR(p, true, base - insertCount, h->Value.data, h->Value.len, NULL);
            if (H3RefInsert(&H3DynamicTable, false, 95, &h->Value) != H3_OK) {
                printf("H3 failed insert literal field line with name reference, index: 95\n");
                return false;
            }
        } else if (memcmp(s[29].Name.data, h->Name.data, h->Name.len) == 0) {
            for (j = 29; j < 31; j++) {
                if (memcmp(s[j].Value.data, h->Value.data, h->Value.len) == 0) {
                    p = H3EncodeFieldIFL(p, false, j, NULL);
                    break;
                }
            }

            if (j == 31) {
                insertCount++;
                p = H3EncodeFieldLFLNR(p, true, base - insertCount, h->Value.data, h->Value.len, NULL);
                if (H3RefInsert(&H3DynamicTable, false, 29, &h->Value) != H3_OK) {
                    printf("H3 failed insert literal field line with name reference, index: 29\n");
                    return false;
                }
            }
        } else {
            // TODO: static & dynamic
            printf("unsupported field line, name: %s, value: %s\n", h->Name.data, h->Value.data);
            return false;
        }
    }

    if (p - buffer > 0) {
        last = p;
        p = QuicVarIntEncode(last - buffer, settingsHdr + 1);

        StreamSend(hRequest, settingsHdr, p - settingsHdr, false);
        StreamSend(hRequest, buffer, last - buffer, true);
    }

    return true;
}

void HttpSend(_In_ HQUIC Connection)
{
    uint8_t data[H3_BUFFER_MAX_LEN] = { 0 };

    streamControl = CreateStream(Connection, false);
    streamEncoder = CreateStream(Connection, false);
    streamDecoder = CreateStream(Connection, false);
    streamRequest = CreateStream(Connection, true);
    data[0] = H3StreamControl;
    StreamSend(streamControl, data, 1, false);
    data[0] = H3StreamEncoder;
    data[1] = H3_ENCODER_INSTRUCTION_CAP | 0x1f;
    memcpy(data + 2, "\xe1\x1f", 2); // Dynamic table size: 4096 (00111111|11000001|000011111)
    StreamSend(streamEncoder, data, 4, false);

    memset(&H3DynamicTable, 0, sizeof(H3HeaderDynamic));
    // TODO: set max size
    H3DynamicTable.capacity = 4096 / 32;
    H3DynamicTable.elts = (H3Header **) malloc(H3DynamicTable.capacity * sizeof(H3Header *));
    if (H3DynamicTable.elts == NULL) {
        return;
    }

    memset(H3DynamicTable.elts, 0, H3DynamicTable.capacity * sizeof(H3Headers *));

    data[0] = H3StreamDecoder;
    StreamSend(streamDecoder, data, 1, false);

    HttpSendRequest(streamRequest, streamEncoder);
}

/* 
    QUIC_CONNECTION_EVENT_CONNECTED                         = 0,
    QUIC_CONNECTION_EVENT_SHUTDOWN_INITIATED_BY_TRANSPORT   = 1,    // The transport started the shutdown process.
    QUIC_CONNECTION_EVENT_SHUTDOWN_INITIATED_BY_PEER        = 2,    // The peer application started the shutdown process.
    QUIC_CONNECTION_EVENT_SHUTDOWN_COMPLETE                 = 3,    // Ready for the handle to be closed.
    QUIC_CONNECTION_EVENT_LOCAL_ADDRESS_CHANGED             = 4,
    QUIC_CONNECTION_EVENT_PEER_ADDRESS_CHANGED              = 5,
    QUIC_CONNECTION_EVENT_PEER_STREAM_STARTED               = 6,
    QUIC_CONNECTION_EVENT_STREAMS_AVAILABLE                 = 7,
    QUIC_CONNECTION_EVENT_PEER_NEEDS_STREAMS                = 8,
    QUIC_CONNECTION_EVENT_IDEAL_PROCESSOR_CHANGED           = 9,
    QUIC_CONNECTION_EVENT_DATAGRAM_STATE_CHANGED            = 10,
    QUIC_CONNECTION_EVENT_DATAGRAM_RECEIVED                 = 11,
    QUIC_CONNECTION_EVENT_DATAGRAM_SEND_STATE_CHANGED       = 12,
    QUIC_CONNECTION_EVENT_RESUMED                           = 13,   // Server-only; provides resumption data, if any.
    QUIC_CONNECTION_EVENT_RESUMPTION_TICKET_RECEIVED        = 14,   // Client-only; provides ticket to persist, if any. 
*/

_Function_class_(QUIC_CONNECTION_CALLBACK)
QUIC_STATUS
QUIC_API
ClientConnectionCallback(
    _In_ HQUIC Connection,
    _In_opt_ void *Context,
    _Inout_ QUIC_CONNECTION_EVENT *Event
)
{
    HQUIC  *tmp;

    switch (Event->Type) {
    case QUIC_CONNECTION_EVENT_CONNECTED:
        WriteSslKeyLogFile("./QuicTlsSecret.log", TlsSecrets);
        //
        // The handshake has completed for the connection.
        //
        printf("[conn][%p] Connected\n", Connection);
        HttpSend(Connection);
        break;
    case QUIC_CONNECTION_EVENT_SHUTDOWN_INITIATED_BY_TRANSPORT:
        //
        // The connection has been shut down by the transport. Generally, this
        // is the expected way for the connection to shut down with this
        // protocol, since we let idle timeout kill the connection.
        //
        if (Event->SHUTDOWN_INITIATED_BY_TRANSPORT.Status == QUIC_STATUS_CONNECTION_IDLE) {
            printf("[conn][%p] Successfully shut down on idle.\n", Connection);
        } else {
            printf("[conn][%p] Shut down by transport, 0x%x\n", Connection, Event->SHUTDOWN_INITIATED_BY_TRANSPORT.Status);
        }
        break;
    case QUIC_CONNECTION_EVENT_SHUTDOWN_COMPLETE:
        printf("[conn][%p] All done\n", Connection);
        if (!Event->SHUTDOWN_COMPLETE.AppCloseInProgress) {
            MsQuic->ConnectionClose(Connection);
        }
        break;
    case QUIC_CONNECTION_EVENT_PEER_STREAM_STARTED:
        printf("[conn][%p] Peer Stream Started [%p]\n", Connection, Event->PEER_STREAM_STARTED.Stream);
        if (Event->PEER_STREAM_STARTED.Flags & QUIC_STREAM_OPEN_FLAG_UNIDIRECTIONAL) {
            MsQuic->SetCallbackHandler(Event->PEER_STREAM_STARTED.Stream, (void *) QuicStreamCallback, NULL);
            if (streamServer.stream == NULL) {
                streamServer.stream = (HQUIC *) malloc(3 * sizeof(HQUIC));
                if (streamServer.stream == NULL) {
                    MsQuic->ConnectionClose(Connection);
                    break;
                }

                streamServer.pos = 0;
                streamServer.size = 3;
            } else if (streamServer.pos == streamServer.size) {
                tmp = (HQUIC *) realloc(streamServer.stream, streamServer.size + 3);
                if (tmp == NULL) {
                    MsQuic->ConnectionClose(Connection);
                    break;
                }

                streamServer.stream = tmp;
                streamServer.size += 3;
            }

            streamServer.stream[streamServer.pos++] = Event->PEER_STREAM_STARTED.Stream;
        } else {
            MsQuic->StreamClose(Event->PEER_STREAM_STARTED.Stream);
        }

        break;
    default:
        printf("[conn][%p] Received connection event: %d\n", Connection, Event->Type);
        break;

    }

    return QUIC_STATUS_SUCCESS;
}

int main(int argc, char **argv)
{
    int                     opt;
    uint32_t                i;
    long                    port;
    QUIC_STATUS             Status = QUIC_STATUS_SUCCESS;
    QUIC_SETTINGS           Settings = { 0 };
    QUIC_CREDENTIAL_CONFIG  CredConfig;
    HQUIC                   Connection = NULL;
    const QUIC_BUFFER       Alpn = { sizeof("h3") - 1, (uint8_t *) "h3" };

    while ((opt = getopt(argc, argv, "h:p:")) != -1) {
        switch (opt) {
        case 'h':
            host_str = optarg;
            break;
        case 'p':
            port_str = optarg;
            break;
        default:
            printf("Usage: %s -h host -p port\n", argv[0]);
            return -1;
        }
    }

    if (host_str == NULL) {
        host_str = "demo.h3test.com";
    }

    if (port_str == NULL) {
        port_str = "443";
    }

	port = strtol(port_str, NULL, 10);
    if (port < 0 || port > 65535 || errno == ERANGE) {
        printf("Invalid port: %s\n", port_str);
        return -2;
	}

    memset(&receiveBuffer, 0, sizeof(receiveBuffer));
    receiveBuffer.type = H3FrameUnknown;
    receiveBuffer.bufferLen = H3_BUFFER_MAX_LEN;

    if (QUIC_FAILED(Status = MsQuicOpen2(&MsQuic))) {
        printf("MsQuicOpen2 failed, 0x%x!\n", Status);
        return Status;
    }

    if (QUIC_FAILED(Status = MsQuic->RegistrationOpen(&RegConfig, &Registration))) {
        printf("RegistrationOpen failed, 0x%x!\n", Status);
        goto Error;
    }

    //
    // Configures the client's idle timeout.
    //
    Settings.IdleTimeoutMs = 1000;                        
    Settings.IsSet.IdleTimeoutMs = TRUE;
    Settings.PeerBidiStreamCount = 1000;
    Settings.IsSet.PeerBidiStreamCount = TRUE;
    Settings.PeerUnidiStreamCount = 3;
    Settings.IsSet.PeerUnidiStreamCount = TRUE;

    memset(&CredConfig, 0, sizeof(CredConfig));
    CredConfig.Type = QUIC_CREDENTIAL_TYPE_NONE;
    CredConfig.Flags = QUIC_CREDENTIAL_FLAG_CLIENT;
    CredConfig.Flags |= QUIC_CREDENTIAL_FLAG_NO_CERTIFICATE_VALIDATION;   // do not check certificate

    if (QUIC_FAILED(Status = MsQuic->ConfigurationOpen(Registration, &Alpn, 1, &Settings, sizeof(Settings), NULL, &Configuration))) {
        printf("ConfigurationOpen failed, 0x%x!\n", Status);
        goto Error;
    }

    if (QUIC_FAILED(Status = MsQuic->ConfigurationLoadCredential(Configuration, &CredConfig))) {
        printf("ConfigurationLoadCredential failed, 0x%x!\n", Status);
        goto Error;
    }

    //
    // Allocate a new connection object.
    //
    if (QUIC_FAILED(Status = MsQuic->ConnectionOpen(Registration, ClientConnectionCallback, NULL, &Connection))) {
        printf("ConnectionOpen failed, 0x%x!\n", Status);
        goto Error;
    }

    Status = MsQuic->SetParam(Connection, QUIC_PARAM_CONN_TLS_SECRETS, sizeof(TlsSecrets), &TlsSecrets);

    //
    // Start the connection to the server.
    //
    if (QUIC_FAILED(Status = MsQuic->ConnectionStart(Connection, Configuration, QUIC_ADDRESS_FAMILY_UNSPEC, host_str, port))) {
        printf("ConnectionStart failed, 0x%x!\n", Status);
        goto Error;
    }

Error:
    if (QUIC_FAILED(Status) && Connection != NULL) {
        MsQuic->ConnectionClose(Connection);
    }

    if (MsQuic != NULL) {
        if (Configuration != NULL) {
            MsQuic->ConfigurationClose(Configuration);
        }
        if (Registration != NULL) {
            //
            // This will block until all outstanding child objects have been
            // closed.
            //
            MsQuic->RegistrationClose(Registration);
        }
        MsQuicClose(MsQuic);
    }

    if (H3DynamicTable.elts) {
        for (i = 0; i < H3DynamicTable.nelts; i++) {
            free(H3DynamicTable.elts[i]);
        }
        free(H3DynamicTable.elts);
    }

    if (streamServer.stream) {
        free(streamServer.stream);
    }

    return 0;
}
