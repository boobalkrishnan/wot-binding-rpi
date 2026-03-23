#include <iostream>
#include <string>
#include <vector>
#include <regex>
#include <Poco/DNSSD/DNSSDResponder.h>
#include <Poco/DNSSD/Avahi/Avahi.h>
#include "BindHttpServer.h"
#include "BindCoAPServer.h"
#include "BindServer.h"
#include "Logger.h"

void BindServer::EmitEvent(std::string EventID, std::string out)
{
    // pthread_mutex_lock(&Event_Mutex);
    cout<< "EventID:"<<EventID.c_str()<<endl;
    if ( ServerType == BIND_HTTP_SERVER)
    {
        BindHttpServer::EmitEvent(EventID,out);
    }
    else if ( ServerType == BIND_COAP_SERVER)
    {
        // BindCoAPServer::EmitEvent(EventID,out);
    }
    else if (ServerType == BIND_WS_SERVER)
    {
        // BindWebSocket::EmitEvent(EventID,out);
    }
}

void BindServer::SetHandlers(Adapter_interaction_get GetHandleIn,
                                 Adapter_interaction_put PutHandleIn, 
                                 Adapter_thing_get thingGetIn,
                                 Adapter_affordance_get affordIn,
                                 Adapter_interaction_get SubsHandleIn)
{
	pLogger->PrintLog(LOG_LEVEL_DEBUG, LOGGER_COMP_BINDSERVER,"SetHandlers call");    
    if ( ServerType == BIND_HTTP_SERVER)
    {
        BindHttpServer::SetHandlers(GetHandleIn,PutHandleIn,thingGetIn,affordIn,SubsHandleIn);
    }
    else if ( ServerType == BIND_COAP_SERVER)
    {
        BindCoAPServer::SetHandlers(GetHandleIn,PutHandleIn,thingGetIn,affordIn,SubsHandleIn);
    }
    else if ( ServerType == BIND_WS_SERVER)
    {

    }
}

void BindServer::Start(void)
{
    if ( ServerType == BIND_HTTP_SERVER)
    {
        BindHttpServer::Start();
    }
    else if ( ServerType == BIND_COAP_SERVER)
    {
        BindCoAPServer::Start();
    }
    else if ( ServerType == BIND_WS_SERVER)
    {

    }
}

void BindServer::Stop(void)
{
    if ( ServerType == BIND_HTTP_SERVER)
    {
        BindHttpServer::Stop();
    }
    else if ( ServerType == BIND_COAP_SERVER)
    {
        BindCoAPServer::Stop();
    }
    else if ( ServerType == BIND_WS_SERVER)
    {

    }
}

BindServer::BindServer(uint8_t ServerNo) //: BindHttpServer(),BindCoAPServer(port) //,BindWebSocket(port)
{
    ServerType = ServerNo;
}

void BindServer::Initialize(unsigned int port)
{
    pLogger = Logger::getInstance();
    pLogger->PrintLog(LOG_LEVEL_DEBUG, LOGGER_COMP_BINDSERVER,"Constructor call");
    if ( ServerType == BIND_HTTP_SERVER)
    {
        BindHttpServer::Initialize(port);
    }
    else if ( ServerType == BIND_COAP_SERVER)
    {
        BindCoAPServer::Initialize(port);
    }
    else if ( ServerType == BIND_WS_SERVER)
    {

    }
}

void BindServer::Process(void)
{
    if ( ServerType == BIND_HTTP_SERVER)
    {
        BindHttpServer::Process();
    }
    else if ( ServerType == BIND_COAP_SERVER)
    {
        BindCoAPServer::Process();
    }
    else if ( ServerType == BIND_WS_SERVER)
    {

    }
}