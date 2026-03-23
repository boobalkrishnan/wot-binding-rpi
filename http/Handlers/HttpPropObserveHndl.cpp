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

pthread_mutex_t Prop_Subscribe_Mutex;
pthread_cond_t Prop_Subscribe_cv;
std::string Prop_SubscribeData;

unsigned int  BindHttpPropertyObservHandler::ParseThingsURI(std::string RxURI,std::string* ArrayOfURI)
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
    std::string SubscribeHandlers;
    // HTTPServerResponse &sseResp;
    std::string SubscribeData;
}SubscribeHandlers_t;

uint8_t SubscribeRequestsCnt=0;

SubscribeHandlers_t SubscribeHandlers[100];


void EmitProperty(std::string EventID, std::string out)
{
    pthread_mutex_lock(&Prop_Subscribe_Mutex);
    cout<< "SubscribeID:"<<EventID.c_str()<<endl;
    for (uint8_t eventcnt=0;eventcnt<SubscribeRequestsCnt;eventcnt++)
    {
        cout<< "sseReqName:"<<SubscribeHandlers[eventcnt].SubscribeHandlers.c_str()<<endl;
        if (SubscribeHandlers[eventcnt].SubscribeHandlers == EventID)
        {
            SubscribeHandlers[eventcnt].SubscribeData = out;
            pthread_cond_signal(&Prop_Subscribe_cv);
        }
    }
    pthread_mutex_unlock(&Prop_Subscribe_Mutex);
}

std::string GetSubscribeStatus(std::string EventID)
{
    std::string RetVal;

    for (uint8_t eventcnt=0;eventcnt<SubscribeRequestsCnt;eventcnt++)
    {
        cout<< "sseReqName:"<<SubscribeHandlers[eventcnt].SubscribeHandlers.c_str()<<endl;
        if (SubscribeHandlers[eventcnt].SubscribeHandlers == EventID)
        {
            RetVal = SubscribeHandlers[eventcnt].SubscribeData;
            break;
        }
    }
    return (RetVal);
}

void BindHttpPropertyObservHandler::handleRequest(HTTPServerRequest &req, HTTPServerResponse &resp)
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
    pLogger->PrintLog(LOG_LEVEL_DEBUG, LOGGER_COMP_HTTPSERVER,"SSE: ",Host.c_str());
    ostream& out = resp.send();
    pLogger->PrintLog(LOG_LEVEL_DEBUG, LOGGER_COMP_HTTPSERVER,"SSE Request URI=",req.getURI().c_str());
    // cout << "Request URI=" << req.getURI() << endl;
    RequestLevel = (UriLevel) ParseThingsURI(req.getURI(),ThingURI);
    pLogger->PrintLog(LOG_LEVEL_DEBUG, LOGGER_COMP_HTTPSERVER,"SSE RequestLevel=",int(RequestLevel));
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
    pLogger->PrintLog(LOG_LEVEL_DEBUG, LOGGER_COMP_HTTPSERVER,"SSE Interaction Type: ",Interaction_Type.c_str());
    pLogger->PrintLog(LOG_LEVEL_DEBUG, LOGGER_COMP_HTTPSERVER,"SSE Interaction Name: ",Interaction_Name.c_str());

    if (RequestMethod == "GET")
    {
        SubscribeHandlers[SubscribeRequestsCnt].SubscribeHandlers = Interaction_Name;
        // sseHandlers[sseRequestsCnt].sseResp = req;
        HandleSubscription(Thing_Name, Root_Name, Interaction_Type,Interaction_Name, &Response);
        Response.stringify(out);
        SubscribeRequestsCnt++;
    }
    else
    {
        std::string content;

        Response.set("status", "success");
        cout << endl << "SSE Request received: " << content << endl;
    }

    while (out.good())
    {
        pthread_mutex_lock(&Prop_Subscribe_Mutex);
        pthread_cond_wait(&Prop_Subscribe_cv, &Prop_Subscribe_Mutex);
        out << "data: " << GetSubscribeStatus(Interaction_Name) << "\n\n";
        // Response.set("data", counter++);
        // Response.stringify(out);//
        // out << EventData;
        out.flush();
        pthread_mutex_unlock(&Prop_Subscribe_Mutex);
        // if ()
        // out << "data: " << counter++ << "\n\n";
        // Response.set("data", counter++);
        // Response.stringify(out);//
        // out.flush();
        // pLogger->PrintLog(LOG_LEVEL_DEBUG, LOGGER_COMP_HTTPSERVER,"SSE counter: ",counter);
        // Poco::Thread::sleep(2000);
    }
    pLogger->PrintLog(LOG_LEVEL_DEBUG, LOGGER_COMP_HTTPSERVER,"SSE: End");
};