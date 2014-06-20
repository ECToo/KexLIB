// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// Copyright(C) 2014 Samuel Villarreal
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
// 02111-1307, USA.
//
//-----------------------------------------------------------------------------
//
// DESCRIPTION:
//
//-----------------------------------------------------------------------------

#include "render.h"
#include "material.h"

static kexCpuVertList cpuVertListLocal;
kexCpuVertList *kexlib::render::cpuVertList = &cpuVertListLocal;

//
// kexCpuVertList::kexCpuVertList
//

kexCpuVertList::kexCpuVertList(void) {
}

//
// kexCpuVertList::~kexCpuVertList
//

kexCpuVertList::~kexCpuVertList(void) {
}

//
// kexCpuVertList::Reset
//

void kexCpuVertList::Reset(void) {
    indiceCount = 0;
    vertexCount = 0;
    roverIndices = drawIndices;
    roverVertices = drawVertices;
    roverTexCoords = drawTexCoords;
    roverRGB = drawRGB;
}

//
// kexCpuVertList::BindDrawPointers
//

void kexCpuVertList::BindDrawPointers(void) {
    dglNormalPointer(GL_FLOAT, sizeof(float)*3, drawNormals);
    dglTexCoordPointer(2, GL_FLOAT, sizeof(float)*2, drawTexCoords);
    dglVertexPointer(3, GL_FLOAT, sizeof(float)*3, drawVertices);
    dglColorPointer(4, GL_UNSIGNED_BYTE, sizeof(byte)*4, drawRGB);
    
    Reset();
}

//
// kexCpuVertList::AddTriangle
//

void kexCpuVertList::AddTriangle(int v0, int v1, int v2) {
    if((vertexCount * 3) >= CPU_VERT_NUM_XYZ) {
        return;
    }

    *(roverIndices++) = v0;
    *(roverIndices++) = v1;
    *(roverIndices++) = v2;
    
    indiceCount += 3;
}

//
// kexCpuVertList::AddVertex
//

void kexCpuVertList::AddVertex(float x, float y, float z, float s, float t,
                                byte r, byte g, byte b, byte a) {
    if((vertexCount * 3) >= CPU_VERT_NUM_XYZ) {
        return;
    }

    *(roverVertices++)   = x;
    *(roverVertices++)   = y;
    *(roverVertices++)   = z;
    *(roverTexCoords++)  = s;
    *(roverTexCoords++)  = t;
    *(roverRGB++)        = r;
    *(roverRGB++)        = g;
    *(roverRGB++)        = b;
    *(roverRGB++)        = a;

    vertexCount++;
}

//
// kexCpuVertList::AddVertex
//

void kexCpuVertList::AddVertex(float x, float y, float z, float s, float t, byte *rgba) {
    AddVertex(x, y, z, s, t, rgba[0], rgba[1], rgba[2], rgba[3]);
}

//
// kexCpuVertList::AddLine
//

void kexCpuVertList::AddLine(float x1, float y1, float z1,
                          float x2, float y2, float z2,
                          byte r, byte g, byte b, byte a) {
    
    *(roverIndices++) = vertexCount; indiceCount++;
    AddVertex(x1, y1, z1, 0, 0, r, g, b, a);
    *(roverIndices++) = vertexCount; indiceCount++;
    AddVertex(x2, y2, z2, 0, 0, r, g, b, a);
}

//
// kexCpuVertList::AddLine
//

void kexCpuVertList::AddLine(float x1, float y1, float z1,
                          float x2, float y2, float z2,
                          byte r1, byte g1, byte b1, byte a1,
                          byte r2, byte g2, byte b2, byte a2) {
    
    *(roverIndices++) = vertexCount; indiceCount++;
    AddVertex(x1, y1, z1, 0, 0, r1, g1, b1, a1);
    *(roverIndices++) = vertexCount; indiceCount++;
    AddVertex(x2, y2, z2, 0, 0, r2, g2, b2, a2);
}

//
// kexCpuVertList::DrawElements
//

void kexCpuVertList::DrawElements(const bool bClearCount) {
    dglDrawElements(GL_TRIANGLES, indiceCount, GL_UNSIGNED_SHORT, drawIndices);

    if(bClearCount) {
        Reset();
    }
}

//
// kexCpuVertList::DrawElements
//
// Draws using the specified material
//

void kexCpuVertList::DrawElements(const kexMaterial *material, const bool bClearCount) {
    kexShaderObj *shader;
    
    shader = ((kexMaterial*)material)->ShaderObj();
    shader->Enable();
    shader->CommitGlobalUniforms(material);
    
    material->SetRenderState();
    material->BindImages();
    
    kexlib::render::backend->SetPolyMode(GLPOLY_FILL);
    
    dglDrawElements(GL_TRIANGLES, indiceCount, GL_UNSIGNED_SHORT, drawIndices);
    
    if(bClearCount) {
        Reset();
    }
}

//
// kexCpuVertList::DrawElementsNoShader
//

void kexCpuVertList::DrawElementsNoShader(const bool bClearCount) {
    kexlib::render::backend->DisableShaders();
    dglDrawElements(GL_TRIANGLES, indiceCount, GL_UNSIGNED_SHORT, drawIndices);

    if(bClearCount) {
        Reset();
    }
}

//
// kexCpuVertList::DrawLineElements
//

void kexCpuVertList::DrawLineElements(void) {
    kexlib::render::backend->DisableShaders();
    dglDrawElements(GL_LINES, indiceCount, GL_UNSIGNED_SHORT, drawIndices);
    
    Reset();
}
