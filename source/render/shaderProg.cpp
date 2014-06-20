// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// Copyright(C) 2012-2014 Samuel Villarreal
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
// DESCRIPTION: Shader Program Object
//
//-----------------------------------------------------------------------------

#include "render.h"
#include "shaderProg.h"
#include "material.h"

kexShaderManager kexShaderObj::manager;
kexArray<kexGetterBase*> kexShaderObj::customParameters;

static const char *shaderParamNames[RSP_TOTAL+1] = {
    "uDiffuseColor",
    "uViewWidth",
    "uViewHeight",
    "uParam1",
    "uParam2",
    "uParam3",
    "uParam4",
    NULL
};

//
// kexShaderObj::kexShaderObj
//

kexShaderObj::kexShaderObj(void) {
    Init();
}

//
// kexShaderObj::~kexShaderObj
//

kexShaderObj::~kexShaderObj(void) {
}

//
// kexShaderObj::Init
//

void kexShaderObj::Init(void) {
    this->programObj            = 0;
    this->vertexProgram         = 0;
    this->fragmentProgram       = 0;
    this->validCount            = 0;
    this->bHasErrors            = false;
    this->bLoaded               = false;
    this->customParametersLocal = NULL;
}

//
// kexShaderObj::InitFromDefinition
//

void kexShaderObj::InitFromDefinition(kexKeyMap *def) {
    kexStr string;

    Init();
    InitProgram();
            
    if(def->GetString("fragmentProgram", string)) {
        Compile(string.c_str(), RST_FRAGMENT);
    }
    if(def->GetString("vertexProgram", string)) {
        Compile(string.c_str(), RST_VERTEX);
    }
    
    Link();
}

//
// kexShaderObj::InitProgram
//

void kexShaderObj::InitProgram(void) {
    programObj = dglCreateProgramObjectARB();
    validCount = -1;

    for(int i = 0; i < RSP_TOTAL; i++) {
        globalParams[i] = -1;
    }
}

//
// kexShaderObj::Enable
//

void kexShaderObj::Enable(void) {
    if(programObj == kexlib::render::backend->glState.currentProgram) {
        return;
    }
    
    dglUseProgramObjectARB(programObj);
    kexlib::render::backend->glState.currentProgram = programObj;
}

//
// kexShaderObj::Delete
//

void kexShaderObj::Delete(void) {
    if(bLoaded == false) {
        return;
    }
    
    dglDeleteObjectARB(fragmentProgram);
    dglDeleteObjectARB(vertexProgram);
    dglDeleteObjectARB(programObj);
    bLoaded = false;
    
    if(customParametersLocal) {
        delete[] customParametersLocal;
    }
}

//
// kexShaderObj::Compile
//

void kexShaderObj::Compile(const char *name, rShaderType_t type) {
    byte *data;
    rhandle *handle;
    
    if(kexlib::fileSystem->OpenFile(name, &data, hb_static) == 0) {
        kexlib::system->Warning("kexShaderObj::Compile: %s not found\n", name);
        return;
    }
    
    if(type == RST_VERTEX) {
        vertexProgram = dglCreateShaderObjectARB(GL_VERTEX_SHADER_ARB);
        handle = &vertexProgram;
    }
    else if(type == RST_FRAGMENT) {
        fragmentProgram = dglCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB);
        handle = &fragmentProgram;
    }
    else {
        Mem_Free(data);
        return;
    }
    
    dglShaderSourceARB(*handle, 1, (const GLcharARB**)&data, NULL);
    dglCompileShaderARB(*handle);
    dglAttachObjectARB(programObj, *handle);
    
    Mem_Free(data);
}

//
// kexShaderObj::SetUniform
//

void kexShaderObj::SetUniform(const char *name, const int val) {
    int loc = dglGetUniformLocationARB(programObj, name);

    if(loc != -1) {
        dglUniform1iARB(loc, val);
    }
}

//
// kexShaderObj::SetUniform
//

void kexShaderObj::SetUniform(const char *name, const int *val, const int size) {
    int loc = dglGetUniformLocationARB(programObj, name);

    if(loc != -1) {
        dglUniform1ivARB(loc, size, val);
    }
}

//
// kexShaderObj::SetUniform
//

void kexShaderObj::SetUniform(const char *name, const float val) {
    int loc = dglGetUniformLocationARB(programObj, name);

    if(loc != -1) {
        dglUniform1fARB(loc, val);
    }
}

//
// kexShaderObj::SetUniform
//

void kexShaderObj::SetUniform(const char *name, kexVec2 &val) {
    int loc = dglGetUniformLocationARB(programObj, name);

    if(loc != -1) {
        dglUniform2fvARB(loc, 1, val.ToFloatPtr());
    }
}

//
// kexShaderObj::SetUniform
//

void kexShaderObj::SetUniform(const char *name, kexVec3 &val) {
    int loc = dglGetUniformLocationARB(programObj, name);

    if(loc != -1) {
        dglUniform3fvARB(loc, 1, val.ToFloatPtr());
    }
}

//
// kexShaderObj::SetUniform
//

void kexShaderObj::SetUniform(const char *name, kexVec3 *val, const int size) {
    int loc = dglGetUniformLocationARB(programObj, name);

    if(loc != -1) {
        dglUniform3fvARB(loc, size, reinterpret_cast<float*>(&val[0].x));
    }
}

//
// kexShaderObj::SetUniform
//

void kexShaderObj::SetUniform(const char *name, kexVec4 &val) {
    int loc = dglGetUniformLocationARB(programObj, name);

    if(loc != -1) {
        dglUniform4fvARB(loc, 1, val.ToFloatPtr());
    }
}

//
// kexShaderObj::SetUniform
//

void kexShaderObj::SetUniform(const char *name, kexMatrix &val, bool bTranspose) {
    int loc = dglGetUniformLocationARB(programObj, name);

    if(loc != -1) {
        dglUniformMatrix4fvARB(loc, 1, bTranspose, val.ToFloatPtr());
    }
}

//
// kexShaderObj::CommitGlobalUniforms
//

void kexShaderObj::CommitGlobalUniforms(const kexMaterial *material) {
    kexVec4 diffuseColor;
    
    if(material != NULL) {
        diffuseColor = material->DiffuseColor();
    }
    else {
        diffuseColor.Set(1, 1, 1, 1);
    }

    // diffuse color can change a lot per frame
    SetGlobalUniform(RSP_DIFFUSE_COLOR, diffuseColor);

    if(validCount == kexlib::render::backend->ValidFrameNum()) {
        return;
    }
    
    validCount = kexlib::render::backend->ValidFrameNum();
    
    SetGlobalUniform(RSP_VIEW_WIDTH, kexlib::system->VideoWidth());
    SetGlobalUniform(RSP_VIEW_HEIGHT, kexlib::system->VideoHeight());
    
    kexGetterBase *getter;
    unsigned int len = kexShaderObj::customParameters.Length();
    
    if(len <= 0) {
        return;
    }
    
    // allocate local custom parameter list if it hasn't already
    if(!customParametersLocal) {
        customParametersLocal = new int[len];
        
        for(unsigned int i = 0; i < len; i++) {
            customParametersLocal[i] = -1;
            
            kexGetterBase *getter = kexShaderObj::customParameters[i];
            customParametersLocal[i] = dglGetUniformLocationARB(programObj, getter->Name());
        }
    }
    
    for(unsigned int i = 0; i < len; i++) {
        if(customParametersLocal[i] <= -1) {
            continue;
        }
        
        getter = kexShaderObj::customParameters[i];
        
        switch(getter->ReturnType()) {
            case GRT_FLOAT:
                dglUniform1fARB(customParametersLocal[i], getter->GetFloatValue());
                break;
            case GRT_INT:
                dglUniform1iARB(customParametersLocal[i], getter->GetIntValue());
                break;
            case GRT_BOOL:
                dglUniform1iARB(customParametersLocal[i], getter->GetBoolValue());
                break;
            case GRT_VEC2:
                dglUniform2fvARB(customParametersLocal[i], 1, getter->GetVec2Value().ToFloatPtr());
                break;
            case GRT_VEC3:
                dglUniform3fvARB(customParametersLocal[i], 1, getter->GetVec3Value().ToFloatPtr());
                break;
            case GRT_VEC4:
                dglUniform4fvARB(customParametersLocal[i], 1, getter->GetVec4Value().ToFloatPtr());
                break;
            default:
                break;
        }
    }
}

//
// kexShaderObj::SetGlobalUniform
//

void kexShaderObj::SetGlobalUniform(const rShaderGlobalParams_t param, int val) {
    if(globalParams[param] <= -1) {
        return;
    }

    dglUniform1iARB(globalParams[param], val);
}

//
// kexShaderObj::SetGlobalUniform
//

void kexShaderObj::SetGlobalUniform(const rShaderGlobalParams_t param, float val) {
    if(globalParams[param] <= -1) {
        return;
    }

    dglUniform1fARB(globalParams[param], val);
}

//
// kexShaderObj::SetGlobalUniform
//

void kexShaderObj::SetGlobalUniform(const rShaderGlobalParams_t param, kexVec2 &val) {
    if(globalParams[param] <= -1) {
        return;
    }

    dglUniform2fvARB(globalParams[param], 1, val.ToFloatPtr());
}

//
// kexShaderObj::SetGlobalUniform
//

void kexShaderObj::SetGlobalUniform(const rShaderGlobalParams_t param, kexVec3 &val) {
    if(globalParams[param] <= -1) {
        return;
    }

    dglUniform3fvARB(globalParams[param], 1, val.ToFloatPtr());
}

//
// kexShaderObj::SetGlobalUniform
//

void kexShaderObj::SetGlobalUniform(const rShaderGlobalParams_t param, kexVec4 &val) {
    if(globalParams[param] <= -1) {
        return;
    }

    dglUniform4fvARB(globalParams[param], 1, val.ToFloatPtr());
}

//
// kexShaderObj::SetGlobalUniform
//

void kexShaderObj::SetGlobalUniform(const rShaderGlobalParams_t param, kexMatrix &val) {
    if(globalParams[param] <= -1) {
        return;
    }

    dglUniformMatrix4fvARB(globalParams[param], 1, false, val.ToFloatPtr());
}

//
// kexShaderObj::DumpErrorLog
//

void kexShaderObj::DumpErrorLog(const rhandle handle) {
    int logLength;
    
    dglGetObjectParameterivARB(handle, GL_OBJECT_INFO_LOG_LENGTH_ARB, &logLength);
    
    if(logLength > 0) {
        int cw;
        char *log;
        
        log = (char*)Mem_Alloca(logLength);
        
        dglGetInfoLogARB(handle, logLength, &cw, log);
        kexlib::system->Warning("%s\n", log);
    }
}

//
// kexShaderObj::Link
//

bool kexShaderObj::Link(void) {
    int linked;
    
    bHasErrors = false;
    dglLinkProgramARB(programObj);
    dglGetObjectParameterivARB(programObj, GL_OBJECT_LINK_STATUS_ARB, &linked);
    
    if(!linked) {
        bHasErrors = true;
        DumpErrorLog(programObj);
        DumpErrorLog(vertexProgram);
        DumpErrorLog(fragmentProgram);
    }
    else {
        dglUseProgramObjectARB(programObj);

        for(int i = 0; i < RSP_TOTAL; i++) {
            globalParams[i] = dglGetUniformLocationARB(programObj, shaderParamNames[i]);
        }
    }
    
    dglUseProgramObjectARB(0);
    bLoaded = true;
    return (linked > 0);
}

//
// kexShaderManager::OnLoad
//

kexShaderObj *kexShaderManager::OnLoad(const char *file) {
    kexShaderObj *sobj = NULL;
    kexKeyMap *def;
        
    if((def = kexlib::defs->FindDefEntry(file))) {
        sobj = dataList.Add(file);
        strncpy(sobj->fileName, file, MAX_FILEPATH);

        sobj->InitFromDefinition(def);

        if(sobj->HasErrors()) {
            kexlib::system->Warning("There were some errors when compiling %s\n", file);
        }
        
    }
    else {
        return NULL;
    }

    return sobj;
}
