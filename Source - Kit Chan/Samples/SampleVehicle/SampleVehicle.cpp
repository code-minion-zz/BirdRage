// This code contains NVIDIA Confidential Information and is disclosed to you 
// under a form of NVIDIA software license agreement provided separately to you.
//
// Notice
// NVIDIA Corporation and its licensors retain all intellectual property and
// proprietary rights in and to this software and related documentation and 
// any modifications thereto. Any use, reproduction, disclosure, or 
// distribution of this software and related documentation without an express 
// license agreement from NVIDIA Corporation is strictly prohibited.
// 
// ALL NVIDIA DESIGN SPECIFICATIONS, CODE ARE PROVIDED "AS IS.". NVIDIA MAKES
// NO WARRANTIES, EXPRESSED, IMPLIED, STATUTORY, OR OTHERWISE WITH RESPECT TO
// THE MATERIALS, AND EXPRESSLY DISCLAIMS ALL IMPLIED WARRANTIES OF NONINFRINGEMENT,
// MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE.
//
// Information and code furnished is believed to be accurate and reliable.
// However, NVIDIA Corporation assumes no responsibility for the consequences of use of such
// information or for any infringement of patents or other rights of third parties that may
// result from its use. No license is granted by implication or otherwise under any patent
// or patent rights of NVIDIA Corporation. Details are subject to change without notice.
// This code supersedes and replaces all information previously supplied.
// NVIDIA Corporation products are not authorized for use as critical
// components in life support devices or systems without express written approval of
// NVIDIA Corporation.
//
// Copyright (c) 2008-2012 NVIDIA Corporation. All rights reserved.
// Copyright (c) 2004-2008 AGEIA Technologies, Inc. All rights reserved.
// Copyright (c) 2001-2004 NovodeX AG. All rights reserved.  


#include "SampleVehicle.h"
#include "RendererMemoryMacros.h"
#include "RenderMeshActor.h"
#include "SampleVehicle_VehicleCooking.h"
#include "SampleVehicle_SceneQuery.h"
#include "PxPhysicsAPI.h"
#include "extensions/PxExtensionsAPI.h"
#include "RenderPhysX3Debug.h"

#include "SampleVehicleInputEventIds.h"
#include <SampleFrameworkInputEventIds.h>
#include <SampleBaseInputEventIds.h>
#include <SamplePlatform.h>
#include <SampleUserInput.h>
#include <SampleUserInputIds.h>
#include <SampleUserInputDefines.h>


#ifdef PX_PS3
#include "ps3/PxPS3Config.h"
#endif

REGISTER_SAMPLE(SampleVehicle, "SampleVehicle")	// PT: registers the sample. Comment this to revert to "hello world" sample.

enum
{
	LOAD_TYPE_VEHICLE=0,
	LOAD_TYPE_LOOPTHELOOP,
	LOAD_TYPE_SKYDOME,
	MAX_NUM_LOAD_TYPES
};
static PxU32 gLoadType = MAX_NUM_LOAD_TYPES;

using namespace physx;
using namespace SampleRenderer;
using namespace SampleFramework;

///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////
//VEHICLE START PARAMETERS
///////////////////////////////////

#define NUM_PLAYER_CARS 1
static PxTransform gPlayerCarStartTransforms[NUM_PLAYER_CARS] = 
{
	PxTransform(PxVec3(-154.699753f, 9.863837f, 87.684113f), PxQuat(-0.000555f, -0.267015, 0.000155, -0.963692 ))
};

#define NUM_NONPLAYER_4W_VEHICLES 7
static PxTransform gVehicle4WStartTransforms[NUM_NONPLAYER_4W_VEHICLES] = 
{
	//Stack of 2 cars
	PxTransform(PxVec3(-78.796158f,9.9764671f,155.47554f), PxQuat(0.00062184106f,0.97696519f,-0.0029967001f,0.21337670f)),
	PxTransform(PxVec3(-78.796158f,12.9764671f,155.47554f), PxQuat(0.00062184106f,0.97696519f,-0.0029967001f,0.21337670f)),

	//Stack of 2 cars
	PxTransform(PxVec3(-80.391357f,9.9760214f,161.93578f), PxQuat(1.0251222e-005f, 0.98799443f, -6.0369461e-005f, 0.15448983f)),
	PxTransform(PxVec3(-80.391357f,12.9760214f,161.93578f), PxQuat(1.0251222e-005f, 0.98799443f, -6.0369461e-005f, 0.15448983f)),

	//Bookends to car stacks
	PxTransform(PxVec3(-77.945328f,9.9765568f,151.78404f), PxQuat(0.0014836629f,-0.86193967f,0.0024594457f,0.50700283f)),
	PxTransform(PxVec3(-80.395328f,9.0786600f,167.44662f), PxQuat(0.0014836629f,-0.86193967f,0.0024594457f,0.50700283f)),

	//Car in-between two ramps
	PxTransform(PxVec3(116.04498f,9.9760214f,139.02933f), PxQuat(7.5981094e-005f,-0.38262743f,-3.1461415e-005f,-0.92390275f)),
};

PxU32 gNumVehicleAdded=0;

////////////////////////////////////////////////////////////////
//VEHICLE SETUP DATA 
////////////////////////////////////////////////////////////////

PxF32 gChassisMass=1500.0f;
PxF32 gSuspensionShimHeight=0.125f;

////////////////////////////////////////////////////////////////
//RENDER USER DATA TO ASSOCIATE EACH RENDER MESH WITH A 
//VEHICLE PHYSICS COMPONENT
////////////////////////////////////////////////////////////////

enum
{
	CAR_PART_FRONT_LEFT_WHEEL=0,
	CAR_PART_FRONT_RIGHT_WHEEL,
	CAR_PART_REAR_LEFT_WHEEL,
	CAR_PART_REAR_RIGHT_WHEEL,
	CAR_PART_CHASSIS,
	CAR_PART_WINDOWS,
	NUM_CAR4W_RENDER_COMPONENTS
};

static char gCarPartNames[NUM_CAR4W_RENDER_COMPONENTS][64]=
{
	"frontwheelleftshape",	
	"frontwheelrightshape",	
	"backwheelleftshape",	
	"backwheelrightshape",
	"car_02_visshape",
	"car_02_windowsshape"	
};

struct CarRenderUserData
{
	PxU8	carId;
	PxU8	carPart;
	PxU8	carPartDependency;
	PxU8	pad;
};

static CarRenderUserData gCar4WRenderUserData[NUM_CAR4W_RENDER_COMPONENTS] = 
{
	//wheel fl		wheel fr		wheel rl		wheel rl		chassis			windows			
	{0,0,255},		{0,1,255},		{0,2,255},		{0,3,255},		{0,4,255},		{0,4,4}	
};

CarRenderUserData gVehicleRenderUserData[NUM_PLAYER_CARS + NUM_NONPLAYER_4W_VEHICLES][NUM_CAR4W_RENDER_COMPONENTS];

static PxVec3 g4WCarPartDependencyOffsets[NUM_CAR4W_RENDER_COMPONENTS]=
{
	PxVec3(0,0,0),
	PxVec3(0,0,0),
	PxVec3(0,0,0),
	PxVec3(0,0,0),
	PxVec3(0,0,0),
	PxVec3(0,0,0)
};

RenderMeshActor* gRenderMeshActors[NUM_CAR4W_RENDER_COMPONENTS]={NULL,NULL,NULL,NULL,NULL,NULL};

////////////////////////////////////////////////////////////////
//TRANSFORM APPLIED TO CHASSIS RENDER MESH VERTS 
//THAT IS REQUIRED TO PLACE AABB OF CHASSIS RENDER MESH AT ORIGIN
//AT CENTRE-POINT OF WHEELS.
////////////////////////////////////////////////////////////////

static PxVec3 gChassisMeshTransform(0,0,0);

////////////////////////////////////////////////////////////////
//WHEEL CENTRE OFFSETS FROM CENTRE OF CHASSIS RENDER MESH AABB
//OF 4-WHEELED VEHICLE 
////////////////////////////////////////////////////////////////

static PxVec3 gWheelCentreOffsets4[4];

////////////////////////////////////////////////////////////////
//CONVEX HULL OF RENDER MESH FOR CHASSIS AND WHEELS OF
//4-WHEELED VEHICLE
////////////////////////////////////////////////////////////////

static PxConvexMesh* gChassisConvexMesh=NULL;
static PxConvexMesh* gWheelConvexMeshes4[4]={NULL,NULL,NULL,NULL};

////////////////////////////////////////////////////////////////
//Revolute joints and gian pendula
////////////////////////////////////////////////////////////////

#define MAX_NUM_PENDULA 4

PxTransform gPendulaBallStartTransforms[MAX_NUM_PENDULA]=
{
	PxTransform(PxVec3( 151.867020 , 11.364232 , 97.350906 ), PxQuat( -0.000106 , -0.951955 , 0.000370 , -0.306239 ) ) ,
	PxTransform(PxVec3( 163.001953 , 11.364203 , 77.854568 ), PxQuat( -0.000070 , -0.971984 , 0.000323 , -0.235048 ) ) ,
	PxTransform(PxVec3( 173.971405 , 11.364145 , 45.944290 ), PxQuat( -0.000050 , -0.994988 , 0.000448 , -0.099990 ) ) ,
	PxTransform(PxVec3( 179.876785 , 11.364090 , 3.679581 ), PxQuat( -0.000024 , -1.000000 , 0.000431 , -0.000193 ) ) 
};

PxF32 gPendulumBallRadius=2.0f;
PxF32 gPendulumBallMass=1000.0f;
PxF32 gPendulumShaftLength=10.0f;
PxF32 gPendulumShaftWidth=0.1f;
PxF32 gPendulumSuspensionStructureWidth=13.0f;

PxRevoluteJoint* gRevoluteJoints[MAX_NUM_PENDULA]={NULL,NULL,NULL,NULL};
PxF32 gRevoluteJointDriveSpeeds[MAX_NUM_PENDULA]={0.932f,1.0f,1.237f,0.876f};
PxF32 gRevoluteJointTimers[MAX_NUM_PENDULA]={0,0,0,0};
PxU32 gNumRevoluteJoints=0;
PxF32 gRevoluteJointMaxTheta=0;

////////////////////////////////////////////////////////////////
//Route
////////////////////////////////////////////////////////////////

PxTransform gWayPoints[35]=
{
	PxTransform(PxVec3( -154.699753 , 9.863837 , 87.684113 ), PxQuat( -0.000555 , -0.267015 , 0.000155 , -0.963692 ) ) ,
	PxTransform(PxVec3( -133.385757 , 9.863703 , 118.680717 ), PxQuat( -0.000620 , -0.334975 , 0.000219 , -0.942227 ) ) ,
	PxTransform(PxVec3( -103.853020 , 9.863734 , 146.631256 ), PxQuat( -0.000560 , -0.474673 , 0.000301 , -0.880162 ) ) ,
	PxTransform(PxVec3( -20.397881 , 9.862607 , 177.345657 ), PxQuat( -0.000938 , -0.674493 , 0.000867 , -0.738281 ) ) ,
	PxTransform(PxVec3( 14.272619 , 9.863173 , 178.745789 ), PxQuat( -0.000736 , -0.723563 , 0.000740 , -0.690257 ) ) ,
	PxTransform(PxVec3( 49.057743 , 9.862507 , 173.464539 ), PxQuat( -0.000387 , -0.794072 , 0.001316 , -0.607822 ) ) ,
	PxTransform(PxVec3( 82.226036 , 9.862642 , 159.649200 ), PxQuat( -0.000670 , -0.853613 , 0.001099 , -0.520905 ) ) ,
	PxTransform(PxVec3( 158.675018 , 9.864265 , 86.166740 ), PxQuat( -0.000052 , -0.973169 , 0.000286 , -0.230092 ) ) ,
	PxTransform(PxVec3( 165.983170 , 9.857763 , 67.031097 ), PxQuat( -0.014701 , -0.979587 , 0.002133 , 0.200471 ) ) ,
	PxTransform(PxVec3( 177.577393 , 9.864532 , 30.462427 ), PxQuat( 0.000022 , -0.998473 , -0.000423 , -0.055239 ) ) ,
	PxTransform(PxVec3( 180.127686 , 9.864151 , -10.116920 ), PxQuat( 0.000011 , 0.999869 , 0.000295 , -0.016194 ) ) ,
	PxTransform(PxVec3( 174.281616 , 9.863608 , -42.319435 ), PxQuat( 0.000168 , 0.989981 , -0.001008 , -0.141194 ) ) ,
	PxTransform(PxVec3( 166.054413 , 9.864248 , -68.973045 ), PxQuat( -0.000052 , 0.985021 , -0.000213 , -0.172433 ) ) ,
	PxTransform(PxVec3( 154.505814 , 9.864180 , -77.992622 ), PxQuat( -0.000098 , 0.754213 , -0.000129 , -0.656630 ) ) ,
	PxTransform(PxVec3( 140.834076 , 9.863158 , -78.122841 ), PxQuat( -0.000730 , 0.709598 , -0.000737 , -0.704606 ) ) ,
	PxTransform(PxVec3( 100.152573 , 9.853898 , -78.407364 ), PxQuat( 0.066651 , 0.683499 , 0.070412 , -0.723484 ) ) ,
	PxTransform(PxVec3( 68.866241 , 10.293923 , -77.979095 ), PxQuat( -0.045687 , 0.703115 , -0.044258 , -0.708225 ) ), 
	PxTransform(PxVec3( 26.558704 , 10.495461 , -77.729164 ), PxQuat( -0.018349 , 0.708445 , -0.018563 , -0.705283 ) ) ,
	PxTransform(PxVec3( -2.121686 , 9.860541 , -77.735886 ), PxQuat( -0.000792 , 0.706926 , -0.000792 , -0.707287 ) ) ,
	PxTransform(PxVec3( -28.991186 , 9.867218 , -77.722122 ), PxQuat( -0.000251 , 0.706926 , -0.000251 , -0.707287 ) ) ,
	PxTransform(PxVec3( -59.285870 , 9.861916 , -77.746246 ), PxQuat( -0.001042 , 0.708723 , -0.001048 , -0.705485 ) ) ,
	PxTransform(PxVec3( -73.170433 , 9.863565 , -78.034157 ), PxQuat( -0.001567 , 0.736642 , -0.001323 , -0.676280 ) ) ,
	PxTransform(PxVec3( -77.792152 , 9.864209 , -87.821510 ), PxQuat( 0.000006 , 0.999772 , -0.000294 , -0.021342 ) ) ,
	PxTransform(PxVec3( -78.164032 , 9.862655 , -114.313690 ), PxQuat( -0.000010 , 0.999962 , -0.001280 , -0.008618 ) ), 
	PxTransform(PxVec3( -78.567627 , 9.862757 , -137.274292 ), PxQuat( -0.000010 , 0.999961 , -0.001146 , -0.008796 ) ) ,
	PxTransform(PxVec3( -80.392769 , 9.863695 , -149.740082 ), PxQuat( -0.000149 , 0.987276 , -0.000512 , -0.159015 ) ) ,
	PxTransform(PxVec3( -88.452507 , 9.864114 , -152.396698 ), PxQuat( -0.000355 , 0.622921 , -0.000278 , -0.782285 ) ) ,
	PxTransform(PxVec3( -106.042450 , 9.863844 , -144.640076 ), PxQuat( -0.000485 , 0.459262 , -0.000133 , -0.888301 ) ) ,
	PxTransform(PxVec3( -134.893997 , 15.093562 , -114.433586 ), PxQuat( 0.073759 , 0.354141 , 0.027569 , -0.931871 ) ) ,
	PxTransform(PxVec3( -145.495453 , 9.864394 , -101.019417 ), PxQuat( -0.000351 , 0.305924 , -0.000104 , -0.952056 ) ) ,
	PxTransform(PxVec3( -152.808212 , 9.864192 , -86.800613 ), PxQuat( -0.000099 , 0.206095 , -0.000015 , -0.978532 ) ) ,
	PxTransform(PxVec3( -156.457855 , 9.864244 , -81.270035 ), PxQuat( -0.000420 , 0.201378 , -0.000086 , -0.979514 ) ) ,
	PxTransform(PxVec3( -169.079376 , 10.192255 , -48.906967 ), PxQuat( 0.003633 , 0.179159 , 0.000892 , -0.983813 ) ) ,
	PxTransform(PxVec3( -151.470123 , 7.783551 , -17.838057 ), PxQuat( 0.029215 , -0.077987 , -0.050732 , -0.995234 ) ) ,
	PxTransform(PxVec3( -171.549225 , 9.864206 , 47.239426 ), PxQuat( -0.000319 , -0.159609 , 0.000039 , -0.987180 ) ) 
};
PxU32 gNumWayPoints=35;

///////////////////////////////////////////////////////////////////////////////


SampleVehicle::SampleVehicle(PhysXSampleApplication& app) :
	PhysXSample							(app),
	mTerrainVB							(NULL),
	mNbTerrainVerts						(0),
	mNbIB								(0),
	mTerrainMaterial					(NULL),
	mPlayerVehicle						(0),
	mTerrainSize						(256),
	mTerrainWidth						(2.0f),
	mHFActor							(NULL),
	mHideScreenText						(false),
	mDebugRenderFlag					(false),
	mFixCar								(false),
	mBackToStart						(false),
	mForwardSpeedHud					(0.0f)
{
	mCreateGroundPlane	= false;
	mUseDebugStepper	= false;

#if PX_DEBUG_VEHICLE_ON
	mDebugRenderActiveGraphChannelWheel = 0;
	mDebugRenderActiveGraphChannelEngine = 0;
	mTelemetryData4W = NULL;
#endif
}

SampleVehicle::~SampleVehicle()
{
}

///////////////////////////////////////////////////////////////////////////////

void SampleVehicle::customizeSample(SampleSetup& setup)
{
	setup.mName	= "SampleVehicle";
}

void SampleVehicle::setupTelemetryData()
{
#if PX_DEBUG_VEHICLE_ON

	mDebugRenderActiveGraphChannelWheel=PxVehicleGraph::eCHANNEL_WHEEL_OMEGA;
	mDebugRenderActiveGraphChannelEngine=PxVehicleGraph::eCHANNEL_ENGINE_REVS;

	mTelemetryData4W = PxVehicleTelemetryData::allocate(4);

	const PxF32 graphSizeX=0.25f;
	const PxF32 graphSizeY=0.25f;
	const PxF32 engineGraphPosX=0.5f;
	const PxF32 engineGraphPosY=0.5f;
	const PxF32 wheelGraphPosX[4]={0.75f,0.25f,0.75f,0.25f};
	const PxF32 wheelGraphPosY[4]={0.75f,0.75f,0.25f,0.25f};
	const PxVec3 backgroundColor(255,255,255);
	const PxVec3 lineColorHigh(255,0,0);
	const PxVec3 lineColorLow(0,0,0);
	mTelemetryData4W->setup
		 (graphSizeX,graphSizeY,
		  engineGraphPosX,engineGraphPosY,
		  wheelGraphPosX,wheelGraphPosY,
		  backgroundColor,lineColorHigh,lineColorLow);
	
#endif
}

///////////////////////////////////////////////////////////////////////////////

void SampleVehicle::onInit()
{
#if defined(PX_PS3)
	mNbThreads = 1;
#elif defined(PX_X360)
	mNbThreads = 2;
#else
	mNbThreads = 3;
#endif

	PhysXSample::onInit();

#ifdef PX_PS3
	PxPS3Config::setSceneParamInt(getActiveScene(), PxPS3ConfigParam::eMEM_CONTACT_BLOCKS, 18);
	PxPS3Config::setSceneParamInt(getActiveScene(), PxPS3ConfigParam::eMEM_FRICTION_BLOCKS, 17);
	PxPS3Config::setSceneParamInt(getActiveScene(), PxPS3ConfigParam::eMEM_SHADER_BLOCKS, 5);
#endif

	//Create physx objects.
	createStandardMaterials();	
	createVehicles();
	createTrack(mTerrainSize, mTerrainWidth, mTerrainSize*4.0f);
	createObstacles();	

	//Setup camera.
	setCameraController(NULL);

	//Setup debug render data.
	setupTelemetryData();

	//Setup the waypoint system.
	mWayPoints.setWayPoints(gWayPoints,gNumWayPoints);

	//Load the skydome.
	gLoadType = LOAD_TYPE_SKYDOME;
	importRAWFile("sky_mission_race1.raw", 1.0f);

	//Set up the fog.
	getRenderer()->setFog(SampleRenderer::RendererColor(40,40,40), 225.0f);
}

void SampleVehicle::newMesh(const RAWMesh& data)
{
	if(LOAD_TYPE_LOOPTHELOOP==gLoadType)
	{
		PxMaterial* saved = mMaterial;
		mMaterial = mStandardMaterials[SURFACE_TYPE_TARMAC];
		PhysXSample::newMesh(data);
		mMaterial = saved;
	}
	else if(LOAD_TYPE_VEHICLE==gLoadType)
	{
		PxU32 carPart=0xffffffff;
		if(0==strcmp(data.mName,gCarPartNames[CAR_PART_CHASSIS]))
		{
			carPart=CAR_PART_CHASSIS;

			//Find the min and max of the set of verts.
			PxVec3 min(PX_MAX_F32,PX_MAX_F32,PX_MAX_F32);
			PxVec3 max(-PX_MAX_F32,-PX_MAX_F32,-PX_MAX_F32);
			for(PxU32 i=0;i<data.mNbVerts;i++)
			{
				min.x=PxMin(min.x,data.mVerts[i].x);
				min.y=PxMin(min.y,data.mVerts[i].y);
				min.z=PxMin(min.z,data.mVerts[i].z);
				max.x=PxMax(max.x,data.mVerts[i].x);
				max.y=PxMax(max.y,data.mVerts[i].y);
				max.z=PxMax(max.z,data.mVerts[i].z);
			}

			//Make sure the chassis has an aabb that is centred at (0,0,0).
			//This just makes it a lot easier to set the centre of mass offset relative to the centre of the actor.
			gChassisMeshTransform=(min+max)*0.5f;

			//Make sure wheel offsets are symmetric (they are not quite symmetric left to right or front to back).
			gWheelCentreOffsets4[CAR_PART_FRONT_LEFT_WHEEL].x=-gWheelCentreOffsets4[CAR_PART_FRONT_RIGHT_WHEEL].x;
			gWheelCentreOffsets4[CAR_PART_REAR_LEFT_WHEEL].x=-gWheelCentreOffsets4[CAR_PART_REAR_RIGHT_WHEEL].x;
			gWheelCentreOffsets4[CAR_PART_FRONT_LEFT_WHEEL].y=gWheelCentreOffsets4[CAR_PART_REAR_LEFT_WHEEL].y;
			gWheelCentreOffsets4[CAR_PART_FRONT_RIGHT_WHEEL].y=gWheelCentreOffsets4[CAR_PART_REAR_RIGHT_WHEEL].y;

			//Make an adjustment so that the the centre of the four wheels is at the origin.
			//Again, this just makes it a lot easier to set the centre of mass because we have a known point 
			//that represents the origin.  Also, it kind of makes sense that the default centre of mass is at the 
			//centre point of the wheels.
			PxF32 wheelOffsetZ=0;
			for(PxU32 i=0;i<=CAR_PART_REAR_RIGHT_WHEEL;i++)
			{
				wheelOffsetZ+=gWheelCentreOffsets4[i].z;
			}
			wheelOffsetZ*=0.25f;
			gChassisMeshTransform.z+=wheelOffsetZ;

			//Reposition the mesh verts.
			PxVec3* verts = const_cast<PxVec3*>(data.mVerts);
			for(PxU32 i=0;i<data.mNbVerts;i++)
			{
				verts[i]-=gChassisMeshTransform;
			}

			//Need a convex mesh for the chassis.
			gChassisConvexMesh=createChassisConvexMesh(data.mVerts,data.mNbVerts,getPhysics(),getCooking());
		}
		else if(0==strcmp(data.mName,gCarPartNames[CAR_PART_FRONT_LEFT_WHEEL]))
		{
			carPart=CAR_PART_FRONT_LEFT_WHEEL;
			gWheelConvexMeshes4[CAR_PART_FRONT_LEFT_WHEEL]=createWheelConvexMesh(data.mVerts,data.mNbVerts,getPhysics(),getCooking());
		}
		else if(0==strcmp(data.mName,gCarPartNames[CAR_PART_FRONT_RIGHT_WHEEL]))
		{
			carPart=CAR_PART_FRONT_RIGHT_WHEEL;
			gWheelConvexMeshes4[CAR_PART_FRONT_RIGHT_WHEEL]=createWheelConvexMesh(data.mVerts,data.mNbVerts,getPhysics(),getCooking());
		}
		else if(0==strcmp(data.mName,gCarPartNames[CAR_PART_REAR_LEFT_WHEEL]))
		{
			carPart=CAR_PART_REAR_LEFT_WHEEL;
			gWheelConvexMeshes4[CAR_PART_REAR_LEFT_WHEEL]=createWheelConvexMesh(data.mVerts,data.mNbVerts,getPhysics(),getCooking());
		}
		else if(0==strcmp(data.mName,gCarPartNames[CAR_PART_REAR_RIGHT_WHEEL]))
		{
			carPart=CAR_PART_REAR_RIGHT_WHEEL;
			gWheelConvexMeshes4[CAR_PART_REAR_RIGHT_WHEEL]=createWheelConvexMesh(data.mVerts,data.mNbVerts,getPhysics(),getCooking());
		}
		else if(0==strcmp(data.mName,gCarPartNames[CAR_PART_WINDOWS]))
		{
			carPart=CAR_PART_WINDOWS;
			//Take the offset that was required to centre the chassis and apply it to everything that is dependent on the chassis transform.
			PxVec3* verts = const_cast<PxVec3*>(data.mVerts);
			for(PxU32 i=0;i<data.mNbVerts;i++)
			{
				verts[i]-=gChassisMeshTransform;
			}
			for(PxU32 i=0;i<=CAR_PART_REAR_RIGHT_WHEEL;i++)
			{
				gWheelCentreOffsets4[i]-=gChassisMeshTransform;
			}
		}

		RenderMeshActor* meshActor = createRenderMeshFromRawMesh(data);
		PX_ASSERT(carPart!=0xffffffff);
		gRenderMeshActors[carPart]=meshActor;

		//Associate a car part with data that tells us how to compute the render transform.
		for(PxU32 i=0;i<NUM_CAR4W_RENDER_COMPONENTS;i++)
		{
			if(0==strcmp(data.mName,gCarPartNames[i]))
			{
				g4WCarPartDependencyOffsets[i]=meshActor->getTransform().p;
			}
		}

		//Store the wheel offsets from the centre.
		for(PxU32 i=0;i<=CAR_PART_REAR_RIGHT_WHEEL;i++)
		{
			if(0==strcmp(data.mName,gCarPartNames[i]))
			{
				gWheelCentreOffsets4[i]=meshActor->getTransform().p;
				gWheelCentreOffsets4[i].y-=gSuspensionShimHeight;

				//The left and right wheels seem to be mixed up.
				//Swap them to correct this.
				gWheelCentreOffsets4[i].x*=-1.0f;
			}
		}
	}
	else if(LOAD_TYPE_SKYDOME==gLoadType)
	{
		createRenderMeshFromRawMesh(data);
	}
	else
	{
		getSampleErrorCallback().reportError(PxErrorCode::eINTERNAL_ERROR, "Unknown mesh type", __FILE__, __LINE__);
	}
}

void SampleVehicle::onShutdown()
{
	for(PxU32 i=0;i<mNbIB;i++)
		SAMPLE_FREE(mIB[i]);
	SAMPLE_FREE(mTerrainVB);

#if PX_DEBUG_VEHICLE_ON
	mTelemetryData4W->free();
#endif

	mVehicleManager.shutdown();

	PhysXSample::onShutdown();
}


void SampleVehicle::onTickPreRender(PxF32 dtime)
{
	if(mFixCar)
	{
		//Get the transform of the last crossed waypoint and reset the player car 
		//at this transform.  
		resetFocusVehicleAtWaypoint();
		mFixCar=false;
		//Reset the control logic driving the car back to start state to avoid mismatch
		//between vehicle gears and state stored in vehicle controller.
		mVehicleController.clear();
#if PX_DEBUG_VEHICLE_ON
		//Clear all the graph data because the discontinuity in car position makes the stored data redundant.
		clearTelemetryData();
#endif
	}
	else if(mBackToStart)
	{
		//Set the progress back to zero.
		mWayPoints.setBackAtStart();
		//Get the transform of the last crossed waypoint (this will now be the start
		//waypoint) and reset the player car at this transform.
		resetFocusVehicleAtWaypoint();
		mBackToStart=false;
		//Reset the control logic driving the car back to start state to avoid mismatch
		//between vehicle gears and state stored in vehicle controller.
		mVehicleController.clear();
#if PX_DEBUG_VEHICLE_ON
		//Clear all the graph data because the discontinuity in car position makes the stored data redundant.
		clearTelemetryData();
#endif
	}

	//Make sure we can still update the camera in pause model.
	if(mPause)
	{
		mCameraController.setInputs(
			mControlInputs.getRotateY(),
			mControlInputs.getRotateZ());
		updateCameraController(dtime, getActiveScene());
	}

	//Set the global transforms of all actors that have user data.
	const size_t nbVehicleGraphicsMeshes = mVehicleGraphics.size();
	for(PxU32 i=0;i<nbVehicleGraphicsMeshes;i++)
	{
		//Get the mesh actor that represents a vehicle render component.
		RenderMeshActor* actor = mVehicleGraphics[i];

		//Get the data that associates the render component with a physics vehicle component.
		const CarRenderUserData* carRenderUserData=(CarRenderUserData*)actor->mUserData;
		const PxU8 carId=carRenderUserData->carId;
		const PxU8 carPart=carRenderUserData->carPart;
		const PxU8 carPartDependency=carRenderUserData->carPartDependency;

		//Get the physics shapes of the vehicle.  
		//The transform of these shapes will be applied to the render meshes.
		PxShape* carShapes[PX_MAX_NUM_WHEELS+1];
		const PxVehicleWheels& vehicle=*mVehicleManager.getVehicle(carId);
		const PxU32 numShapes=vehicle.getRigidDynamicActor()->getNbShapes();
		vehicle.getRigidDynamicActor()->getShapes(carShapes,numShapes);

		//Set the transform of the render component from the associated physics shape transform.
		if(255==carPartDependency)
		{
			//The transform of this car component has been computed by the vehicle physics
			//and stored in the composite bound of the car (chassis + wheels).	
			actor->setTransform(PxShapeExt::getGlobalPose(*carShapes[carPart]));
		}
		else
		{
			//The transform of this car component hasn't been computed by the vehicle physics.
			//The transform is just an offset from another vehicle physics component.
			//(This is kind of like a very,very simple skeleton of hierarchical transforms that would normally
			//be used to render a vehicle).
			actor->setTransform(PxShapeExt::getGlobalPose(*carShapes[carPartDependency]));
		}
	}

	getCamera().lookAt(mCameraController.getCameraPos(), mCameraController.getCameraTar());

	// Update the physics
	PhysXSample::onTickPreRender(dtime);

}

void SampleVehicle::onTickPostRender(PxF32 dtime)
{
	// Fetch results
	PhysXSample::onTickPostRender(dtime);


	if(mDebugRenderFlag)
	{
		drawWheels();
		drawVehicleDebug();
	}

	//Draw the next three way-points.
	const RendererColor colors[3]={RendererColor(255,0,0),RendererColor(0,255,0),RendererColor(0,0,255)};
	PxVec3 v[3];
	PxVec3 w[3];
	PxU32 numPoints=0;
	mWayPoints.getNextWayPointsAndLineDirs(numPoints,v[0],v[1],v[2],w[0],w[1],w[2]);
	for(PxU32 i=0;i<numPoints;i++)
	{
		getDebugRenderer()->addLine(v[i],v[i]+PxVec3(0,5,0),colors[i]);
		getDebugRenderer()->addLine(v[i]-w[i],v[i]+w[i],colors[i]);
	}
}

void SampleVehicle::onSubstep(PxF32 dtime)
{
	//Update the digital vehicle controls.
	mVehicleController.setCarKeyboardInputs(
		mControlInputs.getAccelKeyPressed(),
		mControlInputs.getBrakeKeyPressed(),
		mControlInputs.getHandbrakeKeyPressed(),
		mControlInputs.getSteerLeftKeyPressed(),
		mControlInputs.getSteerRightKeyPressed(),
		mControlInputs.getGearUpKeyPressed(),
		mControlInputs.getGearDownKeyPressed());
	//Update the analog vehicle controls.
	mVehicleController.setCarGamepadInputs(
		mControlInputs.getAccel(),
		mControlInputs.getBrake(),
		mControlInputs.getSteer(),
		mControlInputs.getGearUp(),
		mControlInputs.getGearDown(),
		mControlInputs.getHandbrake());
	//Update the controller.
	updateVehicleController(dtime);

	//Update the vehicles.
	mVehicleManager.suspensionRaycasts(&getActiveScene());

	if (dtime > 0.0f)
	{
#if PX_DEBUG_VEHICLE_ON
		updateVehicleManager(dtime,getActiveScene().getGravity());
#else
		mVehicleManager.update(dtime,getActiveScene().getGravity());
#endif
	}

	//Update the camera.
	mCameraController.setInputs(
		mControlInputs.getRotateY(),
		mControlInputs.getRotateZ());
	updateCameraController(dtime,getActiveScene());

	//Update the revolute joints.
	//If the joint has exceeded the rotation limit then reverse the drive velocity
	//to make the joint rotate in the opposite direction.
	for(PxU32 i=0;i<gNumRevoluteJoints;i++)
	{
		//Get the two actors of the joint.
		PxRigidActor* actor0=NULL;
		PxRigidActor* actor1=NULL;
		gRevoluteJoints[i]->getActors(actor0,actor1);

		//Work out the rotation angle of the joint.
		const PxF32 cosTheta=PxAbs(actor1->is<PxRigidDynamic>()->getGlobalPose().q.getBasisVector1().y);
		const PxF32 theta=PxAcos(cosTheta);

		//If the joint rotation limit has been exceeded then reverse the drive direction.
		//It's possible to reverse the direction but then after a timestep or two for it to still be beyond the rotation limit.
		//To avoid switching back and forth don't switch drive direction until a minimum time has passed since the last drive direction change.
		//Its possible that the pendulum has hit the car and is unable to reach the limit.  A nice fix for this is to keep a track of the time
		//passed since the last direction change and reverse the joint drive direction if a time limit has been reached.  This gives the car 
		//a chance to escape the pendulum.
		if((theta > gRevoluteJointMaxTheta && gRevoluteJointTimers[i]>4*dtime) || gRevoluteJointTimers[i] > 4*gRevoluteJointMaxTheta/gRevoluteJointDriveSpeeds[i])
		{
			//Help the joint by setting the actor momenta to zero.
			((PxRigidDynamic*)actor1)->setLinearVelocity(PxVec3(0,0,0));
			((PxRigidDynamic*)actor1)->setAngularVelocity(PxVec3(0,0,0));

			//Switch the joint drive direction.
			const PxF32 currDriveVel=gRevoluteJoints[i]->getDriveVelocity();
			const PxF32 newDriveVel=-currDriveVel;
			gRevoluteJoints[i]->setDriveVelocity(newDriveVel);

			//Reset the timer.
			gRevoluteJointTimers[i]=0;
		}

		//Increment the joint timer.
		gRevoluteJointTimers[i]+=dtime;
	}

	//Update the progress around the track with the latest vehicle transform.
	PxRigidDynamic* actor=getFocusVehicleRigidDynamicActor();
	mWayPoints.update(actor->getGlobalPose(),dtime);

	//Cache forward speed for the HUD to avoid making API calls while vehicle update is running
	const PxVehicleWheels& focusVehicle = *mVehicleManager.getVehicle(mPlayerVehicle);
	mForwardSpeedHud = focusVehicle.computeForwardSpeed();
}

void SampleVehicle::helpRender(PxU32 x, PxU32 y, PxU8 textAlpha)
{
	Renderer* renderer = getRenderer();
	const PxU32 yInc=18;
	const PxF32 scale=0.5f;
	const PxF32 shadowOffset=6.0f;
	const RendererColor textColor(255, 255, 255, textAlpha);
	const bool isKeyboardSupported = getApplication().getPlatform()->getSampleUserInput()->keyboardSupported();
	const bool isPadSupported = getApplication().getPlatform()->getSampleUserInput()->gamepadSupported();
	const char* msg;

	if (isPadSupported && isKeyboardSupported)
		renderer->print(x, y += yInc, "Use right stick or numpad keys to rotate the camera", scale, shadowOffset, textColor);
	else if (isPadSupported)
		renderer->print(x, y += yInc, "Use right stick to rotate the camera", scale, shadowOffset, textColor);
	else if (isKeyboardSupported)
		renderer->print(x, y += yInc, "Use numpad keys to rotate the camera", scale, shadowOffset, textColor);

	msg = mApplication.inputInfoMsg("Press "," for accelerate", ACCELERATE, -1);
	if(msg)
		renderer->print(x, y += yInc, msg, scale, shadowOffset, textColor);
	msg = mApplication.inputInfoMsg("Press "," for brake", BRAKE, -1);
	if(msg)
		renderer->print(x, y += yInc, msg, scale, shadowOffset, textColor);
	msg = mApplication.inputInfoMsg_Aor_BandC("Press "," to steer", STEER, STEER_LEFT, STEER_RIGHT);
	if(msg)
		renderer->print(x, y += yInc, msg, scale, shadowOffset, textColor);
	msg = mApplication.inputInfoMsg("Press "," for handbrake", HANDBRAKE_PAD, HANDBRAKE_KBD);
	if(msg)
		renderer->print(x, y += yInc, msg, scale, shadowOffset, textColor);
	if(!getFocusVehicleUsesAutoGears())
	{
		msg = mApplication.inputInfoMsg("Press "," to gear up", GEAR_UP_PAD, GEAR_UP_KBD);
		if(msg)
			renderer->print(x, y += yInc, msg, scale, shadowOffset, textColor);
		msg = mApplication.inputInfoMsg("Press "," to gear down", GEAR_DOWN_PAD, GEAR_DOWN_KBD);
		if(msg)
			renderer->print(x, y += yInc, msg, scale, shadowOffset, textColor);
	}

	msg = mApplication.inputInfoMsg("Press "," to toggle car debug render",DEBUG_RENDER_FLAG , -1);
	if(msg)
		renderer->print(x, y += yInc, msg, scale, shadowOffset, textColor);
	msg = mApplication.inputInfoMsg("Press "," to go back to start",RETRY, -1);
	if(msg)
		renderer->print(x, y += yInc, msg, scale, shadowOffset, textColor);
	msg = mApplication.inputInfoMsg("Press "," to go back to last waypoint",FIX_CAR, -1);
	if(msg)
		renderer->print(x, y += yInc, msg, scale, shadowOffset, textColor);

#if PX_DEBUG_VEHICLE_ON
	if(mDebugRenderFlag)
	{
		msg = mApplication.inputInfoMsg("Press "," to increment wheel graphs", DEBUG_RENDER_WHEEL,-1);
		if(msg)
			renderer->print(x, y += yInc, msg, scale, shadowOffset, textColor);
		msg = mApplication.inputInfoMsg("Press "," to increment central graphs", DEBUG_RENDER_ENGINE,-1);
		if(msg)
			renderer->print(x, y += yInc, msg, scale, shadowOffset, textColor);
	}
#endif
}

void SampleVehicle::descriptionRender(PxU32 x, PxU32 y, PxU8 textAlpha)
{
	bool print=(textAlpha!=0.0f);

	if(print)
	{
		Renderer* renderer = getRenderer();
		const PxU32 yInc = 24;
		const PxReal scale = 0.5f;
		const PxReal shadowOffset = 6.0f;
		const RendererColor textColor(255, 255, 255, textAlpha);

		char line0[256]="This sample demonstrates vehicle simulation using PhysX. In particular,";
		char line1[256]="vehicle instantiation, the setup of vehicle description data, and";
		char line2[256]="integration with the PhysX SDK are all presented.  Key concepts such";
		char line3[256]="as raycast and simulation filtering are illustrated as a means to";
		char line4[256]="tailor vehicle interaction with the scene. The setup and rendering";
		char line5[256]="of vehicle telemetry data is also shown.  Finally, physics obstacles";
		char line6[256]="such as swinging pendula exemplify the configuration and run-time ";
		char line7[256]="control of driven joints using PhysX.";

		renderer->print(x, y+=yInc, line0, scale, shadowOffset, textColor);
		renderer->print(x, y+=yInc, line1, scale, shadowOffset, textColor);
		renderer->print(x, y+=yInc, line2, scale, shadowOffset, textColor);
		renderer->print(x, y+=yInc, line3, scale, shadowOffset, textColor);
		renderer->print(x, y+=yInc, line4, scale, shadowOffset, textColor);
		renderer->print(x, y+=yInc, line5, scale, shadowOffset, textColor);
		renderer->print(x, y+=yInc, line6, scale, shadowOffset, textColor);
		renderer->print(x, y+=yInc, line7, scale, shadowOffset, textColor);
	}
}

void SampleVehicle::customizeRender()
{
	drawHud();
	if(mDebugRenderFlag)
	{
		drawFocusVehicleGraphsAndPrintTireSurfaces();
	}

	const PxU32 width=getCamera().getScreenWidth();
	const PxU32 height=getCamera().getScreenHeight();

	const PxU32 xCentre=280*width/800;
	const PxU32 xRight=570*width/800;
	const PxU32 yBottom=600*height/600;

	const PxU32 yInc=18;

	Renderer* renderer = getRenderer();

	char time[64];
	sprintf(time, "Curr Lap Time: %1.1f \n", mWayPoints.getTimeElapsed());		
	renderer->print(xRight, yBottom - yInc*2, time);	
	sprintf(time, "Best Lap Time: %1.1f \n", mWayPoints.getMinTimeElapsed());		
	renderer->print(xRight, yBottom - yInc*3, time);	

	if(!mCameraController.getIsLockedOnVehicleTransform())
	{
		renderer->print(xCentre, yBottom - yInc*4, "Camera decoupled from car");
	}
}

///////////////////////////////////////////////////////////////////////////////

enum Word3
{
	SWEPT_INTEGRATION_LINEAR = 1,
};

PxFilterFlags SampleVehicleFilterShader(	
	PxFilterObjectAttributes attributes0, PxFilterData filterData0, 
	PxFilterObjectAttributes attributes1, PxFilterData filterData1,
	PxPairFlags& pairFlags, const void* constantBlock, PxU32 constantBlockSize)
{
	PX_FORCE_PARAMETER_REFERENCE(constantBlock);
	PX_FORCE_PARAMETER_REFERENCE(constantBlockSize);

	// let triggers through
	if(PxFilterObjectIsTrigger(attributes0) || PxFilterObjectIsTrigger(attributes1))
	{
		pairFlags = PxPairFlag::eTRIGGER_DEFAULT;
		return PxFilterFlags();
	}



	// use a group-based mechanism for all other pairs:
	// - Objects within the default group (mask 0) always collide
	// - By default, objects of the default group do not collide
	//   with any other group. If they should collide with another
	//   group then this can only be specified through the filter
	//   data of the default group objects (objects of a different
	//   group can not choose to do so)
	// - For objects that are not in the default group, a bitmask
	//   is used to define the groups they should collide with
	if ((filterData0.word0 != 0 || filterData1.word0 != 0) &&
		!(filterData0.word0&filterData1.word1 || filterData1.word0&filterData0.word1))
		return PxFilterFlag::eSUPPRESS;

	pairFlags = PxPairFlag::eCONTACT_DEFAULT;

	//enable CCD stuff -- for now just for everything or nothing.
	if((filterData0.word3|filterData1.word3) & SWEPT_INTEGRATION_LINEAR)
		pairFlags |= PxPairFlag::eSWEPT_INTEGRATION_LINEAR;

	// The pairFlags for each object are stored in word2 of the filter data. Combine them.
	pairFlags |= PxPairFlags(PxU16(filterData0.word2 | filterData1.word2));
	return PxFilterFlags();
}


void SampleVehicle::customizeSceneDesc(PxSceneDesc& sceneDesc)
{
	sceneDesc.filterShader = SampleVehicleFilterShader;
}

///////////////////////////////////////////////////////////////////////////////

void SampleVehicle::createStandardMaterials()
{
	PxF32 restitutions[MAX_NUM_SURFACE_TYPES] = {0.2f, 0.2f, 0.2f, 0.2f};
	PxF32 staticFrictions[MAX_NUM_SURFACE_TYPES] = {0.5f, 0.5f, 0.5f, 0.5f};
	PxF32 dynamicFrictions[MAX_NUM_SURFACE_TYPES] = {0.5f, 0.5f, 0.5f, 0.5f};

	for(PxU32 i=0;i<MAX_NUM_SURFACE_TYPES;i++) 
	{
		//Create a new material.
		mStandardMaterials[i] = getPhysics().createMaterial(staticFrictions[i], dynamicFrictions[i], restitutions[i]);
		if(!mStandardMaterials[i])
		{
			getSampleErrorCallback().reportError(PxErrorCode::eINTERNAL_ERROR, "createMaterial failed", __FILE__, __LINE__);
		}

		//Set up the drivable surface type that will be used for the new material.
		mVehicleDrivableSurfaceTypes[i].mType = i;
	}
}

void SampleVehicle::createVehicles()
{
	//Make sure that we set the foundation before doing anything.
	mVehicleManager.init(getPhysics(),(const PxMaterial**)mStandardMaterials,mVehicleDrivableSurfaceTypes);

	//Not added any vehicles yet.
	gNumVehicleAdded=0;

	//Load the vehicle model (this will add the render actors for the chassis, 4 wheels, and windows).
	gLoadType = LOAD_TYPE_VEHICLE;
	importRAWFile("car2.raw", 1.0f);

	//Clear the array of render actors before adding the actors for each vehicle.
	for(PxU32 i=0;i<mRenderActors.size();i++)
	{
		RenderBaseActor* renderActor=mRenderActors[i];
		renderActor->setRendering(false);
	}

	//Load the player car.
	for(PxU32 i=0;i<NUM_CAR4W_RENDER_COMPONENTS;i++)
	{
		gVehicleRenderUserData[gNumVehicleAdded][i]=gCar4WRenderUserData[i];
		gVehicleRenderUserData[gNumVehicleAdded][i].carId=PxU8(gNumVehicleAdded);

		RenderMeshActor* clone=SAMPLE_NEW(RenderMeshActor)(*gRenderMeshActors[i]);
		clone->setRendering(true);
		clone->mUserData=&gVehicleRenderUserData[gNumVehicleAdded][i];;

		mVehicleGraphics.push_back(clone);
		mRenderActors.push_back(clone);
	}

	mPlayerVehicle=0;

	mVehicleManager.create4WVehicle(getActiveScene(),getPhysics(),getCooking(),*mStandardMaterials[SURFACE_TYPE_TARMAC],gChassisMass,gWheelCentreOffsets4,gChassisConvexMesh,gWheelConvexMeshes4,gPlayerCarStartTransforms[0],true);

	gNumVehicleAdded++;


	for(PxU32 i=0;i<NUM_NONPLAYER_4W_VEHICLES;i++)
	{
		//Clone the meshes from the instanced meshes.
		for(PxU32 j=0;j<NUM_CAR4W_RENDER_COMPONENTS;j++)
		{
			gVehicleRenderUserData[gNumVehicleAdded][j]=gCar4WRenderUserData[j];
			gVehicleRenderUserData[gNumVehicleAdded][j].carId=PxU8(gNumVehicleAdded);

			RenderMeshActor* clone = SAMPLE_NEW(RenderMeshActor)(*gRenderMeshActors[j]);
			clone->setRendering(true);
			clone->mUserData=&gVehicleRenderUserData[gNumVehicleAdded][j];

			mVehicleGraphics.push_back(clone);
			mRenderActors.push_back(clone);
		}

		//Add the next vehicle.
		//Again, in the absence of a material for vehicles just set the material of the car to be tarmac.
		mVehicleManager.create4WVehicle(getActiveScene(),getPhysics(),getCooking(),*mStandardMaterials[SURFACE_TYPE_TARMAC],gChassisMass,gWheelCentreOffsets4,gChassisConvexMesh,gWheelConvexMeshes4,gVehicle4WStartTransforms[i],true);

		gNumVehicleAdded++;
	}
}

PxRigidStatic* SampleVehicle::addStaticObstacle
(const PxTransform& transform, const PxU32 numShapes, PxTransform* shapeTransforms, PxGeometry** shapeGeometries, PxMaterial** shapeMaterials)
{
	PxFilterData simFilterData;
	simFilterData.word0=COLLISION_FLAG_GROUND;
	simFilterData.word1=COLLISION_FLAG_GROUND_AGAINST;
	PxFilterData qryFilterData;
	SampleVehicleSetupDrivableShapeQueryFilterData(&qryFilterData);

	PxRigidStatic* actor=getPhysics().createRigidStatic(transform);
	for(PxU32 i=0;i<numShapes;i++)
	{
		PxShape* shape=actor->createShape(*shapeGeometries[i], *shapeMaterials[i], shapeTransforms[i]);
		shape->setSimulationFilterData(simFilterData);
		shape->setQueryFilterData(qryFilterData);
	}
	getActiveScene().addActor(*actor);
	createRenderObjectsFromActor(actor);
	return actor;
}

PxRigidDynamic* SampleVehicle::addDynamicObstacle
(const PxTransform& transform, const PxF32 mass, const PxU32 numShapes, PxTransform* shapeTransforms,  PxGeometry** shapeGeometries, PxMaterial** shapeMaterials)
{
	PxFilterData simFilterData;
	simFilterData.word0=COLLISION_FLAG_OBSTACLE;
	simFilterData.word1=COLLISION_FLAG_OBSTACLE_AGAINST;
	PxFilterData qryFilterData;
	SampleVehicleSetupNonDrivableShapeQueryFilterData(&qryFilterData);

	PxRigidDynamic* actor = getPhysics().createRigidDynamic(transform);
	for(PxU32 i=0;i<numShapes;i++)
	{
		PxShape* shape = actor->createShape(*shapeGeometries[i], *shapeMaterials[i], shapeTransforms[i]);
		shape->setSimulationFilterData(simFilterData);
		shape->setQueryFilterData(qryFilterData);
	}

	PxRigidBodyExt::setMassAndUpdateInertia(*actor,mass);
	getActiveScene().addActor(*actor);
	createRenderObjectsFromActor(actor);
	return actor;
}

PxRigidDynamic* SampleVehicle::addDynamicDrivableObstacle
(const PxTransform& transform, const PxF32 mass, const PxU32 numShapes, PxTransform* shapeTransforms,  PxGeometry** shapeGeometries, PxMaterial** shapeMaterials)
{
	PxFilterData simFilterData;
	simFilterData.word0=COLLISION_FLAG_DRIVABLE_OBSTACLE;
	simFilterData.word1=COLLISION_FLAG_DRIVABLE_OBSTACLE_AGAINST;
	PxFilterData qryFilterData;
	SampleVehicleSetupDrivableShapeQueryFilterData(&qryFilterData);

	PxRigidDynamic* actor = getPhysics().createRigidDynamic(transform);
	for(PxU32 i=0;i<numShapes;i++)
	{
		PxShape* shape = actor->createShape(*shapeGeometries[i], *shapeMaterials[i], shapeTransforms[i]);
		shape->setSimulationFilterData(simFilterData);
		shape->setQueryFilterData(qryFilterData);
	}

	PxRigidBodyExt::setMassAndUpdateInertia(*actor,mass);
	getActiveScene().addActor(*actor);
	createRenderObjectsFromActor(actor);
	return actor;
}

void SampleVehicle::createStack(PxU32 numBaseBoxes, PxF32 boxSize, const PxVec3& pos, const PxQuat& quat)
{
	const PxF32 density=50.0f;

	const PxF32 sizeX = boxSize;
	const PxF32 sizeY = boxSize;
	const PxF32 sizeZ = boxSize;

	const PxF32 mass = boxSize*boxSize*boxSize*density;
	const PxVec3 halfExtents(sizeX*0.5f,sizeY*0.5f,sizeZ*0.5f);
	PxBoxGeometry geometry(halfExtents);
	PxTransform shapeTransforms[1]={PxTransform::createIdentity()};
	PxGeometry* shapeGeometries[1]={&geometry};
	PxMaterial* shapeMaterials[1]={mStandardMaterials[SURFACE_TYPE_TARMAC]};

	const PxF32 spacing = 0.0001f;
	PxVec3 relPos(0.0f, sizeY/2, 0.0f);
	PxF32 offsetX = -(numBaseBoxes * (sizeX + spacing) * 0.5f);
	PxF32 offsetZ = 0.0f;
	while(numBaseBoxes)
	{
		for(PxU32 i=0;i<numBaseBoxes ;i++)
		{
			relPos.x = offsetX + (PxF32)i * (sizeX + spacing);
			relPos.z = offsetZ;

			PxTransform transform(pos + quat.rotate(relPos),quat);
			addDynamicObstacle(transform,mass,1,shapeTransforms,shapeGeometries,shapeMaterials);
		}

		offsetX += sizeX/2;
		relPos.y += (sizeY + spacing);
		numBaseBoxes--;
	}
}

void SampleVehicle::createWall(const PxU32 numHorizontalBoxes, const PxU32 numVerticalBoxes, const PxF32 boxSize, const PxVec3& pos, const PxQuat& quat)
{
	const PxF32 density=50.0f;

	const PxF32 sizeX = boxSize;
	const PxF32 sizeY = boxSize;
	const PxF32 sizeZ = boxSize;

	const PxF32 mass = sizeX*sizeY*sizeZ*density;
	const PxVec3 halfExtents(sizeX*0.5f,sizeY*0.5f,sizeZ*0.5f);
	PxBoxGeometry geometry(halfExtents);
	PxTransform shapeTransforms[1] = {PxTransform::createIdentity()};
	PxGeometry* shapeGeometries[1] = {&geometry};
	PxMaterial* shapeMaterials[1] = {mStandardMaterials[SURFACE_TYPE_TARMAC]};

	const PxF32 spacing = 0.0001f;
	PxVec3 relPos(0.0f, sizeY/2, 0.0f);
	PxF32 offsetX = -(numHorizontalBoxes * (sizeX + spacing) * 0.5f);
	PxF32 offsetZ = 0.0f;

	for(PxU32 k=0;k<numVerticalBoxes;k++)
	{
		for(PxU32 i=0;i<numHorizontalBoxes;i++)
		{
			relPos.x = offsetX + (sizeX + spacing)*i;
			relPos.z = offsetZ;
			PxTransform transform(pos + quat.rotate(relPos),quat);
			addDynamicObstacle(transform,mass,1,shapeTransforms,shapeGeometries,shapeMaterials);
		}

		if(0==(k%2))
		{
			offsetX += sizeX/2;
		}
		else
		{
			offsetX -= sizeX/2;
		}
		relPos.y += (sizeY + spacing);
	}
}

void SampleVehicle::createObstacles()
{
	//Create some giant pendula
	{
		PxTransform shapeTransforms[3]={PxTransform::createIdentity(),PxTransform::createIdentity(),PxTransform::createIdentity()};
		PxMaterial* shapeMaterials[3]={NULL,NULL,NULL};
		PxGeometry* shapeGeometries[3]={NULL,NULL,NULL};
		PxShape* shapes[3]={NULL,NULL,NULL};

		gNumRevoluteJoints=0;

		for(PxU32 i=0;i<MAX_NUM_PENDULA;i++)
		{
			//Get the transform to position the next pendulum ball.
			const PxTransform& ballStartTransform=gPendulaBallStartTransforms[i];

			//Pendulum made of two shapes : a sphere for the ball and a cylinder for the shaft.
			//In the absence of a special material for pendula just use the tarmac material. 
			PxSphereGeometry geomBall(gPendulumBallRadius);
			shapeGeometries[0]=&geomBall;
			shapeTransforms[0]=PxTransform::createIdentity();
			shapeMaterials[0]=mStandardMaterials[SURFACE_TYPE_TARMAC];
			PxConvexMeshGeometry geomShaft(createCylinderConvexMesh(gPendulumShaftLength, gPendulumShaftWidth, 8, getPhysics(), getCooking()));
			shapeGeometries[1]=&geomShaft;
			shapeTransforms[1]=PxTransform(PxVec3(0, 0.5f*gPendulumShaftLength, 0), PxQuat(PxHalfPi, PxVec3(0,0,1))),
			shapeMaterials[1]=mStandardMaterials[SURFACE_TYPE_TARMAC];

			//Ready to add the pendulum as a dynamic object.
			PxRigidDynamic* actor=addDynamicObstacle(ballStartTransform,gPendulumBallMass,2,shapeTransforms,shapeGeometries,shapeMaterials);

			//As an optimization we don't want pendulum to intersect with static geometry because the position and 
			//limits on joint rotation will ensure that this is already impossible.
			actor->getShapes(shapes,2);
			PxFilterData simFilterData=shapes[0]->getSimulationFilterData();
			simFilterData.word1 &= ~COLLISION_FLAG_GROUND;
			shapes[0]->setSimulationFilterData(simFilterData);
			shapes[1]->setSimulationFilterData(simFilterData);
			//As a further optimization lets set the pendulum shapes to be non-drivable surfaces.
			PxFilterData qryFilterData;
			SampleVehicleSetupNonDrivableShapeQueryFilterData(&qryFilterData);
			shapes[0]->setQueryFilterData(qryFilterData);
			shapes[1]->setQueryFilterData(qryFilterData);
			
			//Add static geometry to give the appearance that something is physically supporting the pendulum.
			//This supporting geometry is just a vertical bar and two horizontal bars.
			const PxF32 groundClearance=3.0f;
			PxConvexMeshGeometry geomHorizontalBar(createCylinderConvexMesh(gPendulumSuspensionStructureWidth, gPendulumShaftWidth, 8, getPhysics(), getCooking())); 
			PxConvexMeshGeometry geomVerticalBar(createCylinderConvexMesh(gPendulumShaftLength+groundClearance, gPendulumShaftWidth, 8, getPhysics(), getCooking()));
			shapeGeometries[0]=&geomHorizontalBar;
			shapeMaterials[0]=mStandardMaterials[SURFACE_TYPE_TARMAC];
			shapeTransforms[0]=PxTransform(PxVec3(0, gPendulumShaftLength, 0), PxQuat::createIdentity());
			shapeGeometries[1]=&geomVerticalBar;
			shapeMaterials[1]=mStandardMaterials[SURFACE_TYPE_TARMAC];
			shapeTransforms[1]=PxTransform(PxVec3(0.5f*gPendulumSuspensionStructureWidth, 0.5f*(gPendulumShaftLength-groundClearance), 0), PxQuat(PxHalfPi, PxVec3(0,0,1)));
			shapeGeometries[2]=&geomVerticalBar;
			shapeMaterials[2]=mStandardMaterials[SURFACE_TYPE_TARMAC];
			shapeTransforms[2]=PxTransform(PxVec3(-0.5f*gPendulumSuspensionStructureWidth, 0.5f*(gPendulumShaftLength-groundClearance), 0), PxQuat(PxHalfPi, PxVec3(0,0,1)));

			//Ready to add the support geometry as a static object.
			PxRigidStatic* staticActor=addStaticObstacle(ballStartTransform,3,shapeTransforms,shapeGeometries,shapeMaterials);

			//As an optimization lets disable collision with the dynamic pendulum because the joint limits will make 
			//collision impossible.
			staticActor->getShapes(shapes,3);
			simFilterData=shapes[0]->getSimulationFilterData();
			simFilterData.word1 &= ~COLLISION_FLAG_OBSTACLE;
			shapes[0]->setSimulationFilterData(simFilterData);
			shapes[1]->setSimulationFilterData(simFilterData);
			shapes[2]->setSimulationFilterData(simFilterData);

			//Now finally add the joint that will create the pendulum behaviour : rotation around a single axis.
			const PxVec3 pendulumPos=ballStartTransform.p + PxVec3(0, gPendulumShaftLength, 0);
			PxQuat pendulumRotation=PxQuat(PxHalfPi,PxVec3(0,1,0));
			PxRevoluteJoint* joint=PxRevoluteJointCreate
				(getPhysics(), 
				 NULL, PxTransform(pendulumPos, ballStartTransform.q*pendulumRotation),
				 actor, PxTransform(PxVec3(0,gPendulumShaftLength,0), pendulumRotation));
			joint->setDriveVelocity(gRevoluteJointDriveSpeeds[i]);
			joint->setRevoluteJointFlag(PxRevoluteJointFlag::eDRIVE_ENABLED, true);
			gRevoluteJoints[gNumRevoluteJoints]=joint;
			gNumRevoluteJoints++;
		}

		//Work out the maximum angle of rotation allowed before the pendulum hits the support geometry.
		//We're going to monitor the pendulum's progress at each update and reverse the pendulum drive speed
		//if the limit is exceed.
		const PxF32 sinTheta=(gPendulumSuspensionStructureWidth*0.5f - gPendulumBallRadius)/gPendulumShaftLength;
		gRevoluteJointMaxTheta=PxAsin(sinTheta);
	}

	//Create a see-saw
	{
		//See-saw made of two separate objects : a static prism-shaped base and a ramp that balances on top of the prism.
		//In the absence of a special material for see-saws just reuse the tarmac material.
		PxTransform shapeTransforms[1]={PxTransform::createIdentity()};
		PxMaterial* shapeMaterials[1]={mStandardMaterials[SURFACE_TYPE_TARMAC]};
		PxGeometry* shapeGeometries[1]={NULL};

		//Add the static prism-shaped object.
		PxTransform tBase(PxVec3( -132.940292 , 8.664189 , -116.392891 ), PxQuat( -0.000319 , 0.348920 , -0.000129 , -0.937153 ) );
		PxConvexMesh* meshBase=createPrismConvexMesh(12,3.0f,4.5f,getPhysics(), getCooking());
		PxConvexMeshGeometry geomBase(meshBase);
		shapeGeometries[0]=&geomBase;
		addStaticObstacle(tBase,1,shapeTransforms,shapeGeometries,shapeMaterials);

		//Add the dynamic ramp that balances on the prism.
		PxTransform tRamp(PxVec3( -130.357010 , 12.584847 , -119.381256 ), PxQuat( 0.088919 , 0.347436 , 0.033112 , -0.932891 ) ) ;
		PxConvexMesh* meshRamp=createSquashedCuboidMesh(6.0f, 40.0f, 0.25f, 0.065f, getPhysics(), getCooking());
		PxConvexMeshGeometry geomRamp(meshRamp);
		shapeGeometries[0]=&geomRamp;
		addDynamicDrivableObstacle(tRamp,1000.0f,1,shapeTransforms,shapeGeometries,shapeMaterials);
	}

	//Add a really big ramp to jump over the car stack.
	{
		PxVec3 halfExtentsRamp(5.0f,1.9f,7.0f);
		PxConvexMeshGeometry geomRamp(createWedgeConvexMesh(halfExtentsRamp,getPhysics(),getCooking()));
		PxTransform shapeTransforms[1]={PxTransform::createIdentity()};
		PxMaterial* shapeMaterials[1]={mStandardMaterials[SURFACE_TYPE_TARMAC]};
		PxGeometry* shapeGeometries[1]={&geomRamp};
		PxTransform tRamp(PxVec3( -89.849564 , 9.950000 , 154.516617 ), PxQuat( -0.000002 , -0.837118 , -0.000004 , 0.547022 ) );
		addStaticObstacle(tRamp,1,shapeTransforms,shapeGeometries,shapeMaterials);
	}

	//Add two ramps side by side with a gap in between
	{
		PxVec3 halfExtents(3.0f,1.5f,3.5f);
		PxConvexMeshGeometry geometry(createWedgeConvexMesh(halfExtents,getPhysics(),getCooking()));
		PxTransform shapeTransforms[1]={PxTransform::createIdentity()};
		PxMaterial* shapeMaterials[1]={mStandardMaterials[SURFACE_TYPE_TARMAC]};
		PxGeometry* shapeGeometries[1]={&geometry};
		PxTransform t1(PxVec3( 112.797081 , 10.086022 , 134.419052 ), PxQuat( 0.000013 , -0.406322 , 0.000006 , 0.913730 ) );
		addStaticObstacle(t1,1,shapeTransforms,shapeGeometries,shapeMaterials);
		PxTransform t2(PxVec3( 120.135361 , 10.086023 , 140.594055 ), PxQuat( 0.000013 , -0.406322 , 0.000006 , 0.913730 ) );
		addStaticObstacle(t2,1,shapeTransforms,shapeGeometries,shapeMaterials);
	}

	//Add a wall made of dynamic objects with cuboid shapes for bricks.
	{
		PxTransform t(PxVec3( -37.525650 , 9.864201 , -77.926567 ), PxQuat( -0.000286 , 0.728016 , -0.000290 , -0.685561 ) ) ;
		createWall(12,4,1.0f,t.p,t.q);
	}

	//Create a kind of cattle grid of cylindrical rods.
	{
		//Cattle grid made out of 128 identical cylindrical rods.
		//Need to instantiate geometry used for the rods.
		const PxF32 logRadius=0.125f;
		PxConvexMesh* mesh=createCylinderConvexMesh(10.0f,logRadius,8,getPhysics(),getCooking());
		PxConvexMeshGeometry geom(mesh);

		//Set up transform, geometry, and material of each cylindrical rod shape.
		PxGeometry* shapeGeometries[128];
		PxMaterial* shapeMaterials[128];
		PxTransform shapeTransforms[128];
		PxTransform shapeTransform=PxTransform(PxVec3(0,0,0),PxQuat::createIdentity());
		for(PxU32 i=0;i<128;i++)
		{
			shapeTransforms[i]=shapeTransform;
			shapeGeometries[i]=&geom;
			shapeMaterials[i]=mStandardMaterials[SURFACE_TYPE_TARMAC];

			//Work out the position of the next cylindrical rod.
			shapeTransform.p.z += (2.40f*logRadius);
			shapeTransform.p.y += 0.0025f;
		}

		//Ready to add the cattle grid as a static object with a composite bound of 128 cylindrical rods.
		PxTransform t(PxVec3( -160.972595 , 8.739249 , -74.141998 ), PxQuat( 0.000407 , -0.165312 , 0.000060 , 0.986241 ) ) ;
		addStaticObstacle(t,128,shapeTransforms,shapeGeometries,shapeMaterials);
	}

	//Add three static walls
	{
		PxBoxGeometry box(8,2,1);
		PxTransform shapeTransforms[1]={PxTransform::createIdentity()};
		PxGeometry* shapeGeometries[1]={&box};
		PxMaterial* shapeMaterials[1]={mStandardMaterials[SURFACE_TYPE_TARMAC]};

		PxTransform t1(PxVec3( -175.981186 , 9.864196 , -25.040220 ), PxQuat( -0.000417 , 0.042262 , -0.000009 , -0.999106 ) ); 
		addStaticObstacle(t1,1,shapeTransforms,shapeGeometries,shapeMaterials);
		PxTransform t2(PxVec3( -174.972122 , 9.864214 , 32.517246 ), PxQuat( -0.000384 , -0.080625 , 0.000019 , -0.996744 ) ) ;
		addStaticObstacle(t2,1,shapeTransforms,shapeGeometries,shapeMaterials);
		PxTransform t3(PxVec3( 157.825897 , 9.864218 , -83.961632 ), PxQuat( -0.000124 , 0.997806 , -0.000358 , -0.066201 ) );
		addStaticObstacle(t3,1,shapeTransforms,shapeGeometries,shapeMaterials);
	}
}

void SampleVehicle::collectInputEvents(std::vector<const SampleFramework::InputEvent*>& inputEvents)
{
	PhysXSample::collectInputEvents(inputEvents);

	getApplication().getPlatform()->getSampleUserInput()->unregisterInputEvent(CAMERA_GAMEPAD_MOVE_LEFT_RIGHT);
	getApplication().getPlatform()->getSampleUserInput()->unregisterInputEvent(CAMERA_GAMEPAD_MOVE_FORWARD_BACK);
	getApplication().getPlatform()->getSampleUserInput()->unregisterInputEvent(SPAWN_DEBUG_OBJECT);
	getApplication().getPlatform()->getSampleUserInput()->unregisterInputEvent(MENU_VISUALIZATIONS);
	getApplication().getPlatform()->getSampleUserInput()->unregisterInputEvent(HIDE_GRAPHICS);

	//digital keyboard events
	DIGITAL_INPUT_EVENT_DEF(ACCELERATE,						SCAN_CODE_FORWARD,	XKEY_W,			PS3KEY_W,		AKEY_UNKNOWN,	SCAN_CODE_FORWARD,	PSP2KEY_UNKNOWN,	IKEY_UNKNOWN,	SCAN_CODE_FORWARD);
	DIGITAL_INPUT_EVENT_DEF(BRAKE,							SCAN_CODE_BACKWARD,	XKEY_S,			PS3KEY_S,		AKEY_UNKNOWN,	SCAN_CODE_BACKWARD,	PSP2KEY_UNKNOWN,	IKEY_UNKNOWN,	SCAN_CODE_BACKWARD);
	DIGITAL_INPUT_EVENT_DEF(STEER_LEFT,						SCAN_CODE_LEFT,		XKEY_A,			PS3KEY_A,		AKEY_UNKNOWN,	SCAN_CODE_LEFT,		PSP2KEY_UNKNOWN,	IKEY_UNKNOWN,	SCAN_CODE_LEFT);
	DIGITAL_INPUT_EVENT_DEF(STEER_RIGHT,					SCAN_CODE_RIGHT,	XKEY_D,			PS3KEY_D,		AKEY_UNKNOWN,	SCAN_CODE_RIGHT,	PSP2KEY_UNKNOWN,	IKEY_UNKNOWN,	SCAN_CODE_RIGHT);
	DIGITAL_INPUT_EVENT_DEF(GEAR_DOWN_KBD,					SCAN_CODE_9,		XKEY_9,			PS3KEY_9,		AKEY_UNKNOWN,	SCAN_CODE_9,		PSP2KEY_UNKNOWN,	IKEY_UNKNOWN,	SCAN_CODE_9);
	DIGITAL_INPUT_EVENT_DEF(GEAR_UP_KBD,					SCAN_CODE_0,		XKEY_0,			PS3KEY_0,		AKEY_UNKNOWN,	SCAN_CODE_0,		PSP2KEY_UNKNOWN,	IKEY_UNKNOWN,	SCAN_CODE_0);
	DIGITAL_INPUT_EVENT_DEF(HANDBRAKE_KBD,					SCAN_CODE_L,		XKEY_L,			PS3KEY_L,		AKEY_UNKNOWN,	SCAN_CODE_L,		PSP2KEY_UNKNOWN,	IKEY_UNKNOWN,	SCAN_CODE_L);
	DIGITAL_INPUT_EVENT_DEF(CAMERA_KBD_ROTATE_LEFT,			WKEY_NUMPAD4,		XKEY_NUMPAD4,	PS3KEY_NUMPAD4,	AKEY_UNKNOWN,	OSXKEY_UNKNOWN,		PSP2KEY_UNKNOWN,	IKEY_UNKNOWN,	LINUXKEY_NUMPAD4);
	DIGITAL_INPUT_EVENT_DEF(CAMERA_KBD_ROTATE_RIGHT,		WKEY_NUMPAD6,		XKEY_NUMPAD6,	PS3KEY_NUMPAD6,	AKEY_UNKNOWN,	OSXKEY_UNKNOWN,		PSP2KEY_UNKNOWN,	IKEY_UNKNOWN,	LINUXKEY_NUMPAD6);
	DIGITAL_INPUT_EVENT_DEF(CAMERA_KBD_ROTATE_UP,			WKEY_NUMPAD8,		XKEY_NUMPAD8,	PS3KEY_NUMPAD8,	AKEY_UNKNOWN,	OSXKEY_UNKNOWN,		PSP2KEY_UNKNOWN,	IKEY_UNKNOWN,	LINUXKEY_NUMPAD8);
	DIGITAL_INPUT_EVENT_DEF(CAMERA_KBD_ROTATE_DOWN,			WKEY_NUMPAD2,		XKEY_NUMPAD2,	PS3KEY_NUMPAD2,	AKEY_UNKNOWN,	OSXKEY_UNKNOWN,		PSP2KEY_UNKNOWN,	IKEY_UNKNOWN,	LINUXKEY_NUMPAD2);
	DIGITAL_INPUT_EVENT_DEF(DEBUG_RENDER_FLAG,				WKEY_F9,			XKEY_F9,		PS3KEY_F9,		AKEY_UNKNOWN,	OSXKEY_T,			PSP2KEY_UNKNOWN,	IKEY_UNKNOWN,	LINUXKEY_F9);
	DIGITAL_INPUT_EVENT_DEF(DEBUG_RENDER_ENGINE,			WKEY_2,				XKEY_2,			PS3KEY_2,		AKEY_UNKNOWN,	OSXKEY_2,			PSP2KEY_UNKNOWN,	IKEY_UNKNOWN,	LINUXKEY_2);
	DIGITAL_INPUT_EVENT_DEF(DEBUG_RENDER_WHEEL,				WKEY_1,				XKEY_1,			PS3KEY_1,		AKEY_UNKNOWN,	OSXKEY_1,			PSP2KEY_UNKNOWN,	IKEY_UNKNOWN,	LINUXKEY_1);
	DIGITAL_INPUT_EVENT_DEF(FIX_CAR,						WKEY_K,				XKEY_K,			PS3KEY_K,		AKEY_UNKNOWN,	OSXKEY_K,			PSP2KEY_UNKNOWN,	IKEY_UNKNOWN,	LINUXKEY_K);

	//digital gamepad events
	DIGITAL_INPUT_EVENT_DEF(GEAR_DOWN_PAD,					GAMEPAD_EAST,               GAMEPAD_EAST,               GAMEPAD_EAST,               AKEY_UNKNOWN,   OSXKEY_UNKNOWN, GAMEPAD_EAST,       IKEY_UNKNOWN,	LINUXKEY_UNKNOWN);
	DIGITAL_INPUT_EVENT_DEF(GEAR_UP_PAD,					GAMEPAD_NORTH,              GAMEPAD_NORTH,              GAMEPAD_NORTH,              AKEY_UNKNOWN,   OSXKEY_UNKNOWN,	GAMEPAD_NORTH,      IKEY_UNKNOWN,	LINUXKEY_UNKNOWN);
	DIGITAL_INPUT_EVENT_DEF(HANDBRAKE_PAD,					GAMEPAD_SOUTH,              GAMEPAD_SOUTH,              GAMEPAD_SOUTH,              AKEY_UNKNOWN,   OSXKEY_UNKNOWN,	GAMEPAD_SOUTH,      IKEY_UNKNOWN,	LINUXKEY_UNKNOWN);
	DIGITAL_INPUT_EVENT_DEF(DEBUG_RENDER_FLAG,				GAMEPAD_WEST,               GAMEPAD_WEST,               GAMEPAD_WEST,               AKEY_UNKNOWN,   OSXKEY_UNKNOWN,	GAMEPAD_WEST,       IKEY_UNKNOWN,	LINUXKEY_UNKNOWN);
	DIGITAL_INPUT_EVENT_DEF(DEBUG_RENDER_ENGINE,			GAMEPAD_LEFT_SHOULDER_TOP,	GAMEPAD_LEFT_SHOULDER_TOP,  GAMEPAD_LEFT_SHOULDER_TOP,	AKEY_UNKNOWN,   OSXKEY_UNKNOWN,	GAMEPAD_DIGI_LEFT,	IKEY_UNKNOWN,	LINUXKEY_UNKNOWN);
	DIGITAL_INPUT_EVENT_DEF(DEBUG_RENDER_WHEEL,				GAMEPAD_RIGHT_SHOULDER_TOP,	GAMEPAD_RIGHT_SHOULDER_TOP, GAMEPAD_RIGHT_SHOULDER_TOP,	AKEY_UNKNOWN,	OSXKEY_UNKNOWN,	GAMEPAD_DIGI_RIGHT,	IKEY_UNKNOWN,	LINUXKEY_UNKNOWN);
	DIGITAL_INPUT_EVENT_DEF(FIX_CAR,						GAMEPAD_RIGHT_STICK,        GAMEPAD_RIGHT_STICK,        GAMEPAD_RIGHT_STICK,		AKEY_UNKNOWN,	OSXKEY_UNKNOWN,	GAMEPAD_DIGI_DOWN,	IKEY_UNKNOWN,	LINUXKEY_UNKNOWN);
    
	//analog gamepad events
	ANALOG_INPUT_EVENT_DEF(ACCELERATE, GAMEPAD_DEFAULT_SENSITIVITY,							GAMEPAD_RIGHT_SHOULDER_BOT,	GAMEPAD_RIGHT_SHOULDER_BOT,	GAMEPAD_RIGHT_SHOULDER_BOT,	AKEY_UNKNOWN,			GAMEPAD_RIGHT_SHOULDER_BOT,	GAMEPAD_RIGHT_SHOULDER_BOT,	IKEY_UNKNOWN, LINUXKEY_UNKNOWN);
	ANALOG_INPUT_EVENT_DEF(BRAKE, GAMEPAD_DEFAULT_SENSITIVITY,								GAMEPAD_LEFT_SHOULDER_BOT,	GAMEPAD_LEFT_SHOULDER_BOT,	GAMEPAD_LEFT_SHOULDER_BOT,	AKEY_UNKNOWN,			GAMEPAD_LEFT_SHOULDER_BOT,	GAMEPAD_LEFT_SHOULDER_BOT,	IKEY_UNKNOWN, LINUXKEY_UNKNOWN);
	ANALOG_INPUT_EVENT_DEF(ACCELERATE_BRAKE, GAMEPAD_DEFAULT_SENSITIVITY,					WKEY_UNKNOWN,				XKEY_UNKNOWN,				PS3KEY_UNKNOWN,				GAMEPAD_LEFT_STICK_Y,	OSXKEY_UNKNOWN,				PSP2KEY_UNKNOWN,			GAMEPAD_LEFT_STICK_Y, LINUXKEY_UNKNOWN);
	ANALOG_INPUT_EVENT_DEF(STEER, GAMEPAD_DEFAULT_SENSITIVITY,								GAMEPAD_LEFT_STICK_X,		GAMEPAD_LEFT_STICK_X,		GAMEPAD_LEFT_STICK_X,		GAMEPAD_LEFT_STICK_X,	GAMEPAD_LEFT_STICK_X,		GAMEPAD_LEFT_STICK_X,		GAMEPAD_LEFT_STICK_X, LINUXKEY_UNKNOWN);
	ANALOG_INPUT_EVENT_DEF(CAMERA_GAMEPAD_ROTATE_LEFT_RIGHT, GAMEPAD_ROTATE_SENSITIVITY,	GAMEPAD_RIGHT_STICK_X,		GAMEPAD_RIGHT_STICK_X,		GAMEPAD_RIGHT_STICK_X,		GAMEPAD_RIGHT_STICK_X,	GAMEPAD_RIGHT_STICK_X,		GAMEPAD_RIGHT_STICK_X,		GAMEPAD_RIGHT_STICK_X, LINUXKEY_UNKNOWN);
	ANALOG_INPUT_EVENT_DEF(CAMERA_GAMEPAD_ROTATE_UP_DOWN, GAMEPAD_ROTATE_SENSITIVITY,		GAMEPAD_RIGHT_STICK_Y,		GAMEPAD_RIGHT_STICK_Y,		GAMEPAD_RIGHT_STICK_Y,		GAMEPAD_RIGHT_STICK_Y,	GAMEPAD_RIGHT_STICK_Y,		GAMEPAD_RIGHT_STICK_Y,		GAMEPAD_RIGHT_STICK_Y, LINUXKEY_UNKNOWN);

    //touch events
	TOUCH_INPUT_EVENT_DEF(FIX_CAR,          "Reset Car",		ABUTTON_5,          IBUTTON_5);
    TOUCH_INPUT_EVENT_DEF(HANDBRAKE_PAD,    "Handbrake",    AQUICK_BUTTON_1,    IQUICK_BUTTON_1);
}

bool SampleVehicle::onDigitalInputEvent(const SampleFramework::InputEvent& ie, bool val)
{
	switch (ie.m_Id)
	{
	case ACCELERATE:
		{
			mControlInputs.setAccelKeyPressed(val);
		}
		break;
	case BRAKE:
		{
			mControlInputs.setBrakeKeyPressed(val);
		}
		break;
	case STEER_LEFT:
		{
			mControlInputs.setSteerRightKeyPressed(val);
		}
		break;
	case STEER_RIGHT:
		{
			mControlInputs.setSteerLeftKeyPressed(val);			
		}
		break;
	case GEAR_DOWN_KBD:
		{
			mControlInputs.setGearDownKeyPressed(val);
		}
		break;
	case GEAR_UP_KBD:
		{
			mControlInputs.setGearUpKeyPressed(val);
		}
		break;
	case HANDBRAKE_KBD:
		{
			mControlInputs.setHandbrakeKeyPressed(val);
		}
		break;
	case GEAR_DOWN_PAD:
		{
			mControlInputs.setGearDown(val);
		}
		break;
	case GEAR_UP_PAD:
		{
			mControlInputs.setGearUp(val);
		}
		break;
	case HANDBRAKE_PAD:
		{
			mControlInputs.setHandbrake(val);
		}
		break;
	case AUTOMATIC_GEAR:
		{
			if(val)
			{
				mVehicleController.toggleAutoGearFlag();
			}
		}
		break;
	case DEBUG_RENDER_FLAG:
		{
			if(val)
			{
				mDebugRenderFlag = !mDebugRenderFlag;
			}
		}
		break;
	case DEBUG_RENDER_WHEEL:
		{
			if(val)
			{
#if PX_DEBUG_VEHICLE_ON
				mDebugRenderActiveGraphChannelWheel++;
				if(mDebugRenderActiveGraphChannelWheel==PxVehicleGraph::eMAX_NUM_WHEEL_CHANNELS)
				{
					mDebugRenderActiveGraphChannelWheel=0;
				}
#endif
			}
		}
		break;
	case DEBUG_RENDER_ENGINE:
		{
			if(val)
			{
#if PX_DEBUG_VEHICLE_ON
				mDebugRenderActiveGraphChannelEngine++;
				if(mDebugRenderActiveGraphChannelEngine==PxVehicleGraph::eMAX_NUM_ENGINE_CHANNELS)
				{
					mDebugRenderActiveGraphChannelEngine=0;
				}
#endif
			}
		}
		break;
	case RETRY:
		{
			if(val)
				mBackToStart=true;
		}
		break;
	case FIX_CAR:
		{
			if(val)
				mFixCar = true;
		}
		break;
	case CAMERA_KBD_ROTATE_LEFT:
		{
			mControlInputs.setRotateY(val ? -0.5f : 0.0f);
		}
		break;
	case CAMERA_KBD_ROTATE_RIGHT:
		{
			mControlInputs.setRotateY(val ? 0.5f : 0.0f);
		}
		break;
	case CAMERA_KBD_ROTATE_UP:
		{
			mControlInputs.setRotateZ(val ? 0.5f : 0.0f);
		}
		break;
	case CAMERA_KBD_ROTATE_DOWN:
		{
			mControlInputs.setRotateZ(val ? -0.5f : 0.0f);
		}
		break;
	default:
		break;
	}

	PhysXSample::onDigitalInputEvent(ie,val);

	return true;
}

void SampleVehicle::onAnalogInputEvent(const SampleFramework::InputEvent& ie, float val)
{
	switch (ie.m_Id)
	{
	case ACCELERATE:
		{
			mControlInputs.setAccel(val);
		}
		break;
	case BRAKE:
		{
			mControlInputs.setBrake(val);
		}
		break;
	case ACCELERATE_BRAKE:
		{
			if (val >= 0.0f)
			{
				mControlInputs.setAccel(val);
				mControlInputs.setBrake(0.0f);
			}
			else
			{
				mControlInputs.setBrake(-val);
				mControlInputs.setAccel(0.0f);
			}
		}
		break;
	case STEER:
		{
			mControlInputs.setSteer(-val);
		}
		break;
	case CAMERA_GAMEPAD_ROTATE_LEFT_RIGHT:
		{
			mControlInputs.setRotateY(-val);
		}
		break;
	case CAMERA_GAMEPAD_ROTATE_UP_DOWN:
		{
			mControlInputs.setRotateZ(-val);
		}
		break;
	default:
		break;
	}
}

///////////////////////////////////////////////////////////////////////////////

#if PX_DEBUG_VEHICLE_ON
void SampleVehicle::clearTelemetryData()
{
	mTelemetryData4W->clear();
}
#endif

void SampleVehicle::updateCameraController(const PxF32 dtime, PxScene& scene) 
{	
	mCameraController.update(dtime,*mVehicleManager.getVehicle(mPlayerVehicle),scene);
}

void SampleVehicle::updateVehicleController(const PxF32 dtime)
{
	mVehicleController.update(dtime,*mVehicleManager.getVehicle(mPlayerVehicle));
}

void SampleVehicle::updateVehicleManager(const PxF32 dtime, const PxVec3& gravity)
{
#if PX_DEBUG_VEHICLE_ON
	mVehicleManager.updateAndRecordTelemetryData(dtime,gravity,mVehicleManager.getVehicle(mPlayerVehicle),mTelemetryData4W);
#else
	mVehicleManager.update(dtime,gravity);
#endif
}

void SampleVehicle::resetFocusVehicleAtWaypoint()
{
	mVehicleManager.resetNWCar(mWayPoints.getResetTransform(),mPlayerVehicle);
}

PxRigidDynamic*	SampleVehicle::getFocusVehicleRigidDynamicActor()
{
	return mVehicleManager.getVehicle(mPlayerVehicle)->getRigidDynamicActor();
}

void SampleVehicle::drawFocusVehicleGraphsAndPrintTireSurfaces()
{
	drawGraphsAndPrintTireSurfaceTypes(*mVehicleManager.getVehicle(mPlayerVehicle));
}

bool SampleVehicle::getFocusVehicleUsesAutoGears()
{
	PxVehicleWheels* vehWheels=mVehicleManager.getVehicle(mPlayerVehicle);
	PxVehicleDriveDynData* driveDynData=NULL;
	PX_ASSERT(eVEHICLE_TYPE_DRIVE4W==vehWheels->getVehicleType());
	PxVehicleDrive4W* vehDrive4W=(PxVehicleDrive4W*)vehWheels;
	driveDynData=&vehDrive4W->mDriveDynData;
	return driveDynData->getUseAutoGears();
}