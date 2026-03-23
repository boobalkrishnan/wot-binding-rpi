#include <iostream>
#include <string>
#include <vector>
#include <regex>
#include <Poco/DNSSD/DNSSDResponder.h>
#include <Poco/DNSSD/Avahi/Avahi.h>
#include <Poco/Net/NetworkInterface.h>
#include <Poco/Net/IPAddress.h>

#include "BindServer.h"
#include "BindHttpServer.h"
#include "Logger.h"


using namespace Poco::Net;
using namespace Poco::Util;
using namespace std;
using namespace Poco::DNSSD;
pthread_mutex_t Handler_Mutex;
pthread_mutex_t Event_Mutex;
pthread_cond_t Event_cv;
std::string EventData;
// unsigned int  BindHttpServerHandler::count = 0;
Adapter_interaction_get HandleSubscription;
Adapter_interaction_get HandleAffordanceGet;
Adapter_interaction_put HandleAffordancePut;
Adapter_thing_get HandleThing;
Adapter_affordance_get HandleAffordance;
uint8_t counter=0;

std::string GetLocalIpAddress(void)
{
    std::string localIp;
    std::vector<Poco::Net::NetworkInterface> interfaces = Poco::Net::NetworkInterface::list();

    for (const auto& iface : interfaces) {
        if (iface.supportsIPv4() && !iface.isLoopback() && iface.address().isUnicast()) {
            std::cout << "Interface: " << iface.name() << std::endl;
            std::cout << "  IPv4 Address: " << iface.address().toString() << std::endl;
            localIp = iface.address().toString();
            break; // Return the first valid IPv4 address found
        }
    }
    return localIp;
}

unsigned int ParseThingsURI(std::string RxURI,std::string* ArrayOfURI)
{
    std::string delimiter = "/";
    int LevelOfUTI=0;
    size_t pos = 0;
    std::string token;
    cout << "Parsing:" << RxURI << std::endl;
    while ((pos = RxURI.find(delimiter)) != std::string::npos)
    {
        // cout << "Parsing:" << RxURI << std::endl;  
        token = RxURI.substr(0, pos);
        if (token != "")
        {
            ArrayOfURI[LevelOfUTI]=token;
            LevelOfUTI++;
        }
        RxURI.erase(0, pos + delimiter.length());

    }
    cout << "Parsed:" << RxURI << std::endl;
    ArrayOfURI[LevelOfUTI]=RxURI;
    // pLogger->PrintLog(LOG_LEVEL_DEBUG, LOGGER_COMP_HTTPSERVER,"Parsed paths:",LevelOfUTI);
    // cout << "Parsed Levels:" << LevelOfUTI << std::endl; 
    return (LevelOfUTI);  
}

unsigned int  BindHttpServerHandler::ParseThingsURI(std::string RxURI,std::string* ArrayOfURI)
{
    std::string delimiter = "/";
    int LevelOfUTI=0;
    size_t pos = 0;
    std::string token;
    pLogger->PrintLog(LOG_LEVEL_DEBUG, LOGGER_COMP_HTTPSERVER,"Parsing:",RxURI.c_str());
    cout << "Parsing:" << RxURI << std::endl;
    while ((pos = RxURI.find(delimiter)) != std::string::npos)
    {

        // cout << "Parsing:" << RxURI << std::endl;  
        token = RxURI.substr(0, pos);
        if (token != "")
        {
            ArrayOfURI[LevelOfUTI]=token;
            pLogger->PrintLog(LOG_LEVEL_DEBUG, LOGGER_COMP_HTTPSERVER,"Parsed:",ArrayOfURI[LevelOfUTI].c_str());
            LevelOfUTI++;
        }
        RxURI.erase(0, pos + delimiter.length());

    }
    cout << "Parsed:" << RxURI << std::endl;
    ArrayOfURI[LevelOfUTI]=RxURI;
    // pLogger->PrintLog(LOG_LEVEL_DEBUG, LOGGER_COMP_HTTPSERVER,"Parsed paths:",LevelOfUTI);
    // cout << "Parsed Levels:" << LevelOfUTI << std::endl; 
    return (LevelOfUTI);  
}

void BindHttpServerHandler::handleRequest(HTTPServerRequest &req, HTTPServerResponse &resp)
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

    pthread_mutex_lock(&Handler_Mutex);

    pLogger = Logger::getInstance();

    resp.setStatus(HTTPResponse::HTTP_OK);
    Host = req.getHost();
    pLogger->PrintLog(LOG_LEVEL_DEBUG, LOGGER_COMP_HTTPSERVER,"Host: ",Host.c_str());
    // if (Host == "192.168.137.100:9090")
    if (req.getMethod() == "OPTIONS")
    {
        /* Required to handle for CORS Policy error in client */
        pLogger->PrintLog(LOG_LEVEL_DEBUG, LOGGER_COMP_HTTPSERVER,"Request Method: HTTP_OPTIONS");
        resp.setContentType("application/json");
        resp.setKeepAlive(true); 
        resp.add("Access-Control-Allow-Origin", "*");
        resp.add("Access-Control-Allow-Headers", "Origin, X-Requested-With, Content-Type, Accept, Authorization");
        resp.add("Access-Control-Allow-Method","POST, GET, PUT");

        resp.send();
        //return;
    }
    else
    {
        /* Only JSON content type supported currently */
        resp.setContentType("application/json");
         /* Required to handle for CORS Policy error in client */
        resp.add("Access-Control-Allow-Origin", "*");
        ostream& out = resp.send();
        pLogger->PrintLog(LOG_LEVEL_DEBUG, LOGGER_COMP_HTTPSERVER,"Request URI=",req.getURI().c_str());
        // cout << "Request URI=" << req.getURI() << endl;
        RequestLevel = (UriLevel) ParseThingsURI(req.getURI(),ThingURI);
        pLogger->PrintLog(LOG_LEVEL_DEBUG, LOGGER_COMP_HTTPSERVER,"Requested Path Level:",RequestLevel);

        RequestMethod = req.getMethod();
        pLogger->PrintLog(LOG_LEVEL_DEBUG, LOGGER_COMP_HTTPSERVER,"Request Method: ",RequestMethod.c_str());
        switch(RequestLevel)
        {
            // case URI_THINGS_INDEX:
            case URI_THING_INDEX:
            {
                Root_Name = ThingURI[0];
                Thing_Name = ThingURI[0];
                if (RequestMethod == "GET")
                {
                    HandleThing(Host,Thing_Name,Root_Name, &Response);
                    Response.stringify(out);
                    // Response.stringify(ResponseStr);
                    // pLogger->PrintLog(LOG_LEVEL_DEBUG, LOGGER_COMP_HTTPSERVER,ResponseStr.str().c_str());                
                }
            }
            break;
            /* Handler for specific Interaction */
            case URI_AFFORDANCE_INDEX:
            {
                Interaction_Name = ThingURI[URI_AFFORDANCE_INDEX]; // Name of the interaction
            }
            /* Handler for reteriving all Interaction */
            case URI_AFFORDANCES_INDEX:
            {
                Root_Name = ThingURI[0];    // Root Name is not used
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
                pLogger->PrintLog(LOG_LEVEL_DEBUG, LOGGER_COMP_HTTPSERVER,"Interaction Type: ",Interaction_Type.c_str());
                pLogger->PrintLog(LOG_LEVEL_DEBUG, LOGGER_COMP_HTTPSERVER,"Interaction Name: ",Interaction_Name.c_str());
                if (Interaction_Name == "")
                {
                    /* Handler for reteriving all Interaction */
                    HandleAffordance(Host, Thing_Name, Root_Name, Interaction_Type, &Response);
                    Response.stringify(out);
                }
                else
                {
                    /* Handler for specific Interaction request*/
                    if (RequestMethod == "GET")
                    {
                        /* Data will be updated in ostream out parameter in handler function*/
                        HandleAffordanceGet(Thing_Name, Root_Name, Interaction_Type,Interaction_Name, &Response);
                        Response.stringify(out);
                    }
                    else
                    {
                        size_t size = (size_t)req.getContentLength();
                        cout << "size:" << size <<endl;
                        std::istream& stream = req.stream();
                        /* Data will be extracted in istream stream paramter in handler function */
                        HandleAffordancePut(Thing_Name, Root_Name, Interaction_Type,Interaction_Name, stream,size);
                    
                        /* To be deleted */
                        // std::string encoded_content;
                        // std::string content;

                        // encoded_content.resize(size);
                        // stream.read(&encoded_content[0], size);
                        // Poco::JSON::Parser parser;
                        // Poco::Dynamic::Var result = parser.parse(encoded_content);
                        // Poco::JSON::Object::Ptr pObject = result.extract<Poco::JSON::Object::Ptr>();
                        // content = pObject->getValue<std::string>("value");

                        // Response.set("status", "success");
                        // cout << endl << "Request received: " << content << endl;
                        // Response.stringify(out);
                    }
                    /* Send the stream out*/
                    out.flush();
                }
            }
            break;
        }

        // if ((RequestMethod == "GET")||(RequestMethod=="POST"))
        // {
        // Response.stringify(out);
        // }
    }
    // else
    // {
    //     pLogger->PrintLog(LOG_LEVEL_DEBUG, LOGGER_COMP_HTTPSERVER,"Host IP is not 192.168.137.100");
    // }
    pthread_mutex_unlock(&Handler_Mutex);
};


unsigned int  BindSseServerHandler::ParseThingsURI(std::string RxURI,std::string* ArrayOfURI)
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
    std::string sseReqName;
    // HTTPServerResponse &sseResp;
    std::string EventData;
}sseHandlers_t;

uint8_t sseRequestsCnt=0;

sseHandlers_t sseHandlers[100];


void BindHttpServer::EmitEvent(std::string EventID, std::string out)
{
    // pthread_mutex_lock(&Event_Mutex);
    cout<< "EventID:"<<EventID.c_str()<<endl;

    EmitSubscribedEvent(EventID,out);
    // for (uint8_t eventcnt=0;eventcnt<sseRequestsCnt;eventcnt++)
    // {
    //     cout<< "sseReqName:"<<sseHandlers[eventcnt].sseReqName.c_str()<<endl;
    //     if (sseHandlers[eventcnt].sseReqName == EventID)
    //     {
    //         sseHandlers[eventcnt].EventData = out;
    //         pthread_cond_signal(&Event_cv);
    //     }
    // }
    // pthread_mutex_unlock(&Event_Mutex);
}

std::string GetEventsStatus(std::string EventID)
{
    std::string RetVal;

    for (uint8_t eventcnt=0;eventcnt<sseRequestsCnt;eventcnt++)
    {
        cout<< "sseReqName:"<<sseHandlers[eventcnt].sseReqName.c_str()<<endl;
        if (sseHandlers[eventcnt].sseReqName == EventID)
        {
            RetVal = sseHandlers[eventcnt].EventData;
            break;
        }
    }
    return (RetVal);
}

void BindSseServerHandler::handleRequest(HTTPServerRequest &req, HTTPServerResponse &resp)
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
    switch(RequestLevel)
    {
        case URI_SUBSCRIPTION_INDEX:
        {
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
            if (Interaction_Name == "")
            {
                // HandleAffordance(Host, Thing_Name, Root_Name, Interaction_Type, &Response);
            }
            else
            {
                if (RequestMethod == "GET")
                {
                    sseHandlers[sseRequestsCnt].sseReqName = Interaction_Name;
                    // sseHandlers[sseRequestsCnt].sseResp = req;
                    HandleSubscription(Thing_Name, Root_Name, Interaction_Type,Interaction_Name, &Response);
                    Response.stringify(out);
                    sseRequestsCnt++;
                }
                else
                {
                    size_t size = (size_t)req.getContentLength();
                    std::istream& stream = req.stream();
                    std::string encoded_content;
                    std::string content;

                    // encoded_content.resize(size);
                    // stream.read(&encoded_content[0], size);
                    // Poco::JSON::Parser parser;
                    // Poco::Dynamic::Var result = parser.parse(encoded_content);
                    // Poco::JSON::Object::Ptr pObject = result.extract<Poco::JSON::Object::Ptr>();
                    // content = pObject->getValue<std::string>("value");
                    // HandleAffordancePut(Thing_Name, Root_Name, Interaction_Type,Interaction_Name, stream,size);
                    Response.set("status", "success");
                    cout << endl << "SSE Request received: " << content << endl;
                }
            }
        }

        break;
    }
    while (out.good())
    {
        pthread_mutex_lock(&Event_Mutex);
        pthread_cond_wait(&Event_cv, &Event_Mutex);
        out << "data: " << GetEventsStatus(Interaction_Name) << "\n\n";
        // Response.set("data", counter++);
        // Response.stringify(out);//
        // out << EventData;
        out.flush();
        pthread_mutex_unlock(&Event_Mutex);
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

HTTPRequestHandler* BindHttpServerHandlerFactory::createRequestHandler(const HTTPServerRequest & request)
{
    std::string ExpectedURI = "/";
    std::smatch matches;
    std::regex ThingRead("/([^/]+)");
    std::regex AffortanceRead("/([^/]+)/([^/]+)");
    std::regex PropertyRW("/([^/]+)/property/([^/]+)");
    std::regex ActionInvoke("/([^/]+)/action/([^/]+)");
    // std::regex InteractionSubs("/(?P<thing_name>[^/]+)/(?P<interaction>[^/]+)/(?P<name>[^/]+)/subscription");
    std::regex EventSubs("/([^/]+)/event/([^/]+)/subscription");
    std::regex PropertySubs("/([^/]+)/property/([^/]+)/subscription");
    // std::regex InteractionSubs("/([^/]+)/([^/]+)/([^/]+)/subscription");

    std::cout << "Requested URI:"<<request.getURI()<<"\n";
    if(std::regex_search(request.getURI(), matches, PropertyRW)) 
    {
        std::cout << "PropertyRW Handler\n";
        return new BindHttpPropertyHandler;
    } 
    else 
    if(std::regex_search(request.getURI(), matches, ActionInvoke)) 
    {
        std::cout << "ActionInvoke Handler\n";
        return new BindHttpActionHandler;
    } 
    else 
    if(std::regex_search(request.getURI(), matches, EventSubs)) 
    {
        std::cout << "EventSubs Handler\n";
        return new BindHttpEventObservHandler;
    } 
    else 
    if(std::regex_search(request.getURI(), matches, PropertySubs)) 
    {
        std::cout << "PropertySubs Handler\n";
        return new BindHttpPropertyObservHandler;
    }
    else if(std::regex_search(request.getURI(), matches, AffortanceRead)) 
    {
        std::cout << "AffortanceRead Handler\n";
        return new BindHttpAffordanceHandler;
    } 
    else
    if(std::regex_search(request.getURI(), matches, ThingRead)) 
    {
        std::cout << "ThingRead Handler\n";
        return new BindHttpThingsHandler;
    } 
    else
    {
        std::cout << "default Handler\n";
        return new BindHttpServerHandler;      
    }

    // if ( request.getURI() != ExpectedURI ) 
    // {
    //     return new BindHttpServerHandler;     
    // } 
    // How do I send response to client that the URI is invalid?
    // Example I need to send HTTP_NOT_FOUND 

};

HTTPRequestHandler* BindSseServerHandlerFactory::createRequestHandler(const HTTPServerRequest & request)
{
    std::string ExpectedURI = "/";

    if ( request.getURI() != ExpectedURI ) 
    {
        return new BindSseServerHandler;      
    // How do I send response to client that the URI is invalid?
    // Example I need to send HTTP_NOT_FOUND 
    }
};


void BindHttpServer::SetHandlers(Adapter_interaction_get GetHandleIn,
                                 Adapter_interaction_put PutHandleIn, 
                                 Adapter_thing_get thingGetIn,
                                 Adapter_affordance_get affordIn,
                                 Adapter_interaction_get SubsHandleIn)
{
	pLogger->PrintLog(LOG_LEVEL_DEBUG, LOGGER_COMP_HTTPSERVER,"SetHandlers call");    
    HandleAffordanceGet = GetHandleIn;
    HandleAffordancePut = PutHandleIn;
    HandleThing = thingGetIn;
    HandleAffordance = affordIn;
    HandleSubscription = SubsHandleIn;
}

BindHttpServer::BindHttpServer(void)
{

}
void BindHttpServer::Process(void)
{
}
void BindHttpServer::Initialize(unsigned int port)
{
    Port = port;
    pLogger = Logger::getInstance();
    Poco::DNSSD::initializeDNSSD();
    pLogger->PrintLog(LOG_LEVEL_DEBUG, LOGGER_COMP_HTTPSERVER,"SetHandlers call");    
    // SocketAddress WifiHotspot_Addr ("192.168.4.1",9999);
    // ServerSocket WifiHotspot_Socket(WifiHotspot_Addr);
    // HTTPServer WifiHotspot(new BindHttpServerHandlerFactory, WifiHotspot_Socket, new HTTPServerParams);    
    WotServer = new HTTPServer(new BindHttpServerHandlerFactory, ServerSocket(Port), new HTTPServerParams);
    WotSseServer = new HTTPServer(new BindSseServerHandlerFactory, ServerSocket(9091), new HTTPServerParams);
    // SocketAddress WotServer_Addr ("192.168.137.100",9090);
    // ServerSocket WotServer_Socket(WotServer_Addr);
    // WotServer = new HTTPServer(new BindHttpServerHandlerFactory, WotServer_Socket, new HTTPServerParams);
}

void BindHttpServer::RegisterServer(string ThingName)
{
    Poco::DNSSD::Service::Properties props;
    string LocalIp = GetLocalIpAddress();
    string tdpath = "http://"+LocalIp+":"+to_string(Port)+"/"+ThingName;
    props.add("td", tdpath);
    props.add("type", "Thing");
    props.add("scheme", "http");
 
    Service service(0,ThingName,"","_wot._tcp","","", Port, props);
    ServiceHandle serviceHandle = dnssdResponder->registerService(service);
}

void BindHttpServer::Start(void)
{
	pLogger->PrintLog(LOG_LEVEL_DEBUG, LOGGER_COMP_HTTPSERVER,":Initializing");
    pthread_mutex_init(&Event_Mutex, NULL);
    pthread_mutex_init(&Handler_Mutex, NULL);
    pthread_cond_init (&Event_cv, NULL);
    // WifiHotspot->start();
    WotServer->start();
    WotSseServer->start();
    // waitForTerminationRequest();  // wait for CTRL-C or kill
    dnssdResponder = new DNSSDResponder();
	dnssdResponder->start();
}

void BindHttpServer::Stop(void)
{
    // WifiHotspot->stop(); 
    WotServer->stop(); 
    WotSseServer->stop();
}

int BindHttpServer::main(const vector<string> &)
{
    // waitForTerminationRequest();  // wait for CTRL-C or kill

    cout << endl << "Shutting down..." << endl;
    // WifiHotspot->stop();
    WotServer->stop();
    WotSseServer->stop();
    return Application::EXIT_OK;
}