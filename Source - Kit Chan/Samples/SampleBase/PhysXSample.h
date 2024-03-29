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
#ifndef _PHYSX_SAMPLE_H
#define _PHYSX_SAMPLE_H

#include "PhysXSampleApplication.h"
#include "SampleStepper.h"
#include "SampleDirManager.h"
#include "PxPhysicsAPI.h"
#include "extensions/PxExtensionsAPI.h"
#include "RenderParticleSystemActor.h"
#include "RenderClothActor.h"

class PxDefaultSpuDispatcher;

namespace SampleFramework
{
	class SampleInputAsset;
}

namespace physx
{
	class Picking;
	namespace debugger { namespace comm {}}
}

namespace PVD {
	using namespace physx::debugger;
	using namespace physx::debugger::comm;
}

class PhysXSample	: public PVD::PvdConnectionHandler //receive notifications when pvd is connected and disconnected.
					, public RAWImportCallback
					, public SampleAllocateable
{
public:
															PhysXSample(PhysXSampleApplication& app, PxU32 maxSubSteps=8);
	virtual													~PhysXSample();

public:
					void									render();
					void									displayFPS();

					SampleFramework::SampleAsset*			getAsset(const char* relativePath, SampleFramework::SampleAsset::Type type, bool abortOnError = true);
					void									importRAWFile(const char* relativePath, PxReal scale, bool recook=false);
					void									removeActor(PxRigidActor* actor);
					void									removeRenderObject(RenderBaseActor *renderAcotr);
#if PX_USE_CLOTH_API
					void									createRenderObjectsFromCloth(const PxCloth& cloth, const PxClothMeshDesc &meshDesc, RenderMaterial* material = NULL, const PxReal* uvs = NULL, bool enableDebugRender = true, const PxVec3& color = PxVec3(0.5f, 0.5f, 0.5f), PxReal scale = 1.0f);
#endif
					void									createRenderObjectsFromActor(PxRigidActor* actor, RenderMaterial* material=NULL);
					RenderBaseActor*						createRenderBoxFromShape(PxShape* shape, RenderMaterial* material=NULL, const PxReal* uvs=NULL);
					RenderBaseActor*						createRenderObjectFromShape(PxShape* shape, RenderMaterial* material=NULL);
					RenderMeshActor*						createRenderMeshFromRawMesh(const RAWMesh& data, PxShape* shape = NULL);
					RenderTexture*							createRenderTextureFromRawTexture(const RAWTexture& data);
					RenderMaterial*							createRenderMaterialFromTextureFile(const char* filename);
					PxCloth*								createClothFromMeshDesc(PxClothMeshDesc &meshDesc, const PxTransform &pose, PxClothCollisionData* collData,	const PxVec3& gravityDir = PxVec3(0,0,-1), 
																const PxReal* uv = 0, const char *textureFile = 0, const PxVec3& color = PxVec3(0.5f, 0.5f, 0.5f), PxReal scale = 1.0f);
					PxCloth*								createClothFromObj(const char *objFileName, const PxTransform& pose, PxClothCollisionData* collData, const char* textureFileName);
					PxCloth*                                createGridCloth(PxReal sizeX, PxReal sizeY, PxU32 numX, PxU32 numY,	const PxTransform &pose, PxClothCollisionData *colData = NULL,	const char* filename = NULL);
					PxRigidActor*							createGrid(RenderMaterial* material=NULL);
					PxRigidDynamic*							createBox(const PxVec3& pos, const PxVec3& dims, const PxVec3* linVel=NULL, RenderMaterial* material=NULL, PxReal density=1.0f);
					PxRigidDynamic*							createSphere(const PxVec3& pos, PxReal radius, const PxVec3* linVel=NULL, RenderMaterial* material=NULL, PxReal density=1.0f);
					PxRigidDynamic*							createCapsule(const PxVec3& pos, PxReal radius, PxReal halfHeight, const PxVec3* linVel=NULL, RenderMaterial* material=NULL, PxReal density=1.0f);
					PxRigidDynamic*							createConvex(const PxVec3& pos, const PxVec3* linVel=NULL, RenderMaterial* material=NULL, PxReal density=1.0f);
					PxRigidDynamic*							createCompound(const PxVec3& pos, const std::vector<PxTransform>& localPoses, const std::vector<const PxGeometry*>& geometries, const PxVec3* linVel=NULL, RenderMaterial* material=NULL, PxReal density=1.0f);
					PxRigidDynamic*							createTestCompound(const PxVec3& pos, PxU32 nbBoxes, float boxSize, float amplitude, const PxVec3* linVel, RenderMaterial* material, PxReal density, bool makeSureVolumeEmpty = false);
#if PX_USE_PARTICLE_SYSTEM_API
					RenderParticleSystemActor*				createRenderObjectFromParticleSystem(ParticleSystem& ps, RenderMaterial* material = NULL);
#endif
					void									createRenderObjectsFromScene();
					
					void									addRenderParticleSystemActor(RenderParticleSystemActor& renderActor);
					void									removeRenderParticleSystemActor(RenderParticleSystemActor& renderActor);

					void									setSubStepper(const PxReal stepSize, const PxU32 maxSteps) { getStepper()->setSubStepper(stepSize, maxSteps); }
					void									togglePause();
					void									initRenderObjects();

public:
	virtual			void									onInit();
	virtual			void									onInit(bool restart) { onInit(); }
	virtual			void									onShutdown();

	// called after simulate() has completed
	virtual			void									onSubstep(float dtime) {}

	// called before simulate() is called
	virtual			void									onSubstepSetup(float dtime, pxtask::BaseTask* cont) {}
	// called after simulate() has started
	virtual			void									onSubstepStart(float dtime) {}

	virtual			void									onTickPreRender(float dtime);
	virtual			void									customizeRender() {}
	virtual			void									helpRender(PxU32 x, PxU32 y, PxU8 textAlpha) {}
	virtual			void									descriptionRender(PxU32 x, PxU32 y, PxU8 textAlpha) {}
	virtual			void									onTickPostRender(float dtime);

	virtual			void									onAnalogInputEvent(const SampleFramework::InputEvent& , float val);
	virtual			bool									onDigitalInputEvent(const SampleFramework::InputEvent& , bool val);
	virtual			void									onPointerInputEvent(const SampleFramework::InputEvent& ie, physx::PxU32 x, physx::PxU32 y, physx::PxReal dx, physx::PxReal dy, bool val);

	virtual			void									onKeyDownEx(SampleFramework::SampleUserInput::KeyCode keyCode, PxU32 param);

	virtual			void									onResize(PxU32 width, PxU32 height);

	virtual			void									newMaterial(const RAWMaterial&);
	virtual			void									newMesh(const RAWMesh&);
	virtual			void									newShape(const RAWShape&);
	virtual			void									newHelper(const RAWHelper&);
	virtual			void									newTexture(const RAWTexture&);
					void									unregisterInputEvents();
					void									registerInputEvents(bool ignoreSaved = false);					
	virtual			void									collectInputEvents(std::vector<const SampleFramework::InputEvent*>& inputEvents);
	virtual			SampleFramework::SampleDirManager&		getSampleOutputDirManager();

protected:
	// Let samples override this
	virtual			void									getDefaultSceneDesc(PxSceneDesc&)  {}
	virtual			void									customizeSceneDesc(PxSceneDesc&)	{}
	virtual			void									renderScene() {}
	// this lets samples customize the debug objects
	enum DebugObjectType
	{
		DEBUG_OBJECT_BOX		=	(1 << 0),
		DEBUG_OBJECT_SPHERE		=	(1 << 1),
		DEBUG_OBJECT_CAPSULE	=	(1 << 2),
		DEBUG_OBJECT_CONVEX		=	(1 << 3),
		DEBUG_OBJECT_COMPOUND	=	(1 << 4),
		DEBUG_OBJECT_ALL		=	(DEBUG_OBJECT_BOX | DEBUG_OBJECT_SPHERE | DEBUG_OBJECT_CAPSULE | DEBUG_OBJECT_CONVEX | DEBUG_OBJECT_COMPOUND)
	};
	virtual			PxU32									getDebugObjectTypes()				const	{ return DEBUG_OBJECT_ALL;			}
	virtual			PxReal									getDebugObjectsVelocity()			const	{ return 20.0f;						}
	virtual			PxVec3									getDebugBoxObjectExtents()			const	{ return PxVec3(0.3f, 0.3f, 1.0f);	}
	virtual			PxReal									getDebugSphereObjectRadius()		const	{ return 0.3f;						}
	virtual			PxReal									getDebugCapsuleObjectRadius()		const	{ return 0.3f;						}
	virtual			PxReal									getDebugCapsuleObjectHalfHeight()	const	{ return 1.0f;						}
	virtual			PxReal									getDebugConvexObjectScale()			const	{ return 0.3f;						}
	virtual			void									onDebugObjectCreation(PxRigidDynamic* actor){									}
	Stepper*		getStepper();

	void													prepareInputEventUserInputInfo(const char* sampleName,PxU32 &userInputCS, PxU32 &inputEventCS);

private:
	///////////////////////////////////////////////////////////////////////////////
	// Implements PvdConnectionFactoryHandler
	virtual			void									onPvdSendClassDescriptions(PVD::PvdConnection&) {}
	virtual			void									onPvdConnected(PVD::PvdConnection& inFactory);
	virtual			void									onPvdDisconnected(PVD::PvdConnection& inFactory);

public:	// Helpers from PhysXSampleApplication
	PX_FORCE_INLINE	void									fatalError(const char* msg)					{ mApplication.fatalError(msg); }
	PX_FORCE_INLINE	void									setCameraController(CameraController* c)	{ mApplication.setCameraController(c); }

	PX_FORCE_INLINE	void									toggleVisualizationParam(PxScene& scene, PxVisualizationParameter::Enum param)
															{
                                                                const bool visualization = scene.getVisualizationParameter(param) == 1.0f;
                                                                scene.setVisualizationParameter(param, visualization ? 0.0f : 1.0f);
																mApplication.refreshVisualizationMenuState(param);						
															}

public:	// getter & setter
	PX_FORCE_INLINE	void									setDefaultMaterial(PxMaterial* material)	{ mMaterial = material; }
	PX_FORCE_INLINE	void									setFilename(const char* name)				{ mFilename = name; }

	PX_FORCE_INLINE	PhysXSampleApplication&					getApplication()					const	{ return mApplication; }
	PX_FORCE_INLINE	PxPhysics&								getPhysics()						const	{ return *mPhysics; }
	PX_FORCE_INLINE	PxCooking&								getCooking()						const	{ return *mCooking; }
	PX_FORCE_INLINE	PxScene&								getActiveScene()					const	{ return *mScene; }
	PX_FORCE_INLINE	PxMaterial&								getDefaultMaterial()				const	{ return *mMaterial; }
					RenderMaterial*							getMaterial(PxU32 materialID);

	PX_FORCE_INLINE	Camera&									getCamera()							const	{ return mApplication.getCamera(); }
	PX_FORCE_INLINE	SampleRenderer::Renderer*				getRenderer()						const	{ return mApplication.getRenderer(); }
	PX_FORCE_INLINE	RenderPhysX3Debug*						getDebugRenderer()					const	{ return mApplication.getDebugRenderer(); }
	PX_FORCE_INLINE	Console*								getConsole()						const	{ return mApplication.mConsole; }
	PX_FORCE_INLINE	CameraController*						getCurrentCameraController()		const	{ return mApplication.mCurrentCameraController; }
	PX_FORCE_INLINE	DefaultCameraController&				getDefaultCameraController()		const   { return mCameraController;						}
	PX_FORCE_INLINE const PxMat44&							getEyeTransform(void)				const	{ return mApplication.m_worldToView.getInverseTransform();}
	PX_FORCE_INLINE	PxReal									getSimulationTime()					const	{ return mSimulationTime; }
	PX_FORCE_INLINE	PxReal									getDebugRenderScale()				const	{ return mDebugRenderScale; }

	PX_FORCE_INLINE	bool									isPaused()							const	{ return mApplication.isPaused(); }
#ifdef PX_WINDOWS
	PX_FORCE_INLINE	bool									isGpuSupported()					const	{ return mCudaContextManager && mCudaContextManager->contextIsValid(); }
#else
	PX_FORCE_INLINE	bool									isGpuSupported()					const	{ return false; }
#endif
	PX_FORCE_INLINE void									setMenuExpandState(bool menuExpand)		 { mApplication.mMenuExpand = menuExpand; }
	PX_FORCE_INLINE void									setEyeTransform(const PxVec3& pos, const PxVec3& rot) {mApplication.setEyeTransform(pos, rot); }
	PX_FORCE_INLINE void									resetExtendedHelpText()					{ mExtendedHelpPage = 0; }

	PX_FORCE_INLINE void									addPhysicsActors(PxRigidActor* actor)	{ mPhysicsActors.push_back(actor); }

	const char*												getSampleOutputFilePath(const char* inFilePath, const char* outExtension);
	
	void													showExtendedInputEventHelp(PxU32 x, PxU32 y);

	void													freeDeletedActors();

private:
					void									togglePvdConnection();
					void									createPvdConnection();
					void									updateSweepSettingForShapes();
					void									updateRenderObjectsFromRigidActor(PxRigidActor& actor);
					void									updateRenderObjectsFromArticulation(PxArticulation& articulation);					

					void									bufferActiveTransforms();
					void									updateRenderObjectsDebug(float dtime); // update of render actors debug draw information, will be called while the simulation is NOT running
					void									updateRenderObjectsSync(float dtime);  // update of render objects while the simulation is NOT running (for particles, cloth etc. because data is not double buffered)
					void									updateRenderObjectsAsync(float dtime); // update of render objects, potentially while the simulation is running (for rigid bodies etc. because data is double buffered)

					void									saveUserInputs();
					void									saveInputEvents(const std::vector<const SampleFramework::InputEvent*>& );					
					void									parseSampleOutputAsset(const char* sampleName, PxU32 , PxU32 );
					void									spawnDebugObject();

protected:	// configurations
					bool									mInitialDebugRender;
					bool									mCreateCudaCtxManager;
					bool									mCreateGroundPlane;
					bool									mUseDebugStepper;
					bool									mUseFixedStepper;
					PxU32									mMaxNumSubSteps;
					PxU32									mNbThreads;
                    PxU32									mMaxNumSpus;
					PxReal									mDefaultDensity;
					bool									mUseFullPvdConnection;

protected:	// control
					bool									mDisplayFPS;

					bool&									mPause;
					bool&									mOneFrameUpdate;
					bool&									mShowHelp;
					bool&									mShowDescription;
					bool&									mShowExtendedHelp;
					bool									mHideGraphics;
					bool									mEnableAutoFlyCamera;

					DefaultCameraController&				mCameraController;
					DefaultCameraController					mFlyCameraController;

protected:
					PhysXSampleApplication&					mApplication;
					PxFoundation*							mFoundation;
					PxProfileZoneManager*					mProfileZoneManager;
					PxPhysics*								mPhysics;
					PxCooking*								mCooking;
					PxScene*								mScene;
					PxMaterial*								mMaterial;
					PxDefaultCpuDispatcher*					mCpuDispatcher;
#ifdef PX_PS3
					PxDefaultSpuDispatcher*					mSpuDispatcher;
#endif
#ifdef PX_WINDOWS
					pxtask::CudaContextManager*				mCudaContextManager;
#endif
					std::vector<PxRigidActor*>				mPhysicsActors;
					std::vector<RenderBaseActor*>			mDeletedRenderActors;
					std::vector<RenderBaseActor*>			mRenderActors;
					std::vector<RenderParticleSystemActor*>	mRenderParticleSystemActors;
					std::vector<RenderClothActor*>			mRenderClothActors;
					

					std::vector<RenderTexture*>					mRenderTextures;
					std::vector<SampleFramework::SampleAsset*>	mManagedAssets;
					std::vector<RenderMaterial*>				mRenderMaterials;

					RenderMaterial*								(&mManagedMaterials)[MATERIAL_COUNT];

					SampleFramework::SampleInputAsset*		mSampleInputAsset;
					
					PxActiveTransform*						mBufferedActiveTransforms;
					std::vector<PxActor*>					mDeletedActors;
					PxU32									mActiveTransformCount;
					PxU32									mActiveTransformCapacity;


private:
					PxU32									mMeshTag;
					const char*								mFilename;
					PxReal									mScale;

					PxReal									mDebugRenderScale;
					bool									mWaitForResults;
					PxToolkit::FPS							mFPS;

					CameraController*						mSavedCameraController;

					DebugStepper							mDebugStepper;
					FixedStepper							mFixedStepper;
					VariableStepper							mVariableStepper;
					bool									mWireFrame;

					PxReal									mSimulationTime;
					physx::Picking*							mPicking;
					bool									mPicked;
					physx::PxU8								mExtendedHelpPage;
					physx::PxU32							mDebugObjectType;

					static const PxU32						SCRATCH_BLOCK_SIZE = 0;		// 4MB scratch block
					void*									mScratchBlock;

};

PxToolkit::BasicRandom& getSampleRandom();
PxErrorCallback& getSampleErrorCallback();

#endif // _PHYSX_SAMPLE_H
