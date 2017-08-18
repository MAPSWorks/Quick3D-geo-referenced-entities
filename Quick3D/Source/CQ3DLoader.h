
#pragma once

// qt-plus
#include "CSingleton.h"

// Application
#include "quick3d_global.h"
#include "CMesh.h"
#include "CXMLNode.h"

class QUICK3D_EXPORT CQ3DLoader : public CSingleton<CQ3DLoader>
{
    friend class CSingleton<CQ3DLoader>;

public:

    //!
    QSP<CMeshGeometry> load(const QString& sBaseFile, CComponent* pContainer, QString sText);

    //!
    void loadComponent(const QString& sBaseFile, CComponent* pContainer, QSP<CMeshGeometry> pMesh, CXMLNode xNode, QVector<QSP<CMaterial> > &vMaterials, CComponent* pParent = nullptr);

    //!
    void addBounds(CComponent* pContainer, CBoundingBox& bBox, Math::CMatrix4 mTransform);

protected:

    CQ3DLoader();
};
