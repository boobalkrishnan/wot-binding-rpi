#include <iostream>
#include <string>
#include <vector>
#include <regex>
#include <Poco/DNSSD/DNSSDResponder.h>
#include <Poco/DNSSD/Avahi/Avahi.h>
#include "BindHttpServer.h"

#include "Logger.h"


using namespace Poco::Net;
using namespace Poco::Util;
using namespace std;
using namespace Poco::DNSSD;

pthread_mutex_t Evnt_Subscribe_Mutex;
pthread_cond_t Evnt_Subscribe_cv;
std::string Evnt_SubscribeData;

unsigned int  BindHttpEventObservHandler::ParseThingsURI(std::string RxURI,std::string* ArrayOfURI)
{
    std::string delimiter = "/";
    int LevelOfUTI=0;
    size_t pos = 0;
    std::string token;
    pLogger->PrintLog(LOG_LEVEL_DEBUG, LOGGER_COMP_HTTPSERVER,"Parsing:",RxURI.c_str());
    // cout << "Parsing:" << RxURI << std::endl;
    while ((pos = RxURI.find(delimiter)) != std::string::npos)
    {

        // cout << "Parsing:" << RxURI << std::endl;  
        token = RxURI.substr(0, pos);
        if (token != "")
        {
            ArrayOfURI[LevelOfUTI]=token;
            // pLogger->PrintLog(LOG_LEVEL_DEBUG, LOGGER_COMP_HTTPSERVER,"Parsed:",ArrayOfURI[LevelOfUTI].c_str());
            LevelOfUTI++;
        }
        RxURI.erase(0, pos + delimiter.length());

    }
    // cout << "Parsing:" << RxURI << std::endl;
    ArrayOfURI[LevelOfUTI]=RxURI;
    // pLogger->PrintLog(LOG_LEVEL_DEBUG, LOGGER_COMP_HTTPSERVER,"Parsed paths:",LevelOfUTI);
    // cout << "Parsed Levels:" << LevelOfUTI << std::endl; 
    return (LevelOfUTI);  
}

typedef struct{
    std::string Evnt_SubscribeHandlers;
    // HTTPServerResponse &sseResp;
    std::string Evnt_SubscribeData;
}Evnt_SubscribeHandlers_t;

uint8_t Evnt_SubscribeRequestsCnt=0;

Evnt_SubscribeHandlers_t Evnt_SubscribeHandlers[100];


void EmitSubscribedEvent(std::string EventID, std::string out)
{
    pthread_mutex_lock(&Evnt_Subscribe_Mutex);
    // cout<< "Event SubscribeID:"<<EventID.c_str()<<endl;
    for (uint8_t eventcnt=0;eventcnt<Evnt_SubscribeRequestsCnt;eventcnt++)
    {
        cout<< "Event Subscribe ID:"<<Evnt_SubscribeHandlers[eventcnt].Evnt_SubscribeHandlers.c_str()<<endl;
        cout<< "Subscribed Data:"<<out.c_str()<<endl;
        if (Evnt_SubscribeHandlers[eventcnt].Evnt_SubscribeHandlers == EventID)
        {
            Evnt_SubscribeHandlers[eventcnt].Evnt_SubscribeData = out;
            pthread_cond_signal(&Evnt_Subscribe_cv);
        }
    }
    pthread_mutex_unlock(&Evnt_Subscribe_Mutex);
}

std::string GetEventStatus(std::string EventID)
{
    std::string RetVal;

    for (uint8_t eventcnt=0;eventcnt<Evnt_SubscribeRequestsCnt;eventcnt++)
    {
        // cout<< "sseReqName:"<<Evnt_SubscribeHandlers[eventcnt].Evnt_SubscribeHandlers.c_str()<<endl;
        if (Evnt_SubscribeHandlers[eventcnt].Evnt_SubscribeHandlers == EventID)
        {
            RetVal = Evnt_SubscribeHandlers[eventcnt].Evnt_SubscribeData;
            break;
        }
    }
    return (RetVal);
}

void BindHttpEventObservHandler::handleRequest(HTTPServerRequest &req, HTTPServerResponse &resp)
{
    Poco::JSON::Object Response;
    stringstream ResponseStr;
    std::string RequestMethod;
    bool ResponseSt;
    std::string Host;
    std::string ThingURI[10];
    std::string Interaction_Type="";
    std::string Interaction_Name="";
    std::string Thing_Name;
    std::string Root_Name;
    UriLevel RequestLevel;
    Poco::JSON::Object Temp;

    resp.setStatus(HTTPResponse::HTTP_OK);

    resp.add("Content-Type", "text/event-stream");
    resp.add("Cache-Control", "no-cache");
    resp.add("Access-Control-Allow-Origin", "*");
    pLogger = Logger::getInstance();
    Host = req.getHost();
    pLogger->PrintLog(LOG_LEVEL_DEBUG, LOGGER_COMP_HTTPSERVER,"Long Poll: ",Host.c_str());
    ostream& out = resp.send();
    pLogger->PrintLog(LOG_LEVEL_DEBUG, LOGGER_COMP_HTTPSERVER,"Long Poll Request URI=",req.getURI().c_str());
    // cout << "Request URI=" << req.getURI() << endl;
    RequestLevel = (UriLevel) ParseThingsURI(req.getURI(),ThingURI);
    pLogger->PrintLog(LOG_LEVEL_DEBUG, LOGGER_COMP_HTTPSERVER,"Long Poll RequestLevel=",int(RequestLevel));
    RequestMethod = req.getMethod();

    Root_Name = ThingURI[0];
    Thing_Name = ThingURI[0];
    if (ThingURI[URI_AFFORDANCES_INDEX] == PROPERTY_INTERACT)
    {
        Interaction_Type = PROPERTY_INTERACT;
    }
    else if (ThingURI[URI_AFFORDANCES_INDEX] == ACTION_INTERACT)
    {
        Interaction_Type = ACTION_INTERACT;
    }
    else if (ThingURI[URI_AFFORDANCES_INDEX] == EVENT_INTERACT)
    {
        Interaction_Type = EVENT_INTERACT;
    }
    Interaction_Name = ThingURI[URI_AFFORDANCE_INDEX];
    pLogger->PrintLog(LOG_LEVEL_DEBUG, LOGGER_COMP_HTTPSERVER,"Long Poll Interaction Type: ",Interaction_Type.c_str());
    pLogger->PrintLog(LOG_LEVEL_DEBUG, LOGGER_COMP_HTTPSERVER,"Long Poll Interaction Name: ",Interaction_Name.c_str());

    if (RequestMethod == "GET")
    {
        Evnt_SubscribeHandlers[Evnt_SubscribeRequestsCnt].Evnt_SubscribeHandlers = Interaction_Name;
        // sseHandlers[sseRequestsCnt].sseResp = req;
        HandleSubscription(Thing_Name, Root_Name, Interaction_Type,Interaction_Name, &Response);
        Response.stringify(out);

        Evnt_SubscribeRequestsCnt++;
    }
    else
    {
        std::string content;

        Response.set("status", "success");
        cout << endl << "Long Poll Request received: " << content << endl;
    }

    // while (out.good())
    // {
    pthread_mutex_lock(&Evnt_Subscribe_Mutex);
    pthread_cond_wait(&Evnt_Subscribe_cv, &Evnt_Subscribe_Mutex);
    out << GetEventStatus(Interaction_Name) << "\n\n";
    // Response.set("data", counter++);
    // Response.stringify(out);//
    // out << EventData;
    out.flush();
    pthread_mutex_unlock(&Evnt_Subscribe_Mutex);
    // if ()
    // out << "data: " << counter++ << "\n\n";
    // Response.set("data", counter++);
    // Response.stringify(out);//
    // out.flush();
    // pLogger->PrintLog(LOG_LEVEL_DEBUG, LOGGER_COMP_HTTPSERVER,"SSE counter: ",counter);
    // Poco::Thread::sleep(2000);
    // }
    pLogger->PrintLog(LOG_LEVEL_DEBUG, LOGGER_COMP_HTTPSERVER,"Long Poll: End");
};