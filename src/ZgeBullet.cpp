 /*
ZgeBullet Library
Copyright (c) 2012-2014 Radovan Cervenka

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required.

2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

3. This notice may not be removed or altered from any source distribution.
*/

/// The main file used to compile Windows DLL and Android shared library

#pragma unmanaged

// Definitions

#ifdef _WIN32
#define export extern "C" __declspec(dllexport)
#else
#define export extern "C"
#endif

#define BT_EULER_DEFAULT_ZYX

#define DONE 0
#define ERROR -1
#define TRUE 1
#define FALSE 0

// Includes

#include "btBulletDynamicsCommon.h"
#include "BulletCollision/CollisionShapes/btHeightfieldTerrainShape.h"
#include "BulletCollision/Gimpact/btGImpactShape.h"
//#include "BulletCollision/CollisionDispatch/btGhostObject.h"

// Macros for list operations

#define GET_ITEM_FROM_LIST(list, type, var, id) \
	if(id<0 || id>=list.size() || !list[id]) return ERROR; \
	type var =(type)list[id]; \
	if(!var) return ERROR

#define GET_ITEM_FROM_MIXED_LIST(list, type, var, id) \
	if(id<0 || id>=list.size() || !list[id]) return ERROR; \
	type var; \
	try { var=(type)list[id];} catch (...) {return ERROR;} \
	if(!var) return ERROR

#define ADD_TO_LIST(list, item) \
	int _i = list.findLinearSearch(NULL); \
	if(_i == list.size()) { \
		list.push_back(item); \
		return list.size()-1; \
	} else { \
		list[_i] = item; \
		return _i; \
	}

//#define CHECK_INDEX_IN_VECTOR(vector, index) \
//	if(index<0 || (unsigned)index>=vector.size()) return ERROR

#define IS_INITIALIZED if(!gIsInitialized) return ERROR

#define CHECK_WHEEL(wheelId) if(wheelId<0 || wheelId>=rv->getNumWheels()) return ERROR

// Globals

// Variables
bool gIsInitialized = false;
btBroadphaseInterface* gBroadphase;
btDefaultCollisionConfiguration* gCollisionConfiguration;
btCollisionDispatcher* gDispatcher;
btSequentialImpulseConstraintSolver* gSolver;
btDiscreteDynamicsWorld* gWorld;
btTriangleMesh* gTmpTriangleMesh = NULL;
btVector3 gRayTestHitPoint;
btVector3 gRayTestHitNormal;
btVehicleRaycaster* gVehicleRaycaster = NULL;
btRaycastVehicle::btVehicleTuning gTuning;

// Global lists
btAlignedObjectArray<btCollisionShape*> gCollisionShapeList;
btAlignedObjectArray<btCollisionObject*> gCollisionObjectList;
btAlignedObjectArray<btTypedConstraint*> gConstraintList;

// Declarations of functions

export int zbtDeleteRigidBody(int id);
//export int zbtDeleteGhostObject(int ghostObjectId);
export int zbtDeleteConstraint(int id);

// World

export int zbtCreateWorld() {
	if(gIsInitialized)
		return ERROR;

	gBroadphase = new btDbvtBroadphase();
	gCollisionConfiguration = new btDefaultCollisionConfiguration();
	gDispatcher = new btCollisionDispatcher(gCollisionConfiguration);
	gSolver = new btSequentialImpulseConstraintSolver();
	gWorld = new btDiscreteDynamicsWorld(gDispatcher, gBroadphase, gSolver, gCollisionConfiguration);

	gRayTestHitPoint = btVector3(0, 0, 0);
	gRayTestHitNormal = btVector3(0, 0, 0);

	gIsInitialized = true;
	return DONE;
}

export int zbtDestroyWorld() {
	IS_INITIALIZED;

	int i;

	// delete rigid bodies
	for(i = gCollisionObjectList.size()-1; i >= 0; --i)
		//if(typeid(gCollisionObjectList[i]) == typeid(btRigidBody))
			zbtDeleteRigidBody(i);
		//else
		//	zbtDeleteGhostObject(i);

	gCollisionObjectList.clear();

	// clear list of constraints
	gConstraintList.clear();

	// destroy vehicle raycaster, if any
	//if(gVehicleRaycaster) delete gVehicleRaycaster;

	delete gWorld;
    delete gSolver;
    delete gDispatcher;
    delete gCollisionConfiguration;
    delete gBroadphase;

	gIsInitialized = false;
	return DONE;
}

export int zbtSetWorldGravity(float x, float y, float z) {
	IS_INITIALIZED;
	gWorld->setGravity(btVector3(x, y, z));
	return DONE;
}

export int zbtStepSimulation(float timeStep, int maxSubSteps, float fixedTimeStep) {
	IS_INITIALIZED;
	gWorld->stepSimulation(timeStep, maxSubSteps, fixedTimeStep);
	return DONE;
}

// Collision shapes

export int zbtCreateStaticPlaneShape(float normalX, float normalY, float normalZ, float planeConstant) {
	ADD_TO_LIST(gCollisionShapeList, new btStaticPlaneShape(btVector3(normalX, normalY, normalZ), planeConstant));
}

export int zbtCreateBoxShape(float x, float y, float z) {
	ADD_TO_LIST(gCollisionShapeList, new btBoxShape(btVector3(x, y, z)));
}

export int zbtCreateSphereShape(float radius) {
	ADD_TO_LIST(gCollisionShapeList, new btSphereShape(radius));
}

export int zbtCreateScalableSphereShape(float radius) {
#ifdef _WIN32
	ADD_TO_LIST(gCollisionShapeList, new btMultiSphereShape(&btVector3(0,0,0), &radius, 1));
#else
	btVector3 positions[1] = {btVector3(0,0,0)};
	btScalar radii[1] = {radius};

	ADD_TO_LIST(gCollisionShapeList, new btMultiSphereShape(positions, radii, 1));
#endif
}

export int zbtCreateConeShape(float radius, float height) {
	ADD_TO_LIST(gCollisionShapeList, new btConeShape(radius, height));
}

export int zbtCreateCylinderShape(float radius, float height) {
	ADD_TO_LIST(gCollisionShapeList, new btCylinderShape(btVector3(radius, height, radius)));
}

export int zbtCreateCapsuleShape(float radius, float height) {
	ADD_TO_LIST(gCollisionShapeList, new btCapsuleShape(radius, height));
}

export int zbtCreateCompoundShape() {
	ADD_TO_LIST(gCollisionShapeList, new btCompoundShape());
}

export int zbtAddChildShape(int compoundId, int childId,
	float x, float y, float z, float rx, float ry, float rz) {

	GET_ITEM_FROM_LIST(gCollisionShapeList, btCollisionShape*, compShape, compoundId);
	GET_ITEM_FROM_LIST(gCollisionShapeList, btCollisionShape*, childShape, childId);

	try {
		((btCompoundShape*)compShape)->addChildShape(
			btTransform(btQuaternion(rz*SIMD_2_PI, ry*SIMD_2_PI, rx*SIMD_2_PI), btVector3(x, y, z)), childShape);
	} catch (...) {return ERROR;}

	return DONE;
}

export int zbtRemoveChildShape(int compoundId, int childId) {
	GET_ITEM_FROM_LIST(gCollisionShapeList, btCollisionShape*, compShape, compoundId);
	GET_ITEM_FROM_LIST(gCollisionShapeList, btCollisionShape*, childShape, childId);

	try {
		((btCompoundShape*)compShape)->removeChildShape(childShape);
	} catch (...) {return ERROR;}

	return DONE;
}

export int zbtCreateHeightfieldTerrainShape(void *heightfieldData, int dataType, int width, int length,
	float heightScale, float minHeight, float maxHeight, int upAxis, bool bFlipQuadEdges, bool bDiamondSubdivision) {

	try {
		btHeightfieldTerrainShape* hf = new btHeightfieldTerrainShape(width, length, heightfieldData,
			heightScale, minHeight, maxHeight, upAxis, (dataType == 0 ? PHY_FLOAT : PHY_INTEGER), bFlipQuadEdges);
		hf->setUseDiamondSubdivision(bDiamondSubdivision);
		ADD_TO_LIST(gCollisionShapeList, hf);
	} catch (...) {return ERROR;}
}

export int zbtCreateConvexHullShape(float *points, int numPoints) {

	try {
		ADD_TO_LIST(gCollisionShapeList, new btConvexHullShape(points, numPoints, 3 * sizeof(float)));
	} catch (...) {return ERROR;}

}

export int zbtCreateMultiSphereShape(float *positions, float *radiuses, int num) {

	try {
		ADD_TO_LIST(gCollisionShapeList, new btMultiSphereShape((btVector3*)positions, radiuses, num));
	} catch (...) {return ERROR;}

}

export int zbtStartTriangleMeshShape() {
    if(gTmpTriangleMesh) return ERROR;
    gTmpTriangleMesh = new btTriangleMesh(false,false);
    return DONE;
}

export int zbtAddTriangle(float x1,float y1,float z1,
	float x2,float y2,float z2,float x3,float y3,float z3) {

    if(!gTmpTriangleMesh) return ERROR;
    gTmpTriangleMesh->addTriangle(btVector3(x1,y1,z1), btVector3(x2,y2,z2), btVector3(x3,y3,z3));
    return DONE;
}

export int zbtFinishTriangleMeshShape(int meshType) {
    if(!gTmpTriangleMesh) return ERROR;
	btCollisionShape* tm;

	switch(meshType) {
	case 1: // convex hull
		tm = new btConvexTriangleMeshShape(gTmpTriangleMesh);
		break;
	case 2: // concave static-triangle mesh shape
		tm = new btBvhTriangleMeshShape(gTmpTriangleMesh, true, true);
		break;
	case 3: // concave deformable mesh
		tm = new btGImpactMeshShape(gTmpTriangleMesh);
		((btGImpactMeshShape*)tm)->updateBound();
	}

	gTmpTriangleMesh=NULL;
    ADD_TO_LIST(gCollisionShapeList, tm);
}

export int zbtSetShapeLocalScaling(int shapeId, float x, float y, float z) {
	GET_ITEM_FROM_LIST(gCollisionShapeList, btCollisionShape*, cs, shapeId);
	cs->setLocalScaling(btVector3(x, y, z));
	return DONE;
}

export int zbtSetShapeMargin(int shapeId, float margin) {
	GET_ITEM_FROM_LIST(gCollisionShapeList, btCollisionShape*, cs, shapeId);
	cs->setMargin(margin);
	return DONE;
}

export int zbtDeleteShape(int shapeId) {
	GET_ITEM_FROM_LIST(gCollisionShapeList, btCollisionShape*, cs, shapeId);
	gCollisionShapeList[shapeId] = NULL;
	delete cs;

	return DONE;
}

export int zbtDeleteAllShapes() {
	for(int i = gCollisionShapeList.size()-1; i >= 0; --i)
		zbtDeleteShape(i);
	gCollisionShapeList.clear();

	return DONE;
}

// Rigid bodies

export int zbtAddRigidBodyXYZ(float mass, int shapeId, float x, float y, float z, float rx, float ry, float rz) {
	IS_INITIALIZED;

	btDefaultMotionState* motionState = new btDefaultMotionState(btTransform(btQuaternion(rz*SIMD_2_PI, ry*SIMD_2_PI, rx*SIMD_2_PI), btVector3(x, y, z)));
    btVector3 inertia(0, 0, 0);
	GET_ITEM_FROM_LIST(gCollisionShapeList, btCollisionShape*, shape, shapeId);
	if(mass > 0.f) // mass == 0 - static rigid body
	    shape->calculateLocalInertia(mass, inertia);
    btRigidBody::btRigidBodyConstructionInfo rigidBodyCI(mass, motionState, shape, inertia);
    btRigidBody* rigidBody = new btRigidBody(rigidBodyCI);
    gWorld->addRigidBody(rigidBody);

	ADD_TO_LIST(gCollisionObjectList, rigidBody);
}

export int zbtAddRigidBody(float mass, int shapeId, float pos[3], float rot[3]) {
	return zbtAddRigidBodyXYZ(mass, shapeId, pos[0], pos[1], pos[2], rot[0], rot[1], rot[2]);
}

export int zbtDeleteRigidBody(int rigidBodyId) {
	GET_ITEM_FROM_MIXED_LIST(gCollisionObjectList, btRigidBody*, rb, rigidBodyId);

	// remove related constraints
	while (rb->getNumConstraintRefs()) {
		btTypedConstraint* tc = rb->getConstraintRef(0);

		int i = gConstraintList.findLinearSearch(tc);
		if(i < gConstraintList.size()) gConstraintList[i] = NULL;

		gWorld->removeConstraint(tc);
		delete tc;
	}

	gWorld->removeRigidBody(rb);
	gCollisionObjectList[rigidBodyId] = NULL;
	delete rb->getMotionState();
	delete rb;

	return DONE;
}

export int zbtSetMass(int rigidBodyId, float mass) {
	GET_ITEM_FROM_MIXED_LIST(gCollisionObjectList, btRigidBody*, rb, rigidBodyId);
	btVector3 inertia;
	rb->getCollisionShape()->calculateLocalInertia(mass, inertia);
	rb->setMassProps(mass, inertia);
	return DONE;
}

export int zbtSetDamping(int rigidBodyId, float linDamping, float angDamping) {
	GET_ITEM_FROM_MIXED_LIST(gCollisionObjectList, btRigidBody*, rb, rigidBodyId);
	rb->setDamping(linDamping, angDamping*SIMD_2_PI);
	return DONE;
}

export int zbtSetLinearFactor(int rigidBodyId, float x, float y, float z) {
	GET_ITEM_FROM_MIXED_LIST(gCollisionObjectList, btRigidBody*, rb, rigidBodyId);
	rb->setLinearFactor(btVector3(x, y, z));
	return DONE;
}

export int zbtSetAngularFactor(int rigidBodyId, float x, float y, float z) {
	GET_ITEM_FROM_MIXED_LIST(gCollisionObjectList, btRigidBody*, rb, rigidBodyId);
	rb->setAngularFactor(btVector3(x, y, z));
	return DONE;
}

export int zbtSetGravity(int rigidBodyId, float x, float y, float z) {
	GET_ITEM_FROM_MIXED_LIST(gCollisionObjectList, btRigidBody*, rb, rigidBodyId);
	rb->setGravity(btVector3(x, y, z));
	return DONE;
}

export int zbtGetLinearVelocity(int rigidBodyId, float &x, float &y, float &z) {
	GET_ITEM_FROM_MIXED_LIST(gCollisionObjectList, btRigidBody*, rb, rigidBodyId);
	x = rb->getLinearVelocity().getX();
	y = rb->getLinearVelocity().getY();
	z = rb->getLinearVelocity().getZ();
	return DONE;
}

export int zbtSetLinearVelocity(int rigidBodyId, float x, float y, float z) {
	GET_ITEM_FROM_MIXED_LIST(gCollisionObjectList, btRigidBody*, rb, rigidBodyId);
	rb->setLinearVelocity(btVector3(x, y, z));
	rb->activate(true);
	return DONE;
}

export int zbtGetAngularVelocity(int rigidBodyId, float &x, float &y, float &z) {
	GET_ITEM_FROM_MIXED_LIST(gCollisionObjectList, btRigidBody*, rb, rigidBodyId);
	x = rb->getAngularVelocity().getX() / SIMD_2_PI;
	y = rb->getAngularVelocity().getY() / SIMD_2_PI;
	z = rb->getAngularVelocity().getZ() / SIMD_2_PI;
	return DONE;
}

export int zbtSetAngularVelocity(int rigidBodyId, float x, float y, float z) {
	GET_ITEM_FROM_MIXED_LIST(gCollisionObjectList, btRigidBody*, rb, rigidBodyId);
	rb->setAngularVelocity(btVector3(x*SIMD_2_PI, y*SIMD_2_PI, z*SIMD_2_PI));
	rb->activate(true);
	return DONE;
}

export int zbtApplyCentralImpulse(int rigidBodyId, float x, float y, float z) {
	GET_ITEM_FROM_MIXED_LIST(gCollisionObjectList, btRigidBody*, rb, rigidBodyId);
	rb->applyCentralImpulse(btVector3(x, y, z));
	rb->activate(true);
	return DONE;
}

export int zbtApplyTorqueImpulse(int rigidBodyId, float rx, float ry, float rz) {
	GET_ITEM_FROM_MIXED_LIST(gCollisionObjectList, btRigidBody*, rb, rigidBodyId);
	rb->applyTorqueImpulse(btVector3(rx*SIMD_2_PI, ry*SIMD_2_PI, rz*SIMD_2_PI));
	rb->activate(true);
	return DONE;
}

export int zbtApplyImpulse(int rigidBodyId, float x, float y, float z, float relX, float relY, float relZ) {
	GET_ITEM_FROM_MIXED_LIST(gCollisionObjectList, btRigidBody*, rb, rigidBodyId);
	rb->applyImpulse(btVector3(x, y, z), btVector3(relX, relY, relZ));
	rb->activate(true);
	return DONE;
}

// Constraints and limits

export int zbtAreConnected(int rigidBodyAId, int rigidBodyBId) {
	GET_ITEM_FROM_LIST(gCollisionObjectList, btRigidBody*, rb1, rigidBodyAId);
	GET_ITEM_FROM_LIST(gCollisionObjectList, btRigidBody*, rb2, rigidBodyBId);

	return int(!rb1->checkCollideWithOverride(rb2));
}

export int zbtAddPoint2PointConstraint(int rigidBodyAId, int rigidBodyBId,
	float pivotAx, float pivotAy, float pivotAz,
	float pivotBx, float pivotBy, float pivotBz, bool bDisableCollision) {

	GET_ITEM_FROM_LIST(gCollisionObjectList, btRigidBody*, rbA, rigidBodyAId);
	GET_ITEM_FROM_LIST(gCollisionObjectList, btRigidBody*, rbB, rigidBodyBId);

	btTypedConstraint* tc = new btPoint2PointConstraint(*rbA, *rbB,
		btVector3(pivotAx, pivotAy, pivotAz), btVector3(pivotBx, pivotBy, pivotBz));
	gWorld->addConstraint(tc, bDisableCollision);

	ADD_TO_LIST(gConstraintList, tc);
}

export int zbtAddPoint2PointConstraint1(int rigidBodyId, float pivotX, float pivotY, float pivotZ,
	bool bDisableCollision) {

	GET_ITEM_FROM_LIST(gCollisionObjectList, btRigidBody*, rb, rigidBodyId);

	btTypedConstraint* tc = new btPoint2PointConstraint(*rb, btVector3(pivotX, pivotY, pivotZ));
	gWorld->addConstraint(tc, bDisableCollision);

	ADD_TO_LIST(gConstraintList, tc);
}

export int zbtAddHingeConstraint(int rigidBodyAId, int rigidBodyBId,
	float pivotAx, float pivotAy, float pivotAz,
	float pivotBx, float pivotBy, float pivotBz,
	float axisAx, float axisAy, float axisAz,
	float axisBx, float axisBy, float axisBz, bool bDisableCollision) {

	GET_ITEM_FROM_LIST(gCollisionObjectList, btRigidBody*, rbA, rigidBodyAId);
	GET_ITEM_FROM_LIST(gCollisionObjectList, btRigidBody*, rbB, rigidBodyBId);

	btTypedConstraint* tc = new btHingeConstraint(*rbA, *rbB,
		btVector3(pivotAx, pivotAy, pivotAz), btVector3(pivotBx, pivotBy, pivotBz),
		btVector3(axisAx, axisAy, axisAz), btVector3(axisBx, axisBy, axisBz));
	gWorld->addConstraint(tc, bDisableCollision);

	ADD_TO_LIST(gConstraintList, tc);
}

export int zbtAddHingeConstraint1(int rigidBodyId,
	float pivotX, float pivotY, float pivotZ,
	float axisX, float axisY, float axisZ, bool bDisableCollision) {

	GET_ITEM_FROM_LIST(gCollisionObjectList, btRigidBody*, rb, rigidBodyId);

	btTypedConstraint* tc = new btHingeConstraint(*rb,
		btVector3(pivotX, pivotY, pivotZ), btVector3(axisX, axisY, axisZ));
	gWorld->addConstraint(tc, bDisableCollision);

	ADD_TO_LIST(gConstraintList, tc);
}

export int zbtSetHingeLimits(int constraintId, float low, float high,
	float softness, float biasFactor, float relaxationFactor) {

	GET_ITEM_FROM_MIXED_LIST(gConstraintList, btHingeConstraint*, hc, constraintId);
	hc->setLimit(low*SIMD_2_PI , high*SIMD_2_PI, softness, biasFactor, relaxationFactor);

	return DONE;
}

export int zbtEnableHingeAngularMotor(int constraintId, bool bEnableMotor, float targetVelocity, float maxMotorImpulse) {
	GET_ITEM_FROM_MIXED_LIST(gConstraintList, btHingeConstraint*, hc, constraintId);
	hc->enableAngularMotor(bEnableMotor, targetVelocity, maxMotorImpulse);

	return DONE;
}

export int zbtAddConeTwistConstraint(int rigidBodyAId, int rigidBodyBId,
	float pivotAx, float pivotAy, float pivotAz,
	float pivotBx, float pivotBy, float pivotBz,
	float rotAx, float rotAy, float rotAz,
	float rotBx, float rotBy, float rotBz, bool bDisableCollision) {

	GET_ITEM_FROM_LIST(gCollisionObjectList, btRigidBody*, rbA, rigidBodyAId);
	GET_ITEM_FROM_LIST(gCollisionObjectList, btRigidBody*, rbB, rigidBodyBId);

	btTypedConstraint* tc = new btConeTwistConstraint(*rbA, *rbB,
		btTransform(btQuaternion(rotAz*SIMD_2_PI, rotAy*SIMD_2_PI, rotAx*SIMD_2_PI), btVector3(pivotAx, pivotAy, pivotAz)),
		btTransform(btQuaternion(rotBz*SIMD_2_PI, rotBy*SIMD_2_PI, rotBx*SIMD_2_PI), btVector3(pivotBx, pivotBy, pivotBz)));
	gWorld->addConstraint(tc, bDisableCollision);

	ADD_TO_LIST(gConstraintList, tc);
}

export int zbtAddConeTwistConstraint1(int rigidBodyId,
	float pivotX, float pivotY, float pivotZ,
	float rotX, float rotY, float rotZ, bool bDisableCollision) {

	GET_ITEM_FROM_LIST(gCollisionObjectList, btRigidBody*, rb, rigidBodyId);

	btTypedConstraint* tc = new btConeTwistConstraint(*rb,
		btTransform(btQuaternion(rotZ*SIMD_2_PI, rotY*SIMD_2_PI, rotX*SIMD_2_PI), btVector3(pivotX, pivotY, pivotZ)));
	gWorld->addConstraint(tc, bDisableCollision);

	ADD_TO_LIST(gConstraintList, tc);
}

export int zbtSetConeTwistLimits(int constraintId, float swingX, float swingY, float swingZ,
	float softness, float biasFactor, float relaxationFactor) {

	GET_ITEM_FROM_MIXED_LIST(gConstraintList, btConeTwistConstraint*, cc, constraintId);

	cc->setLimit(swingZ*SIMD_2_PI, swingY*SIMD_2_PI, swingX*SIMD_2_PI,
		softness, biasFactor, relaxationFactor);

	return DONE;
}

export int zbtAddSliderConstraint(int rigidBodyAId, int rigidBodyBId,
	float pivotAx, float pivotAy, float pivotAz,
	float pivotBx, float pivotBy, float pivotBz,
	float rotAx, float rotAy, float rotAz,
	float rotBx, float rotBy, float rotBz, bool bUseLinearReferenceFrameA, bool bDisableCollision) {

	GET_ITEM_FROM_LIST(gCollisionObjectList, btRigidBody*, rbA, rigidBodyAId);
	GET_ITEM_FROM_LIST(gCollisionObjectList, btRigidBody*, rbB, rigidBodyBId);

	btTypedConstraint* tc = new btSliderConstraint(*rbA, *rbB,
		btTransform(btQuaternion(rotAz*SIMD_2_PI, rotAy*SIMD_2_PI, rotAx*SIMD_2_PI), btVector3(pivotAx, pivotAy, pivotAz)),
		btTransform(btQuaternion(rotBz*SIMD_2_PI, rotBy*SIMD_2_PI, rotBx*SIMD_2_PI), btVector3(pivotBx, pivotBy, pivotBz)),
		bUseLinearReferenceFrameA);
	gWorld->addConstraint(tc, bDisableCollision);

	ADD_TO_LIST(gConstraintList, tc);
}

export int zbtSetSliderLimits(int constraintId, float linLower, float linUpper, float angLower, float angUpper) {
	GET_ITEM_FROM_MIXED_LIST(gConstraintList, btSliderConstraint*, sc, constraintId);

	sc->setLowerLinLimit(linLower);
	sc->setUpperLinLimit(linUpper);
	sc->setLowerAngLimit(angLower*SIMD_2_PI);
	sc->setUpperAngLimit(angUpper*SIMD_2_PI);

	return DONE;
}

export int zbtSetSliderSoftness(int constraintId, float dirLin, float dirAng, float limLin, float limAng,
	float orthoLin, float orthoAng) {

	GET_ITEM_FROM_MIXED_LIST(gConstraintList, btSliderConstraint*, sc, constraintId);

	sc->setSoftnessDirLin(dirLin);
	sc->setSoftnessDirAng(dirAng*SIMD_2_PI);
	sc->setSoftnessLimLin(limLin);
	sc->setSoftnessLimAng(limAng*SIMD_2_PI);
	sc->setSoftnessOrthoLin(orthoLin);
	sc->setSoftnessOrthoAng(orthoAng*SIMD_2_PI);

	return DONE;
}

export int zbtSetSliderRestitution(int constraintId, float dirLin, float dirAng, float limLin, float limAng,
	float orthoLin, float orthoAng) {

	GET_ITEM_FROM_MIXED_LIST(gConstraintList, btSliderConstraint*, sc, constraintId);

	sc->setRestitutionDirLin(dirLin);
	sc->setRestitutionDirAng(dirAng*SIMD_2_PI);
	sc->setRestitutionLimLin(limLin);
	sc->setRestitutionLimAng(limAng*SIMD_2_PI);
	sc->setRestitutionOrthoLin(orthoLin);
	sc->setRestitutionOrthoAng(orthoAng*SIMD_2_PI);

	return DONE;
}

export int zbtSetSliderDamping(int constraintId, float dirLin, float dirAng, float limLin, float limAng,
	float orthoLin, float orthoAng) {

	GET_ITEM_FROM_MIXED_LIST(gConstraintList, btSliderConstraint*, sc, constraintId);

	sc->setDampingDirLin(dirLin);
	sc->setDampingDirAng(dirAng*SIMD_2_PI);
	sc->setDampingLimLin(limLin);
	sc->setDampingLimAng(limAng*SIMD_2_PI);
	sc->setDampingOrthoLin(orthoLin);
	sc->setDampingOrthoAng(orthoAng*SIMD_2_PI);

	return DONE;
}

export int zbtEnableSliderMotor(int constraintId, bool bEnableLinMotor, float targetLinVelocity, float maxLinForce,
	bool bEnableAngMotor, float targetAngVelocity, float maxAngForce) {

	GET_ITEM_FROM_MIXED_LIST(gConstraintList, btSliderConstraint*, sc, constraintId);

	sc->setPoweredLinMotor(bEnableLinMotor);
	if(bEnableLinMotor) {
		sc->setTargetLinMotorVelocity(targetLinVelocity);
		sc->setMaxLinMotorForce(maxLinForce);
	}

	sc->setPoweredAngMotor(bEnableAngMotor);
	if(bEnableAngMotor) {
		sc->setTargetAngMotorVelocity(targetAngVelocity*SIMD_2_PI);
		sc->setMaxAngMotorForce(maxAngForce);
	}

	return DONE;
}

export int zbtAddGeneric6DofConstraint(int rigidBodyAId, int rigidBodyBId,
	float pivotAx, float pivotAy, float pivotAz,
	float pivotBx, float pivotBy, float pivotBz,
	float rotAx, float rotAy, float rotAz,
	float rotBx, float rotBy, float rotBz, bool bUseLinearReferenceFrameA, bool bDisableCollision) {

	GET_ITEM_FROM_LIST(gCollisionObjectList, btRigidBody*, rbA, rigidBodyAId);
	GET_ITEM_FROM_LIST(gCollisionObjectList, btRigidBody*, rbB, rigidBodyBId);

	btTypedConstraint* tc = new btGeneric6DofConstraint(*rbA, *rbB,
		btTransform(btQuaternion(rotAz*SIMD_2_PI, rotAy*SIMD_2_PI, rotAx*SIMD_2_PI), btVector3(pivotAx, pivotAy, pivotAz)),
		btTransform(btQuaternion(rotBz*SIMD_2_PI, rotBy*SIMD_2_PI, rotBx*SIMD_2_PI), btVector3(pivotBx, pivotBy, pivotBz)),
		bUseLinearReferenceFrameA);
	gWorld->addConstraint(tc, bDisableCollision);

	ADD_TO_LIST(gConstraintList, tc);
}

export int zbtSetGeneric6DofLimits(int constraintId, int axis, float lo, float hi) {
	GET_ITEM_FROM_MIXED_LIST(gConstraintList, btGeneric6DofConstraint*, dc, constraintId);

	if(axis < 3)
		dc->setLimit(axis, lo, hi); // linear
	else
		dc->setLimit(axis, lo*SIMD_2_PI, hi*SIMD_2_PI); // angular

	return DONE;
}

export int zbtSetGeneric6DofLinearLimits(int constraintId, float lowerX, float lowerY, float lowerZ,
	float upperX, float upperY, float upperZ) {

	GET_ITEM_FROM_MIXED_LIST(gConstraintList, btGeneric6DofConstraint*, dc, constraintId);

	dc->setLinearLowerLimit(btVector3(lowerX, lowerY, lowerZ));
	dc->setLinearUpperLimit(btVector3(upperX, upperY, upperZ));

	return DONE;
}

export int zbtSetGeneric6DofAngularLimits(int constraintId, float lowerX, float lowerY, float lowerZ,
	float upperX, float upperY, float upperZ) {

	GET_ITEM_FROM_MIXED_LIST(gConstraintList, btGeneric6DofConstraint*, dc, constraintId);

	dc->setAngularLowerLimit(btVector3(lowerX*SIMD_2_PI, lowerY*SIMD_2_PI, lowerZ*SIMD_2_PI));
	dc->setAngularUpperLimit(btVector3(upperX*SIMD_2_PI, upperY*SIMD_2_PI, upperZ*SIMD_2_PI));

	return DONE;
}

export int zbtDeleteConstraint(int constraintId) {
	GET_ITEM_FROM_LIST(gConstraintList, btTypedConstraint*, tc, constraintId);
	gWorld->removeConstraint(tc);
	gConstraintList[constraintId] = NULL;
	delete tc;

	return DONE;
}


// Raycast vehicle

// use before zbtCreateRaycastVehicle or zbtAddWheel
export int zbtSetVehicleTunning(float suspStiffness, float suspCompression,
	float suspDamping, float maxSuspTravelCm, float maxSuspForce,
	float frictionSlip) {

	gTuning.m_suspensionStiffness = suspStiffness;
	gTuning.m_suspensionCompression = suspCompression;
	gTuning.m_suspensionDamping = suspDamping;
	gTuning.m_maxSuspensionTravelCm = maxSuspTravelCm;
	gTuning.m_frictionSlip = frictionSlip;
	gTuning.m_maxSuspensionForce = maxSuspForce;

	return DONE;
}

export int zbtCreateRaycastVehicle(int rigidBodyId, int rightIndex, int upIndex, int forwardIndex) {
	GET_ITEM_FROM_LIST(gCollisionObjectList, btRigidBody*, carChassis, rigidBodyId);

	if(!gVehicleRaycaster) gVehicleRaycaster = new btDefaultVehicleRaycaster(gWorld);
	btRaycastVehicle* rv = new btRaycastVehicle(gTuning, carChassis, gVehicleRaycaster);
	rv->setCoordinateSystem(rightIndex, upIndex, forwardIndex);
	gWorld->addVehicle(rv);
	carChassis->setActivationState(DISABLE_DEACTIVATION); //necessary
	ADD_TO_LIST(gConstraintList, (btTypedConstraint*) rv);
}

export int zbtAddWheel(int vehicleId,
	float connectionPointX, float connectionPointY, float connectionPointZ,
	float directionX, float directionY, float directionZ,
	float wheelAxleX, float wheelAxleY, float wheelAxleZ,
	float wheelRadius, float suspRestLength, bool bIsFrontWheel) {

	GET_ITEM_FROM_MIXED_LIST(gConstraintList, btRaycastVehicle*, rv, vehicleId);

	btWheelInfo wi = rv->addWheel(btVector3(connectionPointX, connectionPointY, connectionPointZ),
		btVector3(directionX, directionY, directionZ), btVector3(wheelAxleX, wheelAxleY, wheelAxleZ),
		suspRestLength, wheelRadius, gTuning, bIsFrontWheel);

	return rv->getNumWheels()-1;
}

export int zbtSetWheelIsFront(int vehicleId, int wheelId, bool bIsFront) {
	GET_ITEM_FROM_MIXED_LIST(gConstraintList, btRaycastVehicle*, rv, vehicleId);
	CHECK_WHEEL(wheelId);
	rv->getWheelInfo(wheelId).m_bIsFrontWheel = bIsFront;
	return DONE;
}

export int zbtSetWheelRadius(int vehicleId, int wheelId, float radius) {
	GET_ITEM_FROM_MIXED_LIST(gConstraintList, btRaycastVehicle*, rv, vehicleId);
	CHECK_WHEEL(wheelId);
	rv->getWheelInfo(wheelId).m_wheelsRadius = radius;
	return DONE;
}

export int zbtSetWheelEngineForce(int vehicleId, int wheelId, float engineForce) {
	GET_ITEM_FROM_MIXED_LIST(gConstraintList, btRaycastVehicle*, rv, vehicleId);
	CHECK_WHEEL(wheelId);
	rv->getWheelInfo(wheelId).m_engineForce = engineForce;
	return DONE;
}

export int zbtSetWheelBreakAmount(int vehicleId, int wheelId, float brakeAmount) {
	GET_ITEM_FROM_MIXED_LIST(gConstraintList, btRaycastVehicle*, rv, vehicleId);
	CHECK_WHEEL(wheelId);
	rv->getWheelInfo(wheelId).m_brake = brakeAmount;
	return DONE;
}

export int zbtSetWheelRollInfluence(int vehicleId, int wheelId, float rollInfluence) {
	GET_ITEM_FROM_MIXED_LIST(gConstraintList, btRaycastVehicle*, rv, vehicleId);
	CHECK_WHEEL(wheelId);
	rv->getWheelInfo(wheelId).m_rollInfluence = rollInfluence;
	return DONE;
}

export int zbtSetWheelFrictionSlip(int vehicleId, int wheelId, float frictionSlip) {
	GET_ITEM_FROM_MIXED_LIST(gConstraintList, btRaycastVehicle*, rv, vehicleId);
	CHECK_WHEEL(wheelId);
	rv->getWheelInfo(wheelId).m_frictionSlip = frictionSlip;
	return DONE;
}

export int zbtSetWheelSuspRestLength(int vehicleId, int wheelId, float suspRestLength) {
	GET_ITEM_FROM_MIXED_LIST(gConstraintList, btRaycastVehicle*, rv, vehicleId);
	CHECK_WHEEL(wheelId);
	rv->getWheelInfo(wheelId).m_suspensionRestLength1 = suspRestLength;
	return DONE;
}

export int zbtSetWheelMaxSuspTravel(int vehicleId, int wheelId, float maxSuspTravel) {
	GET_ITEM_FROM_MIXED_LIST(gConstraintList, btRaycastVehicle*, rv, vehicleId);
	CHECK_WHEEL(wheelId);
	rv->getWheelInfo(wheelId).m_maxSuspensionTravelCm = maxSuspTravel;
	return DONE;
}

export int zbtSetWheelSuspStiffness(int vehicleId, int wheelId, float suspStiffness) {
	GET_ITEM_FROM_MIXED_LIST(gConstraintList, btRaycastVehicle*, rv, vehicleId);
	CHECK_WHEEL(wheelId);
	rv->getWheelInfo(wheelId).m_suspensionStiffness = suspStiffness;
	return DONE;
}

export int zbtSetWheelDampingCompression(int vehicleId, int wheelId, float dampingCompression) {
	GET_ITEM_FROM_MIXED_LIST(gConstraintList, btRaycastVehicle*, rv, vehicleId);
	CHECK_WHEEL(wheelId);
	rv->getWheelInfo(wheelId).m_wheelsDampingCompression = dampingCompression;
	return DONE;
}

export int zbtSetWheelDampingRelaxation(int vehicleId, int wheelId, float dampingRelaxation) {
	GET_ITEM_FROM_MIXED_LIST(gConstraintList, btRaycastVehicle*, rv, vehicleId);
	CHECK_WHEEL(wheelId);
	rv->getWheelInfo(wheelId).m_wheelsDampingRelaxation = dampingRelaxation;
	return DONE;
}

export int zbtSetVehicleSteeringValue(int vehicleId, int wheelId, float steering) {
	GET_ITEM_FROM_MIXED_LIST(gConstraintList, btRaycastVehicle*, rv, vehicleId);
	CHECK_WHEEL(wheelId);
	rv->setSteeringValue(steering, wheelId);
	return DONE;
}

export int zbtSetVehicleEngineForce(int vehicleId, int wheelId, float force) {
	GET_ITEM_FROM_MIXED_LIST(gConstraintList, btRaycastVehicle*, rv, vehicleId);
	CHECK_WHEEL(wheelId);
	rv->applyEngineForce(force, wheelId);
	return DONE;
}

export int zbtSetVehicleBrake(int vehicleId, int wheelId, float brake) {
	GET_ITEM_FROM_MIXED_LIST(gConstraintList, btRaycastVehicle*, rv, vehicleId);
	CHECK_WHEEL(wheelId);
	rv->setBrake(brake, wheelId);
	return DONE;
}

export int zbtSetVehiclePitchControl(int vehicleId, float pitch) {
	GET_ITEM_FROM_MIXED_LIST(gConstraintList, btRaycastVehicle*, rv, vehicleId);
	rv->setPitchControl(pitch);
	return DONE;
}

export int zbtResetVehicleSusp(int vehicleId) {
	GET_ITEM_FROM_MIXED_LIST(gConstraintList, btRaycastVehicle*, rv, vehicleId);
	rv->resetSuspension();
	return DONE;
}

export float zbtGetVehicleCurrentSpeed(int vehicleId) {
	GET_ITEM_FROM_MIXED_LIST(gConstraintList, btRaycastVehicle*, rv, vehicleId);
	return rv->getCurrentSpeedKmHour();
}

export int zbtGetWheelPositionXYZ(int vehicleId, int wheelId, float &x, float &y, float &z) {
	GET_ITEM_FROM_MIXED_LIST(gConstraintList, btRaycastVehicle*, rv, vehicleId);
	CHECK_WHEEL(wheelId);
	x = rv->getWheelTransformWS(wheelId).getOrigin().getX();
	y = rv->getWheelTransformWS(wheelId).getOrigin().getY();
	z = rv->getWheelTransformWS(wheelId).getOrigin().getZ();
	return DONE;
}

export int zbtGetWheelPosition(int vehicleId, int wheelId, float pos[3]) {
	return zbtGetWheelPositionXYZ(vehicleId, wheelId, pos[0], pos[1], pos[2]);
}

export int zbtGetWheelRotationXYZ(int vehicleId, int wheelId, float &rx, float &ry, float &rz) {
	GET_ITEM_FROM_MIXED_LIST(gConstraintList, btRaycastVehicle*, rv, vehicleId);
	CHECK_WHEEL(wheelId);

	//rv->getWheelInfo(wheelId).m_worldTransform.getBasis().getEulerZYX(rz, ry, rx);
	rv->getWheelTransformWS(wheelId).getBasis().getEulerZYX(rz, ry, rx);
	rx /= SIMD_2_PI;
	ry /= SIMD_2_PI;
	rz /= SIMD_2_PI;
	return DONE;
}

export int zbtGetWheelRotation(int vehicleId, int wheelId, float rot[3]) {
	return zbtGetWheelRotationXYZ(vehicleId, wheelId, rot[0], rot[1], rot[2]);
}

export int zbtGetWheelPosRotXYZ(int vehicleId, int wheelId,
	float &x, float &y, float &z, float &rx, float &ry, float &rz) {

	zbtGetWheelPositionXYZ(vehicleId, wheelId, x, y, z);
	zbtGetWheelRotationXYZ(vehicleId, wheelId, rx, ry, rz);
	return DONE;
}

export int zbtGetWheelPosRot(int vehicleId, int wheelId, float pos[3], float rot[3]) {
	zbtGetWheelPositionXYZ(vehicleId, wheelId, pos[0], pos[1], pos[2]);
	zbtGetWheelRotationXYZ(vehicleId, wheelId, rot[0], rot[1], rot[2]);
	return DONE;	
}

export int zbtDeleteRaycastVehicle(int vehicleId) {
	GET_ITEM_FROM_MIXED_LIST(gConstraintList, btRaycastVehicle*, rv, vehicleId);
	gWorld->removeVehicle(rv);
	gConstraintList[vehicleId] = NULL;
	delete rv;

	return DONE;
}


/* ***************************************************************************
// Ghost object

// TODO !!! WORKING HERE !!!
// TODO ghost objects - find more examples and check the existing code

export int zbtAddGhostObject(int shapeId, float x, float y, float z, float rx, float ry, float rz)
{
	IS_INITIALIZED;

	GET_ITEM_FROM_LIST(gCollisionShapeList, btCollisionShape*, shape, shapeId);
	btPairCachingGhostObject* ghostObject = new btPairCachingGhostObject();
	ghostObject->setCollisionShape(shape);
	ghostObject->setWorldTransform(btTransform(btQuaternion(rz*SIMD_2_PI, ry*SIMD_2_PI, rx*SIMD_2_PI), btVector3(x, y, z)));
	ghostObject->setCollisionFlags (btCollisionObject::CF_CHARACTER_OBJECT); // ???
	gWorld->addCollisionObject(ghostObject);
	ADD_TO_LIST(gCollisionObjectList, ghostObject);
}

export int zbtDeleteGhostObject(int ghostObjectId)
{
	GET_ITEM_FROM_MIXED_LIST(gCollisionObjectList, btGhostObject*, go, ghostObjectId);
	gWorld->removeCollisionObject(go);
	gCollisionObjectList[ghostObjectId] = NULL;
	delete go;

	return DONE;
}

// TODO Kinematic character controller - is it really necessary in ZGE???, is not ghost object sufficient?

// ***************************************************************************
*/

// Collision objects (in general)

export int zbtSetFriction(int collisionObjectId, float friction) {
	GET_ITEM_FROM_LIST(gCollisionObjectList, btCollisionObject*, co, collisionObjectId);
	co->setFriction(friction);
	return DONE;
}

export int zbtSetRestitution(int collisionObjectId, float restitution) {
	GET_ITEM_FROM_LIST(gCollisionObjectList, btCollisionObject*, co, collisionObjectId);
	co->setRestitution(restitution);
	return DONE;
}

export int zbtSetHitFraction(int collisionObjectId, float hitFraction) {
	GET_ITEM_FROM_LIST(gCollisionObjectList, btCollisionObject*, co, collisionObjectId);
	co->setHitFraction(hitFraction);
	return DONE;
}

export int zbtGetPositionXYZ(int collisionObjectId, float &x, float &y, float &z) {
	GET_ITEM_FROM_LIST(gCollisionObjectList, btCollisionObject*, co, collisionObjectId);
	x = co->getWorldTransform().getOrigin().getX();
	y = co->getWorldTransform().getOrigin().getY();
	z = co->getWorldTransform().getOrigin().getZ();
	return DONE;
}

export int zbtGetPosition(int collisionObjectId, float pos[3]) {
	return zbtGetPositionXYZ(collisionObjectId, pos[0], pos[1], pos[2]);
}

export int zbtSetPosition(int collisionObjectId, float x, float y, float z) {
	GET_ITEM_FROM_LIST(gCollisionObjectList, btCollisionObject*, co, collisionObjectId);
	btTransform trans = co->getWorldTransform();
	trans.setOrigin(btVector3(x, y, z));
	co->setWorldTransform(trans);
	return DONE;
}

export int zbtGetRotationXYZ(int collisionObjectId, float &rx, float &ry, float &rz) {
	GET_ITEM_FROM_LIST(gCollisionObjectList, btCollisionObject*, co, collisionObjectId);

	co->getWorldTransform().getBasis().getEulerZYX(rz, ry, rx);
	rx /= SIMD_2_PI;
	ry /= SIMD_2_PI;
	rz /= SIMD_2_PI;

	return DONE;
}

export int zbtGetRotation(int collisionObjectId, float rot[3]) {
	return zbtGetRotationXYZ(collisionObjectId, rot[0], rot[1], rot[2]);
}

export int zbtSetRotation(int collisionObjectId, float rx, float ry, float rz) {
	GET_ITEM_FROM_LIST(gCollisionObjectList, btCollisionObject*, co, collisionObjectId);
	btTransform trans = co->getWorldTransform();
	trans.setRotation(btQuaternion(rz*SIMD_2_PI, ry*SIMD_2_PI, rx*SIMD_2_PI));
	co->setWorldTransform(trans);
	return DONE;
}

export int zbtGetPosRotXYZ(int collisionObjectId, float &x, float &y, float &z,
	float &rx, float &ry, float &rz) {

	zbtGetPositionXYZ(collisionObjectId, x, y, z);
	zbtGetRotationXYZ(collisionObjectId, rx, ry, rz);
	return DONE;
}

export int zbtGetPosRot(int collisionObjectId, float pos[3], float rot[3]) {

	zbtGetPositionXYZ(collisionObjectId, pos[0], pos[1], pos[2]);
	zbtGetRotationXYZ(collisionObjectId, rot[0], rot[1], rot[2]);
	return DONE;
}

// Activation and deactivation

export int zbtIsActive(int collisionObjectId) {
	GET_ITEM_FROM_LIST(gCollisionObjectList, btCollisionObject*, co, collisionObjectId);
	return int(co->isActive());
}

export int zbtActivate(int collisionObjectId, bool bForceActivation) {
	GET_ITEM_FROM_LIST(gCollisionObjectList, btCollisionObject*, co, collisionObjectId);
	co->activate(bForceActivation);
	return DONE;
}

export int zbtSetActivationState(int collisionObjectId, int newState) {
	GET_ITEM_FROM_LIST(gCollisionObjectList, btCollisionObject*, co, collisionObjectId);
	co->setActivationState(newState);
	return DONE;
}

export int zbtSetDeactivationTime(int collisionObjectId, float time) {
	GET_ITEM_FROM_LIST(gCollisionObjectList, btCollisionObject*, co, collisionObjectId);
	co->setDeactivationTime(time);
	return DONE;
}

export int zbtSetDeactivationThresholds(int rigidBodyId, float linear, float angular) {
	GET_ITEM_FROM_LIST(gCollisionObjectList, btRigidBody*, rb, rigidBodyId);
	rb->setSleepingThresholds(linear, angular*SIMD_2_PI);
	return DONE;
}

// Collision detection

export int zbtGetCollisionNum() {
	IS_INITIALIZED;
	return gWorld->getDispatcher()->getNumManifolds();
}

export int zbtGetCollisionObjects(int index, int &collisionObject1, int &collisionObject2) {
	IS_INITIALIZED;

	try 	{
		collisionObject1 = gCollisionObjectList.findLinearSearch((btCollisionObject*)gWorld->getDispatcher()->
			getManifoldByIndexInternal(index)->getBody0());

		collisionObject2 = gCollisionObjectList.findLinearSearch((btCollisionObject*)gWorld->getDispatcher()->
			getManifoldByIndexInternal(index)->getBody0());

		if(collisionObject1 < gCollisionObjectList.size() && collisionObject2 < gCollisionObjectList.size()) return DONE;
		else return ERROR;
	} catch (...) {return ERROR;}
}

// NEEDED? !!!
/*export int zbtGetCollisionObject1(int index)
{
	IS_INITIALIZED;
	try
	{
		int i = gCollisionObjectList.findLinearSearch((btCollisionObject*)gWorld->getDispatcher()->
			getManifoldByIndexInternal(index)->getBody0());

		if(i < gCollisionObjectList.size()) return i;
		else return ERROR;
	} catch (...) {return ERROR;}
}*/

// NEEDED? !!!
/*export int zbtGetCollisionObject2(int index)
{
	IS_INITIALIZED;
	try
	{
		int i = gCollisionObjectList.findLinearSearch((btCollisionObject*)gWorld->getDispatcher()->
			getManifoldByIndexInternal(index)->getBody1());

		if(i < gCollisionObjectList.size()) return i;
		else return ERROR;
	} catch (...) {return ERROR;}
}*/

export int zbtGetCollisionOfNum(int collisionObjectId) {
	GET_ITEM_FROM_LIST(gCollisionObjectList, btCollisionObject*, co, collisionObjectId);

	int ret = 0;
	for(int i = gWorld->getDispatcher()->getNumManifolds()-1; i >= 0; --i) {
		btPersistentManifold* pm = gWorld->getDispatcher()->getManifoldByIndexInternal(i);

		if((btCollisionObject*)pm->getBody0() == co || (btCollisionObject*)pm->getBody1() == co) {
			int j = pm->getNumContacts()-1;
			for(;j>=0;--j) if(pm->validContactDistance(pm->getContactPoint(j))) break;
			if(j>=0) ret++;
		}
	}
	return ret;
}

// REWORK !!!
/*export int zbtGetCollisionOf(int collisionObjectId, int index)
{
	if(!gComputeCollisions || collisionObjectId < 0 || collisionObjectId >= int(gCollisionList.size()) ||
		index < 0 || index >= int(gCollisionList[collisionObjectId].size())) return ERROR;

	return gCollisionList[collisionObjectId][index];
}
*/

export int zbtIsCollidedWith(int collisionObjectId1, int collisionObjectId2) {
	GET_ITEM_FROM_LIST(gCollisionObjectList, btCollisionObject*, co1, collisionObjectId1);
	GET_ITEM_FROM_LIST(gCollisionObjectList, btCollisionObject*, co2, collisionObjectId2);

	for(int i = gWorld->getDispatcher()->getNumManifolds()-1; i >= 0; --i) {
		btPersistentManifold* pm = gWorld->getDispatcher()->getManifoldByIndexInternal(i);
		btCollisionObject* bo1 = (btCollisionObject*)pm->getBody0();
		btCollisionObject* bo2 = (btCollisionObject*)pm->getBody1();

		if((bo1 == co1 && bo2 == co2) || (bo2 == co1 && bo1 == co2)) {
			int j = pm->getNumContacts()-1;
			for(;j>=0;--j) if(pm->validContactDistance(pm->getContactPoint(j))) break;
			if(j>=0) return TRUE;
			else return FALSE;
		}
	}
	return FALSE;
}

// Raycasting

// TODO Check code of raycasting with btDefaultVehicleRaycaster::castRay

export int zbtRayTest(float fromX, float fromY, float fromZ, float toX, float toY, float toZ) {
	IS_INITIALIZED;

	btCollisionWorld::ClosestRayResultCallback crrc(btVector3(fromX, fromY, fromZ), btVector3(toX, toY, toZ)); 
	gWorld->rayTest(btVector3(fromX, fromY, fromZ), btVector3(toX, toY, toZ), crrc);
	if(crrc.hasHit())
		try {
			int i = gCollisionObjectList.findLinearSearch((btRigidBody*)crrc.m_collisionObject);

			if(i < gCollisionObjectList.size()) {
				gRayTestHitPoint = crrc.m_hitPointWorld;
				gRayTestHitNormal = crrc.m_hitNormalWorld;
				return i;
			}
			else return ERROR;

		} catch (...) {return ERROR;}
	else
		return ERROR;
}

export int zbtGetRayTestHitPoint(float &x, float &y, float &z) {
	IS_INITIALIZED;
	x = gRayTestHitPoint.getX();
	y = gRayTestHitPoint.getY();
	z = gRayTestHitPoint.getZ();
	return DONE;
}

export int zbtGetRayTestHitNormal(float &x, float &y, float &z) {
	IS_INITIALIZED;
	x = gRayTestHitNormal.getX();
	y = gRayTestHitNormal.getY();
	z = gRayTestHitNormal.getZ();
	return DONE;
}