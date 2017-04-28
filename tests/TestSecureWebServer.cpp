/**
* Copyright 2017 IBM Corp. All Rights Reserved.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
*/


#include "utils/UnitTest.h"
#include "utils/IWebClient.h"
#include "utils/IWebServer.h"
#include "utils/Log.h"
#include "utils/ThreadPool.h"
#include "utils/Time.h"

class TestSecureWebServer : UnitTest
{
public:
	//! Construction
	TestSecureWebServer() : UnitTest("TestSecureWebServer"),
		m_bHTTPSTested(false),
		m_bWSSTested(false),
		m_bClientClosed(false)
	{}

	virtual void RunTest()
	{
		Time start;

		ThreadPool pool(1);

		// test web requests
		IWebServer * pSecureServer = IWebServer::Create("./etc/tests/server.crt",
			"./etc/tests/server.key", "","", 8080 );
		pSecureServer->AddEndpoint("/test_https", DELEGATE(TestSecureWebServer, OnTestHTTPS, IWebServer::RequestSP, this));
		pSecureServer->AddEndpoint("/test_wss", DELEGATE(TestSecureWebServer, OnTestWSS, IWebServer::RequestSP, this));
		Test(pSecureServer->Start());

		m_bClientClosed = false;

		IWebClient::SP spClient = IWebClient::Request("https://127.0.0.1:8080/test_https", IWebClient::Headers(), "GET", "",
			DELEGATE(TestSecureWebServer, OnSecureResponse, IWebClient::RequestData *, this),
			DELEGATE(TestSecureWebServer, OnState, IWebClient *, this));

		start = Time();
		while (!m_bClientClosed && (Time().GetEpochTime() - start.GetEpochTime()) < 15.0)
		{
			pool.ProcessMainThread();
			boost::this_thread::sleep(boost::posix_time::milliseconds(50));
		}
		Test(m_bHTTPSTested);

		m_bClientClosed = false;
		spClient->SetURL("wss://127.0.0.1:8080/test_wss");
		spClient->SetStateReceiver(DELEGATE(TestSecureWebServer, OnState, IWebClient *, this));
		spClient->SetDataReceiver(DELEGATE(TestSecureWebServer, OnWebSocketResponse, IWebClient::RequestData *, this));
		spClient->SetFrameReceiver(DELEGATE(TestSecureWebServer, OnSecureClientFrame, IWebSocket::FrameSP, this));
		Test(spClient->Send());

		start = Time();
		while ((Time().GetEpochTime() - start.GetEpochTime()) < 10.0)
		{
			spClient->SendText("Testing text");
			spClient->SendBinary("Testing binary");

			pool.ProcessMainThread();
			boost::this_thread::sleep(boost::posix_time::milliseconds(0));
		}
		Test(spClient->Close());
		Test(m_bWSSTested);

		while (!m_bClientClosed)
		{
			pool.ProcessMainThread();
			boost::this_thread::sleep(boost::posix_time::milliseconds(50));
		}

		spClient.reset();
		delete pSecureServer;
	}

	void OnTestHTTPS(IWebServer::RequestSP a_spRequest)
	{
		Log::Debug("TestSecureWebServer", "OnTestHTTPS()");
		Test(a_spRequest.get() != NULL);
		Test(a_spRequest->m_RequestType == "GET");

		a_spRequest->m_spConnection->SendAsync("HTTP/1.1 200 Hello World\r\nConnection: close\r\n\r\n");
	}

	void OnTestWSS(IWebServer::RequestSP a_spRequest)
	{
		Log::Debug("TestSecureWebServer", "OnTestWSS()");
		Test(a_spRequest.get() != NULL);
		Test(a_spRequest->m_RequestType == "GET");

		IWebServer::Headers::iterator iWebSocketKey = a_spRequest->m_Headers.find("Sec-WebSocket-Key");
		Test(iWebSocketKey != a_spRequest->m_Headers.end());

		a_spRequest->m_spConnection->SetFrameReceiver(DELEGATE(TestSecureWebServer, OnServerFrame, IWebSocket::FrameSP, this));
		a_spRequest->m_spConnection->StartWebSocket(iWebSocketKey->second);
		Test(a_spRequest->m_spConnection->IsWebSocket());

		//a_spRequest->m_spConnection->SendAsync("HTTP/1.1 200 Hello World\r\nConnection: close\r\n\r\n");
	}

	void OnWebSocketResponse(IWebClient::RequestData * a_pResonse)
	{
		Log::Debug("TestSecureWebServer", "WebSocket response, status code %u : %s",
			a_pResonse->m_StatusCode, a_pResonse->m_StatusMessage.c_str());
	}

	void OnServerFrame(IWebSocket::FrameSP a_spFrame)
	{
		IWebSocket::SP spSocket = a_spFrame->m_wpSocket.lock();

		Log::Debug("TestSecureWebServer", "OnServerFrame() OpCode: %d, Data: %s", a_spFrame->m_Op, 
			a_spFrame->m_Op == IWebSocket::TEXT_FRAME ? a_spFrame->m_Data.c_str() : StringUtil::Format( "%u bytes", a_spFrame->m_Data.size()).c_str() );

		if ( spSocket )
		{
			if (a_spFrame->m_Op == IWebSocket::BINARY_FRAME)
				spSocket->SendBinary(a_spFrame->m_Data);
			else if (a_spFrame->m_Op == IWebSocket::TEXT_FRAME)
				spSocket->SendText(a_spFrame->m_Data);
		}
	}

	void OnSecureClientFrame(IWebSocket::FrameSP a_spFrame)
	{
		Log::Debug("TestSecureWebServer", "OnSecureClientFrame() OpCode: %d, Data: %s", a_spFrame->m_Op, a_spFrame->m_Data.c_str());
		m_bWSSTested = true;
	}
	void OnError()
	{
		Log::Debug("TestSecureWebServer", "OnError()");
	}

	void OnState(IWebClient * a_pConnector)
	{
		Log::Debug("TestSecureWebServer", "OnState(): %d", a_pConnector->GetState());
		if (a_pConnector->GetState() == IWebClient::CLOSED || a_pConnector->GetState() == IWebClient::DISCONNECTED)
			m_bClientClosed = true;
	}

	void OnSecureResponse(IWebClient::RequestData * a_pResponse)
	{
		Log::Debug("TestSecureWebServer", "OnSecureResponse(): Version: %s, Status: %u, Content: %s",
			a_pResponse->m_Version.c_str(), a_pResponse->m_StatusCode, a_pResponse->m_Content.c_str());

		Test(a_pResponse->m_StatusCode == 200);
		m_bHTTPSTested = true;
	}

	bool m_bHTTPSTested;
	bool m_bWSSTested;
	bool m_bClientClosed;
};

TestSecureWebServer TEST_SECURE_WEB_SERVER;
