
/*
    pbrt source code is Copyright(c) 1998-2015
                        Matt Pharr, Greg Humphreys, and Wenzel Jakob.

    This file is part of pbrt.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are
    met:

    - Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.

    - Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
    IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
    TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
    PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
    HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
    SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
    LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 */

#include "stdafx.h"

// materials/subsurface.cpp*
#include "materials/subsurface.h"
#include "textures/constant.h"
#include "spectrum.h"
#include "texture.h"
#include "interpolation.h"
#include "paramset.h"
#include "interaction.h"

// SubsurfaceMaterial Method Definitions
void SubsurfaceMaterial::ComputeScatteringFunctions(
    SurfaceInteraction *si, MemoryArena &arena, TransportMode mode,
    bool allowMultipleLobes) const {
    // Perform bump mapping with _bumpMap_, if present
    if (bumpMap) Bump(bumpMap, si);

    // Add _BxDF_s for specular reflection and transmission for
    // _SubsurfaceMaterial_
    si->bsdf = ARENA_ALLOC(arena, BSDF)(*si, eta);
    Spectrum R = Kr->Evaluate(*si).Clamp();
    Spectrum T = Kt->Evaluate(*si).Clamp();
    if (allowMultipleLobes)
        si->bsdf->Add(
            ARENA_ALLOC(arena, FresnelSpecular)(R, T, 1.f, eta, mode));
    else {
        if (!R.IsBlack())
            si->bsdf->Add(ARENA_ALLOC(arena, SpecularReflection)(
                R, ARENA_ALLOC(arena, FresnelDielectric)(1.f, eta)));
        if (!T.IsBlack())
            si->bsdf->Add(
                ARENA_ALLOC(arena, SpecularTransmission)(T, 1.f, eta, mode));
    }
    Spectrum sig_a = scale * sigma_a->Evaluate(*si).Clamp();
    Spectrum sig_s = scale * sigma_s->Evaluate(*si).Clamp();
    si->bssrdf = ARENA_ALLOC(arena, TabulatedBSSRDF)(*si, this, table, sig_a,
                                                     sig_s, eta, mode);
}

SubsurfaceMaterial *CreateSubsurfaceMaterial(const TextureParams &mp) {
    Float sig_a_rgb[3] = {.0011f, .0024f, .014f},
          sig_s_rgb[3] = {2.55f, 3.21f, 3.77f};
    Spectrum sig_a = Spectrum::FromRGB(sig_a_rgb),
             sig_s = Spectrum::FromRGB(sig_s_rgb);
    std::string name = mp.FindString("name");
    bool found = GetMediumScatteringProperties(name, &sig_a, &sig_s);
    if (name != "" && !found)
        Warning("Named material \"%s\" not found.  Using defaults.",
                name.c_str());
    Float scale = mp.FindFloat("scale", 1.f);
    Float eta = mp.FindFloat("eta", 1.33f);
    Float g = mp.FindFloat("g", 0.0f);

    std::shared_ptr<Texture<Spectrum>> sigma_a, sigma_s;
    sigma_a = mp.GetSpectrumTexture("sigma_a", sig_a);
    sigma_s = mp.GetSpectrumTexture("sigma_s", sig_s);
    std::shared_ptr<Texture<Spectrum>> Kr =
        mp.GetSpectrumTexture("Kr", Spectrum(1.f));
    std::shared_ptr<Texture<Spectrum>> Kt =
        mp.GetSpectrumTexture("Kt", Spectrum(1.f));
    std::shared_ptr<Texture<Float>> bumpMap =
        mp.GetFloatTextureOrNull("bumpmap");
    return new SubsurfaceMaterial(scale, Kr, Kt, sigma_a, sigma_s, g, eta,
                                  bumpMap);
}
