#include <iostream>
#include <string>
#include <vector>
#include <regex>
#include "Poco/URI.h"
#include <Poco/DNSSD/DNSSDResponder.h>
#include <Poco/DNSSD/Avahi/Avahi.h>
#include "BindHttpServer.h"

#include "Logger.h"


using namespace Poco::Net;
using namespace Poco::Util;
using namespace std;
using namespace Poco::DNSSD;

unsigned int  BindHttpThingsHandler::ParseThingsURI(std::string RxURI,std::string* ArrayOfURI)
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

void BindHttpThingsHandler::handleRequest(HTTPServerRequest &req, HTTPServerResponse &resp)
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
    const Poco::Net::SocketAddress& serverAddr = req.serverAddress();
    Host = "http://" + serverAddr.toString();
    pLogger = Logger::getInstance();

    resp.setStatus(HTTPResponse::HTTP_OK);

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
        Root_Name = ThingURI[0];
        Thing_Name = ThingURI[0];
        if (RequestMethod == "GET")
        {
            HandleThing(Host,Thing_Name,Root_Name, &Response);
            Response.stringify(out);         
        }
    }
};