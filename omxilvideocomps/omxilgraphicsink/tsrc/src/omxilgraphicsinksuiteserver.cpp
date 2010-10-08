/*
* Copyright (c) 2009-2010 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description: 
*
*/


/**
 * @file
 * @internalTechnology
 */

#include "omxilgraphicsinksuiteserver.h"
#include "graphicsinkteststeps.h"

_LIT(KServerName,"tsu_omxilgraphicsink");
COmxGsTestSuiteServer* COmxGsTestSuiteServer::NewL()
/**
 * @return - Instance of the test server
 * Same code for Secure and non-secure variants
 * Called inside the MainL() function to create and start the
 * CTestServer derived server.
 */
	{
	COmxGsTestSuiteServer * server = new (ELeave) COmxGsTestSuiteServer();
	CleanupStack::PushL(server);

	server->ConstructL(KServerName);
	CleanupStack::Pop(server);
	return server;
	}


// Secure variants much simpler
// For EKA2, just an E32Main and a MainL()
LOCAL_C void MainL()
/**
 * Secure variant
 * Much simpler, uses the new Rendezvous() call to sync with the client
 */
	{
	// Leave the hooks in for platform security
#if (defined __DATA_CAGING__)
	RProcess().DataCaging(RProcess::EDataCagingOn);
	RProcess().DataCaging(RProcess::ESecureApiOn);
#endif
	CActiveScheduler* sched=NULL;
	sched=new(ELeave) CActiveScheduler;
	CActiveScheduler::Install(sched);
	COmxGsTestSuiteServer* server = NULL;
	// Create the CTestServer derived server
	TRAPD(err,server = COmxGsTestSuiteServer::NewL());
	if(!err)
		{
		// Sync with the client and enter the active scheduler
		RProcess::Rendezvous(KErrNone);
		sched->Start();
		}
	delete server;
	delete sched;
	}



GLDEF_C TInt E32Main()
/**
 * @return - Standard Epoc error code on process exit
 * Secure variant only
 * Process entry point. Called by client using RProcess API
 */
	{
	__UHEAP_MARK;
	CTrapCleanup* cleanup = CTrapCleanup::New();
	if(cleanup == NULL)
		{
		return KErrNoMemory;
		}
	TRAPD(err,MainL());
	delete cleanup;
	__UHEAP_MARKEND;
	return err;
    }


CTestStep* COmxGsTestSuiteServer::CreateTestStep(const TDesC& aStepName)
/**
 * @return - A CTestStep derived instance
 * Secure and non-secure variants
 * Implementation of CTestServer pure virtual
 */
	{
	CTestStep* testStep = NULL;
	
	// ---------------------------------------------------------------
	// Graphics Sink Tests
	// ---------------------------------------------------------------
	if(aStepName == KOmxGsTest0001Step00) 
			      testStep = new COmxGsTest0001Step00();
	else if(aStepName == KOmxGsTest0001Step01) 
		              testStep = new COmxGsTest0001Step01();
	else if(aStepName == KOmxGsTest0001Step02)
	              testStep = new COmxGsTest0001Step02();
	else if(aStepName == KOmxGsTest0001Step03)
	              testStep = new COmxGsTest0001Step03();
	else if(aStepName == KOmxGsTest0001Step04)
	              testStep = new COmxGsTest0001Step04();
	else if(aStepName == KOmxGsTest0001Step05)
					testStep = new COmxGsTest0001Step05();
	else if(aStepName == KOmxGsTest0001Step06)
	              testStep = new COmxGsTest0001Step06();
	else if(aStepName == KOmxGsTest0001Step07)
	              testStep = new COmxGsTest0001Step07();
	else if(aStepName == KOmxGsTest0001Step08)
	              testStep = new COmxGsTest0001Step08();
	else if(aStepName == KOmxGsTest0001Step09)
	              testStep = new COmxGsTest0001Step09();
	
	else if(aStepName == KOmxGsTest0002Step01)
	              testStep = new COmxGsTest0002Step01();
	else if(aStepName == KOmxGsTest0002Step02)
	              testStep = new COmxGsTest0002Step02();
	else if(aStepName == KOmxGsTest0002Step03)
	              testStep = new COmxGsTest0002Step03();
	else if(aStepName == KOmxGsTest0002Step04)
	              testStep = new COmxGsTest0002Step04();
	
	else if(aStepName == KOmxGsTest0003Step01)
	              testStep = new COmxGsTest0003Step01();
	else if(aStepName == KOmxGsTest0003Step02)
	              testStep = new COmxGsTest0003Step02();
	else if(aStepName == KOmxGsTest0003Step03)
	              testStep = new COmxGsTest0003Step03();
	else if(aStepName == KOmxGsTest0003Step04)
	              testStep = new COmxGsTest0003Step04();
	
	else if(aStepName == KOmxGsTest0004Step01)
	              testStep = new COmxGsTest0004Step01();
	else if(aStepName == KOmxGsTest0004Step02)
	              testStep = new COmxGsTest0004Step02();
	else if(aStepName == KOmxGsTest0004Step03)
	              testStep = new COmxGsTest0004Step03();
	else if(aStepName == KOmxGsTest0004Step04)
	              testStep = new COmxGsTest0004Step04();
	else if(aStepName == KOmxGsTest0004Step05)
	              testStep = new COmxGsTest0004Step05();
	else if(aStepName == KOmxGsTest0004Step06)
	              testStep = new COmxGsTest0004Step06();
	
	else if(aStepName == KOmxGsTest0005Step01)
	              testStep = new COmxGsTest0005Step01();
	else if(aStepName == KOmxGsTest0005Step02)
	              testStep = new COmxGsTest0005Step02();
	else if(aStepName == KOmxGsTest0005Step03)
	              testStep = new COmxGsTest0005Step03();
	else if(aStepName == KOmxGsTest0005Step04)
	              testStep = new COmxGsTest0005Step04();
	else if(aStepName == KOmxGsTest0005Step05)
	              testStep = new COmxGsTest0005Step05();
	
	else if(aStepName == KOmxGsTest0006Step01)
		              testStep = new COmxGsTest0006Step01();
	else if(aStepName == KOmxGsTest0006Step02)
		              testStep = new COmxGsTest0006Step02();

	else if(aStepName == KOmxGsTest0007Step01)
	              testStep = new COmxGsTest0007Step01();
	else if(aStepName == KOmxGsTest0007Step02)
	              testStep = new COmxGsTest0007Step02();
	else if(aStepName == KOmxGsTest0007Step03)
	              testStep = new COmxGsTest0007Step03();
	else if(aStepName == KOmxGsTest0007Step04)
	              testStep = new COmxGsTest0007Step04();
	else if(aStepName == KOmxGsTest0008Step01)
	              testStep = new COmxGsTest0008Step01();
	else if(aStepName == KOmxGsTest0009Step00)
	              testStep = new COmxGsTest0009Step00();
	else if(aStepName == KOmxGsTest0010Step00)
	              testStep = new COmxGsTest0010Step00();

	return testStep;
	}
