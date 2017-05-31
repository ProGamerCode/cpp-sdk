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


#include "UnitTest.h"
#include "utils/RTTI.h"

namespace TestRTTIClasses {

	class TestA
	{
	public:
		RTTI_DECL();

		TestA() : m_A(1), m_B(42)
		{}

		int	m_A;
		int m_B;
	};
	RTTI_IMPL_BASE(TestA);

	class TestB : public TestA
	{
	public:
		RTTI_DECL();

		TestB() : m_C(2)
		{}

		int m_C;
	};
	RTTI_IMPL(TestB, TestA);
	

	class TestC : public TestA
	{
	public:
		RTTI_DECL();

		TestC() : m_D(5)
		{}

		int m_D;
	};
	RTTI_IMPL(TestC, TestA);
	

};

using namespace TestRTTIClasses;

class TestRTTI : UnitTest
{
public:
	//! Construction
	TestRTTI() : UnitTest("TestRTTI")
	{}

	virtual void RunTest()
	{
		TestA * pA = new TestC();

		// test upcasting
		TestC * pTest = DynamicCast<TestC>(pA);		// 
		Test(pTest != NULL);

		TestA * pTest1 = DynamicCast<TestA>(pTest);
		Test(pTest1 != NULL);

		// test null
		TestB * pTest2 = DynamicCast<TestB>(pA);
		Test(pTest2 == NULL);

		// check that downcast works
		TestA * pTest3 = DynamicCast<TestA>(pTest);
		Test( pTest3 != NULL );

		delete pA;
		pA = NULL;
	}

};

TestRTTI TEST_RTTI;


