/*
 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifndef _PATATE_COMMON_GL_UTILS_GLMESH_H
#include "glmesh.h"
#endif


#include <iterator>     // std::iterator, std::input_iterator_tag
#include <numeric>      // std::accumulate



namespace PatateCommon
{


template<typename VertexContainer>
struct GLTri3DMesh::_GrenailleIterator : public std::iterator<std::input_iterator_tag, int>
{
public:
    typedef std::iterator<std::input_iterator_tag, int> Base;
    // override default iterator members
    typedef GLTri3DMesh::GrenaillePoint         value_type;
    typedef Base::difference_type  difference_type;
    typedef GLTri3DMesh::GrenaillePoint*        pointer;
    typedef GLTri3DMesh::GrenaillePoint&        reference;

private:
    typename VertexContainer::iterator _vit, _vend;
    typename VertexContainer::iterator _nit, _nend;

public:

    _GrenailleIterator(VertexContainer& vertices, VertexContainer& normals)
        : _vit(vertices.begin()), _vend(vertices.end()),
          _nit(normals.begin()),  _nend(normals.end()){}

    _GrenailleIterator(typename VertexContainer::iterator end)
        : _vit(end), _vend(end),
          _nit(end),  _nend(end){}

    _GrenailleIterator& operator++() {
        for(int i = 0; i != Dim; ++i) {
            assert(_vit != _vend); ++_vit;
            assert(_nit != _nend); ++_nit;
        }
    return *this;
    }
    //Iterator operator++(int) {Iterator tmp(*this); operator++(); return tmp;}
    bool operator==(const _GrenailleIterator& rhs) {return _vit==rhs._vit;}
    bool operator!=(const _GrenailleIterator& rhs) {return _vit!=rhs._vit;}
    value_type operator*() {return value_type(&(*_vit), &(*_nit));}
};


template<typename VertexContainer>
struct GLTri3DMesh::_VectorIterator : public std::iterator<std::input_iterator_tag, int>
{
public:
    typedef std::iterator<std::input_iterator_tag, int> Base;
    // override default iterator members
    typedef Eigen::Map<GLTri3DMesh::Vector> value_type;
    typedef Base::difference_type  difference_type;
    typedef value_type*   pointer;
    typedef Eigen::Map<GLTri3DMesh::Vector>        reference;
    typedef Eigen::Map<const GLTri3DMesh::Vector>  const_reference;

private:
    typename VertexContainer::iterator _it, _end;

public:

    _VectorIterator(VertexContainer& data)
        : _it(data.begin()), _end(data.end()){}

    _VectorIterator(typename VertexContainer::iterator end)
        : _it(end), _end(end){}

    _VectorIterator& operator++() {
        for(int i = 0; i != Dim; ++i) {
            assert(_it != _end);
            ++_it;
        }
        return *this;
    }

    bool operator==(const _VectorIterator& rhs) {return _it==rhs._it;}
    bool operator!=(const _VectorIterator& rhs) {return _it!=rhs._it;}
    const_reference operator*() const {return const_reference(&(*_it));}
    reference operator*() {return reference(&(*_it));}
};


template<typename VertexContainer>
struct GLTri3DMesh::_FaceIterator : public std::iterator<std::input_iterator_tag, int>
{
public:
    struct VertList {
    private:
        GLTri3DMesh::GrenaillePoint data [3];

    public:
        inline VertList(GLTri3DMesh::GrenaillePoint v0,
                        GLTri3DMesh::GrenaillePoint v1,
                        GLTri3DMesh::GrenaillePoint v2)
            { data[0] = v0; data[1] = v1; data[2] = v2;}

        inline GLTri3DMesh::GrenaillePoint& operator[](int i) { return data[i];}
        inline const GLTri3DMesh::GrenaillePoint& operator[](int i) const { return data[i];}
    };

    typedef std::iterator<std::input_iterator_tag, int> Base;
    // override default iterator members
    typedef VertList               value_type;
    typedef Base::difference_type  difference_type;
    typedef VertList*              pointer;
    typedef VertList&              reference;

private:
    typename FContainer::iterator _it, _end;
    GLTri3DMesh &_mesh;


public:

    _FaceIterator(GLTri3DMesh &mesh, bool end)
        : _it(end ? mesh._faces.end() : mesh._faces.begin()),
          _end(mesh._faces.end()),
          _mesh(mesh){}

    _FaceIterator& operator++() {
        for(int i = 0; i != 3; ++i) {
            assert(_it != _end); ++_it;
        }
    return *this;
    }
    //Iterator operator++(int) {Iterator tmp(*this); operator++(); return tmp;}
    bool operator==(const _FaceIterator& rhs) {return _it==rhs._it;}
    bool operator!=(const _FaceIterator& rhs) {return _it!=rhs._it;}
    value_type operator*() { return value_type(
                    _mesh.getGrenaillePoint( *(_it) ),
                    _mesh.getGrenaillePoint( *(_it+1) ),
                    _mesh.getGrenaillePoint( *(_it+2) ));}
};


GLTri3DMesh::grenailleIterator
GLTri3DMesh::begin() { return grenailleIterator(_vertices, _normals); }

GLTri3DMesh::grenailleIterator
GLTri3DMesh::end()   { return grenailleIterator(_vertices.end()); }

GLTri3DMesh::posIterator
GLTri3DMesh::vertexBegin() { return posIterator(_vertices); }

GLTri3DMesh::posIterator
GLTri3DMesh::vertexEnd()   { return posIterator(_vertices.end()); }

GLTri3DMesh::normalIterator
GLTri3DMesh::normalBegin() { return normalIterator(_normals); }

GLTri3DMesh::normalIterator
GLTri3DMesh::normalEnd()   { return normalIterator(_normals.end()); }

GLTri3DMesh::faceIterator
GLTri3DMesh::faceBegin() { return faceIterator(*this, false); }

GLTri3DMesh::faceIterator
GLTri3DMesh::faceEnd()   { return faceIterator(*this, true); }


GLTri3DMesh::GLTri3DMesh()
    : _init(false),
      _pickingInit(false),
      _vboFaceArray(0),
      _vboVertArray(0){

}

void GLTri3DMesh::setFrom(const GLTri3DMesh &input){
    _init = false;
    _faces    = input._faces;
    _vertices = input._vertices;
}

void GLTri3DMesh::addFace(const std::vector<Vertex> &vertices){
    assert(vertices.size() == 3);

    _init = false;

    int off = _faces.size();
    _faces.resize(off + 3);

    for(int i = 0; i < 3; ++i) _faces[off+i] = vertices[i].idx();
}


void GLTri3DMesh::addFace(Vertex v0, Vertex v1, Vertex v2){

    _init = false;
    _faces.reserve(_faces.size() + 3);
    _faces.push_back(v0.idx());
    _faces.push_back(v1.idx());
    _faces.push_back(v2.idx());
}

void GLTri3DMesh::addFace(int v0, int v1, int v2){

    _init = false;
    _faces.reserve(_faces.size() + 3);
    _faces.push_back(v0);
    _faces.push_back(v1);
    _faces.push_back(v2);
}

void GLTri3DMesh::addVertex(const Vector &v){
    _init = false;
    std::copy (v.data(),v.data()+Dim,back_inserter(_vertices));
}

unsigned int GLTri3DMesh::nVertices() const{
    return _vertices.size() / 3;
}



void GLTri3DMesh::initVBO(bool initForPicking){
    if (! _init){
        if(_vertices.empty() || _faces.empty()){
            _init = true;
            return;
        }
        computeNormals();

        glGenVertexArrays(1, &_vao);

        // set default render mode
        glBindVertexArray(_vao);

        // vertices
        glGenBuffers(1, &_vboVertArray);
        glBindBuffer(GL_ARRAY_BUFFER, _vboVertArray);
        glBufferData(GL_ARRAY_BUFFER, sizeof(Scalar)*_vertices.size(), &(_vertices.front()), GL_STATIC_DRAW);
        glVertexAttribPointer( 0, Dim, GL_FLOAT, GL_FALSE, 0, (GLvoid*)(0) );
        glEnableVertexAttribArray( 0 );

        // normals
        glGenBuffers(1, &_vboNormalArray);
        glBindBuffer(GL_ARRAY_BUFFER, _vboNormalArray);
        glBufferData(GL_ARRAY_BUFFER, sizeof(Scalar)*_normals.size(), &(_normals.front()), GL_STATIC_DRAW);
        glVertexAttribPointer( 1, Dim, GL_FLOAT, GL_FALSE, 0, (GLvoid*)(0) );
        glEnableVertexAttribArray( 1 );

        if (initForPicking) {
            std::vector<GLuint> vids; vids.resize(3*nVertices());
            {
                int i = 0;
                for(std::vector<GLuint>::iterator it = vids.begin();
                    it != vids.end(); ++it, ++i) (*it) = i;
            }
            glGenBuffers(1, &_vboIdsArray);
            glBindBuffer(GL_ARRAY_BUFFER, _vboIdsArray);
            glBufferData(GL_ARRAY_BUFFER, sizeof(GLuint)*vids.size(), &(vids.front()), GL_STATIC_DRAW);
            glVertexAttribPointer( 2, 1, GL_UNSIGNED_INT, GL_FALSE, 0, (GLvoid*)(0) );
            glEnableVertexAttribArray( 2 );

            _pickingInit = true;
        }

        // indices
        glGenBuffers(1, &_vboFaceArray);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _vboFaceArray);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint)*_faces.size(), &(_faces.front()), GL_STATIC_DRAW);

        glBindVertexArray(0);
        _init = true;
    }
}

void
GLTri3DMesh::clearVBO(){

    glDeleteVertexArrays (1, &_vao);
    glDeleteBuffers (1, &_vboVertArray);
    glDeleteBuffers (1, &_vboNormalArray);
    if (_pickingInit) {
        glDeleteBuffers (1, &_vboIdsArray);
    }
    glDeleteBuffers (1, &_vboFaceArray);

//    glBindVertexArray(_vao);
//    glBindBuffer(GL_ARRAY_BUFFER, _vboVertArray);
//    glBufferData(GL_ARRAY_BUFFER, 0, 0, GL_STATIC_DRAW);
//    glBindBuffer(GL_ARRAY_BUFFER, _vboNormalArray);
//    glBufferData(GL_ARRAY_BUFFER, 0, 0, GL_STATIC_DRAW);

//    if (_pickingInit) {
//        glBindBuffer(GL_ARRAY_BUFFER, _vboIdsArray);
//        glBufferData(GL_ARRAY_BUFFER, 0, 0, GL_STATIC_DRAW);
//    }

//    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _vboFaceArray);
//    glBufferData(GL_ELEMENT_ARRAY_BUFFER, 0, 0, GL_STATIC_DRAW);

//    glBindVertexArray(0);
//    glDeleteVertexArrays (1, _vao);
    _init = false;
}

#include <time.h>

void GLTri3DMesh::translateToCentroid(){
    _init = false;

//#define COMPARE_TIMING
#ifdef COMPARE_TIMING


    clock_t start_t, end_t;
    Vector center;

    static const int N = 10000;

    start_t = clock();
    for (int i  = 0; i!= N; ++i)
    center = std::accumulate(vertexBegin(), vertexEnd(),
                             Vector::Zero().eval()) / Scalar(nVertices());
    end_t = clock();
    std::cout << "accumulate: "
              <<  (double)(end_t - start_t) / CLOCKS_PER_SEC
              << " --- " << center.transpose()
              << std::endl;


    start_t = clock();
    for (int i  = 0; i!= N; ++i)
    center =
    Eigen::Map<const Eigen::Matrix <Scalar, Dim, Eigen::Dynamic> >
            (_vertices.data(), Dim, nVertices())
            .rowwise().sum() / Scalar(nVertices()) ;
    end_t = clock();
    std::cout << "eigen sum: "
              <<  (double)(end_t - start_t) / CLOCKS_PER_SEC
               << " --- " << center.transpose()
              << std::endl;

#else
    Vector center = std::accumulate(vertexBegin(), vertexEnd(),
                                    Vector::Zero().eval()) / Scalar(nVertices());

#endif

    for (posIterator it = vertexBegin(); it != vertexEnd(); ++it)
       (*it) -= center;

}

void GLTri3DMesh::draw(){
    if (! _faces.empty()) {
        if( ! _init ) initVBO();

        glBindVertexArray(_vao);
        glDrawElements(GL_TRIANGLES, _faces.size(), GL_UNSIGNED_INT, NULL);
        glBindVertexArray(0);

    }
}

void GLTri3DMesh::drawIds(){
    if (! _faces.empty()) {
        if( ! _init ) initVBO();
        if( ! _pickingInit ){
            std::cerr << "Picking not initialized. Call initVBO(true) first"
                      << std::endl;
        }
        draw();
    }
}

Eigen::Map<GLTri3DMesh::Vector>
GLTri3DMesh::getVertexMap(int id){
    return Eigen::Map<GLTri3DMesh::Vector>(&(_vertices.at(Dim*id)));
}

Eigen::Map<GLTri3DMesh::Vector>
GLTri3DMesh::getNormalVectorMap(int id){
    return Eigen::Map<GLTri3DMesh::Vector>(&(_normals.at(Dim*id)));
}

GLTri3DMesh::GrenaillePoint
GLTri3DMesh::getGrenaillePoint(int id){
    return GrenaillePoint(id, *this);
}

void GLTri3DMesh::computeNormals(){
    _normals.clear();
    _normals.resize(_vertices.size(), Scalar(0));

    typedef Eigen::Map< Vector > VMap;

    // naive computation: just sum up all the faces contributions per vertex
    for(unsigned int i = 0; i < _faces.size(); i += 3){

        VMap v0 (&(_vertices[3*_faces[i  ]]));
        VMap v1 (&(_vertices[3*_faces[i+1]]));
        VMap v2 (&(_vertices[3*_faces[i+2]]));

        Vector n = -(v1 - v0).cross((v2 - v0)).normalized();

        VMap (&(_normals[3*_faces[i  ]])) += n;
        VMap (&(_normals[3*_faces[i+1]])) += n;
        VMap (&(_normals[3*_faces[i+2]])) += n;
    }

    for(unsigned int i = 0; i < _normals.size(); i += 3){
        VMap (&(_normals[i])).normalize();
    }
}

} // namespace PatateCommon
