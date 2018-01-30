#pragma once

#define MemoryDeviceContextEdgeLength 512

#define ExteriorDetectionThreshold 6

#define InteriorOcclusionCullingDepth ((unsigned char)3)
#define InteriorOcclusionCullingStep 2.5f
#define ExteriorOcclusionCullingDepth ((unsigned char)1)
#define ExteriorOcclusionCullingStep 40.0f
#define DisplayListIdForOcclusionCulling 1
#define OcclusionCullingScreenSize 512

#define BigObjectThreshold 5.0
#define MediumObjectThreshold 2.0
#define BoneObjectThreshold 5.0

#define SpatialIndexingDepth ((unsigned char)3)
#define LeafSpatialOctreeSize 24.0

#define MinLegoSize 3.0

#define VboVertexMaxCount 65532

#define MaxUnsignedLong 65532

#define TriangleSizeLevels 4
#define TriangleSizeThresholds {1.0, 0.5, 0.1, 0.05}

// attribute key names
#define ObjectGuid L"objectGuid"
#define TextureName L"textureName"
