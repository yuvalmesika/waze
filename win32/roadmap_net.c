/* roadmap_net.c - Network interface for the RoadMap application.
 *
 * LICENSE:
 *
 *   Copyright 2005 Ehud Shabtai
 *
 *   Based on an implementation by Pascal F. Martin.
 *   This file is part of RoadMap.
 *
 *   RoadMap is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   RoadMap is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with RoadMap; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * SYNOPSYS:
 *
 *   See roadmap_net.h
 */

#include <windows.h>

#include <winsock.h>
#include <Wininet.h>

#if ((defined UNDER_CE)&& !(defined EMBEDDED_CE))
   #define INITGUID
   #include "initguid.h"
   #include <connmgr.h>
   #include <connmgr_proxy.h>

#endif   // UNDER_CE
#include <ctype.h>

#include "../roadmap.h"
#include "../roadmap_config.h"
#include "../roadmap_net_mon.h"
#include "../roadmap_net.h"
#include "../roadmap_start.h"
#include "../websvc_trans/websvc_address.h"
#include "../websvc_trans/web_date_format.h"

#include "roadmap_net_defs.h"
#include "..\roadmap_http_comp.h"

#define CONNECT_TIMEOUT_SEC 5
#define F_SETFL         1
#define F_SETFD         2
#define F_GETFL         3
#define _O_NONBLOCK     0x0004
#define O_NONBLOCK      _O_NONBLOCK
static CRITICAL_SECTION s_cs;


static RoadMapConfigDescriptor RoadMapConfigNetCompressEnabled =
                        ROADMAP_CONFIG_ITEM( "Network", "Compression Enabled");
//static void connect_callback (RoadMapSocket s, RoadMapNetData *data);
typedef struct roadmap_net_data_t {
   RoadMapNetConnectCallback callback;
   void *context;
   char packet[255];
} RoadMapNetData;

//static void io_connect_callback (RoadMapIO *io);
//static void connect_callback (RoadMapSocket s, RoadMapNetData *data);
static int roadmap_net_send_ssl (RoadMapSocket s, const void *data, int length, int wait);
//static void *roadmap_net_connect_internal (const char *protocol, const char *name, const char *resolved_name,
//                                                   time_t update_time,
//                                                   int default_port,
//                                                   int async,
//                                                   int flags,
//                                                   RoadMapNetConnectCallback callback,
//                                                   void *context,
//                                                   RoadMapIO *reuse_connect_io,
//                                                   int num_retries);
static struct servent* roadmap_net_getservbyname( const char *name, const char *protocol );
static RoadMapSocket create_socket (const char *protocol,
                                    const char *name,
                                    int default_port,
                                    struct sockaddr *saddr);
static const char* GetProxyAddress();

static BOOL RoadMapNetCompressEnabled = FALSE;

int roadmap_net_get_fd(RoadMapSocket s) {
   return s;
}

#if ((defined UNDER_CE)&& !(defined EMBEDDED_CE))
static HANDLE           RoadMapConnection = NULL;




struct roadmap_socket_t {
   int s;
   int is_compressed;
   int is_secured;
   RoadMapHttpCompCtx compress_ctx;
   void  *ssl_ctx;
   RoadMapIO   *connect_io;
};
typedef enum tagconnection_status
{
   constt_unknown,
   constt_connected,
   constt_pending,
   constt_failed

}  connection_status;


static int  RoadMapNetNumConnects;

//static void *roadmap_net_connect_internal (const char *protocol, const char *name, const char *resolved_name,
//                                         time_t update_time,
//                                         int default_port,
//                                         int async,
//                                         int flags,
//                                         RoadMapNetConnectCallback callback,
//                                         void *context,
//                                         RoadMapIO *reuse_connect_io,
//                                         int          num_retries) {
//
//   char            server_url  [ WSA_SERVER_URL_MAXSIZE   + 1];
//   char            resolved_server_url  [ WSA_SERVER_URL_MAXSIZE   + 1];
//   char            service_name[ WSA_SERVICE_NAME_MAXSIZE + 1];
//   int             server_port;
//   const char*     proxy_address = GetProxyAddress();
//   char            packet[512];
//   char            update_since[WDF_MODIFIED_HEADER_SIZE + 1];
//   RoadMapSocket   res_socket, temp_socket;
//   const char *   req_type = "GET";
//   struct sockaddr addr;
//   RoadMapNetData *data = NULL;
//   RoadMapIO      *io;
//
//   if( strncmp( protocol, "http", 4) != 0) {
//      temp_socket = create_socket(protocol, name, default_port, &addr);
//
//      if(ROADMAP_INVALID_SOCKET == temp_socket) return NULL;
//      
//         res_socket = temp_socket;
//
//      if (async) {
//         data = (RoadMapNetData *)malloc(sizeof(RoadMapNetData));
//         data->packet[0] = '\0';
//      }
//   } else {
//      // HTTP Connection, using system configuration for Proxy
//
//      WDF_FormatHttpIfModifiedSince (update_time, update_since);
//      if (!strcmp(protocol, "http_post")) req_type = "POST";
//
//      if( !WSA_ExtractParams( name,          //   IN        -   Web service full address (http://...)
//                              server_url,    //   OUT,OPT   -   Server URL[:Port]
//                              &server_port,  //   OUT,OPT   -   Server Port
//                             service_name)) //   OUT,OPT   -   Web service name
//      {
//         roadmap_log( ROADMAP_ERROR, "roadmap_net_connect(HTTP) - Failed to extract information from '%s'", name);
//         return ROADMAP_INVALID_SOCKET;
//      }
//      
//      if( !WSA_ExtractParams( resolved_name,          //   IN        -   Web service full address (http://...)
//                             resolved_server_url,    //   OUT,OPT   -   Server URL[:Port]
//                             &server_port,  //   OUT,OPT   -   Server Port
//                             service_name)) //   OUT,OPT   -   Web service name
//      {
//         roadmap_log( ROADMAP_ERROR, "roadmap_net_connect(HTTP) - Failed to extract information from '%s'", resolved_name);
//         return ROADMAP_INVALID_SOCKET;
//      }
//
//      if (proxy_address) {
//         int   proxy_port  = server_port;
//         char* port_offset = strchr(proxy_address, ':');
//         if (port_offset) proxy_port = atoi(port_offset + 1);
//
//         temp_socket = create_socket("tcp", proxy_address, proxy_port, &addr);
//
//         sprintf(packet,
//                 "%s %s HTTP/1.0\r\n"
//                 "Host: %s\r\n"
//                 "User-Agent: FreeMap/%s\r\n"
//                 "%s"
//                 "%s",
//                 req_type, name, server_url, roadmap_start_version(),
//                 TEST_NET_COMPRESS( flags ) ? "Accept-Encoding: gzip, deflate\r\n" : "",
//                 update_since); 
//      } else {
//
//         temp_socket = create_socket("tcp", resolved_server_url, server_port, &addr);
//
//         sprintf(packet,
//                 "%s %s HTTP/1.0\r\n"
//                 "Host: %s\r\n"
//                 "User-Agent: FreeMap/%s\r\n"
//                 "%s"
//                 "%s",
//                 req_type, service_name, server_url, roadmap_start_version(),
//                 TEST_NET_COMPRESS( flags ) ? "Accept-Encoding: gzip, deflate\r\n" : "",
//                 update_since);
//         
//      }
//
//      if(ROADMAP_INVALID_SOCKET == temp_socket) return ROADMAP_INVALID_SOCKET;
//      
//         res_socket = temp_socket;
//      
//      res_socket->is_compressed = TEST_NET_COMPRESS( flags );
//      res_socket->is_secured = (server_port == 443); //TODO: consider different check
//
//      if (async) {
//         data = (RoadMapNetData *)malloc(sizeof(RoadMapNetData));
//         strncpy_safe(data->packet, packet, sizeof(data->packet));
//      }
//   }
//   
//   if (!reuse_connect_io)
//   io = malloc(sizeof(RoadMapIO));
//   else
//      io = reuse_connect_io;
//   
//   io->os.socket = res_socket;
//   io->os.socket->connect_io = io;
//
//   if (async) {
//      data->callback = callback;
//      data->context = context;
//
//      io->subsystem = ROADMAP_IO_NET;
//      io->context = data;
//
//      io->retry_params.num_retries = num_retries;
//      io->retry_params.protocol = strdup(protocol);
//      io->retry_params.name = strdup(name);
//      io->retry_params.resolved_name = strdup(resolved_name);
//      io->retry_params.update_time = update_time;
//      io->retry_params.default_port = default_port;
//      io->retry_params.flags = flags;
//      io->retry_params.callback = callback;
//      io->retry_params.context = context;
//
////#ifdef IPHONE
////      if (roadmap_main_async_connect(io, &addr, io_connect_callback) == -1)
////#else
////      if (create_async_connection(io, &addr) == -1)
////#endif
////      {
////         free(data);
////         roadmap_net_close(res_socket);
////         return NULL;
////      }
//
//#ifdef IPHONE
//      RoadMapNetNumConnects++;
//
//      if (RoadMapNetNumConnects == 1) {
//         roadmap_main_set_periodic(CONNECT_TIMEOUT_SEC * 1000 /2, check_connect_timeout);
//      }
//#endif
//
//   } else {
//
//      /* Blocking connect */
//      if (create_connection(res_socket, &addr) == -1) {
//         roadmap_net_close(res_socket);
//         return NULL;
//      }
//
//      if( strncmp(protocol, "http", 4) == 0) {
//         if(-1 == roadmap_net_send(res_socket, packet, (int)strlen(packet), 1)) {
//            roadmap_log( ROADMAP_ERROR, "roadmap_net_connect(HTTP) - Failed to send the 'POST' packet");
//            roadmap_net_close(res_socket);
//            return NULL;
//         }
//      }
//   }
//
//   return io;
//}

//static void io_connect_callback (RoadMapIO *io) {
//
//   RoadMapNetData *data = io->context;
//   RoadMapSocket s;
//
//   s = io->os.socket;
//   if (s != ROADMAP_INVALID_SOCKET) {
//      if (io->retry_params.protocol && io->retry_params.protocol[0]) {
//         free(io->retry_params.protocol);
//      }
//      if (io->retry_params.name && io->retry_params.name[0]) {
//         free(io->retry_params.name);
//      }
//
//      if (io->retry_params.resolved_name && io->retry_params.resolved_name[0]) {
//         free(io->retry_params.resolved_name);
//      }
//      
//      roadmap_main_remove_input(io);
//   }
//
//   if (!s->is_secured) {
//      connect_callback(s, data);
//   }
//#ifdef __SSL__
//   else {
//      roadmap_ssl_open(s, data, on_ssl_open_callback);
//   }
//#endif
//}
//static RoadMapSocket create_socket (const char *protocol,
//                                    const char *name,
//                                    int default_port,
//                                    struct sockaddr *saddr) {
//   int   s;
//   char *hostname;
//   char *separator = strchr (name, ':');
//
//   struct hostent *host = NULL;
//   struct sockaddr_in *addr = (struct sockaddr_in *)saddr;
//
//   RoadMapSocket sock_struct;
//
//   roadmap_net_mon_connect ();
//
//   addr->sin_family = AF_INET;
//
//   hostname = strdup(name);
//   roadmap_check_allocated(hostname);
//
//   if (separator != NULL) {
//
//      struct servent *service = NULL;
//
//#ifndef IPHONE
//      service = roadmap_net_getservbyname( separator+1, "tcp" );
//#endif
//
//      if (service == NULL) {
//
//         if (isdigit(separator[1])) {
//
//            addr->sin_port = htons(atoi(separator+1));
//
//            if (addr->sin_port == 0) {
//               roadmap_log (ROADMAP_ERROR, "invalid port in '%s'", name);
//               goto connection_failure;
//            }
//
//         } else {
//            roadmap_log (ROADMAP_ERROR, "invalid service in '%s'", name);
//            goto connection_failure;
//         }
//
//      } else {
//         addr->sin_port = service->s_port;
//      }
//
//      *(strchr(hostname, ':')) = 0;
//
//
//   } else {
//      addr->sin_port = htons(default_port);
//   }
//
//   if (!isdigit(hostname[0])) {
//      roadmap_log(ROADMAP_INFO, "Calling gethostbyname:%s", hostname);
//      host = gethostbyname(hostname);
//   }
//
//   if (host == NULL) {
//
//      if (isdigit(hostname[0])) {
//         addr->sin_addr.s_addr = inet_addr(hostname);
//         if (addr->sin_addr.s_addr == INADDR_NONE) {
//            roadmap_log (ROADMAP_ERROR, "invalid IP address '%s'", hostname);
//            goto connection_failure;
//         }
//      } else {
//         roadmap_log (ROADMAP_ERROR, "invalid host name '%s'", hostname);
//         goto connection_failure;
//      }
//
//   } else {
//      memcpy (&addr->sin_addr, host->h_addr, host->h_length);
//   }
//
//   if (strcmp (protocol, "udp") == 0) {
//      s = socket (PF_INET, SOCK_DGRAM, 0);
//   } else if (strcmp (protocol, "tcp") == 0) {
//      s = socket (PF_INET, SOCK_STREAM, 0);
//   } else {
//      roadmap_log (ROADMAP_ERROR, "unknown protocol %s", protocol);
//      goto connection_failure;
//   }
//
//   if (s < 0) {
//      roadmap_log (ROADMAP_ERROR, "cannot create socket, errno = %d", GetLastError );
//      goto connection_failure;
//   }
//
//   sock_struct = (RoadMapSocket) calloc(sizeof(struct roadmap_socket_t), 1);
//   sock_struct->s = s;
//
//   free(hostname);
//
//   return sock_struct;
//
//connection_failure:
//
//   free(hostname);
//   roadmap_net_mon_disconnect();
//   roadmap_net_mon_error("Error connecting.");
//   return ROADMAP_INVALID_SOCKET;
//}


//static int create_connection (RoadMapSocket s, struct sockaddr *addr) {
//
//   if (connect (s->s, addr, sizeof(*addr)) < 0) {
//
//      return -1;
//   }
//
//   return 0;
//}


/*static void check_connect_timeout (void) {
   RoadMapIO *io;

   time_t timeout = time(NULL) - CONNECT_TIMEOUT_SEC;
	RoadMapNetData *data = io->context;
   /*while ((io = roadmap_main_output_timedout(timeout))) {
      RoadMapNetData *data = io->context;
      RoadMapSocket s = io->os.socket;
      RoadMapIO retry_io = *io;
      RoadMapIO *reuse_connect_io;
      char *protocol = strdup(retry_io.retry_params.protocol);
      char *name = strdup(retry_io.retry_params.name);
      char *resolved_name = strdup(retry_io.retry_params.resolved_name);
      
      if (retry_io.retry_params.protocol && retry_io.retry_params.protocol[0]) {
         free(retry_io.retry_params.protocol);
      }
      if (retry_io.retry_params.name && retry_io.retry_params.name[0]) {
         free(retry_io.retry_params.name);
      }
      if (retry_io.retry_params.resolved_name && retry_io.retry_params.resolved_name[0]) {
         free(retry_io.retry_params.resolved_name);
      }

      roadmap_log(ROADMAP_ERROR, "Connect time out (%d)", s->s);
      reuse_connect_io = s->connect_io;
      s->connect_io = NULL;
      roadmap_main_remove_input(io);
      roadmap_net_close(s);      

      if (retry_io.retry_params.num_retries < 2) {
         RoadMapNetNumConnects--;
         retry_io.retry_params.num_retries++;
         roadmap_log(ROADMAP_ERROR, "Retrying connection (retry # %d)", retry_io.retry_params.num_retries);
         if (ROADMAP_INVALID_SOCKET != roadmap_net_connect_internal (protocol, name, resolved_name,
                                                                     retry_io.retry_params.update_time,
                                                                     retry_io.retry_params.default_port,
                                                                     TRUE,
                                                                     retry_io.retry_params.flags,
                                                                     retry_io.retry_params.callback,
                                                                     retry_io.retry_params.context,
                                                                     reuse_connect_io,
                                                                     retry_io.retry_params.num_retries)) {
            free(protocol);
            free(name);
            free(resolved_name);
            free(data);
            continue;
         }
      }
      
      free(protocol);
      free(name);
      free(resolved_name);
      connect_callback(ROADMAP_INVALID_SOCKET, data);
   }*/
/*
   connect_callback(ROADMAP_INVALID_SOCKET, data);
}
*/


//static void connect_callback (RoadMapSocket s, RoadMapNetData *data) {
//   RoadMapNetConnectCallback callback = data->callback;
//   void *context = data->context;
//
//   RoadMapNetNumConnects--;
//
//   if (RoadMapNetNumConnects == 0) {
//      roadmap_main_remove_periodic(check_connect_timeout);
//   }
//   
//   if ((s != ROADMAP_INVALID_SOCKET) && *data->packet) {
//      if (!s->is_secured) {
//         if( -1 == roadmap_net_send(s, data->packet,
//                                    (int)strlen(data->packet), 1)) {
//            roadmap_log( ROADMAP_ERROR, "roadmap_net callback (HTTP) - Failed to send the 'POST' packet");
//            roadmap_net_close(s);
//            s = ROADMAP_INVALID_SOCKET;
//         }
//      } else {
//         if( -1 == roadmap_net_send_ssl(s, data->packet,
//                                    (int)strlen(data->packet), 1)) {
//            roadmap_log( ROADMAP_ERROR, "roadmap_net callback (HTTP) - Failed to send the 'POST' packet");
//            roadmap_net_close(s);
//            s = ROADMAP_INVALID_SOCKET;
//         }
//      }
//
//      
//   }
//
//   free(data);
//   
//   if (s == ROADMAP_INVALID_SOCKET)
//       (*callback) (s, context, err_net_failed);
//   else
//      (*callback) (s, context, succeeded);   
//}

static connection_status get_connection_status( BOOL* try_to_re_create_object, roadmap_result* res)
{
   DWORD    dwStatus = 0;
   HRESULT  hr       = S_OK;
   static BOOL first_time = TRUE;

   (*try_to_re_create_object) = FALSE;

   if( res)
      (*res) = succeeded;

   if( !RoadMapConnection)
   {
      if (!first_time)
         roadmap_log( ROADMAP_ERROR, "WIN32::roadmap_net::ConnMgrStatus() - Invalid pointer");
      first_time = FALSE;

      if( res)
         (*res) = err_invalid_argument;
      return constt_failed;
   }

   hr = ConnMgrConnectionStatus( RoadMapConnection, &dwStatus);
   if( FAILED( hr))
   {
      roadmap_log( ROADMAP_ERROR, "WIN32::roadmap_net::ConnMgrStatus() - 'WIN32::ConnMgrConnectionStatus()' had failed: 0x%08X", hr);
      if( res)
         (*res) = err_net_failed;

      (*try_to_re_create_object) = TRUE;
      return constt_failed;
   }

   switch( dwStatus)
   {
      case CONNMGR_STATUS_UNKNOWN:
      {
         roadmap_log( ROADMAP_ERROR, "WIN32::roadmap_net::ConnMgrStatus() - UNKNOWN status");
         if( res)
            (*res) = err_net_failed;

         return constt_unknown;
      }

      case CONNMGR_STATUS_CONNECTED:
         roadmap_log( ROADMAP_DEBUG, "WIN32::roadmap_net::ConnMgrStatus() - CONNECTION-MANAGEER - CONNECTED!");
         return constt_connected;

      case CONNMGR_STATUS_CONNECTIONLINKFAILED:
      case CONNMGR_STATUS_CONNECTIONFAILED:
         (*try_to_re_create_object) = TRUE;
         // Roll down to next switch...

      case CONNMGR_STATUS_EXCLUSIVECONFLICT:
      case CONNMGR_STATUS_DISCONNECTED:
      case CONNMGR_STATUS_CONNECTIONCANCELED:
      case CONNMGR_STATUS_CONNECTIONDISABLED:
      case CONNMGR_STATUS_NOPATHTODESTINATION:
      case CONNMGR_STATUS_PHONEOFF:
      case CONNMGR_STATUS_NORESOURCES:
      case CONNMGR_STATUS_AUTHENTICATIONFAILED:
      {
         roadmap_log( ROADMAP_ERROR, "WIN32::roadmap_net::ConnMgrStatus() - Status %s (0x%02X)", ConnMgr_GetStatusString( dwStatus), dwStatus);
         if( res)
            (*res) = err_net_failed;

         if( (CONNMGR_STATUS_NOPATHTODESTINATION == dwStatus) && res)
            (*res) = err_net_no_path_to_destination;

         return constt_failed;
      }

      case CONNMGR_STATUS_WAITINGDISCONNECTION:
      case CONNMGR_STATUS_WAITINGCONNECTIONABORT:
      case CONNMGR_STATUS_WAITINGFORPATH:
      case CONNMGR_STATUS_WAITINGFORPHONE:
      case CONNMGR_STATUS_WAITINGCONNECTION:
      case CONNMGR_STATUS_WAITINGFORRESOURCE:
      case CONNMGR_STATUS_WAITINGFORNETWORK:
      case CONNMGR_STATUS_SUSPENDED:
      {
         if( res)
            (*res) = err_net_request_pending;
         roadmap_log( ROADMAP_DEBUG, "WIN32::roadmap_net::ConnMgrStatus() - Status is pending: %s (0x%02X)", ConnMgr_GetStatusString( dwStatus), dwStatus);
         return constt_pending;
      }
   }

   roadmap_log( ROADMAP_ERROR, "WIN32::roadmap_net::ConnMgrStatus() - Unrecognized status: %d (0x%02X)", dwStatus, dwStatus);
   return constt_unknown;
}

static BOOL is_connected__internal( roadmap_result* res)
{
   CONNMGR_CONNECTIONINFO  ConnInfo;
   connection_status       status;
   BOOL                    try_to_re_create_object;
   HRESULT                 hr       = 0;
   int                     nIndex   = 0;

   if( res)
      (*res) = succeeded;

   status = get_connection_status( &try_to_re_create_object, res);

   if( constt_connected == status)
      return TRUE;

   if( constt_pending == status)
      return FALSE;

   if( RoadMapConnection)
   {
      if( (constt_failed == status) && try_to_re_create_object)
      {
         roadmap_log( ROADMAP_WARNING, "WIN32::roadmap_net::is_connected() - Destroying 'ConnMgr' object due to status error");
         ConnMgrReleaseConnection(RoadMapConnection, 1);
         RoadMapConnection = NULL;
      }
      else
      {
         roadmap_log( ROADMAP_WARNING, "WIN32::roadmap_net::is_connected() - Not connected");
         return FALSE;
      }
   }

   ZeroMemory(&ConnInfo, sizeof(ConnInfo));
   ConnInfo.cbSize      = sizeof(ConnInfo);
   ConnInfo.dwParams    = CONNMGR_PARAM_GUIDDESTNET;
   ConnInfo.dwFlags     = CONNMGR_FLAG_PROXY_HTTP;
   ConnInfo.dwPriority  = CONNMGR_PRIORITY_USERINTERACTIVE;
   ConnInfo.guidDestNet = IID_DestNetInternet;
   ConnInfo.bExclusive  = FALSE;
   ConnInfo.bDisabled   = FALSE;

   if( FAILED( ConnMgrEstablishConnection( &ConnInfo, &RoadMapConnection)))
   {
      if( res)
         (*res) = err_net_failed;

      roadmap_log( ROADMAP_ERROR, "WIN32::roadmap_net::is_connected() - 'ConnMgrEstablishConnection()' FAILED");
      return FALSE;
   }

   roadmap_log( ROADMAP_DEBUG, "WIN32::roadmap_net::is_connected() - 'ConnMgrEstablishConnection()' SUCCEEDED");
   return (constt_connected == get_connection_status( &try_to_re_create_object, res));
}

static BOOL is_connected( roadmap_result* res)
{
   BOOL bool_res;

   EnterCriticalSection( &s_cs);
   bool_res = is_connected__internal( res);
   LeaveCriticalSection( &s_cs);

   return bool_res;
}

BOOL roadmap_net_is_connected()
{ return is_connected( NULL);}

const char* GetProxyAddress__internal()
{
   static BOOL          already_done   = FALSE;
   static const char*   proxy_address  = NULL;
   static char          proxy_address_buffer[CMPROXY_PROXYSERVER_MAXSIZE+1];
   PROXY_CONFIG         ProxyInfo;
   HRESULT              hr = S_OK;

   if( already_done)
      return proxy_address;

   proxy_address  = NULL;

   if( !RoadMapConnection)
   {
      roadmap_log( ROADMAP_ERROR, "roadmap_net::GetProxyAddress() - 'RoadMapConnection' handle is NULL!");
      return NULL;
   }

   ZeroMemory(&ProxyInfo, sizeof(ProxyInfo));

   ProxyInfo.dwType = CONNMGR_FLAG_PROXY_HTTP | CONNMGR_FLAG_PROXY_SOCKS4 | CONNMGR_FLAG_PROXY_SOCKS5;
   hr = ConnMgrProviderMessage(  RoadMapConnection,
                                 &IID_ConnPrv_IProxyExtension,
                                 NULL, 0, 0,
                                 (PBYTE)&ProxyInfo,
                                 sizeof(ProxyInfo));
   if( FAILED(hr))
   {
      if( E_NOINTERFACE == hr)
      {
         roadmap_log( ROADMAP_DEBUG, "roadmap_net::GetProxyAddress() - No proxy is configured");
         already_done = TRUE;
      }
      else
         roadmap_log( ROADMAP_ERROR, "roadmap_net::GetProxyAddress() - '::ConnMgrProviderMessage()' failed with error: 0x%08X", hr);

      return NULL;
   }

   if( !ProxyInfo.szProxyServer[0])
   {
      roadmap_log( ROADMAP_ERROR, "roadmap_net::GetProxyAddress() - Although 'ConnMgrProviderMessage()' succeeded the returned proxy-name is an empty-string!");
      already_done = TRUE;
      return NULL;
   }

   proxy_address_buffer[CMPROXY_PROXYSERVER_MAXSIZE] = '\0';
   if( wcstombs( proxy_address_buffer, ProxyInfo.szProxyServer, CMPROXY_PROXYSERVER_MAXSIZE) < 1)
   {
      roadmap_log( ROADMAP_ERROR, "roadmap_net::GetProxyAddress() - Failed to convert proxy name from UNICODE to ASCII");
      return NULL;
   }

   proxy_address  = proxy_address_buffer;
   already_done   = TRUE;

   roadmap_log( ROADMAP_DEBUG, "roadmap_net::GetProxyAddress() - Found proxy '%s'", proxy_address);
   return proxy_address;
}

#else

#ifndef     CMPROXY_PROXYSERVER_MAXSIZE
   #define  CMPROXY_PROXYSERVER_MAXSIZE 256
#endif   // CMPROXY_PROXYSERVER_MAXSIZE

const char* GetProxyAddress__internal()
{
   static BOOL                   already_done   = FALSE;
   static const char*            proxy_address  = NULL;
   static char                   proxy_address_buffer[CMPROXY_PROXYSERVER_MAXSIZE+1];
   INTERNET_PER_CONN_OPTION_LIST List;
   INTERNET_PER_CONN_OPTION      Option;
   DWORD                         dwSize = sizeof(INTERNET_PER_CONN_OPTION_LIST);

#ifdef EMBEDDED_CE
   return NULL;
#else
   if( already_done)
      return proxy_address;

   List.dwSize          = dwSize;
   List.pszConnection   = NULL;
   List.dwOptionCount   = 1;
   List.dwOptionError   = 0;
   List.pOptions        = &Option;
   Option.dwOption      = INTERNET_PER_CONN_PROXY_SERVER;

   if( !InternetQueryOption( NULL, INTERNET_OPTION_PER_CONNECTION_OPTION, &List, &dwSize))
   {
      roadmap_log( ROADMAP_ERROR, "roadmap_net::GetProxyAddress() - 'InternetQueryOption()' had failed; Last error: %d", GetLastError());
      return NULL;
   }

   proxy_address  = NULL;
   already_done   = TRUE;

   if( !Option.Value.pszValue  || !(*(Option.Value.pszValue)))
   {
      roadmap_log( ROADMAP_DEBUG, "roadmap_net::GetProxyAddress() - No proxy was found");
      return NULL;
   }

   proxy_address_buffer[CMPROXY_PROXYSERVER_MAXSIZE] = '\0';
   if( wcstombs( proxy_address_buffer, Option.Value.pszValue, CMPROXY_PROXYSERVER_MAXSIZE) < 1)
   {
      roadmap_log( ROADMAP_ERROR, "roadmap_net::GetProxyAddress() - Failed to convert proxy name from UNICODE to ASCII");
      return NULL;
   }

   proxy_address  = proxy_address_buffer;
   roadmap_log( ROADMAP_DEBUG, "roadmap_net::GetProxyAddress() - Found proxy '%s'", proxy_address);
   return proxy_address;
#endif
}
#endif   // UNDER_CE

const char* GetProxyAddress()
{
   const char* proxy = NULL;

   EnterCriticalSection( &s_cs);
   proxy = GetProxyAddress__internal();
   LeaveCriticalSection( &s_cs);

   return proxy;
}
#ifdef EMBEDDED_CE
static BOOL is_connected( roadmap_result* res)
{
   BOOL bool_res;
   
   EnterCriticalSection( &s_cs);
   //TEMP_AVI bool_res = is_connected__internal( res);
   LeaveCriticalSection( &s_cs);
   
   return bool_res;
}

BOOL roadmap_net_is_connected()
{ return is_connected( NULL);}
#endif
static RoadMapSocket roadmap_net_connect__internal(
         const char*       protocol,
         const char*       name,
         int               default_port,
         roadmap_result*   res)
{
   SOCKET fd;
   char *hostname;
   char *separator = strchr (name, ':');
   DWORD dwSockOpt = 0;
   DWORD dwSockOptSize = sizeof(DWORD);

   struct hostent *host;
   struct sockaddr_in addr;

   if( res)
      (*res) = succeeded;

   ///[BOOKMARK]:[DISABLED-CODE]:[PAZ] - Multitasking issue
   ///roadmap_net_mon_connect();

   addr.sin_family = AF_INET;

   hostname = strdup(name);
   roadmap_check_allocated(hostname);

   if (separator != NULL) {
#if 0
      struct servent *service;
      service = getservbyname(separator+1, "tcp");
#else
      void *service = NULL;
#endif
      if (service == NULL) {

         if (isdigit(separator[1])) {

            addr.sin_port = htons((unsigned short)atoi(separator+1));

            if (addr.sin_port == 0) {
               roadmap_log (ROADMAP_ERROR, "invalid port in '%s'", name);
               if( res)
                  (*res) = err_invalid_argument;
               goto connection_failure;
            }

         } else {
            roadmap_log (ROADMAP_ERROR, "invalid service in '%s'", name);
            if( res)
               (*res) = err_invalid_argument;
            goto connection_failure;
         }

      } else {
#if 0
         addr.sin_port = service->s_port;
#endif
      }

      *(strchr(hostname, ':')) = 0;


   } else {
      addr.sin_port = htons((unsigned short)default_port);
   }

#if ((defined UNDER_CE)&& !(defined EMBEDDED_CE))
   if( !is_connected( res))
   {
      roadmap_log (ROADMAP_ERROR, "roadmap_net_connect() - 'is_connected()' returned FALSE");
      goto connection_failure;
   }
#endif   // UNDER_CE


   if (!isdigit(hostname[0])) {
       host = gethostbyname(hostname);
   }
   else{
	   host = NULL;
   }

#if((defined _DEBUG)&&(defined UNDER_CE) && (defined EMBEDDED_CE))
   if(( NULL == host) && (!isdigit(hostname[0])))
   {
      int iErr = WSAGetLastError();

      roadmap_log(ROADMAP_DEBUG,
                  "roadmap_net_connect() - 'gethostbyname(%s)' failed with error %d",
                  hostname, iErr);
   }
#endif   // debug CE

   if (host == NULL) {
      if (isdigit(hostname[0])) {
         addr.sin_addr.s_addr = inet_addr(hostname);
         if (addr.sin_addr.s_addr == INADDR_NONE) {
            roadmap_log (ROADMAP_ERROR, "invalid IP address '%s'",
               hostname);
            if( res)
               (*res) = err_invalid_argument;
            goto connection_failure;
         }
      } else {
         roadmap_log (ROADMAP_ERROR, "invalid host name '%s'", hostname);
         if( res)
            (*res) = err_invalid_argument;
         goto connection_failure;
      }
   } else {
      memcpy (&addr.sin_addr, host->h_addr, host->h_length);
   }


   if (strcmp (protocol, "udp") == 0) {
      fd = socket (PF_INET, SOCK_DGRAM, 0);
   } else if (strcmp (protocol, "tcp") == 0) {
      fd = socket (PF_INET, SOCK_STREAM, 0);
   } else {
      roadmap_log (ROADMAP_ERROR, "unknown protocol %s", protocol);
      if( res)
         (*res) = err_net_unknown_protocol;
      goto connection_failure;
   }

   if (fd == SOCKET_ERROR) {
      roadmap_log (ROADMAP_ERROR, "cannot create socket, errno = %d", WSAGetLastError());
      if( res)
         (*res) = err_net_failed;
      goto connection_failure;
   }
   /*
   if( 0 == getsockopt( fd, SOL_SOCKET, SO_REUSEADDR, (char*)&dwSockOpt, &dwSockOptSize))
   {
      if( !dwSockOpt)
      {
         dwSockOpt = TRUE;
         if( 0 != setsockopt( fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&dwSockOpt, dwSockOptSize))
            roadmap_log( ROADMAP_ERROR, "roadmap_net.c::roadmap_net_connect() - 'setsockopt()' failed with neterr: %d", WSAGetLastError());
      }
      else
         roadmap_log( ROADMAP_DEBUG, "roadmap_net.c::roadmap_net_connect() - SO_REUSEADDR already set");
   }
   else
      roadmap_log( ROADMAP_ERROR, "roadmap_net.c::roadmap_net_connect() - 'getsockopt()' failed with neterr: %d", WSAGetLastError());
*/
   if (connect (fd, (struct sockaddr *) &addr, sizeof(addr)) == SOCKET_ERROR) {

      roadmap_log( ROADMAP_ERROR, "roadmap_net.c::roadmap_net_connect() - 'connect()' failed with neterr: %d", WSAGetLastError());
      //WSAEADDRINUSE

      closesocket(fd);
      if( res)
         (*res) = err_net_failed;
      goto connection_failure;
   }

   free(hostname);
   return fd;


connection_failure:

   free(hostname);

   ///[BOOKMARK]:[DISABLED-CODE]:[PAZ] - Multitasking issue
   ///roadmap_net_mon_disconnect();

   return -1;
}

#ifndef IPHONE
//static int create_async_connection (RoadMapIO *io, struct sockaddr *addr) {
//
//   int s_flags;
//   int res;
//
//   RoadMapSocket s = io->os.socket;
//   s_flags = fcntl(s->s, F_GETFL, 0);
//   if (fcntl(s->s, F_SETFL, s_flags|O_NONBLOCK) == -1) {
//      roadmap_log (ROADMAP_ERROR, "Can't set socket nonblocking, errno = %d", GetLastError );
//   }
//
//   if ((res = connect (s->s, addr, sizeof(*addr))) < 0) {
//
//      if (GetLastError  != EINPROGRESS) {
//         return -1;
//      }
//
//      if (fcntl(s->s, F_SETFL, s_flags) == -1) {
//         roadmap_log (ROADMAP_ERROR, "Can't reset socket to blocking, errno = %d", GetLastError );
//         return -1;
//      }
//   }
//
//   //todo roadmap_main_set_output(io, io_connect_callback, TRUE);
//   RoadMapNetNumConnects++;
//
//   if (res == 0) {
//      /* Probably not realistic */
//      io_connect_callback(io);
//      return 0;
//   }
//
//   if (RoadMapNetNumConnects == 1) {
//      roadmap_main_set_periodic(CONNECT_TIMEOUT_SEC * 1000 /2, check_connect_timeout);
//   }
//
//   return 0;
//}
#endif
static struct servent* roadmap_net_getservbyname( const char *name, const char *protocol )
{
   struct servent* service = NULL;
   
#ifndef IPHONE
   static int has_bug = -1;   /* Android bug overriding (returns the port in the wrong order */

   if ( has_bug < 0 )
   {
      service = getservbyname( "http", NULL );
        has_bug = ( service == NULL || service->s_port == 80 );
   }

   service = getservbyname( name, protocol );
    if ( service && has_bug )
    {
        service->s_port = htons( service->s_port );
    }
#endif

   return service;
}
RoadMapSocket roadmap_net_connect(  const char*    protocol,
                                    const char*    name,
                                    time_t         update_time,
                                    int            default_port,
                                    int            flags,
                                    roadmap_result* res)
{
   char           server_url  [ WSA_SERVER_URL_MAXSIZE   + 1];
   char           service_name[ WSA_SERVICE_NAME_MAXSIZE + 1];
   int            server_port;
   char           proxy_address_buffer[CMPROXY_PROXYSERVER_MAXSIZE+1];
   const char*    proxy_address;
   char				update_since[WDF_MODIFIED_HEADER_SIZE + 1];
   char           packet[256];
   RoadMapSocket  res_socket;
   const char *   req_type = "GET";

   proxy_address = GetProxyAddress();
   if( res)
      (*res) = succeeded;

   if( strncmp( protocol, "http", 4) != 0)
      return roadmap_net_connect__internal( protocol, name, default_port, res);

   if( !strcmp(protocol, "http_post"))
      req_type = "POST";

   // HTTP Connection, using system configuration for Proxy
   if( !WSA_ExtractParams( name,          //   IN        -   Web service full address (http://...)
                           server_url,    //   OUT,OPT   -   Server URL[:Port]
                           &server_port,  //   OUT,OPT   -   Server Port
                           service_name)) //   OUT,OPT   -   Web service name
   {
      roadmap_log( ROADMAP_ERROR, "roadmap_net_connect(HTTP) - Failed to extract information from '%s'", name);
      if( res)
         (*res) = err_invalid_argument;
      return -1;
   }

	WDF_FormatHttpIfModifiedSince (update_time, update_since);

   if( proxy_address)
   {
      int   proxy_port  = server_port;
      char* port_offset = strchr( proxy_address, ':');

      if( port_offset)
         proxy_port = atoi( port_offset + 1);

      strncpy_safe(proxy_address_buffer, proxy_address, sizeof (proxy_address_buffer));

      res_socket = roadmap_net_connect__internal( "tcp", proxy_address_buffer, proxy_port, res);
      if( -1 == res_socket)
         return -1;

      sprintf( packet,
               "%s %s HTTP/1.0\r\n"
               "Host: %s\r\n"
               "User-Agent: FreeMap/%s\r\n"
               "%s",
               req_type, name, server_url, roadmap_start_version(), update_since);
   }
   else
   {
      res_socket = roadmap_net_connect__internal( "tcp", server_url, server_port, res);
      if( -1 == res_socket)
         return -1;

      sprintf( packet,
               "%s %s HTTP/1.0\r\n"
               "Host: %s\r\n"
               "User-Agent: FreeMap/%s\r\n"
               "%s",
               req_type, service_name, server_url, roadmap_start_version(), update_since);
   }


   if( -1 == roadmap_net_send( res_socket, packet, (int)strlen(packet), 1))
   {
      ///[BOOKMARK]:[DISABLED-CODE]:[PAZ] - Multitasking issue
      ///roadmap_net_mon_error("Error in send.");

      roadmap_log( ROADMAP_ERROR, "roadmap_net_connect(HTTP) - Failed to send the 'POST' packet");
      roadmap_net_close( res_socket);
      if( res)
         (*res) = err_net_failed;
      return -1;
   }

   ///[BOOKMARK]:[DISABLED-CODE]:[PAZ] - Multitasking issue
   ///roadmap_net_mon_send(strlen(packet));

   return res_socket;
}

int roadmap_net_send( RoadMapSocket socket, const void *data, int length, int wait)
{
   int total = length;
   fd_set fds;
   struct timeval recv_timeout = {0, 0};

   FD_ZERO(&fds);
   FD_SET(socket, &fds);

   if (wait) {
      recv_timeout.tv_sec = 60;
   }

   while (length > 0) {
      int res;

      res = select(0, NULL, &fds, NULL, &recv_timeout);

      if(!res) {
         roadmap_log (ROADMAP_ERROR,
               "Timeout waiting for select in roadmap_net_send");
         if (!wait) return 0;
         else return -1;
      }

      if(res < 0) {
         roadmap_log (ROADMAP_ERROR,
               "Error waiting on select in roadmap_net_send, LastError = %d",
                      WSAGetLastError());

         ///[BOOKMARK]:[DISABLED-CODE]:[PAZ] - Multitasking issue
         ///roadmap_net_mon_error("Error in send - timeout.");

         return -1;
      }

      res = send(socket, data, length, 0);

      if (res < 0) {
         roadmap_log (ROADMAP_ERROR, "Error sending data, LastError = %d",
                      WSAGetLastError());

         ///[BOOKMARK]:[DISABLED-CODE]:[PAZ] - Multitasking issue
         ///roadmap_net_mon_error("Error in send - data.");

         return -1;
      }

      length -= res;
      data = (char *)data + res;

      ///[BOOKMARK]:[DISABLED-CODE]:[PAZ] - Multitasking issue
      ///roadmap_net_mon_send(res);
   }

   return total;
}


int roadmap_net_receive (RoadMapSocket socket, void *data, int size)
{
   int res;
   fd_set fds;
   struct timeval recv_timeout;

   FD_ZERO(&fds);
   FD_SET(socket, &fds);

   recv_timeout.tv_sec = 90;
   recv_timeout.tv_usec = 0;

   res = select(0, &fds, NULL, NULL, &recv_timeout);

   if(!res) {
      roadmap_log (ROADMAP_ERROR,
            "Timeout waiting for select in roadmap_net_send");
      roadmap_net_mon_error("Error in recv - timeout.");
      return -1;
   }

   if(res < 0) {
      roadmap_log (ROADMAP_ERROR,
            "Error waiting on select in roadmap_net_send, LastError = %d",
                   WSAGetLastError());
      roadmap_net_mon_error("Error in recv - select.");
      return -1;
   }

   res = recv(socket, data, size, 0);

   if (res == SOCKET_ERROR) {
      roadmap_net_mon_error("Error in recv.");
      roadmap_log (ROADMAP_ERROR,
            "Error in recv in roadmap_net_send, LastError = %d",
                   WSAGetLastError());
      return -1;
   }

   if (res == 0) {
      roadmap_net_mon_error("Error in recv.");
      return -1;
   } else {
      roadmap_net_mon_recv(res);
      return res;
   }
}




static int roadmap_net_send_ssl (RoadMapSocket s, const void *data, int length, int wait) {
#ifndef IPHONE
   //return roadmap_ssl_send (s->ssl_ctx, data, length);
#endif
}

void roadmap_net_close (RoadMapSocket socket)
{
   closesocket(socket);

   ///[BOOKMARK]:[DISABLED-CODE]:[PAZ] - Multitasking issue
   ///roadmap_net_mon_disconnect();
}


RoadMapSocket roadmap_net_listen(int port)
{
   struct sockaddr_in addr;
   SOCKET fd;
   int res;

   return INVALID_SOCKET; //AviR test

   memset(&addr, 0, sizeof(addr));

   addr.sin_family = AF_INET;
   addr.sin_port = htons((short)port);

   fd = socket (PF_INET, SOCK_STREAM, 0);
   if (fd == INVALID_SOCKET) return fd;

   res = bind(fd, (struct sockaddr*)&addr, sizeof(addr));

   if (res == SOCKET_ERROR) {
      closesocket(fd);
      return INVALID_SOCKET;
   }

   res = listen(fd, 10);

   if (res == SOCKET_ERROR) {
      closesocket(fd);
      return INVALID_SOCKET;
   }

   return fd;
}


RoadMapSocket roadmap_net_accept(RoadMapSocket server_socket)
{
   return accept(server_socket, NULL, NULL);
}


#if ((defined UNDER_CE)&& (defined EMBEDDED_CE))
#include <winioctl.h>
#include "../md5.h"

__declspec(dllimport)
BOOL KernelIoControl( DWORD dwIoControlCode, LPVOID lpInBuf, DWORD nInBufSize, LPVOID lpOutBuf, DWORD nOutBufSize, LPDWORD lpBytesReturned);

#define IOCTL_HAL_GET_DEVICEID  CTL_CODE(FILE_DEVICE_HAL, 21, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef struct _DEVICE_ID {
  DWORD dwSize;
  DWORD dwPresetIDOffset;
  DWORD dwPresetIDBytes;
  DWORD dwPlatformIDOffset;
  DWORD dwPlatformIDBytes;
} DEVICE_ID, *PDEVICE_ID;


int roadmap_net_unique_id(unsigned char *buffer, unsigned int size)
{
   DWORD dwOutBytes;
   int nBuffSize = 128;
   BYTE *arrOutBuff = (byte *) LocalAlloc(LMEM_FIXED, nBuffSize);
   PDEVICE_ID pDevId;
   BOOL bRes;

   if (size < 16) {
      return -1;
   }

   pDevId = (PDEVICE_ID) &arrOutBuff[0];
   pDevId->dwSize = nBuffSize;

   bRes = KernelIoControl(IOCTL_HAL_GET_DEVICEID, 0, 0, arrOutBuff, nBuffSize, &dwOutBytes);

   // if buffer not large enough, reallocate the buffer
   if (!bRes && GetLastError() == 122)
   {
       nBuffSize = pDevId->dwSize;
       arrOutBuff = (byte *) LocalReAlloc(arrOutBuff, nBuffSize, LMEM_MOVEABLE);
       bRes = KernelIoControl(IOCTL_HAL_GET_DEVICEID, 0, 0, arrOutBuff, nBuffSize, &dwOutBytes);
   }

   if (bRes) {

      struct MD5Context context;
      unsigned char digest[16];

      MD5Init (&context);

      MD5Update (&context, arrOutBuff + pDevId->dwPresetIDOffset, pDevId->dwPresetIDBytes);
      MD5Update (&context, arrOutBuff + pDevId->dwPlatformIDOffset, pDevId->dwPlatformIDBytes);
      MD5Final (digest, &context);

      if (size > sizeof(digest)) size = sizeof(digest);
      memcpy(buffer, digest, size);

      LocalFree(arrOutBuff);

      return size;
   }

   /* Some older PPC devices only return the ID if the buffer
    * is exactly the size of a GUID, so attempt to retrieve
    * the ID this way.
    */
   bRes = KernelIoControl(IOCTL_HAL_GET_DEVICEID, 0, 0, arrOutBuff, 16, &dwOutBytes);

   if (bRes) {
      struct MD5Context context;
      unsigned char digest[16];

      MD5Init (&context);

      MD5Update (&context, arrOutBuff, 16);
      MD5Final (digest, &context);

      if (size > sizeof(digest)) size = sizeof(digest);
      memcpy(buffer, digest, size);

      LocalFree(arrOutBuff);

      return size;
   }

   LocalFree(arrOutBuff);

   return -1;
}
#else
int roadmap_net_unique_id(unsigned char *buffer, unsigned int size)
{
   return 16;
}
#endif

extern
BOOL win32_roadmap_net_async_connect(  const char*                protocol,
                                       const char*                name,
                                       time_t                     update_time,
                                       int                        default_port,
                                       int                        flags,
                                       RoadMapNetConnectCallback  on_net_connected,
                                       void*                      context);

void *roadmap_net_connect_async(
                           const char*                protocol,
                           const char*                name,
						   const char *resolved_name,
                           time_t                     update_time,
                           int                        default_port,
                           int                        flags,
                           RoadMapNetConnectCallback  on_net_connected,
                           void*                      context)
{
   if( !win32_roadmap_net_async_connect(  protocol,
                                          name,
                                          update_time,
                                          default_port,
                                          flags,
                                          on_net_connected,
                                          context))
      return -1;

   return 0;
}
//void *roadmap_net_connect_async (const char *protocol, const char *name, const char *resolved_name,
//                               time_t update_time,
//                               int default_port,
//                               int flags,
//                               RoadMapNetConnectCallback callback,
//                               void *context) {
//
//   return roadmap_net_connect_internal
//                        (protocol, name, resolved_name, update_time, default_port, 1, flags, callback, context, NULL, 0);
//}
void roadmap_net_shutdown (void) {
   const char* netcompress_cfg_value = RoadMapNetCompressEnabled ? "yes" : "no";
#if ((defined UNDER_CE)&& !(defined EMBEDDED_CE))
   if (RoadMapConnection) {
      ConnMgrReleaseConnection(RoadMapConnection, 1);
      RoadMapConnection = NULL;
   }
#endif   // UNDER_CE

   // Network compression
   roadmap_config_set( &RoadMapConfigNetCompressEnabled, netcompress_cfg_value );

   roadmap_net_mon_destroy();
   DeleteCriticalSection( &s_cs);
}

void roadmap_net_initialize()
{
   InitializeCriticalSection( &s_cs);
   roadmap_config_declare
       ( "user", &RoadMapConfigNetCompressEnabled, "no", NULL );
   RoadMapNetCompressEnabled = roadmap_config_match( &RoadMapConfigNetCompressEnabled, "yes" );

   roadmap_net_mon_start();
}

const char* ConnMgr_GetStatusString( DWORD dwStatus)
{
   switch(dwStatus)
   {
      CASE_STATUS_RETURN_STRING(UNKNOWN)
      CASE_STATUS_RETURN_STRING(CONNECTED)
      CASE_STATUS_RETURN_STRING(SUSPENDED)
      CASE_STATUS_RETURN_STRING(DISCONNECTED)
      CASE_STATUS_RETURN_STRING(CONNECTIONFAILED)
      CASE_STATUS_RETURN_STRING(CONNECTIONCANCELED)
      CASE_STATUS_RETURN_STRING(CONNECTIONDISABLED)
      CASE_STATUS_RETURN_STRING(NOPATHTODESTINATION)
      CASE_STATUS_RETURN_STRING(WAITINGFORPATH)
      CASE_STATUS_RETURN_STRING(WAITINGFORPHONE)
      CASE_STATUS_RETURN_STRING(PHONEOFF)
      CASE_STATUS_RETURN_STRING(EXCLUSIVECONFLICT)
      CASE_STATUS_RETURN_STRING(NORESOURCES)
      CASE_STATUS_RETURN_STRING(CONNECTIONLINKFAILED)
      CASE_STATUS_RETURN_STRING(AUTHENTICATIONFAILED)
      CASE_STATUS_RETURN_STRING(WAITINGCONNECTION)
      CASE_STATUS_RETURN_STRING(WAITINGFORRESOURCE)
      CASE_STATUS_RETURN_STRING(WAITINGFORNETWORK)
      CASE_STATUS_RETURN_STRING(WAITINGDISCONNECTION)
      CASE_STATUS_RETURN_STRING(WAITINGCONNECTIONABORT)
   }

   return "<unknown>";
}

void roadmap_net_set_compress_enabled( BOOL value )
{
   RoadMapNetCompressEnabled = value;
}

BOOL roadmap_net_get_compress_enabled( void )
{
   return RoadMapNetCompressEnabled;
}

/*void roadmap_net_cancel_connect (RoadMapIO *io) {
   RoadMapNetData *data = io->context;
   RoadMapSocket s = io->os.socket;
   
   if ( io == NULL || io->subsystem == ROADMAP_IO_INVALID )
      return;

   if (io->retry_params.protocol && io->retry_params.protocol[0]) {
      free(io->retry_params.protocol);
   }
   if (io->retry_params.name && io->retry_params.name[0]) {
      free(io->retry_params.name);
   }
   if (io->retry_params.resolved_name && io->retry_params.resolved_name[0]) {
      free(io->retry_params.resolved_name);
   }
   
   roadmap_log(ROADMAP_DEBUG, "Cancelling async connect request (%d)", s);
   roadmap_main_remove_input(io);
   roadmap_net_close(s);
   free(data);
   
   RoadMapNetNumConnects--;
   
   if (RoadMapNetNumConnects == 0) {
      roadmap_main_remove_periodic(check_connect_timeout);
   }
}
*/