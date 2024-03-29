//-----------------------------------------------------------------------------
// NVIDIA(R) GVDB VOXELS
// Copyright 2016 NVIDIA Corporation
// SPDX-License-Identifier: Apache-2.0
//
// Defines a reader for OBJ model files and associated materials
// Originally by Chris Wyman (October 20th, 2011)
//
// Version 1.0: Rama Hoetzlein, 5/1/2017
// Version 1.1: Rama Hoetzlein, 3/25/2018
//-----------------------------------------------------------------------------

#ifndef _OBJ_READER_H
#define _OBJ_READER_H

#include "gvdb_model.h"
#include "loader_Parser.h"

using namespace nvdb;

#pragma warning(disable : 4996)

struct OBJTri;

class OBJReader : public Parser {
  public:
    // Constructor reads from the file.
    OBJReader();
    virtual ~OBJReader();

    bool LoadFile(Model *model, const char *filename, std::vector<std::string> &paths);
    bool Cleanup();
    static bool isMyFile(const char *filename);

    friend Model;

  private:
    // Indicies in OBJ files may be relative (i.e., negative).  OBJ files also
    //    use a Pascal array indexing scheme (i.e., start from 1).  These methods
    //    fixes these issues to give an array index we can actually use.
    unsigned int GetVertexIndex(int relativeIdx);
    unsigned int GetNormalIndex(int relativeIdx);
    // unsigned int GetTextureIndex( int relativeIdx );

    // These methods make sure the vertex array and element array buffers are set up correctly
    void GetCompactArrayBuffer(Model *model);

    // Read appropriate facet tokens
    void Read_V_Token(OBJTri *tri, int idx, char *token = 0);
    void Read_VT_Token(OBJTri *tri, int idx, char *token = 0);
    void Read_VN_Token(OBJTri *tri, int idx, char *token = 0);
    void Read_VTN_Token(OBJTri *tri, int idx, char *token = 0);

    // Helper function for adding data to a float array for our GPU-packed array
    void AddDataToArray(float *arr, int startIdx, Vector4DF *vert, Vector4DF *norm);

    // Helper for centering & resizing the geometry in the array before sending it to OpenGL
    void CenterAndResize(float *arr, int numVerts);

    // This declares an ugly type FnParserPtr that points to one of the Read_???_Token methods
    typedef void (OBJReader::*FnParserPtr)(OBJTri *, int, char *);

    // Copy from prior triangle facets when triangulating
    void CopyForTriangleFan(OBJTri *newTri);

    // A method that is called after finding an 'f' line that identifies
    //    the appropriate Read_???_Token() method to call when parsing
    void SelectReadMethod(FnParserPtr *pPtr);

    // Checks that an f line has enough vertices to be parsed as a triangle.
    // For example, it returns false if the f line has only two entries on it
    bool IsValidFLine(FnParserPtr *pPtr);

    // Simple drop in for expandable arrays.  Sort of dumb.
    void AddVertex(const Vector4DF &vert);
    void AddNormal(const Vector4DF &norm);
    void AddTriangle(OBJTri *tri);

  protected:
    // Basic geometric definitions read from the file
    unsigned int m_numVertices, m_numNormals, m_numTris;
    unsigned int m_allocVert, m_allocNorm, m_allocTri;
    OBJTri **m_triangles;
    Vector4DF *m_normals;
    Vector4DF *m_vertices;

    // Does the user want us to resize and center the object around the origin?
    //    Some/many models use completely arbitrary coordinates, so it's difficult
    //    to know how big and where they'll be relative to each other
    bool m_resize, m_center;

    // Information about the OBJ file we read
    bool m_hasVertices, m_hasNormals, m_hasTexCoords, m_guessNorms;

    // What is the stride of the data?
    int m_vertStride, m_vertOff, m_normOff;
};

#endif
