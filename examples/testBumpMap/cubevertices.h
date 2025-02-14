/* -*-c++-*-
 * testBumpMap - cubevertices (.h & .cpp) - Using 'The MIT License'
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef __CUBE_VERTICES_H__
#define __CUBE_VERTICES_H__

#include <osg/ref_ptr>
#include <osg/Vec2>
#include <osg/Vec3>

/*
   Cube Vertices

   DESC: This data structure encapsulates the data required to generate
            tangent space on the GPU for one cube

   NOTE: Each vertex has a normal, 3 verts, and 3 texture coordinates
            where the two additional verts and tex coords are from the
            opposite and adjacent vertices and the standard triangle.

         As a side note, one may notice this data to be very repetitive.
            And although I could have done this by offsetting indices and
            modding them to wrap back around, I decided to list them by hand
            for the sake of the demo.  This will show better what is happening
            to the user. In practice I recommend just offsetting the indices of
            the original vert and tex coord list.


*/

typedef struct _CV_
{

public:

   _CV_();

    osg::ref_ptr<osg::Vec3Array> mVerts;
    osg::ref_ptr<osg::Vec3Array> mVerts2;
    osg::ref_ptr<osg::Vec3Array> mVerts3;

    osg::ref_ptr<osg::Vec3Array> mNormals;

    osg::ref_ptr<osg::Vec2Array> mTexCoords;
    osg::ref_ptr<osg::Vec2Array> mTexCoords2;
    osg::ref_ptr<osg::Vec2Array> mTexCoords3;

    float mTexRepeat;
    float mSize;

} CubeVertices;

//////////////////////////////////////////////////////////////////////////
//Control Variables
//////////////////////////////////////////////////////////////////////////

_CV_::_CV_()
{

   mTexRepeat = 1.0f;
   mSize = 1000.0f;

   mVerts = new osg::Vec3Array(36);
   mVerts2 = new osg::Vec3Array(36);
   mVerts3 = new osg::Vec3Array(36);
   mNormals = new osg::Vec3Array(36);
   mTexCoords = new osg::Vec2Array(36);
   mTexCoords2 = new osg::Vec2Array(36);
   mTexCoords3 = new osg::Vec2Array(36);

//////////////////////////////////////////////////////////////////////////
//Verts 1
//////////////////////////////////////////////////////////////////////////


//bottom
   (*mVerts)[0].set(mSize, mSize, -mSize);
   (*mVerts)[1].set(-mSize, mSize, -mSize);
   (*mVerts)[2].set(-mSize, -mSize, -mSize);

   (*mVerts)[3].set (mSize, -mSize, -mSize);
   (*mVerts)[4].set (mSize, mSize, -mSize);
   (*mVerts)[5].set (-mSize, -mSize, -mSize);

//top
   (*mVerts)[6].set (-mSize, mSize, mSize);
   (*mVerts)[7].set (mSize, mSize, mSize);
   (*mVerts)[8].set (-mSize, -mSize, mSize);

   (*mVerts)[9].set (mSize, mSize, mSize);
   (*mVerts)[10].set (mSize, -mSize, mSize);
   (*mVerts)[11].set (-mSize, -mSize, mSize);

//left
   (*mVerts)[12].set (-mSize, mSize, -mSize);
   (*mVerts)[13].set (-mSize, mSize, mSize);
   (*mVerts)[14].set (-mSize, -mSize, mSize);

   (*mVerts)[15].set (-mSize, -mSize, -mSize);
   (*mVerts)[16].set (-mSize, mSize, -mSize);
   (*mVerts)[17].set (-mSize, -mSize, mSize);

//right
   (*mVerts)[18].set (mSize, mSize, mSize);
   (*mVerts)[19].set (mSize, mSize, -mSize);
   (*mVerts)[20].set (mSize, -mSize, -mSize);

   (*mVerts)[21].set (mSize, -mSize, mSize);
   (*mVerts)[22].set (mSize, mSize, mSize);
   (*mVerts)[23].set (mSize, -mSize, -mSize);

//front
   (*mVerts)[24].set (mSize, mSize, mSize);
   (*mVerts)[25].set (-mSize, mSize, mSize);
   (*mVerts)[26].set (-mSize, mSize, -mSize);

   (*mVerts)[27].set (mSize, mSize, -mSize);
   (*mVerts)[28].set (mSize, mSize, mSize);
   (*mVerts)[29].set (-mSize, mSize, -mSize);

//back
   (*mVerts)[30].set (mSize, -mSize, -mSize);
   (*mVerts)[31].set (-mSize, -mSize, -mSize);
   (*mVerts)[32].set (-mSize, -mSize, mSize);

   (*mVerts)[33].set (mSize, -mSize, mSize);
   (*mVerts)[34].set (mSize, -mSize, -mSize);
   (*mVerts)[35].set (-mSize, -mSize, mSize);

//////////////////////////////////////////////////////////////////////////
//Verts 2
//////////////////////////////////////////////////////////////////////////

//bottom
   (*mVerts2)[0].set (-mSize, mSize, -mSize);
   (*mVerts2)[1].set (-mSize, -mSize, -mSize);
   (*mVerts2)[2].set (mSize, mSize, -mSize);

   (*mVerts2)[3].set (mSize, mSize, -mSize);
   (*mVerts2)[4].set (-mSize, -mSize, -mSize);
   (*mVerts2)[5].set (mSize, -mSize, -mSize);

//top
   (*mVerts2)[6].set (mSize, mSize, mSize);
   (*mVerts2)[7].set (-mSize, -mSize, mSize);
   (*mVerts2)[8].set (-mSize, mSize, mSize);

   (*mVerts2)[9].set (mSize, -mSize, mSize);
   (*mVerts2)[10].set (-mSize, -mSize, mSize);
   (*mVerts2)[11].set (mSize, mSize, mSize);

//left
   (*mVerts2)[12].set (-mSize, mSize, mSize);
   (*mVerts2)[13].set (-mSize, -mSize, mSize);
   (*mVerts2)[14].set (-mSize, mSize, -mSize);

   (*mVerts2)[15].set (-mSize, mSize, -mSize);
   (*mVerts2)[16].set (-mSize, -mSize, mSize);
   (*mVerts2)[17].set (-mSize, -mSize, -mSize);


//right
   (*mVerts2)[18].set (mSize, mSize, -mSize);
   (*mVerts2)[19].set (mSize, -mSize, -mSize);
   (*mVerts2)[20].set (mSize, mSize, mSize);

   (*mVerts2)[21].set (mSize, mSize, mSize);
   (*mVerts2)[22].set (mSize, -mSize, -mSize);
   (*mVerts2)[23].set (mSize, -mSize, mSize);

//front
   (*mVerts2)[24].set (-mSize, mSize, mSize);
   (*mVerts2)[25].set (-mSize, mSize, -mSize);
   (*mVerts2)[26].set (mSize, mSize, mSize);

   (*mVerts2)[27].set (mSize, mSize, mSize);
   (*mVerts2)[28].set (-mSize, mSize, -mSize);
   (*mVerts2)[29].set (mSize, mSize, -mSize);

//back
   (*mVerts2)[30].set (-mSize, -mSize, -mSize);
   (*mVerts2)[31].set (-mSize, -mSize, mSize);
   (*mVerts2)[32].set (mSize, -mSize, -mSize);

   (*mVerts2)[33].set (mSize, -mSize, -mSize);
   (*mVerts2)[34].set (-mSize, -mSize, mSize);
   (*mVerts2)[35].set (mSize, -mSize, mSize);

//////////////////////////////////////////////////////////////////////////
//Verts 3
//////////////////////////////////////////////////////////////////////////

//bottom
   (*mVerts3)[0].set (-mSize, -mSize, -mSize);
   (*mVerts3)[1].set (mSize, mSize, -mSize);
   (*mVerts3)[2].set (-mSize, mSize, -mSize);

   (*mVerts3)[3].set (-mSize, -mSize, -mSize);
   (*mVerts3)[4].set (mSize, -mSize, -mSize);
   (*mVerts3)[5].set (mSize, mSize, -mSize);

//top
   (*mVerts3)[6].set (-mSize, -mSize, mSize);
   (*mVerts3)[7].set (-mSize, mSize, mSize);
   (*mVerts3)[8].set (mSize, mSize, mSize);

   (*mVerts3)[9].set (-mSize, -mSize, mSize);
   (*mVerts3)[10].set (mSize, mSize, mSize);
   (*mVerts3)[11].set (mSize, -mSize, mSize);

//left
   (*mVerts3)[12].set (-mSize, -mSize, mSize);
   (*mVerts3)[13].set (-mSize, mSize, -mSize);
   (*mVerts3)[14].set (-mSize, mSize, mSize);

   (*mVerts3)[15].set (-mSize, -mSize, mSize   );
   (*mVerts3)[16].set (-mSize, -mSize, -mSize);
   (*mVerts3)[17].set (-mSize, mSize, -mSize);

//right
   (*mVerts3)[18].set (mSize, -mSize, -mSize);
   (*mVerts3)[19].set (mSize, mSize, mSize);
   (*mVerts3)[20].set (mSize, mSize, -mSize);

   (*mVerts3)[21].set (mSize, -mSize, -mSize);
   (*mVerts3)[22].set (mSize, -mSize, mSize);
   (*mVerts3)[23].set (mSize, mSize, mSize);

//front
   (*mVerts3)[24].set (-mSize, mSize, -mSize);
   (*mVerts3)[25].set (mSize, mSize, mSize);
   (*mVerts3)[26].set (-mSize, mSize, mSize);

   (*mVerts3)[27].set (-mSize, mSize, -mSize);
   (*mVerts3)[28].set (mSize, mSize, -mSize );
   (*mVerts3)[29].set (mSize, mSize, mSize);

//back
   (*mVerts3)[30].set (-mSize, -mSize, mSize);
   (*mVerts3)[31].set (mSize, -mSize, -mSize);
   (*mVerts3)[32].set (-mSize, -mSize, -mSize);

   (*mVerts3)[33].set (-mSize, -mSize, mSize);
   (*mVerts3)[34].set (mSize, -mSize, mSize);
   (*mVerts3)[35].set (mSize, -mSize, -mSize);

//////////////////////////////////////////////////////////////////////////
//Normals
//////////////////////////////////////////////////////////////////////////

//bottom
   (*mNormals)[0].set (0.0f, 0.0f, 1.0f);
   (*mNormals)[1].set (0.0f, 0.0f, 1.0f);
   (*mNormals)[2].set (0.0f, 0.0f, 1.0f);

   (*mNormals)[3].set (0.0f, 0.0f, 1.0f);
   (*mNormals)[4].set (0.0f, 0.0f, 1.0f);
   (*mNormals)[5].set (0.0f, 0.0f, 1.0f);

//top
   (*mNormals)[6].set (0.0f, 0.0f, -1.0f);
   (*mNormals)[7].set (0.0f, 0.0f, -1.0f);
   (*mNormals)[8].set (0.0f, 0.0f, -1.0f);

   (*mNormals)[9].set (0.0f, 0.0f, -1.0f);
   (*mNormals)[10].set (0.0f, 0.0f, -1.0f);
   (*mNormals)[11].set (0.0f, 0.0f, -1.0f);

//left
   (*mNormals)[12].set (1.0f, 0.0f, 0.0f);
   (*mNormals)[13].set (1.0f, 0.0f, 0.0f);
   (*mNormals)[14].set (1.0f, 0.0f, 0.0f);

   (*mNormals)[15].set (1.0f, 0.0f, 0.0f);
   (*mNormals)[16].set (1.0f, 0.0f, 0.0f);
   (*mNormals)[17].set (1.0f, 0.0f, 0.0f);

//right
   (*mNormals)[18].set (-1.0f, 0.0f, 0.0f);
   (*mNormals)[19].set (-1.0f, 0.0f, 0.0f);
   (*mNormals)[20].set (-1.0f, 0.0f, 0.0f);

   (*mNormals)[21].set (-1.0f, 0.0f, 0.0f);
   (*mNormals)[22].set (-1.0f, 0.0f, 0.0f);
   (*mNormals)[23].set (-1.0f, 0.0f, 0.0f);

//front
   (*mNormals)[24].set (0.0f, -1.0f, 0.0f);
   (*mNormals)[25].set (0.0f, -1.0f, 0.0f);
   (*mNormals)[26].set (0.0f, -1.0f, 0.0f);

   (*mNormals)[27].set (0.0f, -1.0f, 0.0f);
   (*mNormals)[28].set (0.0f, -1.0f, 0.0f);
   (*mNormals)[29].set (0.0f, -1.0f, 0.0f);

//back
   (*mNormals)[30].set (0.0f, 1.0f, 0.0f);
   (*mNormals)[31].set (0.0f, 1.0f, 0.0f);
   (*mNormals)[32].set (0.0f, 1.0f, 0.0f);

   (*mNormals)[33].set (0.0f, 1.0f, 0.0f);
   (*mNormals)[34].set (0.0f, 1.0f, 0.0f);
   (*mNormals)[35].set (0.0f, 1.0f, 0.0f);

//////////////////////////////////////////////////////////////////////////
// Tex Coords1
//////////////////////////////////////////////////////////////////////////

//bottom
   (*mTexCoords)[0].set (mTexRepeat, mTexRepeat);
   (*mTexCoords)[1].set (-mTexRepeat, mTexRepeat);
   (*mTexCoords)[2].set (-mTexRepeat, -mTexRepeat);

   (*mTexCoords)[3].set (mTexRepeat, -mTexRepeat);
   (*mTexCoords)[4].set (mTexRepeat, mTexRepeat);
   (*mTexCoords)[5].set (-mTexRepeat, -mTexRepeat);

//top

   (*mTexCoords)[6].set (-mTexRepeat, -mTexRepeat);
   (*mTexCoords)[7].set (mTexRepeat, -mTexRepeat);
   (*mTexCoords)[8].set (-mTexRepeat, mTexRepeat);

   (*mTexCoords)[9].set (mTexRepeat, -mTexRepeat);
   (*mTexCoords)[10].set (mTexRepeat, mTexRepeat);
   (*mTexCoords)[11].set (-mTexRepeat, mTexRepeat);

//left
   (*mTexCoords)[12].set (mTexRepeat, -mTexRepeat);
   (*mTexCoords)[13].set (mTexRepeat, mTexRepeat);
   (*mTexCoords)[14].set (-mTexRepeat, mTexRepeat);

   (*mTexCoords)[15].set (-mTexRepeat, -mTexRepeat);
   (*mTexCoords)[16].set (mTexRepeat, -mTexRepeat);
   (*mTexCoords)[17].set (-mTexRepeat, mTexRepeat);

//right

   (*mTexCoords)[18].set (-mTexRepeat, mTexRepeat);
   (*mTexCoords)[19].set (-mTexRepeat, -mTexRepeat);
   (*mTexCoords)[20].set (mTexRepeat, -mTexRepeat);

   (*mTexCoords)[21].set (mTexRepeat, mTexRepeat);
   (*mTexCoords)[22].set (-mTexRepeat, mTexRepeat);
   (*mTexCoords)[23].set (mTexRepeat, -mTexRepeat);

//front
   (*mTexCoords)[24].set (mTexRepeat, mTexRepeat);
   (*mTexCoords)[25].set (-mTexRepeat, mTexRepeat);
   (*mTexCoords)[26].set (-mTexRepeat, -mTexRepeat);

   (*mTexCoords)[27].set (mTexRepeat, -mTexRepeat);
   (*mTexCoords)[28].set (mTexRepeat, mTexRepeat);
   (*mTexCoords)[29].set (-mTexRepeat, -mTexRepeat);

//back
   (*mTexCoords)[30].set (-mTexRepeat, -mTexRepeat);
   (*mTexCoords)[31].set (mTexRepeat, -mTexRepeat);
   (*mTexCoords)[32].set (mTexRepeat, mTexRepeat);

   (*mTexCoords)[33].set (-mTexRepeat, mTexRepeat);
   (*mTexCoords)[34].set (-mTexRepeat, -mTexRepeat);
   (*mTexCoords)[35].set (mTexRepeat, mTexRepeat);

//////////////////////////////////////////////////////////////////////////
// TexCoords 2
//////////////////////////////////////////////////////////////////////////
//bottom

   (*mTexCoords2)[0].set (-mTexRepeat, mTexRepeat);
   (*mTexCoords2)[1].set (-mTexRepeat, -mTexRepeat);
   (*mTexCoords2)[2].set (mTexRepeat, mTexRepeat);

   (*mTexCoords2)[3].set (mTexRepeat, mTexRepeat);
   (*mTexCoords2)[4].set (-mTexRepeat, -mTexRepeat);
   (*mTexCoords2)[5].set (mTexRepeat, -mTexRepeat);


//top

   (*mTexCoords2)[6].set (mTexRepeat, -mTexRepeat);
   (*mTexCoords2)[7].set (-mTexRepeat, mTexRepeat);
   (*mTexCoords2)[8].set (-mTexRepeat, -mTexRepeat);

   (*mTexCoords2)[9].set (mTexRepeat, mTexRepeat);
   (*mTexCoords2)[10].set (-mTexRepeat, mTexRepeat);
   (*mTexCoords2)[11].set (mTexRepeat, -mTexRepeat);

//left

   (*mTexCoords2)[12].set (mTexRepeat, mTexRepeat);
   (*mTexCoords2)[13].set (-mTexRepeat, mTexRepeat);
   (*mTexCoords2)[14].set (mTexRepeat, -mTexRepeat);

   (*mTexCoords2)[15].set (mTexRepeat, -mTexRepeat);
   (*mTexCoords2)[16].set (-mTexRepeat, mTexRepeat);
   (*mTexCoords2)[17].set (-mTexRepeat, -mTexRepeat);


//right
   (*mTexCoords2)[18].set (-mTexRepeat, -mTexRepeat);
   (*mTexCoords2)[19].set (mTexRepeat, -mTexRepeat);
   (*mTexCoords2)[20].set (-mTexRepeat, mTexRepeat);

   (*mTexCoords2)[21].set (-mTexRepeat, mTexRepeat);
   (*mTexCoords2)[22].set (mTexRepeat, -mTexRepeat);
   (*mTexCoords2)[23].set (mTexRepeat, mTexRepeat);

//front
   (*mTexCoords2)[24].set (-mTexRepeat, mTexRepeat);
   (*mTexCoords2)[25].set (-mTexRepeat, -mTexRepeat);
   (*mTexCoords2)[26].set (mTexRepeat, mTexRepeat);

   (*mTexCoords2)[27].set (mTexRepeat, mTexRepeat);
   (*mTexCoords2)[28].set (-mTexRepeat, -mTexRepeat);
   (*mTexCoords2)[29].set (mTexRepeat, -mTexRepeat);

//back
   (*mTexCoords2)[30].set (mTexRepeat, -mTexRepeat);
   (*mTexCoords2)[31].set (mTexRepeat, mTexRepeat);
   (*mTexCoords2)[32].set (-mTexRepeat, -mTexRepeat);

   (*mTexCoords2)[33].set (-mTexRepeat, -mTexRepeat);
   (*mTexCoords2)[34].set (mTexRepeat, mTexRepeat);
   (*mTexCoords2)[35].set (-mTexRepeat, mTexRepeat);

//////////////////////////////////////////////////////////////////////////
// Tex Coords 3
//////////////////////////////////////////////////////////////////////////

//bottom

   (*mTexCoords3)[0].set (-mTexRepeat, -mTexRepeat);
   (*mTexCoords3)[1].set (mTexRepeat, mTexRepeat);
   (*mTexCoords3)[2].set (-mTexRepeat, mTexRepeat);

   (*mTexCoords3)[3].set (-mTexRepeat, -mTexRepeat);
   (*mTexCoords3)[4].set (mTexRepeat, -mTexRepeat);
   (*mTexCoords3)[5].set (mTexRepeat, mTexRepeat);


//top

   (*mTexCoords3)[6].set (-mTexRepeat, mTexRepeat);
   (*mTexCoords3)[7].set (-mTexRepeat, -mTexRepeat);
   (*mTexCoords3)[8].set (mTexRepeat, -mTexRepeat);

   (*mTexCoords3)[9].set (-mTexRepeat, mTexRepeat);
   (*mTexCoords3)[10].set (mTexRepeat, -mTexRepeat);
   (*mTexCoords3)[11].set (mTexRepeat, mTexRepeat);


//left
   (*mTexCoords3)[12].set (-mTexRepeat, mTexRepeat);
   (*mTexCoords3)[13].set (mTexRepeat, -mTexRepeat);
   (*mTexCoords3)[14].set (mTexRepeat, mTexRepeat);

   (*mTexCoords3)[15].set (-mTexRepeat, mTexRepeat);
   (*mTexCoords3)[16].set (-mTexRepeat, -mTexRepeat);
   (*mTexCoords3)[17].set (mTexRepeat, -mTexRepeat);


//right
   (*mTexCoords3)[18].set (mTexRepeat, -mTexRepeat);
   (*mTexCoords3)[19].set (-mTexRepeat, mTexRepeat);
   (*mTexCoords3)[20].set (-mTexRepeat, -mTexRepeat);

   (*mTexCoords3)[21].set (mTexRepeat, -mTexRepeat);
   (*mTexCoords3)[22].set (mTexRepeat, mTexRepeat);
   (*mTexCoords3)[23].set (-mTexRepeat, mTexRepeat);

//front
   (*mTexCoords3)[24].set (-mTexRepeat, -mTexRepeat);
   (*mTexCoords3)[25].set (mTexRepeat, mTexRepeat);
   (*mTexCoords3)[26].set (-mTexRepeat, mTexRepeat);

   (*mTexCoords3)[27].set (-mTexRepeat, -mTexRepeat);
   (*mTexCoords3)[28].set (mTexRepeat, -mTexRepeat);
   (*mTexCoords3)[29].set (mTexRepeat, mTexRepeat);

//back
   (*mTexCoords3)[30].set (mTexRepeat, mTexRepeat);
   (*mTexCoords3)[31].set (-mTexRepeat, -mTexRepeat);
   (*mTexCoords3)[32].set (mTexRepeat, -mTexRepeat);

   (*mTexCoords3)[33].set (mTexRepeat, mTexRepeat);
   (*mTexCoords3)[34].set (-mTexRepeat, mTexRepeat);
   (*mTexCoords3)[35].set (-mTexRepeat, -mTexRepeat);

}

#endif // __CUBE_VERTICES_H__
