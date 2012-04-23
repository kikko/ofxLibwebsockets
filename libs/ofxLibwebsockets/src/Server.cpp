//
//  Server.cpp
//  ofxLibwebsockets
//
//  Created by Brett Renfer on 4/11/12.
//  Copyright (c) 2012 Robotconscience. All rights reserved.
//

#include "ofxLibwebsockets/Server.h"
#include "ofxLibwebsockets/Util.h"

#include "ofEvents.h"
#include "ofUtils.h"

namespace ofxLibwebsockets {

    //--------------------------------------------------------------
    Server::Server(){
        context = NULL;
        waitMillis = 50;
        count_pollfds = 0;
        reactors.push_back(this);
        
        defaultOptions = defaultServerOptions();
    }

    //--------------------------------------------------------------
    bool Server::setup( int _port, bool bUseSSL )
    {
        // setup with default protocol (http) and allow ALL other protocols
        defaultOptions.port     = _port;
        defaultOptions.bUseSSL  = bUseSSL;
        
        if ( defaultOptions.port == 80 && defaultOptions.bUseSSL == true ){
            ofLog( OF_LOG_WARNING, "SSL IS NOT USUALLY RUN OVER DEFAULT PORT (80). THIS MAY NOT WORK!");
        }
        
        setup( defaultOptions );
    }

    //--------------------------------------------------------------
    bool Server::setup( ServerOptions options ){
        port = options.port;
        document_root = options.documentRoot;
        
        // NULL protocol is required by LWS
        struct libwebsocket_protocols null_protocol = { NULL, NULL, 0 };
        
        // NULL name = any protocol
        struct libwebsocket_protocols null_name_protocol = { NULL, lws_callback, sizeof(Connection) };
        
        //setup protocols
        lws_protocols.clear();
        
        //register main protocol
        registerProtocol( options.protocol, serverProtocol );
        
        //register any added protocols
        for (int i=0; i<protocols.size(); ++i){
            struct libwebsocket_protocols lws_protocol = {
                ( protocols[i].first == "NULL" ? NULL : protocols[i].first.c_str() ),
                lws_callback,
                sizeof(Connection)
            };
            lws_protocols.push_back(lws_protocol);
        }
        lws_protocols.push_back(null_protocol);
        
        // make cert paths  null if not using ssl
        const char * sslCert = NULL;
        const char * sslKey = NULL;
        
        if ( defaultOptions.bUseSSL ){
            sslCert = defaultOptions.sslCertPath.c_str();
            sslKey = defaultOptions.sslKeyPath.c_str();
        }
        
        int opts = 0;
        context = libwebsocket_create_context( port, NULL, &lws_protocols[0], libwebsocket_internal_extensions, sslCert, sslKey, -1, -1, opts);
        
        if (context == NULL){
            std::cerr << "libwebsocket init failed" << std::endl;
            return false;
        } else {
            startThread(true, false); // blocking, non-verbose        
            return true;
        }
    }
    
    //--------------------------------------------------------------
    void Server::broadcast( string message ){
        // loop through all protocols and broadcast!
        for (int i=0; i<protocols.size(); i++){
            protocols[i].second->broadcast( message );
        }
    }

    //--------------------------------------------------------------
    void Server::threadedFunction()
    {
        while (isThreadRunning())
        {
            for (int i=0; i<protocols.size(); ++i)
                if (protocols[i].second != NULL){
                    //lock();
                    protocols[i].second->execute();
                    //unlock();
                }
            libwebsocket_service(context, waitMillis);
        }
    }
}