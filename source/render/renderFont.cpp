// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// Copyright(C) 2013 Samuel Villarreal
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
// DESCRIPTION: Font mapping
//
//-----------------------------------------------------------------------------

#include "render.h"
#include "renderFont.h"
#include "material.h"

kexFontManager kexFont::manager;

//
// kexFont::kexFont
//

kexFont::kexFont(void) {
    this->bLoaded = false;
    this->material = NULL;
}

//
// kexFont::~kexFont
//

kexFont::~kexFont(void) {
}

//
// kexFont::Delete
//

void kexFont::Delete(void) {
}

//
// kexFont::LoadKFont
//

void kexFont::LoadKFont(const char *file) {
    kexLexer *lexer;
    filepath_t fileName;

    if(!(lexer = kexlib::parser->Open(file))) {
        kexlib::system->Warning("kexFont::LoadKFont: %s not found\n", file);
        return;
    }

    memset(fileName, 0, sizeof(filepath_t));

    while(lexer->CheckState()) {
        lexer->Find();

        if(lexer->TokenType() != TK_IDENIFIER) {
            continue;
        }

        if(lexer->Matches("material")) {
            lexer->GetString();
            material = kexMaterial::manager.Load(lexer->StringToken());
        }

        if(lexer->Matches("mapchar")) {
            lexer->ExpectNextToken(TK_LBRACK);
            lexer->Find();

            while(lexer->TokenType() != TK_RBRACK) {
                int ch;

                if(lexer->TokenType() == TK_NUMBER) {
                    ch = atoi(lexer->Token());
                }
                else {
                    kexlib::parser->Error("%s is not a number", lexer->Token());
                    kexlib::parser->Close();
                    return;
                }

                atlas[ch].x = lexer->GetNumber();
                atlas[ch].y = lexer->GetNumber();
                atlas[ch].w = lexer->GetNumber();
                atlas[ch].h = lexer->GetNumber();

                lexer->Find();
            }
        }
    }

    // we're done with the file
    kexlib::parser->Close();
    bLoaded = true;
}

//
// kexFont::DrawString
//

void kexFont::DrawString(const char *string, float x, float y, float scale,
                         bool center, byte *rgba1, byte *rgba2) {
    float w;
    float h;
    int tri;
    unsigned int i;
    unsigned int len;
    char ch;
    atlas_t *at;
    float vx1;
    float vy1;
    float vx2;
    float vy2;
    float tx1;
    float tx2;
    float ty1;
    float ty2;
    char *check;
    kexTexture *texture;

    if(scale <= 0.01f) {
        scale = 1;
    }

    if(center) {
        x -= StringWidth(string, scale, 0) * 0.5f;
    }

    if(!(texture = material->Sampler(0)->texture)) {
        texture = &kexlib::render::backend->defaultTexture;
    }

    w = (float)texture->OriginalWidth();
    h = (float)texture->OriginalHeight();

    tri = 0;
    len = strlen(string);

    kexlib::render::cpuVertList->BindDrawPointers();

    for(i = 0; i < len; i++) {
        ch      = string[i];
        at      = &atlas[ch];
        vx1     = x;
        vy1     = y;
        vx2     = vx1 + at->w * scale;
        vy2     = vy1 + at->h * scale;
        tx1     = (at->x / w) + 0.001f;
        tx2     = (tx1 + at->w / w) - 0.002f;
        ty1     = (at->y / h);
        ty2     = (ty1 + at->h / h);
        check   = (char*)string+i;

        kexlib::render::cpuVertList->AddVertex(vx1, vy1, 0, tx1, ty1, rgba1);
        kexlib::render::cpuVertList->AddVertex(vx2, vy1, 0, tx2, ty1, rgba1);
        kexlib::render::cpuVertList->AddVertex(vx1, vy2, 0, tx1, ty2, rgba2);
        kexlib::render::cpuVertList->AddVertex(vx2, vy2, 0, tx2, ty2, rgba2);

        kexlib::render::cpuVertList->AddTriangle(0+tri, 1+tri, 2+tri);
        kexlib::render::cpuVertList->AddTriangle(2+tri, 1+tri, 3+tri);

        x += at->w * scale;
        tri += 4;
    }
    
    kexlib::render::cpuVertList->DrawElements(material);
}

//
// kexFont::StringWidth
//

float kexFont::StringWidth(const char* string, float scale, int fixedLen) {
    float width = 0;
    int len = strlen(string);
    int i;

    if(fixedLen > 0) {
        len = fixedLen;
    }
        
    for(i = 0; i < len; i++) {
        width += (atlas[string[i]].w * scale);
    }
            
    return width;
}

//
// kexFont::StringHeight
//

float kexFont::StringHeight(const char* string, float scale, int fixedLen) {
    float height = 0;
    float th;
    int len = strlen(string);
    int i;
    
    if(fixedLen > 0) {
        len = fixedLen;
    }
    
    for(i = 0; i < len; i++) {
        th = (atlas[string[i]].h * scale);
        if(th > height) {
            height = th;
        }
    }
    
    return height;
}

//
// kexFontManager::OnLoad
//

kexFont *kexFontManager::OnLoad(const char *file) {
    kexFont *font = dataList.Add(file);
    font->LoadKFont(file);

    return font;
}
