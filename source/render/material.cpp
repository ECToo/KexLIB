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
// DESCRIPTION: Material system. Kex materials have no texture stages
//              but rather lets the shader program handle everything.
//
//-----------------------------------------------------------------------------

#include "render.h"
#include "material.h"

kexMaterialManager kexMaterial::manager;

//
// kexMaterial::kexMaterial
//

kexMaterial::kexMaterial(void) {
}

//
// kexMaterial::~kexMaterial
//

kexMaterial::~kexMaterial(void) {
}

//
// kexMaterial::Init
//

void kexMaterial::Init(void) {
    this->next          = NULL;
    this->flags         = 0;
    this->stateBits     = 0;
    this->cullType      = GLCULL_BACK;
    this->alphaFunction = GLFUNC_GEQUAL;
    this->depthMask     = 1;
    this->sortOrder     = MSO_DEFAULT;
    this->alphaMask     = 0.01f;
    this->units         = 0;
    this->genID         = 0;
    this->bShaderErrors = false;
    this->shaderObj     = NULL;

    this->diffuseColor.Set(1, 1, 1, 1);
    
    for(int i = 0; i < MAX_SAMPLER_UNITS; i++) {
        this->samplers[i].param     = "";
        this->samplers[i].unit      = 0;
        this->samplers[i].texture   = NULL;
        this->samplers[i].clamp     = TC_CLAMP;
        this->samplers[i].filter    = TF_LINEAR;
        this->samplers[i].scroll[0] = 0;
        this->samplers[i].scroll[1] = 0;
        this->samplers[i].scale[0]  = 0;
        this->samplers[i].scale[1]  = 0;
        this->samplers[i].scale[2]  = 0;
        this->samplers[i].rotate    = 0;
    }
}

//
// kexMaterial::Delete
//

void kexMaterial::Delete(void) {
    for(unsigned int i = 0; i < units; i++) {
        if(samplers[i].texture != NULL) {
            samplers[i].texture->Delete();
        }
    }
}

//
// kexMaterial::Sampler
//

matSampler_t *kexMaterial::Sampler(const int which) {
    assert(which >= 0 && which < MAX_SAMPLER_UNITS);
    return &samplers[which];
}

//
// kexMaterial::SetDiffuseColor
//

void kexMaterial::SetDiffuseColor(const rcolor color) {
    float r, g, b, a;

    r = (float)(color & 0xff) / 255.0f;
    g = (float)((color >> 8) & 0xff) / 255.0f;
    b = (float)((color >> 16) & 0xff) / 255.0f;
    a = (float)((color >> 24) & 0xff) / 255.0f;

    diffuseColor.Set(r, g, b, a);
}

//
// kexMaterial::SetRenderState
//

void kexMaterial::SetRenderState(void) const {
    kexlib::render::backend->SetState(StateBits());
    kexlib::render::backend->SetAlphaFunc(AlphaFunction(), AlphaMask());
    kexlib::render::backend->SetCull(CullType());
    kexlib::render::backend->SetDepthMask(DepthMask());
}

//
// kexMaterial::BindImages
//

void kexMaterial::BindImages(void) const {
    for(unsigned int i = 0; i < units; i++) {
        const matSampler_t *sampler = &samplers[i];
        
        kexlib::render::backend->SetTextureUnit(sampler->unit);
        
        if(sampler->texture == NULL) {
            kexlib::render::backend->defaultTexture.Bind();
        }
        else {
            if(sampler->texture == kexlib::render::backend->frameBuffer) {
                sampler->texture->BindFrameBuffer();
            }
            else if(sampler->texture == kexlib::render::backend->depthBuffer) {
                sampler->texture->BindDepthBuffer();
            }
            else {
                sampler->texture->Bind();
            }
            sampler->texture->ChangeParameters(sampler->clamp, sampler->filter);
        }
    }
    
    kexlib::render::backend->SetTextureUnit(0);
}

//
// kexMaterial::ParseSampler
//

void kexMaterial::ParseSampler(kexLexer *lexer) {
    kexStr texFile;
    kexStr paramName;
    unsigned int unit;
    matSampler_t *sampler;

    lexer->GetString();
    paramName = lexer->StringToken();
    
    unit = lexer->GetNumber();
    
    lexer->GetString();
    texFile = lexer->StringToken();
    
    sampler = Sampler(unit);
    sampler->param = paramName;
    sampler->filter = TF_LINEAR;
    sampler->clamp = TC_CLAMP;
    sampler->unit = unit;
    
    shaderObj->SetUniform(sampler->param, sampler->unit);
    
    if(unit+1 > units) {
        units = unit+1;
    }
    
    lexer->ExpectNextToken(TK_LBRACK);
    lexer->Find();
    
    while(lexer->TokenType() != TK_RBRACK) {
        if(lexer->Matches("filter")) {
            lexer->Find();
            if(lexer->Matches("linear")) {
                sampler->filter = TF_LINEAR;
            }
            else if(lexer->Matches("nearest")) {
                sampler->filter = TF_NEAREST;
            }
        }
        else if(lexer->Matches("wrap")) {
            lexer->Find();
            if(lexer->Matches("clamp")) {
                sampler->clamp = TC_CLAMP;
            }
            else if(lexer->Matches("repeat")) {
                sampler->clamp = TC_REPEAT;
            }
            else if(lexer->Matches("mirrored")) {
                sampler->clamp = TC_MIRRORED;
            }
        }
        else if(lexer->Matches("scroll")) {
            sampler->scroll[0] = (float)lexer->GetFloat();
            sampler->scroll[1] = (float)lexer->GetFloat();
        }
        else if(lexer->Matches("scale")) {
            sampler->scale[0] = (float)lexer->GetFloat();
            sampler->scale[1] = (float)lexer->GetFloat();
            sampler->scale[1] = (float)lexer->GetFloat();
        }
        else if(lexer->Matches("rotate")) {
            sampler->rotate = (float)lexer->GetFloat();
        }
        
        lexer->Find();
    }

    stateBits |= BIT(GLSTATE_TEXTURE0 + unit);
    sampler->texture = kexlib::render::backend->CacheTexture(texFile.c_str(),
                                                 sampler->clamp,
                                                 sampler->filter);
}

//
// kexMaterial::ParseFunction
//

glFunctions_t kexMaterial::ParseFunction(kexLexer *lexer) {
    lexer->Find();
    
    if(lexer->Matches("lequal")) {
        return GLFUNC_LEQUAL;
    }
    else if(lexer->Matches("gequal")) {
        return GLFUNC_GEQUAL;
    }
    else if(lexer->Matches("equal")) {
        return GLFUNC_EQUAL;
    }
    else if(lexer->Matches("always")) {
        return GLFUNC_ALWAYS;
    }
    
    return GLFUNC_NEVER;
}

//
// kexMaterial::ParseShader
//

void kexMaterial::ParseShader(kexLexer *lexer) {
    lexer->GetString();

    shaderObj = kexShaderObj::manager.Load(lexer->StringToken());
    
    if(!shaderObj || shaderObj->HasErrors()) {
        bShaderErrors = true;
    }
    else {
        // shader needs to be enabled before we can set parameters
        shaderObj->Enable();
    }
}

//
// kexMaterial::Parse
//

void kexMaterial::Parse(kexLexer *lexer) {
    lexer->ExpectNextToken(TK_LBRACK);
    lexer->Find();
    
    while(lexer->TokenType() != TK_RBRACK) {
        switch(lexer->TokenType()) {
            case TK_EOF:
                return;
            case TK_IDENIFIER:
                if(lexer->Matches("shader")) {
                    ParseShader(lexer);
                }
                else if(lexer->Matches("fullbright")) {
                    flags |= MTF_FULLBRIGHT;
                }
                else if(lexer->Matches("nodraw")) {
                    flags |= MTF_NODRAW;
                }
                else if(lexer->Matches("blend")) {
                    stateBits |= BIT(GLSTATE_BLEND);
                }
                else if(lexer->Matches("alphatest")) {
                    stateBits |= BIT(GLSTATE_ALPHATEST);
                }
                else if(lexer->Matches("depthtest")) {
                    stateBits |= BIT(GLSTATE_DEPTHTEST);
                }
                else if(lexer->Matches("allowfog")) {
                    stateBits |= BIT(GLSTATE_FOG);
                }
                else if(lexer->Matches("cull")) {
                    lexer->Find();
                    
                    if(lexer->Matches("back")) {
                        stateBits |= BIT(GLSTATE_CULL);
                        cullType = GLCULL_BACK;
                    }
                    else if(lexer->Matches("front")) {
                        stateBits |= BIT(GLSTATE_CULL);
                        cullType = GLCULL_FRONT;
                    }
                }
                else if(lexer->Matches("sort")) {
                    lexer->Find();

                    if(lexer->Matches("default")) {
                        sortOrder = MSO_DEFAULT;
                    }
                    else if(lexer->Matches("masked")) {
                        sortOrder = MSO_MASKED;
                    }
                    else if(lexer->Matches("translucent")) {
                        sortOrder = MSO_TRANSPARENT;
                    }
                    else if(lexer->Matches("custom1")) {
                        sortOrder = MSO_CUSTOM1;
                    }
                    else if(lexer->Matches("custom2")) {
                        sortOrder = MSO_CUSTOM2;
                    }
                    else if(lexer->Matches("custom3")) {
                        sortOrder = MSO_CUSTOM3;
                    }
                }
                else if(lexer->Matches("alphafunc")) {
                    alphaFunction = ParseFunction(lexer);
                }
                else if(lexer->Matches("alphamask")) {
                    alphaMask = (float)lexer->GetFloat();
                }
                else if(lexer->Matches("sampler")) {
                    ParseSampler(lexer);
                }
                else if(lexer->Matches("color_diffuse")) {
                    diffuseColor = lexer->GetVectorString4();
                    if(diffuseColor.w < 1.0f) {
                        // hack: force transparent sort order and disable depth mask
                        sortOrder = MSO_TRANSPARENT;
                        depthMask = 0;
                    }
                }
                else if(lexer->Matches("nodepthmask")) {
                    depthMask = 0;
                }
                break;
            default:
                kexlib::parser->Error("kexMaterial::Parse - Unknown token: %s\n", lexer->Token());
                break;
        }
        
        lexer->Find();
    }
    
    kexlib::render::backend->DisableShaders();
}

//
// kexMaterialManager::OnLoad
//

kexMaterial *kexMaterialManager::OnLoad(const char *file) {
    kexLexer *lexer;
    kexMaterial *material;
    filepath_t tStr;
    int pos;
    int len;
    bool bFoundMaterial = false;
    
    pos = kexStr::IndexOf(file, "@");
    
    if(pos == -1) {
        return NULL;
    }
    
    len = strlen(file);
    strncpy(tStr, file, pos);
    tStr[pos] = 0;
    
    if(!(lexer = kexlib::parser->Open(tStr))) {
        kexlib::system->Warning("kexMaterialManager::Load: %s not found\n", tStr);
        return NULL;
    }

    strncpy(tStr, file + pos + 1, len - pos);
    tStr[len - pos] = 0;
    
    material = NULL;

    while(lexer->CheckState()) {
        lexer->Find();

        if(lexer->Matches(tStr)) {
            bFoundMaterial = true;
            material = dataList.Add(file);
            strncpy(material->fileName, file, MAX_FILEPATH);

            material->Init();
            material->Parse(lexer);
            break;
        }
    }

    if(bFoundMaterial == false) {
        kexlib::system->Warning("kexMaterialManager::Load: %s not found\n", tStr);
        kexlib::parser->Close();
        return NULL;
    }
    
    // we're done with the file
    kexlib::parser->Close();

    return material;
}
