#include "PolygonRaycast.hpp"


namespace OgreUtility
{
	//
	// RaycastFromPoint
	//
	bool polygonRaycast(Ogre::RaySceneQuery* raySceneQuery, const Ogre::Ray& ray, Ogre::SceneNode*& node_result, Ogre::Vector3& point_result)
	{
		if (!raySceneQuery)
			return false;

		// create a query object
		raySceneQuery->setRay(ray);

		// execute the query, returns a vector of hits
		if (raySceneQuery->execute().size() <= 0)
			// raycast did not hit an objects bounding box
			return false;

		// at this point we have raycast to a series of different objects bounding boxes.
		// we need to test these different objects to see which is the first polygon hit.
		// there are some minor optimizations (distance based) that mean we wont have to
		// check all of the objects most of the time, but the worst case scenario is that
		// we need to test every triangle of every object.
		Ogre::Real closest_distance = -1.0f;
		Ogre::Vector3 closest_result_point;
		Ogre::SceneNode* closest_result_node = nullptr;
		Ogre::RaySceneQueryResult& query_result = raySceneQuery->getLastResults();
		for (size_t qr_idx = 0, size = query_result.size(); qr_idx < size; ++qr_idx)
		{
			// stop checking if we have found a raycast hit that is closer
			// than all remaining entities
			if (closest_distance >= 0.0f && closest_distance < query_result[qr_idx].distance)
				break;

			// only check this result if its a hit against an entity
			if (query_result[qr_idx].movable)
			{
				const std::string& movableType = query_result[qr_idx].movable->getMovableType();
				// mesh data to retrieve
				size_t vertex_count;
				size_t index_count;
				Ogre::Vector3* vertices;
				unsigned long* indices;

				if (movableType == "ManualObject"){
					// get the entity to check
					Ogre::ManualObject* entity = static_cast<Ogre::ManualObject*>(query_result[qr_idx].movable);
					// get the mesh information
					GetMeshInformation(entity, vertex_count, vertices, index_count, indices,
						entity->getParentNode()->_getDerivedPosition(),
						entity->getParentNode()->_getDerivedOrientation(),
						entity->getParentNode()->_getDerivedScale()
						);
				}
				else if (movableType == "Entity"){
					// get the entity to check
					Ogre::Entity* entity = static_cast<Ogre::Entity*>(query_result[qr_idx].movable);
					// get the mesh information
					GetMeshInformation(entity, vertex_count, vertices, index_count, indices,
						entity->getParentNode()->_getDerivedPosition(),
						entity->getParentNode()->_getDerivedOrientation(),
						entity->getParentNode()->_getDerivedScale()
						);
				}
				else{
					continue;
				}

				// test for hitting individual triangles on the mesh
				bool new_closest_found = false;
				for (int i = 0; i < static_cast<int>(index_count); i += 3)
				{
					// check for a hit against this triangle
					std::pair<bool, Ogre::Real> hit = Ogre::Math::intersects(ray, vertices[indices[i]], vertices[indices[i + 1]], vertices[indices[i + 2]], true, false);

					// if it was a hit check if its the closest
					if (hit.first && (closest_distance < 0.0f || hit.second < closest_distance))
					{
						// this is the closest so far, save it off
						closest_distance = hit.second;
						new_closest_found = true;
					}
				}

				// free the verticies and indicies memory
				delete[] vertices;
				delete[] indices;

				// if we found a new closest raycast for this object, update the
				// closest_result before moving on to the next object.
				if (new_closest_found)
				{
					closest_result_point = ray.getPoint(closest_distance);
					closest_result_node = query_result[qr_idx].movable->getParentSceneNode();
				}
			}
		}

		// return the result
		if (closest_distance >= 0.0f){
			// raycast success
			point_result = closest_result_point;
			node_result = closest_result_node;
			return true;
		}
		// raycast failed
		return false;
	}


	//
	// GetMeshInformation (entity)
	//
	void GetMeshInformation(const Ogre::Entity* entity, size_t& vertex_count, Ogre::Vector3*& vertices, size_t& index_count, unsigned long*& indices, const Ogre::Vector3& position, const Ogre::Quaternion& orient, const Ogre::Vector3& scale)
	{
		bool added_shared = false;
		size_t current_offset = 0;
		size_t shared_offset = 0;
		size_t next_offset = 0;
		size_t index_offset = 0;
		vertex_count = index_count = 0;

		Ogre::MeshPtr mesh = entity->getMesh();


		bool useSoftwareBlendingVertices = entity->hasSkeleton();

		if (useSoftwareBlendingVertices)
			const_cast<Ogre::Entity*>(entity)->_updateAnimation();

		// Calculate how many vertices and indices we're going to need
		for (unsigned short i = 0, size = mesh->getNumSubMeshes(); i < size; ++i)
		{
			Ogre::SubMesh* submesh = mesh->getSubMesh(i);

			// We only need to add the shared vertices once
			if (submesh->useSharedVertices){
				if (!added_shared){
					vertex_count += mesh->sharedVertexData->vertexCount;
					added_shared = true;
				}
			}
			else{
				vertex_count += submesh->vertexData->vertexCount;
			}

			// Add the indices
			index_count += submesh->indexData->indexCount;
		}


		// Allocate space for the vertices and indices
		vertices = new Ogre::Vector3[vertex_count];
		indices = new unsigned long[index_count];

		added_shared = false;

		// Run through the submeshes again, adding the data into the arrays
		for (unsigned short i = 0, size = mesh->getNumSubMeshes(); i < size; ++i)
		{
			Ogre::SubMesh* submesh = mesh->getSubMesh(i);

			//----------------------------------------------------------------
			// GET VERTEXDATA
			//----------------------------------------------------------------

			//Ogre::VertexData* vertex_data = submesh->useSharedVertices ? mesh->sharedVertexData : submesh->vertexData;
			Ogre::VertexData* vertex_data;

			//When there is animation:
			if (useSoftwareBlendingVertices)
				vertex_data = submesh->useSharedVertices ? entity->_getSkelAnimVertexData() : entity->getSubEntity(i)->_getSkelAnimVertexData();
			else
				vertex_data = submesh->useSharedVertices ? mesh->sharedVertexData : submesh->vertexData;


			if ((!submesh->useSharedVertices) || (submesh->useSharedVertices && !added_shared))
			{
				if (submesh->useSharedVertices){
					added_shared = true;
					shared_offset = current_offset;
				}

				const Ogre::VertexElement* posElem = vertex_data->vertexDeclaration->findElementBySemantic(Ogre::VES_POSITION);

				Ogre::HardwareVertexBufferSharedPtr vbuf = vertex_data->vertexBufferBinding->getBuffer(posElem->getSource());

				unsigned char* vertex = static_cast<unsigned char*>(vbuf->lock(Ogre::HardwareBuffer::HBL_READ_ONLY));

				// There is _no_ baseVertexPointerToElement() which takes an Ogre::Real or a double
				//  as second argument. So make it float, to avoid trouble when Ogre::Real will
				//  be comiled/typedefed as double:
				//      Ogre::Real* pReal;
				float* pReal = 0;

				for (size_t j = 0; j < vertex_data->vertexCount; ++j, vertex += vbuf->getVertexSize())
				{
					posElem->baseVertexPointerToElement(vertex, &pReal);
					Ogre::Vector3 pt(pReal[0], pReal[1], pReal[2]);
					vertices[current_offset + j] = (orient * (pt * scale)) + position;
				}

				vbuf->unlock();
				next_offset += vertex_data->vertexCount;
			}


			Ogre::IndexData* index_data = submesh->indexData;
			size_t numTris = index_data->indexCount / 3;
			Ogre::HardwareIndexBufferSharedPtr ibuf = index_data->indexBuffer;

			bool use32bitindexes = (ibuf->getType() == Ogre::HardwareIndexBuffer::IT_32BIT);

			unsigned long*  pLong = static_cast<unsigned long*>(ibuf->lock(Ogre::HardwareBuffer::HBL_READ_ONLY));
			unsigned short* pShort = reinterpret_cast<unsigned short*>(pLong);


			size_t offset = (submesh->useSharedVertices) ? shared_offset : current_offset;
			size_t index_start = index_data->indexStart;
			size_t last_index = numTris * 3 + index_start;

			if (use32bitindexes){
				for (size_t k = index_start; k < last_index; ++k)
					indices[index_offset++] = pLong[k] + static_cast<unsigned long>(offset);
			}
			else{
				for (size_t k = index_start; k < last_index; ++k)
					indices[index_offset++] =
					static_cast<unsigned long>(pShort[k]) +
					static_cast<unsigned long>(offset);
			}

			ibuf->unlock();
			current_offset = next_offset;
		}
	}


	//
	// GetMeshInformation (mesh)
	//
	void GetMeshInformation(const Ogre::MeshPtr mesh, size_t &vertex_count, Ogre::Vector3*& vertices, size_t& index_count, unsigned long*& indices, const Ogre::Vector3& position, const Ogre::Quaternion& orient, const Ogre::Vector3& scale)
	{
		bool added_shared = false;
		size_t current_offset = 0;
		size_t shared_offset = 0;
		size_t next_offset = 0;
		size_t index_offset = 0;

		vertex_count = index_count = 0;

		// Calculate how many vertices and indices we're going to need
		for (unsigned short i = 0, size = mesh->getNumSubMeshes(); i < size; ++i)
		{
			Ogre::SubMesh* submesh = mesh->getSubMesh(i);

			// We only need to add the shared vertices once
			if (submesh->useSharedVertices){
				if (!added_shared){
					vertex_count += mesh->sharedVertexData->vertexCount;
					added_shared = true;
				}
			}
			else{
				vertex_count += submesh->vertexData->vertexCount;
			}

			// Add the indices
			index_count += submesh->indexData->indexCount;
		}


		// Allocate space for the vertices and indices
		vertices = new Ogre::Vector3[vertex_count];
		indices = new unsigned long[index_count];

		added_shared = false;

		// Run through the submeshes again, adding the data into the arrays
		for (unsigned short i = 0, size = mesh->getNumSubMeshes(); i < size; ++i)
		{
			Ogre::SubMesh* submesh = mesh->getSubMesh(i);
			Ogre::VertexData* vertex_data = submesh->useSharedVertices ? mesh->sharedVertexData : submesh->vertexData;

			if ((!submesh->useSharedVertices) || (submesh->useSharedVertices && !added_shared))
			{
				if (submesh->useSharedVertices){
					added_shared = true;
					shared_offset = current_offset;
				}

				const Ogre::VertexElement* posElem = vertex_data->vertexDeclaration->findElementBySemantic(Ogre::VES_POSITION);
				Ogre::HardwareVertexBufferSharedPtr vbuf = vertex_data->vertexBufferBinding->getBuffer(posElem->getSource());

				unsigned char* vertex = static_cast<unsigned char*>(vbuf->lock(Ogre::HardwareBuffer::HBL_READ_ONLY));

				// There is _no_ baseVertexPointerToElement() which takes an Ogre::Real or a double
				//  as second argument. So make it float, to avoid trouble when Ogre::Real will
				//  be comiled/typedefed as double:
				//      Ogre::Real* pReal;
				float* pReal;

				for (size_t j = 0; j < vertex_data->vertexCount; ++j, vertex += vbuf->getVertexSize())
				{
					posElem->baseVertexPointerToElement(vertex, &pReal);
					Ogre::Vector3 pt(pReal[0], pReal[1], pReal[2]);
					vertices[current_offset + j] = (orient * (pt * scale)) + position;
				}

				vbuf->unlock();
				next_offset += vertex_data->vertexCount;
			}


			Ogre::IndexData* index_data = submesh->indexData;
			size_t numTris = index_data->indexCount / 3;
			Ogre::HardwareIndexBufferSharedPtr ibuf = index_data->indexBuffer;
			if (ibuf.isNull()) continue; // need to check if index buffer is valid (which will be not if the mesh doesn't have triangles like a pointcloud)

			bool use32bitindexes = (ibuf->getType() == Ogre::HardwareIndexBuffer::IT_32BIT);

			unsigned long*  pLong = static_cast<unsigned long*>(ibuf->lock(Ogre::HardwareBuffer::HBL_READ_ONLY));
			unsigned short* pShort = reinterpret_cast<unsigned short*>(pLong);


			size_t offset = (submesh->useSharedVertices) ? shared_offset : current_offset;
			size_t index_start = index_data->indexStart;
			size_t last_index = numTris * 3 + index_start;

			if (use32bitindexes){
				for (size_t k = index_start; k < last_index; ++k)
				{
					indices[index_offset++] = pLong[k] + static_cast<unsigned long>(offset);
				}

			}
			else{
				for (size_t k = index_start; k < last_index; ++k)
				{
					indices[index_offset++] =
						static_cast<unsigned long>(pShort[k]) +
						static_cast<unsigned long>(offset);
				}
			}

			ibuf->unlock();
			current_offset = next_offset;
		}
	}


	//
	// GetMeshInformation (manual object)
	//
	void GetMeshInformation(const Ogre::ManualObject* manual, size_t& vertex_count, Ogre::Vector3*& vertices, size_t& index_count, unsigned long*& indices, const Ogre::Vector3& position, const Ogre::Quaternion& orient, const Ogre::Vector3& scale)
	{
		std::vector<Ogre::Vector3> returnVertices;
		std::vector<unsigned long> returnIndices;
		unsigned long thisSectionStart = 0;
		for (unsigned int i = 0, size = manual->getNumSections(); i < size; ++i)
		{
			Ogre::ManualObject::ManualObjectSection* section = manual->getSection(i);
			Ogre::RenderOperation* renderOp = section->getRenderOperation();

			std::vector<Ogre::Vector3> pushVertices;
			//Collect the vertices
			{
				const Ogre::VertexElement* vertexElement = renderOp->vertexData->vertexDeclaration->findElementBySemantic(Ogre::VES_POSITION);
				Ogre::HardwareVertexBufferSharedPtr vertexBuffer = renderOp->vertexData->vertexBufferBinding->getBuffer(vertexElement->getSource());

				char* verticesBuffer = static_cast<char*>(vertexBuffer->lock(Ogre::HardwareBuffer::HBL_READ_ONLY));
				float* positionArrayHolder;

				thisSectionStart = returnVertices.size() + pushVertices.size();

				pushVertices.reserve(renderOp->vertexData->vertexCount);

				for (unsigned int j = 0; j < renderOp->vertexData->vertexCount; ++j)
				{
					vertexElement->baseVertexPointerToElement(verticesBuffer + j * vertexBuffer->getVertexSize(), &positionArrayHolder);
					Ogre::Vector3 vertexPos = Ogre::Vector3(positionArrayHolder[0], positionArrayHolder[1], positionArrayHolder[2]);
					vertexPos = (orient * (vertexPos * scale)) + position;
					pushVertices.push_back(vertexPos);
				}

				vertexBuffer->unlock();
			}
			//Collect the indices
		{
			if (renderOp->useIndexes)
			{
				Ogre::HardwareIndexBufferSharedPtr indexBuffer = renderOp->indexData->indexBuffer;

				if (indexBuffer.isNull() || renderOp->operationType != Ogre::RenderOperation::OT_TRIANGLE_LIST)
				{
					//No triangles here, so we just drop the collected vertices and move along to the next section.
					continue;
				}
				else
				{
					returnVertices.reserve(returnVertices.size() + pushVertices.size());
					returnVertices.insert(returnVertices.end(), pushVertices.begin(), pushVertices.end());
				}

				unsigned int* pLong = static_cast<unsigned int*>(indexBuffer->lock(Ogre::HardwareBuffer::HBL_READ_ONLY));
				unsigned short* pShort = reinterpret_cast<unsigned short*>(pLong);

				returnIndices.reserve(returnIndices.size() + renderOp->indexData->indexCount);

				for (size_t j = 0; j < renderOp->indexData->indexCount; ++j)
				{
					unsigned long index;
					//We also have got to remember that for a multi section object, each section has
					//different vertices, so the indices will not be correct. To correct this, we
					//have to add the position of the first vertex in this section to the index

					//(At least I think so...)
					if (indexBuffer->getType() == Ogre::HardwareIndexBuffer::IT_32BIT)
						index = static_cast<unsigned long>(pLong[j]) + thisSectionStart;
					else
						index = static_cast<unsigned long>(pShort[j]) + thisSectionStart;

					returnIndices.push_back(index);
				}

				indexBuffer->unlock();
			}
		}
		}

		//Now we simply return the data.
		index_count = returnIndices.size();
		vertex_count = returnVertices.size();
		vertices = new Ogre::Vector3[vertex_count];
		for (unsigned long i = 0; i < vertex_count; ++i)
			vertices[i] = returnVertices[i];
		indices = new unsigned long[index_count];
		for (unsigned long i = 0; i < index_count; ++i)
			indices[i] = returnIndices[i];

		//All done.
		return;
	}
}