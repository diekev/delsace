//-----------------------------------------------------------------------------
// NVIDIA(R) GVDB VOXELS
// Copyright 2016 NVIDIA Corporation
// SPDX-License-Identifier: Apache-2.0
//
// ObjarReader.cpp by Chris Wyman (September 2nd, 2014)
//
// Version 1.0: Rama Hoetzlein, 5/1/2017
// Version 1.1: Rama Hoetzlein, 3/25/2018
//-----------------------------------------------------------------------------

#ifndef OBJAR_READER
#define OBJAR_READER

#include <GL/glew.h>
#include <stdio.h>
#include <stdlib.h>

#include "gvdb_model.h"
using namespace nvdb;

// Header for the IGLU library's .objar binary object file
//    -> Not real important for the purposes of this sample (other than we have to read this to
//    read the model)
typedef struct {
    unsigned int vboVersionID;  // Magic number / file version header
    unsigned int numVerts;      // Number of vertices in the VBO
    unsigned int numElems;      // Number of elements (i.e., indices in the VBO)
    unsigned int elemType;      // E.g. GL_TRIANGLES
    unsigned int vertStride;    // Number of bytes between subsequent vertices
    unsigned int vertBitfield;  // Binary bitfield describing valid vertex components
    unsigned int matlOffset;    // In vertex, offset to material ID
    unsigned int objOffset;     // In vertex, offset to object ID
    unsigned int vertOffset;    // In vertex, offset to vertex
    unsigned int normOffset;    // In vertex, offset to normal
    unsigned int texOffset;     // In vertex, offset to texture coordinate
    char matlFileName[84];      // Filename containing material information
    float bboxMin[3];           // Minimum point on an axis-aligned bounding box
    float bboxMax[3];           // Maximum point on an axis-aligned bounding box
    char pad[104];              // padding to be used later!
} OBJARHeader;

class OBJARReader {
  public:
    int LoadHeader(FILE *fp, OBJARHeader *hdr);
    bool LoadFile(Model *model, char *filename, std::vector<std::string> &paths);
    static bool isMyFile(const char *filename);

    friend Model;
};
#endif
