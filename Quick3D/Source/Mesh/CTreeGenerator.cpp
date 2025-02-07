
// Application
#include "C3DScene.h"
#include "CTreeGenerator.h"
#include "CPerlin.h"
#include "CBox.h"
#include "CCone.h"

//-------------------------------------------------------------------------------------------------

using namespace Math;

//-------------------------------------------------------------------------------------------------

#define NUM_SEGMENTS			8
#define NUM_HEIGHT_SEGMENTS		4
#define VERT_OFFSET				0.2

//-------------------------------------------------------------------------------------------------

CTreeGenerator::CTreeGenerator(C3DScene* pScene)
    : m_mMutex(QMutex::Recursive)
    , m_pScene(pScene)
{
}

//-------------------------------------------------------------------------------------------------

CTreeGenerator::~CTreeGenerator()
{
}

//-------------------------------------------------------------------------------------------------

CMeshGeometry* CTreeGenerator::createTree(
        int iLODLevel,
        CVector3 vNoisePosition,
        int iNumLevels,
        double dTrunkLength,
        double dTrunkRadius,
        double dLengthScale,
        double dRadiusScale,
        double dLeafScale,
        double dGravityFactor,
        const QVector<Math::CVector3>& vFFDFrom,
        const QVector<Math::CVector3>& vFFDTo
        )
{
    CPerlin* perlin = CPerlin::getInstance();

    // Create trunk
    CMeshGeometry* pMesh = new CMeshGeometry(m_pScene, 10.0 + (10.0 * (double) iLODLevel));

    pMesh->createCone(
                NUM_SEGMENTS / (iLODLevel + 1),
                NUM_HEIGHT_SEGMENTS / (iLODLevel + 1),
                dTrunkLength + VERT_OFFSET,
                dTrunkRadius,
                dTrunkRadius * dRadiusScale
                );

    pMesh->setMaterial(m_pScene->ressourcesManager()->getTreeMaterial());

    CMatrix4 mAccumTransform = CMatrix4().makeTranslation(CVector3(0.0, dTrunkLength, 0.0));

    int iTreeLevel = iNumLevels;

    addBranches(
                pMesh,
                iLODLevel,
                vNoisePosition,
                iNumLevels,
                mAccumTransform,
                dTrunkLength + VERT_OFFSET,
                dTrunkRadius,
                dLengthScale,
                dRadiusScale,
                dLeafScale,
                dGravityFactor,
                vFFDFrom,
                vFFDTo,
                dLengthScale,
                dRadiusScale,
                iTreeLevel
                );

    for (int iVertexIndex = 0; iVertexIndex < pMesh->vertices().count(); iVertexIndex++)
    {
        CVector3 vPosition = pMesh->vertices()[iVertexIndex].position();

        double dX = perlin->noise(vPosition * 2.50) * (dTrunkLength * 0.04);
        double dY = perlin->noise(vPosition * 2.53) * (dTrunkLength * 0.04);
        double dZ = perlin->noise(vPosition * 2.55) * (dTrunkLength * 0.04);

        pMesh->vertices()[iVertexIndex].position() = CVector3(vPosition.X + dX, (vPosition.Y + dY) - VERT_OFFSET, vPosition.Z + dZ);
    }

    return pMesh;
}

//-------------------------------------------------------------------------------------------------

void CTreeGenerator::addBranches(
        CMeshGeometry* pMesh,
        int iLODLevel,
        CVector3 vNoisePosition,
        int iNumLevels,
        CMatrix4 mAccumTransform,
        double dTrunkLength,
        double dTrunkRadius,
        double dLengthScale,
        double dRadiusScale,
        double dLeafScale,
        double dGravityFactor,
        const QVector<Math::CVector3>& vFFDFrom,
        const QVector<Math::CVector3>& vFFDTo,
        double dCurrentLengthScale,
        double dCurrentRadiusScale,
        int& iTreeLevel
        )
{
    iTreeLevel--;

    if (iTreeLevel > 0)
    {
        CPerlin* perlin = CPerlin::getInstance();

        CVector3 vPosition = mAccumTransform * CVector3();

        bool bContinueCurrentBranch = perlin->noise_0_1((vNoisePosition + vPosition) * 3.0) > 0.5;
        int iNumBranches = 2 + (int) (perlin->noise_0_1((vNoisePosition + vPosition) * 3.1) * iTreeLevel);

        for (int iIndex = 0; iIndex < iNumBranches; iIndex++)
        {
            double dRotationX = dGravityFactor * (0.3 + (perlin->noise_0_1((vNoisePosition + vPosition) * 3.2) * (Math::Pi * 1.0)) * 0.25);
            double dRotationY = (((double) iIndex / (double) iNumBranches) * (Math::Pi * 3.3)) + perlin->noise_0_1((vNoisePosition + vPosition) * 1.2) * (Math::Pi / 6.0);

            // If branch must continue, clear flag and reduce X rotation
            if (bContinueCurrentBranch)
            {
                bContinueCurrentBranch = false;
                dRotationX *= 0.02;
            }

            CMeshGeometry mBranch(m_pScene, 100.0);

            mBranch.createCone(
                        NUM_SEGMENTS / (iLODLevel + 1),
                        NUM_HEIGHT_SEGMENTS / (iLODLevel + 1),
                        dTrunkLength * dCurrentLengthScale,
                        dTrunkRadius * dCurrentRadiusScale,
                        dTrunkRadius * dCurrentRadiusScale * dRadiusScale
                        );

            mBranch.scaleUVs(CVector2(0.5, 1.0));

            CMatrix4 mRotate1 = CMatrix4().makeRotation(CVector3(dRotationX, 0.0, 0.0));
            CMatrix4 mRotate2 = CMatrix4().makeRotation(CVector3(0.0, dRotationY, 0.0));
            CMatrix4 mTranslate = CMatrix4().makeTranslation(CVector3(0.0, dTrunkLength * dCurrentLengthScale, 0.0));
            CMatrix4 mTransform = (mRotate1 * mRotate2) * mAccumTransform;

            mBranch.transformVertices(mTransform);

            if (iTreeLevel < iNumLevels - 1)
            {
                addLeaves(
                            pMesh,
                            iLODLevel,
                            mTransform,
                            dTrunkLength,
                            dTrunkRadius,
                            dLeafScale,
                            vFFDFrom,
                            vFFDTo,
                            dCurrentLengthScale,
                            dCurrentRadiusScale
                            );
            }

            mTransform = ((mTranslate * mRotate1) * mRotate2) * mAccumTransform;

            pMesh->merge(mBranch);

            addBranches(
                        pMesh,
                        iLODLevel,
                        vNoisePosition,
                        iNumLevels,
                        mTransform,
                        dTrunkLength,
                        dTrunkRadius,
                        dLengthScale,
                        dRadiusScale,
                        dLeafScale,
                        dGravityFactor,
                        vFFDFrom,
                        vFFDTo,
                        dCurrentLengthScale * dLengthScale,
                        dCurrentRadiusScale * dRadiusScale,
                        iTreeLevel
                        );
        }
    }

    iTreeLevel++;
}

//-------------------------------------------------------------------------------------------------

void CTreeGenerator::addLeaves(
        CMeshGeometry* pMesh,
        int iLODLevel,
        CMatrix4 mAccumTransform,
        double dTrunkLength,
        double dTrunkRadius,
        double dLeafScale,
        const QVector<Math::CVector3>& vFFDFrom,
        const QVector<Math::CVector3>& vFFDTo,
        double dCurrentLengthScale,
        double dCurrentRadiusScale
        )
{
    int iNumLeaves = (5 + (rand() % 10)) / (iLODLevel + 2);
    int iNumVerts = 5 / (iLODLevel + 1);

    if (iNumVerts > 2)
    {
        for (int iIndex = 0; iIndex < iNumLeaves; iIndex++)
        {
            double dRotationX = -0.2 + ((double) rand() / 32768.0) * 0.2;
            double dRotationY = (((double) iIndex / (double) iNumLeaves) * (Math::Pi * 2.0)) + ((double) rand() / 32768.0) * (Math::Pi / 6.0);
            double dTranslationY = ((double) rand() / 32768.0) * (dTrunkLength * dCurrentLengthScale);

            CMeshGeometry mLeaf(m_pScene);
            mLeaf.createSurfaceFromFFD(vFFDFrom, vFFDTo, iNumVerts);
            mLeaf.transformVertices(CMatrix4().makeTranslation(CVector3(0.0, 0.0, 0.5)));
            mLeaf.transformVertices(CMatrix4().makeScale(CVector3(dTrunkLength * 0.1, dTrunkLength * 0.1, dTrunkLength * 0.1)));

            CMatrix4 mScale = CMatrix4().makeScale(CVector3(dLeafScale, dLeafScale, dLeafScale));
            CMatrix4 mRotate1 = CMatrix4().makeRotation(CVector3(dRotationX, 0.0, 0.0));
            CMatrix4 mRotate2 = CMatrix4().makeRotation(CVector3(0.0, dRotationY, 0.0));
            CMatrix4 mTranslate = CMatrix4().makeTranslation(CVector3(0.0, dTranslationY, 0.0));
            CMatrix4 mTransform = (((mScale * mRotate1) * mRotate2) * mTranslate) * mAccumTransform;

            mLeaf.transformVertices(mTransform);

            pMesh->merge(mLeaf);
        }
    }
}
