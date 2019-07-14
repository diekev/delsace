///////////////////////////////////////////////////////////////////////////////////////////////////
// I. LICENSE CONDITIONS
//
// Copyright (c) 2019 by Disney-Pixar
//
// Permission is hereby granted to use this software solely for non-commercial applications 
// and purposes including academic or industrial research, evaluation and not-for-profit media
// production. All other rights are retained by Pixar. For use for or in connection with 
// commercial applications and purposes, including without limitation in or in connection with 
// software products offered for sale or for-profit media production, please contact Pixar at 
// tech-licensing@pixar.com.
//
// THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT
// NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS 
// FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL PIXAR OR ITS AFFILIATES BE 
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING 
// IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
///////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef KELVINLET_DYNA_PULSE_AFFINE_H
#define KELVINLET_DYNA_PULSE_AFFINE_H

#include "kelvinlet/dynaPulseBase.h"

/// 
/// \class DynaPulseAffine
/// \brief Computes the displacement generated by an impulse affine xform.
///
/// \code
///    DynaPulseAffine wave;
///    wave.SetPoint(p);
///    wave.SetTime(t);
///    wave.SetEps(e);
///    wave.SetForce(f);
///    wave.SetMaterial(mu, nu);
///    u = wave.EvalDisp(query, time);
/// \endcode
///

namespace Kelvinlet 
{

class DynaPulseAffine : public DynaPulseBase
{
public:
    using Force = Matrix33;
    inline const Force& GetForce() const { return _force; }
    inline void SetForce(const Force& val) { _force = val; }

    DynaPulseAffine() 
    : _force(Force::Zero())
    { }

    DynaPulseAffine(const DynaPulseAffine& other) 
    { 
        Copy(other);
    }

    DynaPulseAffine& operator=(const DynaPulseAffine& other) 
    { 
        Copy(other);
        return *this;
    }

    virtual void Calibrate() override;

    virtual void Copy(const DynaPulseAffine& other);
    
protected:
    virtual Vector3 _EvalDisp(const Vector3& query, const Scalar time) const override;

    virtual void _Compute(Cache& values, const Scalar r, const Scalar t) const override;

    Force _force;
};

} // namespace Kelvinlet

#endif // KELVINLET_DYNA_PULSE_AFFINE_H
