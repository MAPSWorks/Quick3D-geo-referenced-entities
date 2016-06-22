
#pragma once

#include "quick3d_global.h"

// Fondations
#include "CGeoloc.h"
#include "CBoundingBox.h"

//-------------------------------------------------------------------------------------------------

class C3DScene;

//-------------------------------------------------------------------------------------------------

class QUICK3D_EXPORT CGeoBounds
{
public:

    //-------------------------------------------------------------------------------------------------
    // Constructeurs et destructeur
    //-------------------------------------------------------------------------------------------------

    //!
    CGeoBounds(CGeoloc gPosition, const CBoundingBox& bSize, double dHeading = 0.0);

    //!
    virtual ~CGeoBounds();

    //-------------------------------------------------------------------------------------------------
    // Getters
    //-------------------------------------------------------------------------------------------------

    //!
    CGeoloc position() const { return m_gPosition; }

    //!
    CBoundingBox& size() { return m_bSize; }

    //!
    const CBoundingBox& size() const { return m_bSize; }

    //-------------------------------------------------------------------------------------------------
    // M�thodes h�rit�es
    //-------------------------------------------------------------------------------------------------

    //! Calcul d'intersection avec un rayon
    virtual Math::RayTracingResult intersect(Math::CRay3 ray);

    //-------------------------------------------------------------------------------------------------
    // M�thodes de contr�le
    //-------------------------------------------------------------------------------------------------

    //!
    bool contains(CGeoloc gPosition) const;

    //-------------------------------------------------------------------------------------------------
    // M�thodes prot�g�es
    //-------------------------------------------------------------------------------------------------

    //!
    void compute();

    //-------------------------------------------------------------------------------------------------
    // Propri�t�s
    //-------------------------------------------------------------------------------------------------

protected:

    CGeoloc         m_gPosition;
    CBoundingBox    m_bSize;
};
