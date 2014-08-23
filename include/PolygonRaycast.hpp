#ifndef OGRE_UTILITY_POLYGON_RAYCAST_HPP
#define OGRE_UTILITY_POLYGON_RAYCAST_HPP

#include "Ogre.h"

namespace OgreUtility
{
	// raycast from a point in to the scene.
	// returns success or failure.
	// on success the point is returned in the result.
	bool polygonRaycast(Ogre::RaySceneQuery* raySceneQuery, const Ogre::Ray& ray, Ogre::SceneNode*& node_result, Ogre::Vector3& point_result);

	// Get the mesh information for the given entity.
	void GetMeshInformation(const Ogre::Entity* entity, size_t& vertex_count, Ogre::Vector3*& vertices, size_t& index_count, unsigned long*& indices, const Ogre::Vector3& position, const Ogre::Quaternion& orient, const Ogre::Vector3& scale);
	
	// Get the mesh information for the given mesh.
	void GetMeshInformation(const Ogre::MeshPtr mesh, size_t &vertex_count, Ogre::Vector3*& vertices, size_t& index_count, unsigned long*& indices, const Ogre::Vector3& position, const Ogre::Quaternion& orient, const Ogre::Vector3& scale);

	// Get the mesh information for the given manual object.
	void GetMeshInformation(const Ogre::ManualObject* manual, size_t& vertex_count, Ogre::Vector3*& vertices, size_t& index_count, unsigned long*& indices, const Ogre::Vector3& position, const Ogre::Quaternion& orient, const Ogre::Vector3& scale);
}

#endif//OGRE_UTILITY_POLYGON_RAYCAST_HPP