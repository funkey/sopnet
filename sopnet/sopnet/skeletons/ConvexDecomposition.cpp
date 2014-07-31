#include <hacdHACD.h>
#include <hacdMicroAllocator.h>
#include "ConvexDecomposition.h"
#include <util/Logger.h>

logger::LogChannel convexdecompositionlog("convexdecompositionlog", "[ConvexDecomposition] ");

void
hacdCallback(const char * msg, double /*progress*/, double /*concavity*/, size_t /*nVertices*/) {

	LOG_DEBUG(convexdecompositionlog) << "hacd: " << msg;
}

ConvexDecomposition::ConvexDecomposition() {

	registerInput(_mesh, "mesh");

	registerInput(_compacityWeight,       "compacity weight",        pipeline::Optional);
	registerInput(_volumeWeight,          "volume weight",           pipeline::Optional);
	registerInput(_connectDistance,       "connect distance",        pipeline::Optional);
	registerInput(_minNumClusters,        "min num clusters",        pipeline::Optional);
	registerInput(_maxNumHullVertices,    "max num hull vertices",   pipeline::Optional);
	registerInput(_maxConcavity,          "max concavity",           pipeline::Optional);
	registerInput(_smallClusterThreshold, "small cluster threshold", pipeline::Optional);
	registerInput(_numTargetTriangles,    "num target triangles",    pipeline::Optional);
	registerInput(_addExtraDistPoints,    "add extra dist points",   pipeline::Optional);
	registerInput(_addExtraFacesPoints,   "add extra faces points",  pipeline::Optional);

	registerOutput(_convexified, "convex meshes");
}

void
ConvexDecomposition::updateOutputs() {

	LOG_DEBUG(convexdecompositionlog) << "starting convex decomposition" << std::endl;

	if (!_convexified)
		_convexified = new Meshes();
	else
		_convexified->clear();

	unsigned int numVertices  = _mesh->getNumVertices();
	unsigned int numTriangles = _mesh->getNumTriangles();

	const std::vector<Point3d>&  meshVertices  = _mesh->getVertices();
	const std::vector<Triangle>& meshTriangles = _mesh->getTriangles();

	// convert mesh to HACD data structure

	HACD::Vec3<HACD::Real>* points    = new HACD::Vec3<HACD::Real>[numVertices];
	HACD::Vec3<long>*       triangles = new HACD::Vec3<long>[numTriangles];

	for (unsigned int i = 0; i < numVertices; i++) {

		points[i].X() = meshVertices[i].x;
		points[i].Y() = meshVertices[i].y;
		points[i].Z() = meshVertices[i].z;
	}

	for (unsigned int i = 0; i < numTriangles; i++) {

		triangles[i].X() = meshTriangles[i].v0;
		triangles[i].Y() = meshTriangles[i].v2;
		triangles[i].Z() = meshTriangles[i].v1;
	}

	// create hacd object

	HACD::HeapManager* heapManager = HACD::createHeapManager(65536*(1000));
	HACD::HACD* const  hacd        = HACD::CreateHACD(heapManager);

	// set mesh data

	hacd->SetPoints(points);
	hacd->SetNPoints(numVertices);
	hacd->SetTriangles(triangles);
	hacd->SetNTriangles(numTriangles);

	// set parameters

	double       compacityWeight       = (_compacityWeight.isSet()       ? *_compacityWeight       : 0.0001);
	double       volumeWeight          = (_volumeWeight.isSet()          ? *_volumeWeight          : 0.0);
	double       connectDistance       = (_connectDistance.isSet()       ? *_connectDistance       : 30);
	unsigned int minNumClusters        = (_minNumClusters.isSet()        ? *_minNumClusters        : 1);
	unsigned int maxNumHullVertices    = (_maxNumHullVertices.isSet()    ? *_maxNumHullVertices    : 100);
	double       maxConcavity          = (_maxConcavity.isSet()          ? *_maxConcavity          : 100);
	double       smallClusterThreshold = (_smallClusterThreshold.isSet() ? *_smallClusterThreshold : 0.25);
	unsigned int numTargetTriangles    = (_numTargetTriangles.isSet()    ? *_numTargetTriangles    : numTriangles);
	bool         addExtraDistPoints    = (_addExtraDistPoints.isSet()    ? *_addExtraDistPoints    : false);
	bool         addExtraFacesPoints   = (_addExtraFacesPoints.isSet()   ? *_addExtraFacesPoints   : false);

	hacd->SetCompacityWeight(compacityWeight);
	hacd->SetVolumeWeight(volumeWeight);
	// if two connected components are seperated by distance < ccConnectDist
	// then create a virtual edge between them so they can be merged during the 
	// simplification process
	hacd->SetConnectDist(connectDistance);
	// minimum number of clusters
	hacd->SetNClusters(minNumClusters);
	// max of 100 vertices per convex-hull
	hacd->SetNVerticesPerCH(maxNumHullVertices);
	// maximum concavity
	hacd->SetConcavity(maxConcavity);
	// threshold to detect small clusters
	hacd->SetSmallClusterThreshold(smallClusterThreshold);
	// # triangles in the decimated mesh
	hacd->SetNTargetTrianglesDecimatedMesh(numTargetTriangles);
	hacd->SetCallBack(&hacdCallback);
	hacd->SetAddExtraDistPoints(addExtraDistPoints);
	hacd->SetAddFacesPoints(addExtraFacesPoints);

	// convexify

	hacd->Compute();

	// read back results

	unsigned int numClusters = hacd->GetNClusters();

	LOG_DEBUG(convexdecompositionlog)
			<< "found " << numClusters << " clusters" << std::endl;

	//unsigned int numDecimatedVertices  = hacd->GetNDecimatedPoints();
	//unsigned int numDecimatedTriangles = hacd->GetNDecimatedTriangles();

	//Mesh decimatedMesh;
	//decimatedMesh.setNumVertices(numDecimatedVertices);
	//decimatedMesh.setNumTriangles(numDecimatedTriangles);

	//const HACD::Vec3<HACD::Real>* decimatedPoints    = hacd->GetDecimatedPoints();
	//const HACD::Vec3<long>*       decimatedTriangles = hacd->GetDecimatedTriangles();

	//for (unsigned int i = 0; i < numDecimatedVertices; i++) {

		//decimatedMesh.setVertex(
				//i,
				//Point3d(decimatedPoints[i].X(), decimatedPoints[i].Y(), decimatedPoints[i].Z()));
	//}

	//for (unsigned int i = 0; i < numDecimatedTriangles; i++) {

		//decimatedMesh.setTriangle(
				//i,
				//decimatedTriangles[i].X(), decimatedTriangles[i].Z(), decimatedTriangles[i].Y());
	//}

	//LOG_DEBUG(convexdecompositionlog) << "created decimated mesh" << std::endl;

	for (unsigned int cluster = 0; cluster < numClusters; cluster++) {

		unsigned int numClusterVertices  = hacd->GetNPointsCH(cluster);
		unsigned int numClusterTriangles = hacd->GetNTrianglesCH(cluster);

		HACD::Vec3<HACD::Real>* clusterPoints    = new HACD::Vec3<HACD::Real>[numClusterVertices];
		HACD::Vec3<long>*       clusterTriangles = new HACD::Vec3<long>[numClusterTriangles];

		hacd->GetCH(cluster, clusterPoints, clusterTriangles);

		boost::shared_ptr<Mesh> clusterMesh = boost::make_shared<Mesh>();
		clusterMesh->setNumVertices(numClusterVertices);
		clusterMesh->setNumTriangles(numClusterTriangles);

		for (unsigned int i = 0; i < numClusterVertices; i++) {

			clusterMesh->setVertex(
					i,
					Point3d(clusterPoints[i].X(), clusterPoints[i].Y(), clusterPoints[i].Z()));
		}

		for (unsigned int i = 0; i < numClusterTriangles; i++) {

			clusterMesh->setTriangle(
					i,
					clusterTriangles[i].X(), clusterTriangles[i].Z(), clusterTriangles[i].Y());
		}

		_convexified->add(cluster, clusterMesh);
	}

	// clean up

	HACD::DestroyHACD(hacd);
	HACD::releaseHeapManager(heapManager);

	delete[] points;
	delete[] triangles;
}
